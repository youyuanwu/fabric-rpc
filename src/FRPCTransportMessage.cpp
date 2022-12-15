// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc/FRPCTransportMessage.h"
#include <cassert>

namespace fabricrpc {

std::string FRPCTransportMessage::get_header(IFabricTransportMessage *message) {
  if (message == nullptr) {
    return "";
  }
  const FABRIC_TRANSPORT_MESSAGE_BUFFER *headerbuf = {};
  const FABRIC_TRANSPORT_MESSAGE_BUFFER *msgbuf = {};
  ULONG msgcount = 0;
  message->GetHeaderAndBodyBuffer(&headerbuf, &msgcount, &msgbuf);
  if (headerbuf == nullptr) {
    return "";
  }
  return std::string(headerbuf->Buffer,
                     headerbuf->Buffer + headerbuf->BufferSize);
}

// concat all body chunks to one
std::string FRPCTransportMessage::get_body(IFabricTransportMessage *message) {
  if (message == nullptr) {
    return "";
  }
  const FABRIC_TRANSPORT_MESSAGE_BUFFER *headerbuf = {};
  const FABRIC_TRANSPORT_MESSAGE_BUFFER *msgbuf = {};
  ULONG msgcount = 0;
  message->GetHeaderAndBodyBuffer(&headerbuf, &msgcount, &msgbuf);

  std::string body;
  for (std::size_t i = 0; i < msgcount; i++) {
    const FABRIC_TRANSPORT_MESSAGE_BUFFER *msg_i = msgbuf + i;
    std::string msg_str(msg_i->Buffer, msg_i->Buffer + msg_i->BufferSize);
    body += msg_str;
  }
  return body;
}

FRPCTransportMessage::FRPCTransportMessage() : body_(), header_() {}

void FRPCTransportMessage::Initialize(std::string header, std::string body) {
  header_ = std::move(header);
  body_ = std::move(body);
  // prepare ret pointers
  header_ret_.Buffer = (BYTE *)header_.c_str();
  header_ret_.BufferSize = static_cast<ULONG>(header_.size());
  body_ret_.Buffer = (BYTE *)body_.c_str();
  body_ret_.BufferSize = static_cast<ULONG>(body_.size());
}

// copy content from another msg
// if msg blob has multiple parts, this will concat all msg blobs into one
void FRPCTransportMessage::CopyFrom(IFabricTransportMessage *other) {
  std::string body = get_body(other);
  std::string header = get_header(other);
  this->Initialize(std::move(header), std::move(body));
}

const std::string &FRPCTransportMessage::GetHeader() { return this->header_; }

const std::string &FRPCTransportMessage::GetBody() { return this->body_; }

// IFabricTransportMessage impl

void FRPCTransportMessage::GetHeaderAndBodyBuffer(
    /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **headerBuffer,
    /* [out] */ ULONG *msgBufferCount,
    /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **MsgBuffers) {
  assert(body_ret_.Buffer != nullptr && header_ret_.Buffer != nullptr);
  *headerBuffer = &header_ret_;
  *msgBufferCount = 1;
  *MsgBuffers = &body_ret_;
}

void FRPCTransportMessage::Dispose() {}

} // namespace fabricrpc