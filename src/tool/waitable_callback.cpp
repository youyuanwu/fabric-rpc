// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/waitable_callback.hpp"

namespace fabricrpc {

waitable_callback::waitable_callback() : bs_(0) {}

void waitable_callback::Invoke(
    /* [in] */ IFabricAsyncOperationContext *context) {
  bs_.release();
}

void waitable_callback::Wait() { bs_.acquire(); }

} // namespace fabricrpc