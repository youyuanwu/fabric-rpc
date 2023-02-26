// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

namespace fabricrpc {
// status code that mimicks grpc status code
enum StatusCode {
  OK = 0,
  UNKNOWN = 2,
  INVALID_ARGUMENT = 3,
  DEADLINE_EXCEEDED = 4,
  NOT_FOUND = 5,
  INTERNAL = 13,
  // fabric rpc custom error code

  // If transport error, hresult is stored.
  // This is only used in client, where transport may fail requests.
  FABRIC_TRANSPORT_ERROR = 50,
  DO_NOT_USE = -1
};
} // namespace fabricrpc