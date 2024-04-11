#pragma once
// helper to parse status and payload.

#include "fabricrpc/proto_forward.hpp"

#include "absl/status/status.h"

#include "boost/asio/awaitable.hpp"

namespace fabricrpc {

namespace net = boost::asio;

class reply_header;

absl::Status parse_reply_header(const std::string &data);

absl::Status parse_proto_payload(const std::string_view data,
                                 google::protobuf::MessageLite *ret);

absl::Status parse_request_header(const std::string &data,
                                  std::string &url_ret);

absl::Status serialize_proto_payload(const google::protobuf::MessageLite *data,
                                     std::string *ret);

absl::Status serialize_reply_header(absl::Status st, std::string *ret);

template <typename ReqProto, typename ReplyProto, typename HandlerFunc,
          typename Service>
net::awaitable<absl::Status>
codegen_handler_helper(const std::string_view req, std::string *resp,
                       HandlerFunc fn, Service svc) {
  ReqProto p1;
  ReplyProto p2;
  absl::Status st = fabricrpc::parse_proto_payload(req, &p1);
  if (!st.ok()) {
    co_return st;
  }
  st = co_await (svc->*fn)(&p1, &p2);

  if (!st.ok()) {
    co_return st;
  }
  co_return fabricrpc::serialize_proto_payload(&p2, resp);
}

} // namespace fabricrpc