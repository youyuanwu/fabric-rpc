// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/tool_client_notification_handler.hpp"

namespace fabricrpc {

HRESULT STDMETHODCALLTYPE tool_client_notification_handler::HandleOneWay(
    /* [in] */ IFabricTransportMessage *message) {
  UNREFERENCED_PARAMETER(message);
  return S_OK;
}

} // namespace fabricrpc