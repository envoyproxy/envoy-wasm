extern crate proxy_wasm;

use lazy_static::*;
use log::info;
use proxy_wasm::context::*;
use proxy_wasm::envoy_log::*;
use std::sync::Arc;

// =============== RootContext ============================
struct SampleRootContext {}

impl RootContext for SampleRootContext {
    fn on_start(&self, _vm_confiuration_size: u32) -> u32 {
        info!("Hello Envoy!");
        0
    }
}

struct SampleRootContextFactory {}

impl RootContextFactory for SampleRootContextFactory {
    fn create(&self) -> Arc<dyn RootContext + Sync + Send> {
        Arc::from(SampleRootContext {})
    }
}
// ========================================================

// =================== Context ============================
struct SampleContext {
    pub root_context: Arc<dyn RootContext + Sync + Send>,
}

impl Context for SampleContext {
    fn on_create(&self) {
        info!("Hello Envoy Create!");
    }
}

struct SampleContextFactory {}

impl ContextFactory for SampleContextFactory {
    fn create(
        &self,
        _root_context: Arc<dyn RootContext + Sync + Send>,
    ) -> Arc<dyn Context + Sync + Send> {
        Arc::from(SampleContext {
            root_context: _root_context,
        })
    }
}
// ========================================================

lazy_static! {
    static ref SAMPLE_ROOT_CONTEXT_FACTORY: Box<dyn RootContextFactory + Sync + Send> =
        Box::new(SampleRootContextFactory {});
    static ref SAMPLE_CONTEXT_FACTORY: Box<dyn ContextFactory + Sync + Send> =
        Box::new(SampleContextFactory {});
}

#[no_mangle]
fn _start() {
    Logger::init().unwrap();
    register_factory(
        "my_root_id",
        &SAMPLE_CONTEXT_FACTORY,
        &SAMPLE_ROOT_CONTEXT_FACTORY,
    );
}
