// impl hello client using fabricrpc2

#include "absl/status/status.h"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "fabricrpc.pb.h"
#include "fabricrpc/fabricrpc2.hpp"
#include "fabricrpc_tool/tool_transport_msg.hpp"
#include "helloworld.fabricrpc2.h"

#include <iostream>

namespace net = boost::asio;

int main() {
  boost::system::error_code ec = {};
  net::io_context ioc;
  fabricrpc::endpoint ep(L"localhost", 12345);

  fabricrpc::basic_client_connection<net::io_context::executor_type> conn(
      ioc.get_executor());

  assert(ep.get_url() == L"localhost:12345+/");

  ec = conn.open(ep);
  if (ec.failed()) {
    std::cerr << "open:" << ec.message();
    return EXIT_FAILURE;
  }

  // send requset
  auto send = [&]() -> net::awaitable<absl::Status> {
    fabricrpc::rpc_client<net::io_context::executor_type> rc(conn);
    helloworld::FabricHelloClient<net::io_context::executor_type> hc(rc);
    helloworld::FabricRequest body;
    body.set_fabricname("myname");
    helloworld::FabricResponse reply_body;

    absl::Status st;
    try {
      st = co_await hc.SayHello(&body, &reply_body, net::use_awaitable);
    } catch (const std::exception &e) {
      st = absl::UnknownError(std::string("coro has exception ") + e.what());
      co_return st;
    }

    std::string reply_content = reply_body.fabricmessage();
    std::cout << "Reply: " << reply_content << std::endl;
    co_return st;
  };

  auto f = [&]() -> net::awaitable<void> {
    auto status = co_await send();
    if (!status.ok()) {
      std::cerr << "send failed:" << status.ToString();
    }
  };

  net::co_spawn(ioc, f, net::detached);
  // net::co_spawn(ioc, f, net::detached);

  ioc.run();
}