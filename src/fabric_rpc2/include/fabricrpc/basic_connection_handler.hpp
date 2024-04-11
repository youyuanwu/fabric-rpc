#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

#include "fabricrpc/any_context.hpp"
#include "fabricrpc/basic_connection_manager.hpp"

namespace fabricrpc {

// conn handler passed to fabric transport
template <typename Executor = net::any_io_executor>
class basic_connection_handler
    : public winrt::implements<basic_connection_handler<Executor>,
                               IFabricTransportConnectionHandler> {
public:
  typedef Executor executor_type;

  basic_connection_handler(
      std::shared_ptr<basic_connection_manager<executor_type>> mgr)
      : mgr_(mgr) {}

  HRESULT STDMETHODCALLTYPE BeginProcessConnect(
      /* [in] */ IFabricTransportClientConnection *clientConnection,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback *callback,
      /* [retval][out] */ IFabricAsyncOperationContext **context) override {
    winrt::com_ptr<IFabricTransportClientConnection> conn;
    conn.copy_from(clientConnection);
    mgr_->add_conn(conn);

    // immediately return
    winrt::com_ptr<any_context<bool>> ctx =
        winrt::make_self<any_context<bool>>(callback);
    winrt::com_ptr<IFabricAsyncOperationContext> ret = ctx;
    ret.copy_to(context);
    ctx->complete();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EndProcessConnect(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE BeginProcessDisconnect(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback *callback,
      /* [retval][out] */ IFabricAsyncOperationContext **context) override {
    std::wstring id(clientId);
    mgr_->disconnect(id);

    // immediately finish
    winrt::com_ptr<any_context<bool>> ctx =
        winrt::make_self<any_context<bool>>(callback);
    winrt::com_ptr<IFabricAsyncOperationContext> ret = ctx;
    ret.copy_to(context);
    ctx->complete();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EndProcessDisconnect(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    return S_OK;
  }

private:
  std::shared_ptr<basic_connection_manager<executor_type>> mgr_;
};

} // namespace fabricrpc