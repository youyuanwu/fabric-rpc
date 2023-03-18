// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

namespace fabricrpc {

class msg_disposer
    : public winrt::implements<msg_disposer, IFabricTransportMessageDisposer> {
public:
  void STDMETHODCALLTYPE Dispose(
      /* [in] */ ULONG Count,
      /* [size_is][in] */ IFabricTransportMessage **messages) override;
};

} // namespace fabricrpc