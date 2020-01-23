// Create an expression using a foreign function call.
inline WasmResult createExpression(StringView expr, uint32_t* token) {
  std::string function = "expr_create";
  char* out = nullptr;
  size_t out_size = 0;
  auto result = proxy_call_foreign_function(function.data(), function.size(), expr.data(),
                                            expr.size(), &out, &out_size);
  if (result == WasmResult::Ok && out_size == sizeof(uint32_t)) {
    *token = *reinterpret_cast<uint32_t*>(out);
  }
  ::free(out);
  return result;
}

// Evaluate an expression using an expression token.
inline Optional<WasmDataPtr> exprEvaluate(uint32_t token) {
  std::string function = "expr_evaluate";
  char* out = nullptr;
  size_t out_size = 0;
  auto result = proxy_call_foreign_function(function.data(), function.size(),
                                            reinterpret_cast<const char*>(&token), sizeof(uint32_t), 
                                            &out, &out_size);
  if (result != WasmResult::Ok) {
    return {};
  }
  return std::make_unique<WasmData>(out, out_size);
}

// Delete an expression using an expression token.
inline WasmResult exprDelete(uint32_t token) {
  std::string function = "expr_delete";
  char* out = nullptr;
  size_t out_size = 0;
  auto result = proxy_call_foreign_function(function.data(), function.size(),
                                     reinterpret_cast<const char*>(&token), sizeof(uint32_t),
                                     &out, &out_size);
  ::free(out);
  return result;
}

template <typename T>
inline bool evaluateExpression(uint32_t token, T* out) {
  auto buf = exprEvaluate(token);
  if (!buf.has_value() || buf.value()->size() != sizeof(T)) {
    return false;
  }
  *out = *reinterpret_cast<const T*>(buf.value()->data());
  return true;
}

template <>
inline bool evaluateExpression<std::string>(uint32_t token, std::string* out) {
  auto buf = exprEvaluate(token);
  if (!buf.has_value()) {
    return false;
  }
  out->assign(buf.value()->data(), buf.value()->size());
  return true;
}

// Specialization for message types (including struct value for lists and maps)
template <typename T>
inline bool evaluateMessage(uint32_t token,
                            T* value_ptr) {
  auto buf = exprEvaluate(token);
  if (!buf.has_value()) {
    return false;
  }
  if (buf.value()->size() == 0) {
    // evaluates to null
    return true;
  }
  return value_ptr->ParseFromArray(buf.value()->data(), buf.value()->size());
}

