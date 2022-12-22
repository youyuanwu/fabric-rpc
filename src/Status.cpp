// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc/Status.h"

namespace fabricrpc {

Status::Status(StatusCode code, std::string error_message,
               HRESULT transportErrorCode)
    : code_(code), error_message_(std::move(error_message)),
      transportErrorCode_(transportErrorCode) {}

Status::Status(StatusCode code, std::string error_message)
    : Status(code, std::move(error_message), S_OK) {}

Status::Status() : Status(StatusCode::OK, std::string("OK")) {}

StatusCode Status::GetErrorCode() const { return this->code_; }
const std::string &Status::GetErrorMessage() const {
  return this->error_message_;
}

// quick error check
// return true if failed.
Status::operator bool() const { return this->code_ != StatusCode::OK; }

bool Status::IsTransportError() {
  return this->code_ == StatusCode::FABRIC_TRANSPORT_ERROR;
}
HRESULT Status::GetTransportErrorMessage() const {
  return this->transportErrorCode_;
}

} // namespace fabricrpc