#pragma once

#include "absl/status/status.h"
#include "boost/asio/io_context.hpp"
#include "fabricrpc/middleware.hpp"

// experimental server impl

namespace fabricrpc {

namespace net = boost::asio;

class ex_server {
public:
  ex_server();

  void add_service(std::shared_ptr<service> svc);

  // run the server and block the thread.
  absl::Status serve(int port);

  // stop the server
  void shutdown();

private:
  fabricrpc::middleware md_;
  net::io_context ioc_;
};

} // namespace fabricrpc