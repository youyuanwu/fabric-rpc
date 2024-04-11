#include "absl/status/status.h"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "fabricrpc/ex_server.hpp"
#include "fabricrpc/fabricrpc2.hpp"
#include "helloworld.fabricrpc2.h"

#include <iostream>

namespace net = boost::asio;

class fabric_hello_impl : public helloworld::FabricHello {
public:
  net::awaitable<absl::Status>
  SayHello(helloworld::FabricRequest *request,
           helloworld::FabricResponse *resp) override {
    std::string reply = "Hello" + request->fabricname();
    absl::Status st = absl::OkStatus();
    resp->set_fabricmessage(reply);
    co_return absl::OkStatus();
  }
};

absl::Status amain() {
  fabricrpc::ex_server svr;
  std::shared_ptr<fabricrpc::service> svc =
      std::make_shared<fabric_hello_impl>();
  svr.add_service(svc);
  return svr.serve(12345);
}

int main() {
  absl::Status st = amain();
  if (!st.ok()) {
    std::cerr << "Failed: " << st.ToString() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}