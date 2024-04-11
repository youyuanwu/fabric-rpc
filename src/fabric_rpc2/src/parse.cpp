#include "fabricrpc/parse.hpp"
#include "fabricrpc.pb.h"

#include "absl/strings/str_cat.h"

namespace fabricrpc {
// convert
absl::Status status_from_header(const reply_header *h) {
  if (h == nullptr) {
    return absl::InvalidArgumentError("parse_from_header has nullptr");
  }

  int ec_raw = h->status_code();
  absl::StatusCode ec = static_cast<absl::StatusCode>(ec_raw);
  if (ec > absl::StatusCode::
               kDoNotUseReservedForFutureExpansionUseDefaultInSwitchInstead_ ||
      ec < absl::StatusCode::kOk) {
    return absl::InvalidArgumentError(
        absl::StrCat("reply_header has unknown statuscode: ", ec_raw));
  }
  const std::string &message = h->status_message();
  return absl::Status(ec, message);
}

// public api
absl::Status parse_reply_header(const std::string &data) {
  reply_header h;
  absl::Status ec = parse_proto_payload(data, &h);
  if (!ec.ok()) {
    return ec;
  }
  ec = status_from_header(&h);
  return ec;
}

absl::Status parse_proto_payload(const std::string_view data,
                                 google::protobuf::MessageLite *ret) {
  if (ret == nullptr) {
    return absl::InvalidArgumentError("parse_proto_payload has nullptr");
  }
  bool ok = ret->ParseFromString(data);
  if (!ok) {
    return absl::UnknownError("parse_proto_payload failed");
  }
  return absl::OkStatus();
}

absl::Status serialize_proto_payload(const google::protobuf::MessageLite *data,
                                     std::string *ret) {
  if (data == nullptr || ret == nullptr) {
    return absl::InvalidArgumentError("serialize_proto_payload has nullptr");
  }
  bool ok = data->SerializeToString(ret);
  if (!ok) {
    return absl::UnknownError("serialize_proto_payload failed");
  }
  return absl::OkStatus();
}

// returns the url for request
absl::Status parse_request_header(const std::string &data,
                                  std::string &url_ret) {
  fabricrpc::request_header header;
  {
    bool ok = header.ParseFromString(data);
    if (!ok) {
      return absl::InvalidArgumentError("cannot parse request header");
    }
  }
  url_ret = header.url();
  return absl::OkStatus();
}

absl::Status serialize_reply_header(absl::Status st, std::string *ret) {
  // make the transport msg
  fabricrpc::reply_header header;
  header.set_status_code(st.raw_code());
  header.set_status_message(st.message());
  return serialize_proto_payload(&header, ret);
}

} // namespace fabricrpc