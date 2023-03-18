// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc_tool/sync_async_context.hpp"

namespace fabricrpc {

sync_async_context::sync_async_context(IFabricAsyncOperationCallback *callback)
    : callback_() {
  // this will add ref
  callback_.copy_from(callback);
  // invoke callback
  callback_->Invoke(this);
}

BOOLEAN STDMETHODCALLTYPE sync_async_context::IsCompleted() { return true; }
BOOLEAN STDMETHODCALLTYPE sync_async_context::CompletedSynchronously() {
  return true;
}
HRESULT STDMETHODCALLTYPE sync_async_context::get_Callback(
    /* [retval][out] */ IFabricAsyncOperationCallback **callback) {
  // need to add ref since the caller will use put.
  callback_.copy_to(callback);
  return S_OK;
}
HRESULT STDMETHODCALLTYPE sync_async_context::Cancel() { return S_OK; }

} // namespace fabricrpc