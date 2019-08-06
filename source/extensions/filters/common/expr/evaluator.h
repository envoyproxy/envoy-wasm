#pragma once

#include "envoy/stream_info/stream_info.h"

#include "common/http/headers.h"
#include "common/protobuf/protobuf.h"

#include "extensions/filters/common/expr/context.h"

#include "eval/public/cel_expression.h"
#include "eval/public/cel_value.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

using Builder = google::api::expr::runtime::CelExpressionBuilder;
using BuilderPtr = std::unique_ptr<Builder>;
using Expression = google::api::expr::runtime::CelExpression;
using ExpressionPtr = std::unique_ptr<Expression>;

// Creates an expression builder. The optional arena is used to enable constant folding
// for intermediate evaluation results.
// Throws an exception if fails to construct an expression builder.
BuilderPtr createBuilder(Protobuf::Arena* arena);

// Creates an interpretable expression from a protobuf representation.
// Throws an exception if fails to construct a runtime expression.
ExpressionPtr createExpression(Builder& builder, const google::api::expr::v1alpha1::Expr& expr);

// Evaluates an expression for a request. The arena is used to hold intermediate computational
// results and potentially the final value.
absl::optional<CelValue> evaluate(const Expression& expr, Protobuf::Arena* arena,
                                  const StreamInfo::StreamInfo& info,
                                  const Http::HeaderMap& headers);

// Evaluates an expression and returns true if the expression evaluates to "true".
// Returns false if the expression fails to evaluate.
bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::HeaderMap& headers);

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
