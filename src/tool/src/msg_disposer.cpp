// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/msg_disposer.hpp"

namespace fabricrpc {

void STDMETHODCALLTYPE msg_disposer::Dispose(
    /* [in] */ ULONG Count,
    /* [size_is][in] */ IFabricTransportMessage **messages) {
  UNREFERENCED_PARAMETER(Count);
  UNREFERENCED_PARAMETER(messages);
}

} // namespace fabricrpc