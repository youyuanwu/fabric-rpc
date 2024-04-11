#include "fabricrpc/ex_server.hpp"
#include "absl/status/status.h"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "fabricrpc/basic_acceptor.hpp"
#include "fabricrpc/endpoint.hpp"

namespace fabricrpc {

ex_server::ex_server() : md_(), ioc_() {}

void ex_server::add_service(std::shared_ptr<service> svc) {
  md_.add_service(svc);
}

absl::Status ex_server::serve(int port) {
  fabricrpc::endpoint ep(L"localhost", port);
  fabricrpc::basic_acceptor<net::io_context::executor_type> acceptor(
      ioc_.get_executor(), ep);
  boost::system::error_code ec = {};
  std::wstring addr;
  ec = acceptor.open(&addr);
  if (ec) {
    return absl::AbortedError("failed to open acceptor");
  }
  // assert(addr == L"localhost:12345+/");
  std::wcout << L"Listening on: " << addr << std::endl;

  auto listener = [&, this]() -> net::awaitable<void> {
    auto executor = co_await net::this_coro::executor;
    for (;;) {
      // BOOST_TEST_MESSAGE("acceptor.async_accept");

      fabricrpc::basic_server_connection<net::io_context::executor_type> conn =
          co_await acceptor.async_accept_conn(net::use_awaitable);

      auto handle_conn = [c = std::move(conn),
                          this]() mutable -> net::awaitable<void> {
        for (;;) {
          auto executor = co_await net::this_coro::executor;
          // accept request in loop
          fabricrpc::p_request_t pl =
              co_await c.async_accept(net::use_awaitable);
          // std::cout << "acceptor.async_accept finish" << std::endl;

          auto handle_request = [pl = std::move(pl),
                                 this]() mutable -> net::awaitable<void> {
            winrt::com_ptr<IFabricTransportMessage> req;
            pl->get_request_msg(req.put());
            winrt::com_ptr<IFabricTransportMessage> reply;
            co_await md_.execute(req.get(), reply.put());
            pl->complete(S_OK, reply);
          };
          // handle each request
          net::co_spawn(executor, std::move(handle_request), net::detached);
        }
      };
      // handle each connection
      net::co_spawn(executor, std::move(handle_conn), net::detached);
    }
  };

  net::co_spawn(ioc_, listener, net::detached);
  ioc_.run();
  return absl::OkStatus();
}

void ex_server::shutdown() { ioc_.stop(); }

} // namespace fabricrpc