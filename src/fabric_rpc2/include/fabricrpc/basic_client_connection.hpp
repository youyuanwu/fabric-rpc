#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <fabricrpc/basic_event.hpp>
#include <fabrictransport_.h>
#include <winrt/base.h>

#include <fabricrpc_tool/tool_client_connection_handler.hpp>
#include <fabricrpc_tool/tool_client_notification_handler.hpp>

namespace fabricrpc {

namespace net = boost::asio;

// callback used by connection
template <typename Executor = net::any_io_executor>
class private_callback
    : public winrt::implements<private_callback<Executor>,
                               IFabricAsyncOperationCallback> {
public:
  typedef Executor executor_type;

  private_callback(const executor_type &ex)
      : ev_(std::make_shared<basic_event<executor_type>>(ex)) {}

  void STDMETHODCALLTYPE Invoke(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    assert(ev_);
    ev_->set();
    // release ownership
    ev_.reset();
  }

  // valid only before the invoke.
  std::shared_ptr<basic_event<executor_type>> get_event() {
    assert(ev_);
    return ev_;
  }

private:
  std::shared_ptr<basic_event<executor_type>> ev_;
};

// handler of signature void(error_code, winrt::com_ptr<IFabricTransportMessage>
// msg)
template <typename Executor> class async_req_op : boost::asio::coroutine {
public:
  typedef Executor executor_type;

  async_req_op(const executor_type &ex,
               winrt::com_ptr<IFabricTransportClient> client,
               winrt::com_ptr<IFabricTransportMessage> req)
      : ex_(ex), client_(client), req_(req) {
    callback_ = winrt::make<private_callback<executor_type>>(ex_);
  }

  template <typename Self>
  void operator()(Self &self, boost::system::error_code ec = {}) {
    if (ec) {
      self.complete(ec, {});
      return;
    }

    private_callback<executor_type> *callback_cast =
        dynamic_cast<private_callback<executor_type> *>(callback_.get());
    auto ev = callback_cast->get_event();
    auto ev_copy = ev; // pass to async callback for ownership

    // need to start async wait here.
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
    HRESULT hr =
        client_->BeginRequest(req_.get(), 1000, callback_.get(), ctx.put());
    if (hr != S_OK) {
      ec = boost::system::error_code(hr, net::error::get_system_category());
      self.complete(ec, {});
      return;
    }
    // if errors out it is sync
    // assert(!ctx->CompletedSynchronously());

    auto client_copy = client_;

    // wait for payload to arrive
    ev->async_wait([self = std::move(self), client_copy, ev_copy,
                    ctx](boost::system::error_code ec) mutable {
      winrt::com_ptr<IFabricTransportMessage> reply;
      if (!ec) {
        // success
        HRESULT hr = client_copy->EndRequest(ctx.get(), reply.put());
        if (hr != S_OK) {
          ec = boost::system::error_code(hr, net::error::get_system_category());
        }
      }
      self.complete(ec, reply);
    });
  }

private:
  const executor_type &ex_;
  // req msg
  winrt::com_ptr<IFabricTransportMessage> req_;
  winrt::com_ptr<IFabricTransportClient> client_;

  winrt::com_ptr<IFabricAsyncOperationCallback> callback_;
};

template <typename Executor = net::any_io_executor>
class basic_client_connection {
public:
  typedef Executor executor_type;

  basic_client_connection(const executor_type &ex) : ex_(ex) {}

  basic_client_connection(basic_client_connection<executor_type> &) = delete;

  basic_client_connection(basic_client_connection<executor_type> &&) = default;

  boost::system::error_code open(const endpoint &ep) {
    assert(!client_);
    auto url = ep.get_url();
    winrt::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
        winrt::make<fabricrpc::tool_client_notification_handler>();

    winrt::com_ptr<IFabricTransportClientEventHandler> client_event_h =
        winrt::make<fabricrpc::tool_client_connection_handler>();
    winrt::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
        winrt::make<fabricrpc::msg_disposer>();

    auto settings = ep.get_settings();

    // open client
    HRESULT hr = CreateFabricTransportClient(
        /* [in] */ IID_IFabricTransportClient,
        /* [in] */ (FABRIC_TRANSPORT_SETTINGS *)settings,
        /* [in] */ url.c_str(),
        /* [in] */ client_notify_h.get(),
        /* [in] */ client_event_h.get(),
        /* [in] */ client_msg_disposer.get(),
        /* [retval][out] */ client_.put());
    if (hr != S_OK) {
      return boost::system::error_code(hr, net::error::get_system_category());
    }
    // open connection
    winrt::com_ptr<IWaitableCallback> callback =
        winrt::make<waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = client_->BeginOpen(1000, callback.get(), ctx.put());
    if (hr != S_OK) {
      return boost::system::error_code(
          hr, boost::asio::error::get_system_category());
    }
    callback->Wait();
    // This might not be true in general. Check wait is efficient.
    assert(ctx->IsCompleted());
    assert(ctx->CompletedSynchronously());
    hr = client_->EndOpen(ctx.get());
    return boost::system::error_code(hr,
                                     boost::asio::error::get_system_category());
  }

  // Token type: void(ec, winrt::com_ptr<IFabricTransportMessage> reply)
  template <typename Token>
  auto async_send(winrt::com_ptr<IFabricTransportMessage> msg, Token &&token) {
    return boost::asio::async_compose<
        Token, void(boost::system::error_code,
                    winrt::com_ptr<IFabricTransportMessage>)>(
        async_req_op<executor_type>(ex_, client_, msg), token, this->ex_);
  }

  executor_type get_executor() { return ex_; }

private:
  winrt::com_ptr<IFabricTransportClient> client_;
  const executor_type &ex_;
};

} // namespace fabricrpc