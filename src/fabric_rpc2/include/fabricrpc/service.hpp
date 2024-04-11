#pragma once

#include "absl/status/status.h"
#include "boost/asio/awaitable.hpp"

namespace fabricrpc {

namespace net = boost::asio;

class service {
public:
  // returns the first part of the route
  virtual const std::string_view name() = 0;

  virtual net::awaitable<absl::Status> execute(const std::string &url,
                                               const std::string_view req,
                                               std::string *resp) = 0;
};

} // namespace fabricrpc