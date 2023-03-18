// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/tool_client_connection_handler.hpp"

namespace fabricrpc {

HRESULT STDMETHODCALLTYPE tool_client_connection_handler::OnConnected(
    /* [in] */ LPCWSTR connectionAddress) {
  UNREFERENCED_PARAMETER(connectionAddress);
  return S_OK;
}

HRESULT STDMETHODCALLTYPE tool_client_connection_handler::OnDisconnected(
    /* [in] */ LPCWSTR connectionAddress,
    /* [in] */ HRESULT error) {
  UNREFERENCED_PARAMETER(connectionAddress);
  UNREFERENCED_PARAMETER(error);
  return S_OK;
}

} // namespace fabricrpc