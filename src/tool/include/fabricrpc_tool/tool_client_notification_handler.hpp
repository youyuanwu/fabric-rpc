
// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "fabrictransport_.h"
#include <winrt/base.h>

namespace fabricrpc {

// server handle request
class tool_client_notification_handler
    : public winrt::implements<tool_client_notification_handler,
                               IFabricTransportCallbackMessageHandler> {
public:
  virtual HRESULT STDMETHODCALLTYPE HandleOneWay(
      /* [in] */ IFabricTransportMessage *message) override;
};

} // namespace fabricrpc