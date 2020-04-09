#include "extensions/common/wasm/foreign.h"

#include "common/common/logger.h"

#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "parser/parser.h"
#include "zlib.h"

using proxy_wasm::RegisterForeignFunction;

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

template <typename T> WasmForeignFunction createFromClass() {
  auto c = std::make_shared<T>();
  return c->create(c);
}

RegisterForeignFunction registerCompressForeignFunction(
    "compress",
    [](WasmBase&, absl::string_view arguments,
       std::function<void*(size_t size)> alloc_result) -> WasmResult {
      unsigned long dest_len = compressBound(arguments.size());
      std::unique_ptr<unsigned char[]> b(new unsigned char[dest_len]);
      if (compress(b.get(), &dest_len, reinterpret_cast<const unsigned char*>(arguments.data()),
                   arguments.size()) != Z_OK) {
        return WasmResult::SerializationFailure;
      }
      auto result = alloc_result(dest_len);
      memcpy(result, b.get(), dest_len);
      return WasmResult::Ok;
    });

RegisterForeignFunction registerUncompressForeignFunction(
    "uncompress",
    [](WasmBase&, absl::string_view arguments,
       std::function<void*(size_t size)> alloc_result) -> WasmResult {
      unsigned long dest_len = arguments.size() * 2 + 2; // output estimate.
      while (1) {
        std::unique_ptr<unsigned char[]> b(new unsigned char[dest_len]);
        auto r =
            uncompress(b.get(), &dest_len, reinterpret_cast<const unsigned char*>(arguments.data()),
                       arguments.size());
        if (r == Z_OK) {
          auto result = alloc_result(dest_len);
          memcpy(result, b.get(), dest_len);
          return WasmResult::Ok;
        }
        if (r != Z_MEM_ERROR) {
          return WasmResult::SerializationFailure;
        }
        dest_len = dest_len * 2;
      }
    });

class ExpressionFactory : public Logger::Loggable<Logger::Id::wasm> {
protected:
  struct ExpressionData {
    google::api::expr::v1alpha1::ParsedExpr parsed_expr_;
    Filters::Common::Expr::ExpressionPtr compiled_expr_;
  };

  class ExpressionContext : public StorageObject {
  public:
    friend class ExpressionFactory;
    ExpressionContext(Filters::Common::Expr::BuilderPtr builder) : builder_(std::move(builder)) {}
    uint32_t createToken() {
      uint32_t token = next_expr_token_++;
      for (;;) {
        if (!expr_.count(token)) {
          break;
        }
        token = next_expr_token_++;
      }
      return token;
    }
    bool hasExpression(uint32_t token) { return expr_.contains(token); }
    ExpressionData& getExpression(uint32_t token) { return expr_[token]; }
    void deleteExpression(uint32_t token) { expr_.erase(token); }
    Filters::Common::Expr::Builder* builder() { return builder_.get(); }

  private:
    Filters::Common::Expr::BuilderPtr builder_{};
    uint32_t next_expr_token_ = 0;
    absl::flat_hash_map<uint32_t, ExpressionData> expr_;
  };

  static ExpressionContext& getOrCreateContext(ContextBase* context_base) {
    auto context = static_cast<Context*>(context_base);
    std::string data_name = "cel";
    auto expr_context = context->getForeignData<ExpressionContext>(data_name);
    if (!expr_context) {
      google::api::expr::runtime::InterpreterOptions options;
      auto builder = google::api::expr::runtime::CreateCelExpressionBuilder(options);
      auto status =
          google::api::expr::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
      if (!status.ok()) {
        ENVOY_LOG(warn, "failed to register built-in functions: {}", status.message());
      }
      auto new_context = std::make_unique<ExpressionContext>(std::move(builder));
      expr_context = new_context.get();
      context->setForeignData(data_name, std::move(new_context));
    }
    return *expr_context;
  }
};

class CreateExpressionFactory : public ExpressionFactory {
public:
  WasmForeignFunction create(std::shared_ptr<CreateExpressionFactory> self) const {
    WasmForeignFunction f = [self](WasmBase&, absl::string_view expr,
                                   std::function<void*(size_t size)> alloc_result) -> WasmResult {
      auto parse_status = google::api::expr::parser::Parse(std::string(expr));
      if (!parse_status.ok()) {
        ENVOY_LOG(info, "expr_create parse error: {}", parse_status.status().message());
        return WasmResult::BadArgument;
      }

      auto& expr_context = getOrCreateContext(proxy_wasm::current_context_->root_context());
      auto token = expr_context.createToken();
      auto& handler = expr_context.getExpression(token);

      handler.parsed_expr_ = parse_status.ValueOrDie();
      auto cel_expression_status = expr_context.builder()->CreateExpression(
          &handler.parsed_expr_.expr(), &handler.parsed_expr_.source_info());
      if (!cel_expression_status.ok()) {
        ENVOY_LOG(info, "expr_create compile error: {}", cel_expression_status.status().message());
        expr_context.deleteExpression(token);
        return WasmResult::BadArgument;
      }

      handler.compiled_expr_ = std::move(cel_expression_status.ValueOrDie());
      auto result = reinterpret_cast<uint32_t*>(alloc_result(sizeof(uint32_t)));
      *result = token;
      return WasmResult::Ok;
    };
    return f;
  }
};
RegisterForeignFunction
    registerCreateExpressionForeignFunction("expr_create",
                                            createFromClass<CreateExpressionFactory>());

class EvaluateExpressionFactory : public ExpressionFactory {
public:
  WasmForeignFunction create(std::shared_ptr<EvaluateExpressionFactory> self) const {
    WasmForeignFunction f = [self](WasmBase&, absl::string_view argument,
                                   std::function<void*(size_t size)> alloc_result) -> WasmResult {
      auto& expr_context = getOrCreateContext(proxy_wasm::current_context_->root_context());
      if (argument.size() != sizeof(uint32_t)) {
        return WasmResult::BadArgument;
      }
      uint32_t token = *reinterpret_cast<const uint32_t*>(argument.data());
      if (!expr_context.hasExpression(token)) {
        return WasmResult::NotFound;
      }
      Protobuf::Arena arena;
      auto& handler = expr_context.getExpression(token);
      auto context = static_cast<Context*>(proxy_wasm::current_context_);
      auto eval_status = handler.compiled_expr_->Evaluate(*context, &arena);
      if (!eval_status.ok()) {
        ENVOY_LOG(debug, "expr_evaluate error: {}", eval_status.status().message());
        return WasmResult::InternalFailure;
      }
      std::string result;
      auto serialize_status = serializeValue(eval_status.ValueOrDie(), &result);
      if (serialize_status != WasmResult::Ok) {
        return serialize_status;
      }
      auto output = alloc_result(result.size());
      memcpy(output, result.data(), result.size());
      return WasmResult::Ok;
    };
    return f;
  }
};
RegisterForeignFunction
    registerEvaluateExpressionForeignFunction("expr_evaluate",
                                              createFromClass<EvaluateExpressionFactory>());

class DeleteExpressionFactory : public ExpressionFactory {
public:
  WasmForeignFunction create(std::shared_ptr<DeleteExpressionFactory> self) const {
    WasmForeignFunction f = [self](WasmBase&, absl::string_view argument,
                                   std::function<void*(size_t size)>) -> WasmResult {
      auto& expr_context = getOrCreateContext(proxy_wasm::current_context_->root_context());
      if (argument.size() != sizeof(uint32_t)) {
        return WasmResult::BadArgument;
      }
      uint32_t token = *reinterpret_cast<const uint32_t*>(argument.data());
      expr_context.deleteExpression(token);
      return WasmResult::Ok;
    };
    return f;
  }
};
RegisterForeignFunction
    registerDeleteExpressionForeignFunction("expr_delete",
                                            createFromClass<DeleteExpressionFactory>());

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
