// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "fabricrpc/StatusCode.hpp"
#include "winerror.h"

#include <string>

namespace fabricrpc {

class Status {
public:
  Status(StatusCode code, std::string error_message,
         HRESULT transportErrorCode);
  Status(StatusCode code, std::string error_message);

  Status();
  StatusCode GetErrorCode() const;
  const std::string &GetErrorMessage() const;
  // quick error check
  // return true if failed.
  operator bool() const;

  bool IsTransportError();
  HRESULT GetTransportErrorMessage() const;

private:
  StatusCode code_;
  std::string error_message_;
  HRESULT transportErrorCode_;
};

} // namespace fabricrpc