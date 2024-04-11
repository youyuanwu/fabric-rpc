#pragma once

#include "boost/asio/any_io_executor.hpp"
#include "fabricrpc.pb.h"
#include "fabricrpc/basic_client_connection.hpp"
#include "fabricrpc/parse.hpp"
#include "fabricrpc/proto_forward.hpp"
#include "fabricrpc_tool/tool_transport_msg.hpp"

namespace fabricrpc {

namespace net = boost::asio;

// signature: void(ec, absl::Status)
template <typename Executor> class async_rpc_op : boost::asio::coroutine {
public:
  typedef Executor executor_type;

  async_rpc_op(fabricrpc::basic_client_connection<executor_type> &conn,
               const std::string url, google::protobuf::MessageLite *request,
               google::protobuf::MessageLite *reply)
      : conn_(conn), url_(url), request_(request), reply_(reply) {}

  template <typename Self>
  void operator()(Self &self, boost::system::error_code ec = {}) {
    if (ec) {
      self.complete(ec, {});
      return;
    }

    // make message
    fabricrpc::request_header header;
    header.set_url(url_);
    // body
    winrt::com_ptr<IFabricTransportMessage> req =
        winrt::make<fabricrpc::tool_transport_msg>(
            request_->SerializeAsString(), header.SerializeAsString());

    // to be filled
    google::protobuf::MessageLite *proto_reply = reply_;
    conn_.async_send(
        req, [self = std::move(self), proto_reply](
                 boost::system::error_code ec,
                 winrt::com_ptr<IFabricTransportMessage> reply) mutable {
          if (ec.failed()) {
            self.complete(ec, {});
            return;
          }
          absl::Status st =
              fabricrpc::parse_reply_header(fabricrpc::get_header(reply.get()));
          if (!st.ok()) {
            self.complete({}, st);
            return;
          }
          st = fabricrpc::parse_proto_payload(fabricrpc::get_body(reply.get()),
                                              proto_reply);
          self.complete({}, st);
        });
  }

private:
  fabricrpc::basic_client_connection<executor_type> &conn_;
  const std::string url_; // takes ownership
  google::protobuf::MessageLite *request_;
  google::protobuf::MessageLite *reply_;
};

template <typename Executor = net::any_io_executor> class rpc_client {
public:
  typedef Executor executor_type;

  rpc_client(fabricrpc::basic_client_connection<executor_type> &conn)
      : conn_(conn) {}

  // handler void(ec, absl::Status)
  // reply pointer needs to be valid
  template <typename Token>
  auto async_send(const std::string &url,
                  google::protobuf::MessageLite *request,
                  google::protobuf::MessageLite *reply, Token &&token) {
    return boost::asio::async_compose<Token, void(boost::system::error_code,
                                                  absl::Status)>(
        async_rpc_op<executor_type>(conn_, url, request, reply),
        std::move(token), this->conn_.get_executor());
  }

private:
  fabricrpc::basic_client_connection<executor_type> &conn_;
};

} // namespace fabricrpc