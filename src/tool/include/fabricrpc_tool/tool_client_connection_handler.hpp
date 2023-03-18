// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "fabrictransport_.h"
#include <winrt/base.h>

namespace fabricrpc {

class tool_client_connection_handler
    : public winrt::implements<tool_client_connection_handler,
                               IFabricTransportClientEventHandler> {
public:
  HRESULT STDMETHODCALLTYPE OnConnected(
      /* [in] */ LPCWSTR connectionAddress) override;

  HRESULT STDMETHODCALLTYPE OnDisconnected(
      /* [in] */ LPCWSTR connectionAddress,
      /* [in] */ HRESULT error) override;
};

} // namespace fabricrpc