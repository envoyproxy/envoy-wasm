#include "extensions/common/tap/extension_config_base.h"

#include "envoy/config/tap/v3alpha/common.pb.h"
#include "envoy/extensions/common/tap/v3alpha/common.pb.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {

ExtensionConfigBase::ExtensionConfigBase(
    const envoy::extensions::common::tap::v3alpha::CommonExtensionConfig proto_config,
    TapConfigFactoryPtr&& config_factory, Server::Admin& admin,
    Singleton::Manager& singleton_manager, ThreadLocal::SlotAllocator& tls,
    Event::Dispatcher& main_thread_dispatcher)
    : proto_config_(proto_config), config_factory_(std::move(config_factory)),
      tls_slot_(tls.allocateSlot()) {
  tls_slot_->set([](Event::Dispatcher&) -> ThreadLocal::ThreadLocalObjectSharedPtr {
    return std::make_shared<TlsFilterConfig>();
  });

  switch (proto_config_.config_type_case()) {
  case envoy::extensions::common::tap::v3alpha::CommonExtensionConfig::ConfigTypeCase::
      kAdminConfig: {
    admin_handler_ = AdminHandler::getSingleton(admin, singleton_manager, main_thread_dispatcher);
    admin_handler_->registerConfig(*this, proto_config_.admin_config().config_id());
    ENVOY_LOG(debug, "initializing tap extension with admin endpoint (config_id={})",
              proto_config_.admin_config().config_id());
    break;
  }
  case envoy::extensions::common::tap::v3alpha::CommonExtensionConfig::ConfigTypeCase::
      kStaticConfig: {
    newTapConfig(envoy::config::tap::v3alpha::TapConfig(proto_config_.static_config()), nullptr);
    ENVOY_LOG(debug, "initializing tap extension with static config");
    break;
  }
  default: {
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  }
}

ExtensionConfigBase::~ExtensionConfigBase() {
  if (admin_handler_) {
    admin_handler_->unregisterConfig(*this);
  }
}

const absl::string_view ExtensionConfigBase::adminId() {
  // It is only possible to get here if we had an admin config and registered with the admin
  // handler.
  ASSERT(proto_config_.has_admin_config());
  return proto_config_.admin_config().config_id();
}

void ExtensionConfigBase::clearTapConfig() {
  tls_slot_->runOnAllThreads([this] { tls_slot_->getTyped<TlsFilterConfig>().config_ = nullptr; });
}

void ExtensionConfigBase::newTapConfig(envoy::config::tap::v3alpha::TapConfig&& proto_config,
                                       Sink* admin_streamer) {
  TapConfigSharedPtr new_config =
      config_factory_->createConfigFromProto(std::move(proto_config), admin_streamer);
  tls_slot_->runOnAllThreads(
      [this, new_config] { tls_slot_->getTyped<TlsFilterConfig>().config_ = new_config; });
}

} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy
