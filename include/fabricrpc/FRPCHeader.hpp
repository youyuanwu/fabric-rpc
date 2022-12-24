// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <cassert>
#include <string>

namespace fabricrpc {

class FabricRPCRequestHeader {
public:
  FabricRPCRequestHeader();
  FabricRPCRequestHeader(const std::string &url);
  const std::string &GetUrl() const;
  void SetUrl(std::string url);

private:
  std::string url_;
};

class FabricRPCReplyHeader {
public:
  FabricRPCReplyHeader();
  FabricRPCReplyHeader(int StatusCode, const std::string &StatusMessage);

  int GetStatusCode() const;
  void SetStatusCode(int statusCode);

  const std::string &GetStatusMessage() const;
  void SetStatusMessage(const std::string &StatusMessage);

private:
  int StatusCode_;
  std::string StatusMessage_;
};

// This is needed because we do not want fabric_rpc.lib to have dependency on
// protobuf.lib User should be able to provide the protobuf.lib and link with
// the generated code. Otherwise we will have 2 copies (potentially 2 versions)
// of protobuf.lib in the final executable.
class IFabricRPCHeaderProtoConverter {
public:
  virtual bool SerializeRequestHeader(const FabricRPCRequestHeader *request,
                                      std::string *data) = 0;
  virtual bool SerializeReplyHeader(const FabricRPCReplyHeader *reply,
                                    std::string *data) = 0;
  virtual bool DeserializeRequestHeader(const std::string *data,
                                        FabricRPCRequestHeader *request) = 0;
  virtual bool DeserializeReplyHeader(const std::string *data,
                                      FabricRPCReplyHeader *reply) = 0;

  virtual ~IFabricRPCHeaderProtoConverter() = default;
};

template <typename RequestProto, typename ReplyProto>
class FabricRPCHeaderProtoConverter : public IFabricRPCHeaderProtoConverter {
public:
  // returns true if ok
  bool SerializeRequestHeader(const FabricRPCRequestHeader *request,
                              std::string *data) override {
    assert(request != nullptr);
    assert(data != nullptr);
    RequestProto header;
    header.set_url(request->GetUrl());
    return header.SerializeToString(data);
  }

  bool SerializeReplyHeader(const FabricRPCReplyHeader *reply,
                            std::string *data) override {
    assert(reply != nullptr);
    assert(data != nullptr);
    ReplyProto header;
    header.set_status_code(reply->GetStatusCode());
    header.set_status_message(reply->GetStatusMessage());
    return header.SerializeToString(data);
  }
  bool DeserializeRequestHeader(const std::string *data,
                                FabricRPCRequestHeader *request) override {
    assert(request != nullptr);
    assert(data != nullptr);
    RequestProto header;
    if (!header.ParseFromString(*data)) {
      return false;
    }
    request->SetUrl(header.url());
    return true;
  }
  bool DeserializeReplyHeader(const std::string *data,
                              FabricRPCReplyHeader *reply) override {
    assert(reply != nullptr);
    assert(data != nullptr);
    ReplyProto header;
    if (!header.ParseFromString(*data)) {
      return false;
    }
    reply->SetStatusCode(header.status_code());
    reply->SetStatusMessage(header.status_message());
    return true;
  }
};

} // namespace fabricrpc
