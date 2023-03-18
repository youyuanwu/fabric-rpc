// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

#include <string>

namespace fabricrpc {

// adapted from service-fabric-cpp

class tool_transport_msg
    : public winrt::implements<tool_transport_msg, IFabricTransportMessage> {
public:
  tool_transport_msg(std::string body, std::string headers);

  void STDMETHODCALLTYPE GetHeaderAndBodyBuffer(
      /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **headerBuffer,
      /* [out] */ ULONG *msgBufferCount,
      /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **MsgBuffers) override;

  void STDMETHODCALLTYPE Dispose(void) override;

private:
  std::string body_;
  FABRIC_TRANSPORT_MESSAGE_BUFFER body_ret_;
  std::string headers_;
  FABRIC_TRANSPORT_MESSAGE_BUFFER headers_ret_;
};

std::string get_header(IFabricTransportMessage *message);
// concat all body chunks to one
std::string get_body(IFabricTransportMessage *message);

} // namespace fabricrpc