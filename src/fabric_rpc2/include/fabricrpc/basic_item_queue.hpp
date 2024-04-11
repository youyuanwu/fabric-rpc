#pragma once

#include "fabricrpc/basic_event.hpp"
#include <fabrictransport_.h>
#include <winrt/base.h>

#include "fabricrpc/request.hpp"
#include <mutex>

namespace fabricrpc {

template <typename T, typename Executor = net::any_io_executor>
struct queue_entry {
  typedef Executor executor_type;

  std::shared_ptr<basic_event<executor_type>> event;
  T *item;
};

// queue for keeping items with notifying events.
template <typename T, typename Executor = net::any_io_executor>
class basic_item_queue {
public:
  typedef Executor executor_type;

  basic_item_queue() : items_(), queue_entries_(), mtx_() {}
  ~basic_item_queue() {
    assert(items_.empty());
    assert(queue_entries_.empty());
  }

  void push(T item) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (queue_entries_.empty()) {
      // no waiters so push to queue
      items_.push_back(std::move(item));
    } else {
      // directly finish a waiter.
      queue_entry<T, executor_type> e = std::move(queue_entries_.back());
      queue_entries_.pop_back();
      *e.item = std::move(item);
      e.event->set();
    }
  }

  // return item into outptr. and set the event when done.
  // so msgout needs to be valid until event is invoked.
  void async_pop(std::shared_ptr<basic_event<executor_type>> event, T *msgout) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (items_.empty()) {
      // no ready item, push the event
      queue_entries_.push_back({std::move(event), msgout});
    } else {
      // has item, so directly fill the item and invoke event
      T item = std::move(items_.back());
      items_.pop_back();
      *msgout = std::move(item);
      event->set();
    }
  }

  // cancels the waiting event entry
  // only support cancel tail for now
  void cancel(std::shared_ptr<basic_event<executor_type>> event) {
    if (queue_entries_.empty()) {
      return;
    }
    auto &e = queue_entries_.back();
    if (e.event.get() == event.get()) {
      // same event then removes it.
      queue_entries_.pop_back();
    } else {
      // nothing to cancel
    }
  }

private:
  std::vector<T> items_;
  std::vector<queue_entry<T, executor_type>> queue_entries_;
  std::mutex mtx_;
};

} // namespace fabricrpc