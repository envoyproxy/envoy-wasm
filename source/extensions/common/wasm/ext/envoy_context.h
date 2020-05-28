// NOLINT(namespace-envoy)
#pragma once

class EnvoyContextBase {
public:
  virtual ~EnvoyContextBase() = default;

  virtual void onResolveDns(uint32_t /* token */, uint32_t /* result_size */) {}
};

class EnvoyRootContext : public RootContext, public EnvoyContextBase {
public:
  EnvoyRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}
  ~EnvoyRootContext() override = default;
};

class EnvoyContext : public Context, public EnvoyContextBase {
public:
  EnvoyContext(uint32_t id, RootContext* root) : Context(id, root) {}
  ~EnvoyContext() override = default;
};
