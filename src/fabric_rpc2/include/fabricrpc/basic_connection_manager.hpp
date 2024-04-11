#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

#include <fabricrpc/basic_item_queue.hpp>

#include <map>
#include <mutex>

namespace fabricrpc {

template <typename Executor = net::any_io_executor> struct conn_manager_entry {
public:
  typedef Executor executor_type;

  winrt::com_ptr<IFabricTransportClientConnection> conn;
  std::shared_ptr<basic_item_queue<p_request_t, executor_type>> queue;
};

// holds all connections and their msg queues.
template <typename Executor = net::any_io_executor>
class basic_connection_manager {
public:
  typedef Executor executor_type;

  basic_connection_manager() : conns_(), mtx_(), conn_queue_() {}

  ~basic_connection_manager() { assert(conns_.empty()); }

  // add a connection
  // should be immediate
  void
  add_conn(winrt::com_ptr<IFabricTransportClientConnection> conn) noexcept {
    std::wstring key(conn->get_ClientId());

    std::lock_guard<std::mutex> lk(mtx_);
    // TODO: assert conn is unique

    std::shared_ptr<conn_manager_entry<executor_type>> entry =
        std::make_shared<conn_manager_entry<executor_type>>();
    entry->conn = conn;
    entry->queue =
        std::make_shared<basic_item_queue<p_request_t, executor_type>>();
    conns_.insert({key, entry});

    conn_queue_.push(entry);
  }

  // async pop a connection
  // connection still in the conns_ but not in the queue.
  void async_pop(std::shared_ptr<basic_event<executor_type>> event,
                 std::shared_ptr<conn_manager_entry<executor_type>> *out) {
    conn_queue_.async_pop(event, out);
  }

  // should be immediate
  // removes connection in conns_
  // but the queue needs to pop/drain
  void disconnect(std::wstring const &id) noexcept {
    std::lock_guard<std::mutex> lk(mtx_);
    // find the entry
    auto entry = conns_.at(id);
    // erase from conns
    auto it = conns_.find(id);
    assert(it != conns_.end());
    conns_.erase(it);
    // TODO: erase items in entry?
    // TODO: set disconnected error code in conn entry and propagate to user.
    // needs to handle ec in the async_accept_conn before pass the pipe to user.
  }

  // add a request msg.
  // it will be routed to the right connection.
  void post_request(p_request_t &&req) noexcept {
    std::lock_guard<std::mutex> lk(mtx_);
    auto entry = conns_.at(req->get_conn_id());
    entry->queue->push(std::move(req));
  }

  // cancel previous queued async operation
  void cancel(std::shared_ptr<basic_event<executor_type>> event) {
    conn_queue_.cancel(event);
  }

private:
  // keep track of all connections
  std::map<std::wstring, std::shared_ptr<conn_manager_entry<executor_type>>>
      conns_;
  // achieve async
  basic_item_queue<std::shared_ptr<conn_manager_entry<executor_type>>,
                   executor_type>
      conn_queue_;

  std::mutex mtx_;
};

} // namespace fabricrpc