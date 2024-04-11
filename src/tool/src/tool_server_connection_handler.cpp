// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/tool_server_connection_handler.hpp"
#include "fabricrpc_tool/sync_async_context.hpp"

namespace fabricrpc {

HRESULT STDMETHODCALLTYPE server_tool_connection_handler::BeginProcessConnect(
    /* [in] */ IFabricTransportClientConnection *clientConnection,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) {
  UNREFERENCED_PARAMETER(clientConnection);
  UNREFERENCED_PARAMETER(timeoutMilliseconds);
  winrt::com_ptr<IFabricAsyncOperationContext> ctx =
      winrt::make<sync_async_context>(callback);
  *context = ctx.detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE server_tool_connection_handler::EndProcessConnect(
    /* [in] */ IFabricAsyncOperationContext *context) {
  UNREFERENCED_PARAMETER(context);
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
server_tool_connection_handler::BeginProcessDisconnect(
    /* [in] */ COMMUNICATION_CLIENT_ID clientId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) {
  UNREFERENCED_PARAMETER(clientId);
  UNREFERENCED_PARAMETER(timeoutMilliseconds);
  winrt::com_ptr<IFabricAsyncOperationContext> ctx =
      winrt::make<sync_async_context>(callback);
  *context = ctx.detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE server_tool_connection_handler::EndProcessDisconnect(
    /* [in] */ IFabricAsyncOperationContext *context) {
  UNREFERENCED_PARAMETER(context);
  return S_OK;
}

} // namespace fabricrpc