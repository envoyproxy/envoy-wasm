# protoc plugin to map from FileDescriptorProtos to Envoy doc style RST.
# See https://github.com/google/protobuf/blob/master/src/google/protobuf/descriptor.proto
# for the underlying protos mentioned in this file. See
# https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html for Sphinx RST syntax.

from collections import defaultdict
import functools
import os
import re

from tools.api_proto_plugin import annotations
from tools.api_proto_plugin import plugin
from tools.api_proto_plugin import visitor

from validate import validate_pb2

# Namespace prefix for Envoy core APIs.
ENVOY_API_NAMESPACE_PREFIX = '.envoy.api.v2.'

# Namespace prefix for Envoy top-level APIs.
ENVOY_PREFIX = '.envoy.'

# Namespace prefix for WKTs.
WKT_NAMESPACE_PREFIX = '.google.protobuf.'

# Namespace prefix for RPCs.
RPC_NAMESPACE_PREFIX = '.google.rpc.'

# http://www.fileformat.info/info/unicode/char/2063/index.htm
UNICODE_INVISIBLE_SEPARATOR = u'\u2063'

# Template for data plane API URLs.
DATA_PLANE_API_URL_FMT = 'https://github.com/envoyproxy/envoy/blob/{}/api/%s#L%d'.format(
    os.environ['ENVOY_BLOB_SHA'])


class ProtodocError(Exception):
  """Base error class for the protodoc module."""


def HideNotImplemented(comment):
  """Should a given type_context.Comment be hidden because it is tagged as [#not-implemented-hide:]?"""
  return annotations.NOT_IMPLEMENTED_HIDE_ANNOTATION in comment.annotations


def GithubUrl(type_context):
  """Obtain data plane API Github URL by path from a TypeContext.

  Args:
    type_context: type_context.TypeContext for node.

  Returns:
    A string with a corresponding data plane API GitHub Url.
  """
  if type_context.location is not None:
    return DATA_PLANE_API_URL_FMT % (type_context.source_code_info.name,
                                     type_context.location.span[0])
  return ''


def FormatCommentWithAnnotations(comment, type_name=''):
  """Format a comment string with additional RST for annotations.

  Args:
    comment: comment string.
    type_name: optional, 'message' or 'enum' may be specified for additional
      message/enum specific annotations.

  Returns:
    A string with additional RST from annotations.
  """
  s = annotations.WithoutAnnotations(StripLeadingSpace(comment.raw) + '\n')
  if annotations.NOT_IMPLEMENTED_WARN_ANNOTATION in comment.annotations:
    s += '\n.. WARNING::\n  Not implemented yet\n'
  if annotations.V2_API_DIFF_ANNOTATION in comment.annotations:
    s += '\n.. NOTE::\n  **v2 API difference**: ' + comment.annotations[
        annotations.V2_API_DIFF_ANNOTATION] + '\n'
  if type_name == 'message' or type_name == 'enum':
    if annotations.PROTO_STATUS_ANNOTATION in comment.annotations:
      status = comment.annotations[annotations.PROTO_STATUS_ANNOTATION]
      if status not in ['frozen', 'draft', 'experimental']:
        raise ProtodocError('Unknown proto status: %s' % status)
      if status == 'draft' or status == 'experimental':
        s += ('\n.. WARNING::\n This %s type has :ref:`%s '
              '<config_overview_v2_status>` status.\n' % (type_name, status))
  return s


def MapLines(f, s):
  """Apply a function across each line in a flat string.

  Args:
    f: A string transform function for a line.
    s: A string consisting of potentially multiple lines.

  Returns:
    A flat string with f applied to each line.
  """
  return '\n'.join(f(line) for line in s.split('\n'))


def Indent(spaces, line):
  """Indent a string."""
  return ' ' * spaces + line


def IndentLines(spaces, lines):
  """Indent a list of strings."""
  return map(functools.partial(Indent, spaces), lines)


def FormatInternalLink(text, ref):
  return ':ref:`%s <%s>`' % (text, ref)


def FormatExternalLink(text, ref):
  return '`%s <%s>`_' % (text, ref)


def FormatHeader(style, text):
  """Format RST header.

  Args:
    style: underline style, e.g. '=', '-'.
    text: header text

  Returns:
    RST formatted header.
  """
  return '%s\n%s\n\n' % (text, style * len(text))


def FormatHeaderFromFile(style, source_code_info, proto_name):
  """Format RST header based on special file level title

  Args:
    style: underline style, e.g. '=', '-'.
    source_code_info: SourceCodeInfo object.
    proto_name: If the file_level_comment does not contain a user specified
      title, use this as page title.

  Returns:
    RST formatted header, and file level comment without page title strings.
  """
  anchor = FormatAnchor(FileCrossRefLabel(proto_name))
  stripped_comment = annotations.WithoutAnnotations(
      StripLeadingSpace('\n'.join(c + '\n' for c in source_code_info.file_level_comments)))
  if annotations.DOC_TITLE_ANNOTATION in source_code_info.file_level_annotations:
    return anchor + FormatHeader(
        style,
        source_code_info.file_level_annotations[annotations.DOC_TITLE_ANNOTATION]), stripped_comment
  return anchor + FormatHeader(style, proto_name), stripped_comment


def FormatFieldTypeAsJson(type_context, field):
  """Format FieldDescriptorProto.Type as a pseudo-JSON string.

  Args:
    type_context: contextual information for message/enum/field.
    field: FieldDescriptor proto.
  Return: RST formatted pseudo-JSON string representation of field type.
  """
  if TypeNameFromFQN(field.type_name) in type_context.map_typenames:
    return '"{...}"'
  if field.label == field.LABEL_REPEATED:
    return '[]'
  if field.type == field.TYPE_MESSAGE:
    return '"{...}"'
  return '"..."'


def FormatMessageAsJson(type_context, msg):
  """Format a message definition DescriptorProto as a pseudo-JSON block.

  Args:
    type_context: contextual information for message/enum/field.
    msg: message definition DescriptorProto.
  Return: RST formatted pseudo-JSON string representation of message definition.
  """
  lines = []
  for index, field in enumerate(msg.field):
    field_type_context = type_context.ExtendField(index, field.name)
    leading_comment = field_type_context.leading_comment
    if HideNotImplemented(leading_comment):
      continue
    lines.append('"%s": %s' % (field.name, FormatFieldTypeAsJson(type_context, field)))

  if lines:
    return '.. code-block:: json\n\n  {\n' + ',\n'.join(IndentLines(4, lines)) + '\n  }\n\n'
  else:
    return '.. code-block:: json\n\n  {}\n\n'


def NormalizeFieldTypeName(field_fqn):
  """Normalize a fully qualified field type name, e.g.

  .envoy.foo.bar.

  Strips leading ENVOY_API_NAMESPACE_PREFIX and ENVOY_PREFIX.

  Args:
    field_fqn: a fully qualified type name from FieldDescriptorProto.type_name.
  Return: Normalized type name.
  """
  if field_fqn.startswith(ENVOY_API_NAMESPACE_PREFIX):
    return field_fqn[len(ENVOY_API_NAMESPACE_PREFIX):]
  if field_fqn.startswith(ENVOY_PREFIX):
    return field_fqn[len(ENVOY_PREFIX):]
  return field_fqn


def NormalizeTypeContextName(type_name):
  """Normalize a type name, e.g.

  envoy.foo.bar.

  Strips leading ENVOY_API_NAMESPACE_PREFIX and ENVOY_PREFIX.

  Args:
    type_name: a name from a TypeContext.
  Return: Normalized type name.
  """
  return NormalizeFieldTypeName(QualifyTypeName(type_name))


def QualifyTypeName(type_name):
  return '.' + type_name


def TypeNameFromFQN(fqn):
  return fqn[1:]


def FormatEmph(s):
  """RST format a string for emphasis."""
  return '*%s*' % s


def FormatFieldType(type_context, field):
  """Format a FieldDescriptorProto type description.

  Adds cross-refs for message types.
  TODO(htuch): Add cross-refs for enums as well.

  Args:
    type_context: contextual information for message/enum/field.
    field: FieldDescriptor proto.
  Return: RST formatted field type.
  """
  if field.type_name.startswith(ENVOY_API_NAMESPACE_PREFIX) or field.type_name.startswith(
      ENVOY_PREFIX):
    type_name = NormalizeFieldTypeName(field.type_name)
    if field.type == field.TYPE_MESSAGE:
      if type_context.map_typenames and TypeNameFromFQN(
          field.type_name) in type_context.map_typenames:
        return 'map<%s, %s>' % tuple(
            map(functools.partial(FormatFieldType, type_context),
                type_context.map_typenames[TypeNameFromFQN(field.type_name)]))
      return FormatInternalLink(type_name, MessageCrossRefLabel(type_name))
    if field.type == field.TYPE_ENUM:
      return FormatInternalLink(type_name, EnumCrossRefLabel(type_name))
  elif field.type_name.startswith(WKT_NAMESPACE_PREFIX):
    wkt = field.type_name[len(WKT_NAMESPACE_PREFIX):]
    return FormatExternalLink(
        wkt, 'https://developers.google.com/protocol-buffers/docs/reference/google.protobuf#%s' %
        wkt.lower())
  elif field.type_name.startswith(RPC_NAMESPACE_PREFIX):
    rpc = field.type_name[len(RPC_NAMESPACE_PREFIX):]
    return FormatExternalLink(
        rpc,
        'https://cloud.google.com/natural-language/docs/reference/rpc/google.rpc#%s' % rpc.lower())
  elif field.type_name:
    return field.type_name

  pretty_type_names = {
      field.TYPE_DOUBLE: 'double',
      field.TYPE_FLOAT: 'float',
      field.TYPE_INT32: 'int32',
      field.TYPE_SFIXED32: 'int32',
      field.TYPE_SINT32: 'int32',
      field.TYPE_FIXED32: 'uint32',
      field.TYPE_UINT32: 'uint32',
      field.TYPE_INT64: 'int64',
      field.TYPE_SFIXED64: 'int64',
      field.TYPE_SINT64: 'int64',
      field.TYPE_FIXED64: 'uint64',
      field.TYPE_UINT64: 'uint64',
      field.TYPE_BOOL: 'bool',
      field.TYPE_STRING: 'string',
      field.TYPE_BYTES: 'bytes',
  }
  if field.type in pretty_type_names:
    return FormatExternalLink(pretty_type_names[field.type],
                              'https://developers.google.com/protocol-buffers/docs/proto#scalar')
  raise ProtodocError('Unknown field type ' + str(field.type))


def StripLeadingSpace(s):
  """Remove leading space in flat comment strings."""
  return MapLines(lambda s: s[1:], s)


def FileCrossRefLabel(msg_name):
  """File cross reference label."""
  return 'envoy_api_file_%s' % msg_name


def MessageCrossRefLabel(msg_name):
  """Message cross reference label."""
  return 'envoy_api_msg_%s' % msg_name


def EnumCrossRefLabel(enum_name):
  """Enum cross reference label."""
  return 'envoy_api_enum_%s' % enum_name


def FieldCrossRefLabel(field_name):
  """Field cross reference label."""
  return 'envoy_api_field_%s' % field_name


def EnumValueCrossRefLabel(enum_value_name):
  """Enum value cross reference label."""
  return 'envoy_api_enum_value_%s' % enum_value_name


def FormatAnchor(label):
  """Format a label as an Envoy API RST anchor."""
  return '.. _%s:\n\n' % label


def FormatFieldAsDefinitionListItem(outer_type_context, type_context, field):
  """Format a FieldDescriptorProto as RST definition list item.

  Args:
    outer_type_context: contextual information for enclosing message.
    type_context: contextual information for message/enum/field.
    field: FieldDescriptorProto.

  Returns:
    RST formatted definition list item.
  """
  field_annotations = []

  anchor = FormatAnchor(FieldCrossRefLabel(NormalizeTypeContextName(type_context.name)))
  if field.options.HasExtension(validate_pb2.rules):
    rule = field.options.Extensions[validate_pb2.rules]
    if ((rule.HasField('message') and rule.message.required) or
        (rule.HasField('string') and rule.string.min_bytes > 0) or
        (rule.HasField('repeated') and rule.repeated.min_items > 0)):
      field_annotations = ['*REQUIRED*']
  leading_comment = type_context.leading_comment
  formatted_leading_comment = FormatCommentWithAnnotations(leading_comment)
  if HideNotImplemented(leading_comment):
    return ''

  if field.HasField('oneof_index'):
    oneof_context = outer_type_context.ExtendOneof(field.oneof_index,
                                                   type_context.oneof_names[field.oneof_index])
    oneof_comment = oneof_context.leading_comment
    formatted_oneof_comment = FormatCommentWithAnnotations(oneof_comment)
    if HideNotImplemented(oneof_comment):
      return ''

    # If the oneof only has one field and marked required, mark the field as required.
    if len(type_context.oneof_fields[field.oneof_index]) == 1 and type_context.oneof_required[
        field.oneof_index]:
      field_annotations = ['*REQUIRED*']

    if len(type_context.oneof_fields[field.oneof_index]) > 1:
      # Fields in oneof shouldn't be marked as required when we have oneof comment below it.
      field_annotations = []
      oneof_template = '\nPrecisely one of %s must be set.\n' if type_context.oneof_required[
          field.oneof_index] else '\nOnly one of %s may be set.\n'
      formatted_oneof_comment += oneof_template % ', '.join(
          FormatInternalLink(
              f,
              FieldCrossRefLabel(NormalizeTypeContextName(
                  outer_type_context.ExtendField(i, f).name)))
          for i, f in type_context.oneof_fields[field.oneof_index])
  else:
    formatted_oneof_comment = ''

  comment = '(%s) ' % ', '.join([FormatFieldType(type_context, field)] +
                                field_annotations) + formatted_leading_comment
  return anchor + field.name + '\n' + MapLines(functools.partial(Indent, 2),
                                               comment + formatted_oneof_comment)


def FormatMessageAsDefinitionList(type_context, msg):
  """Format a DescriptorProto as RST definition list.

  Args:
    type_context: contextual information for message/enum/field.
    msg: DescriptorProto.

  Returns:
    RST formatted definition list item.
  """
  type_context.oneof_fields = defaultdict(list)
  type_context.oneof_required = defaultdict(bool)
  type_context.oneof_names = defaultdict(list)
  for index, field in enumerate(msg.field):
    if field.HasField('oneof_index'):
      leading_comment = type_context.ExtendField(index, field.name).leading_comment
      if HideNotImplemented(leading_comment):
        continue
      type_context.oneof_fields[field.oneof_index].append((index, field.name))
  for index, oneof_decl in enumerate(msg.oneof_decl):
    if oneof_decl.options.HasExtension(validate_pb2.required):
      type_context.oneof_required[index] = oneof_decl.options.Extensions[validate_pb2.required]
    type_context.oneof_names[index] = oneof_decl.name
  return '\n'.join(
      FormatFieldAsDefinitionListItem(type_context, type_context.ExtendField(index, field.name),
                                      field) for index, field in enumerate(msg.field)) + '\n'


def FormatEnumValueAsDefinitionListItem(type_context, enum_value):
  """Format a EnumValueDescriptorProto as RST definition list item.

  Args:
    type_context: contextual information for message/enum/field.
    enum_value: EnumValueDescriptorProto.

  Returns:
    RST formatted definition list item.
  """
  anchor = FormatAnchor(EnumValueCrossRefLabel(NormalizeTypeContextName(type_context.name)))
  default_comment = '*(DEFAULT)* ' if enum_value.number == 0 else ''
  leading_comment = type_context.leading_comment
  formatted_leading_comment = FormatCommentWithAnnotations(leading_comment)
  if HideNotImplemented(leading_comment):
    return ''
  comment = default_comment + UNICODE_INVISIBLE_SEPARATOR + formatted_leading_comment
  return anchor + enum_value.name + '\n' + MapLines(functools.partial(Indent, 2), comment)


def FormatEnumAsDefinitionList(type_context, enum):
  """Format a EnumDescriptorProto as RST definition list.

  Args:
    type_context: contextual information for message/enum/field.
    enum: DescriptorProto.

  Returns:
    RST formatted definition list item.
  """
  return '\n'.join(
      FormatEnumValueAsDefinitionListItem(type_context.ExtendEnumValue(index, enum_value.name),
                                          enum_value)
      for index, enum_value in enumerate(enum.value)) + '\n'


def FormatProtoAsBlockComment(proto):
  """Format a proto as a RST block comment.

  Useful in debugging, not usually referenced.
  """
  return '\n\nproto::\n\n' + MapLines(functools.partial(Indent, 2), str(proto)) + '\n'


class RstFormatVisitor(visitor.Visitor):
  """Visitor to generate a RST representation from a FileDescriptor proto.

  See visitor.Visitor for visitor method docs comments.
  """

  def VisitEnum(self, enum_proto, type_context):
    normal_enum_type = NormalizeTypeContextName(type_context.name)
    anchor = FormatAnchor(EnumCrossRefLabel(normal_enum_type))
    header = FormatHeader('-', 'Enum %s' % normal_enum_type)
    github_url = GithubUrl(type_context)
    proto_link = FormatExternalLink('[%s proto]' % normal_enum_type, github_url) + '\n\n'
    leading_comment = type_context.leading_comment
    formatted_leading_comment = FormatCommentWithAnnotations(leading_comment, 'enum')
    if HideNotImplemented(leading_comment):
      return ''
    return anchor + header + proto_link + formatted_leading_comment + FormatEnumAsDefinitionList(
        type_context, enum_proto)

  def VisitMessage(self, msg_proto, type_context, nested_msgs, nested_enums):
    normal_msg_type = NormalizeTypeContextName(type_context.name)
    anchor = FormatAnchor(MessageCrossRefLabel(normal_msg_type))
    header = FormatHeader('-', normal_msg_type)
    github_url = GithubUrl(type_context)
    proto_link = FormatExternalLink('[%s proto]' % normal_msg_type, github_url) + '\n\n'
    leading_comment = type_context.leading_comment
    formatted_leading_comment = FormatCommentWithAnnotations(leading_comment, 'message')
    if HideNotImplemented(leading_comment):
      return ''
    return anchor + header + proto_link + formatted_leading_comment + FormatMessageAsJson(
        type_context, msg_proto) + FormatMessageAsDefinitionList(
            type_context, msg_proto) + '\n'.join(nested_msgs) + '\n' + '\n'.join(nested_enums)

  def VisitFile(self, file_proto, type_context, msgs, enums):
    # Find the earliest detached comment, attribute it to file level.
    # Also extract file level titles if any.
    header, comment = FormatHeaderFromFile('=', type_context.source_code_info, file_proto.name)
    debug_proto = FormatProtoAsBlockComment(file_proto)
    return header + comment + '\n'.join(msgs) + '\n'.join(enums)  # + debug_proto


def Main():
  plugin.Plugin('.rst', RstFormatVisitor())


if __name__ == '__main__':
  Main()
