#pragma once

// Some gRPC headers contains old style cast and unused parameter which doesn't
// compile with -Werror, ignoring those compiler warning since we don't have
// control on those source codes. This works with GCC and Clang.

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include "grpc/grpc_security.h"
#include "src/core/tsi/alts/handshaker/alts_shared_resource.h"
#include "src/core/tsi/alts/handshaker/alts_tsi_handshaker.h"
#include "src/core/tsi/transport_security_interface.h"

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include "common/common/c_smart_ptr.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Alts {

void wrapped_tsi_handshaker_destroy(tsi_handshaker* self);

using CFrameProtectorPtr = CSmartPtr<tsi_frame_protector, tsi_frame_protector_destroy>;

using CHandshakerResultPtr = CSmartPtr<tsi_handshaker_result, tsi_handshaker_result_destroy>;
using CHandshakerPtr = CSmartPtr<tsi_handshaker, wrapped_tsi_handshaker_destroy>;

} // namespace Alts
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
