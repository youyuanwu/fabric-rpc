#pragma once

#include "fabricrpc/basic_item_queue.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>

namespace fabricrpc {

// struct srv_conn_entry {

// };

// typedef std::unique_ptr<srv_conn_entry> p_srv_conn_entry_t;
namespace net = boost::asio;

// handler of signature void(error_code, payload)
template <typename Executor>
class async_move_msg_accept_op : boost::asio::coroutine {
public:
  typedef Executor executor_type;

  async_move_msg_accept_op(
      p_request_t *pl,
      std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue,
      std::shared_ptr<basic_event<executor_type>> ev)
      : pl_(pl), queue_(queue), ev_(ev) {}

  template <typename Self>
  void operator()(Self &self, boost::system::error_code ec = {}) {
    if (ec) {
      self.complete(ec, std::move(*pl_));
      return;
    }

    // initiate async op
    queue_->async_pop(ev_, pl_);

    // wait for payload to arrive
    ev_->async_wait([self = std::move(self),
                     p = pl_](boost::system::error_code ec) mutable {
      self.complete(ec, std::move(*p));
    });
  }

private:
  p_request_t *pl_;
  std::shared_ptr<basic_event<executor_type>> ev_;
  std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue_;
};

template <typename Executor = net::any_io_executor>
class basic_server_connection {
public:
  typedef Executor executor_type;

  basic_server_connection(const executor_type &ex)
      : queue_(), // queue needs to be set on accept
        ev_(std::make_shared<basic_event<executor_type>>(ex)), pl_() {}

  ~basic_server_connection() {
    if (queue_) {
      queue_->cancel(ev_);
    }
  }

  basic_server_connection(const basic_server_connection<executor_type> &) =
      delete;

  basic_server_connection(basic_server_connection<executor_type> &&) = default;

  // accepts a request on this connection
  // handler type void(ec, p_request_t)
  template <typename Token> auto async_accept(Token &&token) {
    assert(queue_);
    assert(!pl_);
    ev_->reset();
    return boost::asio::async_compose<Token, void(boost::system::error_code,
                                                  p_request_t)>(
        async_move_msg_accept_op<executor_type>(&this->pl_, this->queue_,
                                                this->ev_),
        token, this->ev_->get_executor());
  }

  // std::shared_ptr<basic_item_queue<p_request_t, executor_type>> get_queue() {
  //   return queue_;
  // }

  void set_queue(
      std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue) {
    assert(!queue_);
    queue_ = queue;
  }

private:
  std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue_;
  // event used for waiting arriving msg
  std::shared_ptr<basic_event<executor_type>> ev_;
  // payload holder. payload will be passed into handler.
  p_request_t pl_;
};

} // namespace fabricrpc