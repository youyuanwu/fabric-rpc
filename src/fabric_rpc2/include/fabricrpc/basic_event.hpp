#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/windows/basic_object_handle.hpp>

// async event

namespace fabricrpc {

namespace net = boost::asio;

template <typename Executor = net::any_io_executor> class basic_event {
public:
  typedef Executor executor_type;

  basic_event(const executor_type &ex) : oh_(ex) {
    HANDLE h = CreateEventA(NULL,  // attr
                            true,  // manual reset
                            false, // initial state
                            NULL   // name
    );
    assert(h != NULL);
    assert(h != INVALID_HANDLE_VALUE);
    oh_.assign(h);
  }

  // token type: void(ec)
  template <typename Token> auto async_wait(Token &&token) {
    return oh_.async_wait(std::move(token));
  }

  void set() {
    HANDLE h = oh_.native_handle();
    [[maybe_unused]] bool ok = SetEvent(h);
    assert(ok);
  }

  void reset() {
    HANDLE h = oh_.native_handle();
    [[maybe_unused]] bool ok = ResetEvent(h);
    assert(ok);
  }

  executor_type get_executor() { return oh_.get_executor(); }

private:
  net::windows::basic_object_handle<executor_type> oh_;
};

} // namespace fabricrpc