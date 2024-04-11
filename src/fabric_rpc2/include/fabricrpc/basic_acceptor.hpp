#pragma once

#include "fabricrpc/basic_item_queue.hpp"
#include "fabricrpc/basic_msg_handler.hpp"
#include "fabricrpc/endpoint.hpp"
#include "fabricrpc_tool/msg_disposer.hpp"
// #include "fabricrpc_tool/tool_server_connection_handler.hpp"
#include "fabricrpc/basic_connection_handler.hpp"
#include "fabricrpc/basic_server_connection.hpp"
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>
#include <fabricrpc_tool/waitable_callback.hpp>
#include <fabrictransport_.h>

#include "fabricrpc/basic_connection_manager.hpp"

namespace fabricrpc {

namespace net = boost::asio;

// handler of signature void(error_code, basic_server_connection)
template <typename Executor>
class async_move_accept_conn_op : boost::asio::coroutine {
public:
  typedef Executor executor_type;

  async_move_accept_conn_op(
      std::shared_ptr<conn_manager_entry<executor_type>> *entry_holder,
      std::shared_ptr<basic_connection_manager<executor_type>> mgr,
      std::shared_ptr<basic_event<executor_type>> ev)
      : entry_holder_(entry_holder), ev_(ev), mgr_(mgr) {}

  template <typename Self>
  void operator()(Self &self, boost::system::error_code ec = {}) {
    // to be returned to user
    basic_server_connection<executor_type> conn(ev_->get_executor());
    if (ec) {
      self.complete(ec, std::move(conn));
      return;
    }

    // initiate async op
    mgr_->async_pop(ev_, entry_holder_);

    // wait for payload to arrive
    ev_->async_wait([self = std::move(self), c = std::move(conn),
                     e = entry_holder_](boost::system::error_code ec) mutable {
      // construct a connection and move to user
      // holder should be filledup
      std::shared_ptr<conn_manager_entry<executor_type>> e_copy = *e;
      assert(e_copy);
      c.set_queue(e_copy->queue);
      self.complete(ec, std::move(c));
    });
  }

private:
  //  entry to be poped from mgr, to be used to construct server connection pipe
  //  passed to user.
  std::shared_ptr<conn_manager_entry<executor_type>> *entry_holder_;
  std::shared_ptr<basic_event<executor_type>> ev_;
  std::shared_ptr<basic_connection_manager<executor_type>> mgr_;
};

template <typename Executor = net::any_io_executor> class basic_acceptor {
public:
  typedef Executor executor_type;

  basic_acceptor(const executor_type &ex, endpoint &ep)
      : ep_(ep), listener_(), entry_holder_(),
        mgr_(std::make_shared<basic_connection_manager<executor_type>>()),
        ev_(std::make_shared<basic_event<executor_type>>(ex)) {

    // TODO: make the handler.
    // maybe move this to async_accept
    winrt::com_ptr<IFabricTransportMessageHandler> req_handler =
        winrt::make<fabricrpc::basic_msg_handler<executor_type>>(mgr_);

    winrt::com_ptr<IFabricTransportConnectionHandler> conn_handler =
        winrt::make<fabricrpc::basic_connection_handler<executor_type>>(mgr_);
    winrt::com_ptr<IFabricTransportMessageDisposer> msg_disposer =
        winrt::make<fabricrpc::msg_disposer>();
    winrt::com_ptr<IFabricTransportListener> listener;

    // create listener
    HRESULT hr = CreateFabricTransportListener(
        IID_IFabricTransportListener,
        (FABRIC_TRANSPORT_SETTINGS *)ep_.get_settings(),
        (FABRIC_TRANSPORT_LISTEN_ADDRESS *)ep_.get_addr(), req_handler.get(),
        conn_handler.get(), msg_disposer.get(), listener.put());
    assert(hr == S_OK);
    listener.copy_to(listener_.put());
  }

  // open the listener
  boost::system::error_code open(std::wstring *addr) {
    assert(listener_);

    winrt::com_ptr<IWaitableCallback> callback =
        winrt::make<waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
    HRESULT hr = listener_->BeginOpen(callback.get(), ctx.put());
    if (hr != S_OK) {
      return boost::system::error_code(
          hr, boost::asio::error::get_system_category());
    }
    callback->Wait();
    // make sure that Wait is efficient.
    assert(ctx->CompletedSynchronously());
    winrt::com_ptr<IFabricStringResult> str;
    hr = listener_->EndOpen(ctx.get(), str.put());
    if (hr == S_OK) {
      *addr = std::wstring(str->get_String());
    }
    return boost::system::error_code(hr,
                                     boost::asio::error::get_system_category());
  }

  boost::system::error_code close() {
    // cancel conn mgr operations
    mgr_->cancel(ev_);

    assert(listener_);
    winrt::com_ptr<IWaitableCallback> callback =
        winrt::make<waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
    HRESULT hr = listener_->BeginClose(callback.get(), ctx.put());
    if (hr != S_OK) {
      return boost::system::error_code(
          hr, boost::asio::error::get_system_category());
    }
    callback->Wait();
    assert(ctx->CompletedSynchronously());
    hr = listener_->EndClose(ctx.get());
    return boost::system::error_code(hr,
                                     boost::asio::error::get_system_category());
  }

  // accepts connection
  // handler type void(ec, basic_server_connection)
  template <typename Token> auto async_accept_conn(Token &&token) {
    ev_->reset();
    entry_holder_.reset();
    return boost::asio::async_compose<
        Token, void(boost::system::error_code,
                    basic_server_connection<executor_type>)>(
        async_move_accept_conn_op<executor_type>(&this->entry_holder_,
                                                 this->mgr_, this->ev_),
        token, this->ev_->get_executor());
  }

private:
  endpoint ep_;
  winrt::com_ptr<IFabricTransportListener> listener_;

  // holder for building server_connection to be passed to user.
  // each accept will populate this field.
  std::shared_ptr<conn_manager_entry<executor_type>> entry_holder_;

  // event used for waiting arriving connection
  std::shared_ptr<basic_event<executor_type>> ev_;

  std::shared_ptr<basic_connection_manager<executor_type>> mgr_;
};

} // namespace fabricrpc