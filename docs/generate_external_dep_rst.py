#!/usr/bin/env python3

# Generate RST lists of external dependencies.

from collections import defaultdict, namedtuple
import pathlib
import sys
import urllib.parse

from importlib.util import spec_from_loader, module_from_spec
from importlib.machinery import SourceFileLoader

# bazel/repository_locations.bzl must have a .bzl suffix for Starlark import, so
# we are forced to do this workaround.
_repository_locations_spec = spec_from_loader(
    'repository_locations',
    SourceFileLoader('repository_locations', 'bazel/repository_locations.bzl'))
repository_locations = module_from_spec(_repository_locations_spec)
_repository_locations_spec.loader.exec_module(repository_locations)


# Render a CSV table given a list of table headers, widths and list of rows
# (each a list of strings).
def CsvTable(headers, widths, rows):
  csv_rows = '\n  '.join(', '.join(row) for row in rows)
  return f'''.. csv-table::
  :header: {', '.join(headers)}
  :widths: {', '.join(str(w) for w in widths) }

  {csv_rows}

'''


# Anonymous external RST link for a given URL.
def RstLink(text, url):
  return f'`{text} <{url}>`__'


# NIST CPE database search URL for a given CPE.
def NistCpeUrl(cpe):
  encoded_cpe = urllib.parse.quote(cpe)
  return 'https://nvd.nist.gov/products/cpe/search/results?keyword=%s&status=FINAL&orderBy=CPEURI&namingFormat=2.3' % encoded_cpe


# Render version strings human readable.
def RenderVersion(version):
  # Heuristic, almost certainly a git SHA
  if len(version) == 40:
    # Abbreviate git SHA
    return version[:7]
  return version


if __name__ == '__main__':
  security_rst_root = sys.argv[1]

  Dep = namedtuple('Dep', ['name', 'sort_name', 'version', 'cpe'])
  use_categories = defaultdict(list)
  # Bin rendered dependencies into per-use category lists.
  for k, v in repository_locations.DEPENDENCY_REPOSITORIES.items():
    cpe = v.get('cpe', '')
    if cpe == 'N/A':
      cpe = ''
    if cpe:
      cpe = RstLink(cpe, NistCpeUrl(cpe))
    project_name = v['project_name']
    project_url = v['project_url']
    name = RstLink(project_name, project_url)
    version = RstLink(RenderVersion(v['version']), v['urls'][0])
    dep = Dep(name, project_name.lower(), version, cpe)
    for category in v['use_category']:
      use_categories[category].append(dep)

  def CsvRow(dep):
    return [dep.name, dep.version, dep.cpe]

  # Generate per-use category RST with CSV tables.
  for category, deps in use_categories.items():
    output_path = pathlib.Path(security_rst_root, f'external_dep_{category}.rst')
    content = CsvTable(['Name', 'Version', 'CPE'], [2, 1, 2],
                       [CsvRow(dep) for dep in sorted(deps, key=lambda d: d.sort_name)])
    output_path.write_text(content)
