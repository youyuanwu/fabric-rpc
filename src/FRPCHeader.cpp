#include "fabricrpc/FRPCHeader.h"

namespace fabricrpc {

FabricRPCRequestHeader::FabricRPCRequestHeader() : url_() {}

FabricRPCRequestHeader::FabricRPCRequestHeader(const std::string &url)
    : url_(url) {}

const std::string &FabricRPCRequestHeader::GetUrl() const { return url_; }

void FabricRPCRequestHeader::SetUrl(std::string url) { url_ = std::move(url); }

FabricRPCReplyHeader::FabricRPCReplyHeader() : FabricRPCReplyHeader(0, "") {}

FabricRPCReplyHeader::FabricRPCReplyHeader(int StatusCode,
                                           const std::string &StatusMessage)
    : StatusCode_(StatusCode), StatusMessage_(StatusMessage) {}

int FabricRPCReplyHeader::GetStatusCode() const { return StatusCode_; }
void FabricRPCReplyHeader::SetStatusCode(int statusCode) {
  StatusCode_ = statusCode;
}

const std::string &FabricRPCReplyHeader::GetStatusMessage() const {
  return StatusMessage_;
}

void FabricRPCReplyHeader::SetStatusMessage(const std::string &StatusMessage) {
  StatusMessage_ = StatusMessage;
}

} // namespace fabricrpc