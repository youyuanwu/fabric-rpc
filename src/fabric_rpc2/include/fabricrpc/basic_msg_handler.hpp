#pragma once

#include "fabricrpc/basic_connection_manager.hpp"
#include "fabricrpc/basic_item_queue.hpp"
#include <fabrictransport_.h>
#include <winrt/base.h>

#include <memory>
#include <mutex>
#include <vector>

namespace fabricrpc {

template <typename Executor = net::any_io_executor>
class basic_msg_handler
    : public winrt::implements<basic_msg_handler<Executor>,
                               IFabricTransportMessageHandler> {
public:
  typedef Executor executor_type;

  basic_msg_handler(
      // std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue,
      std::shared_ptr<basic_connection_manager<executor_type>> mgr)
      : // queue_(queue),
        mgr_(mgr) {}

  // easier impl is to block in begin process request until acceptor pulls it.

  HRESULT STDMETHODCALLTYPE BeginProcessRequest(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback *callback,
      /* [retval][out] */ IFabricAsyncOperationContext **context) override {

    winrt::com_ptr<IFabricTransportMessage> msg;
    msg.copy_from(message);

    winrt::com_ptr<IFabricAsyncOperationCallback> usr_callback;
    usr_callback.copy_from(callback);

    winrt::com_ptr<IFabricAsyncOperationContext> ctx =
        winrt::make<request_context>(callback);
    ctx.copy_to(context);

    std::wstring id(clientId);

    p_request_t pl = std::make_unique<request>(
        std::move(id), std::move(msg), std::move(usr_callback), std::move(ctx));
    mgr_->post_request(std::move(pl));

    // TODO: have a queue max length limit.
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EndProcessRequest(
      /* [in] */ IFabricAsyncOperationContext *context,
      /* [retval][out] */ IFabricTransportMessage **reply) override {
    assert(context != nullptr);

    // extract context for result;
    request_context *ctx = dynamic_cast<request_context *>(context);
    assert(ctx != nullptr);

    auto content = ctx->get_content_view();
    if (content.hr != S_OK) {
      // request failed
      return content.hr;
    }

    assert(content.reply_msg);
    content.reply_msg.copy_to(reply);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE HandleOneWay(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message) override {
    return S_OK;
  }

private:
  // const executor_type &ex_;
  // std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue_;

  std::shared_ptr<basic_connection_manager<executor_type>> mgr_;
};

} // namespace fabricrpc