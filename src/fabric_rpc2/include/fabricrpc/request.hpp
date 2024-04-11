#pragma once

#include "fabricrpc/any_context.hpp"
#include <absl/status/status.h>
#include <fabrictransport_.h>
#include <winrt/base.h>

namespace fabricrpc {

struct context_content {
  // operation result
  HRESULT hr;
  // if hr is ok, msg should be valid
  winrt::com_ptr<IFabricTransportMessage> reply_msg;
};

typedef any_context<context_content> request_context;

// real payload used by acceptor
class request {
public:
  request(std::wstring id, winrt::com_ptr<IFabricTransportMessage> msg,
          winrt::com_ptr<IFabricAsyncOperationCallback> callback,
          winrt::com_ptr<IFabricAsyncOperationContext> ctx);

  request_context *get_request_context();

  void complete(HRESULT hr, winrt::com_ptr<IFabricTransportMessage> reply_msg);

  // complete rpc header with status
  void complete_rpc_error(absl::Status st);

  // void complete_rpc_ok(winrt::com_ptr<IFabricTransportMessage> reply_msg);

  void get_request_msg(IFabricTransportMessage **msgout);

  const std::wstring &get_conn_id() const { return id_; }

private:
  // connection id
  const std::wstring id_;
  // incomming message
  winrt::com_ptr<IFabricTransportMessage> msg_;
  // user callback
  winrt::com_ptr<IFabricAsyncOperationCallback> callback_;
  // ctx returned to user
  winrt::com_ptr<IFabricAsyncOperationContext> ctx_;
};

// real payload used by acceptor
typedef std::unique_ptr<request> p_request_t;

} // namespace fabricrpc