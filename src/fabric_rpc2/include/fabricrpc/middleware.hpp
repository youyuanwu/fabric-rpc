#pragma once

#include <fabricrpc/parse.hpp>
#include <fabricrpc/service.hpp>
#include <fabricrpc_tool/tool_transport_msg.hpp>
#include <fabrictransport_.h>
#include <winrt/base.h>

#include <memory>
#include <vector>

namespace fabricrpc {

class middleware {
public:
  middleware() : svc_vec_() {}

  void add_service(std::shared_ptr<service> svc) { svc_vec_.push_back(svc); }

  net::awaitable<void> execute(IFabricTransportMessage *req,
                               IFabricTransportMessage **resp) {
    std::string resp_str;
    absl::Status st = co_await execute_inner(req, &resp_str);
    // return st to clients
    std::string resp_header;
    [[maybe_unused]] absl::Status must_ok =
        fabricrpc::serialize_reply_header(st, &resp_header);
    assert(must_ok.ok());
    winrt::com_ptr<IFabricTransportMessage> msg =
        winrt::make<fabricrpc::tool_transport_msg>(std::move(resp_str),
                                                   std::move(resp_header));
    msg.copy_to(resp);
  }

private:
  net::awaitable<absl::Status> execute_inner(IFabricTransportMessage *req,
                                             std::string *resp_str) {
    absl::Status st;
    // parse header
    std::string url;
    st = fabricrpc::parse_request_header(fabricrpc::get_header(req), url);
    if (!st.ok()) {
      co_return st;
    }
    std::string_view url_view = url;
    if (url.size() == 0) {
      co_return absl::InvalidArgumentError("url is empty");
    }
    if (!url_view.starts_with("/")) {
      co_return absl::InvalidArgumentError("invalid url");
    }

    std::string payload = fabricrpc::get_body(req);
    // TODO: if use returns not found in service we may have routing problem.
    // May need to parse an route by url path
    st = absl::UnimplementedError("url not found");
    for (auto svc = svc_vec_.begin(); svc != svc_vec_.end(); svc++) {
      if (!url_view.substr(1).starts_with((*svc)->name())) {
        continue;
      }
      // found the svc
      st = co_await (*svc)->execute(url, payload, resp_str);
      co_return st;
    }
    co_return st;
  }

  std::vector<std::shared_ptr<service>> svc_vec_;
};

} // namespace fabricrpc