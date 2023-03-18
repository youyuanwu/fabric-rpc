// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <FabricCommon.h>
#include <semaphore>
#include <winrt/base.h>

namespace fabricrpc {

MIDL_INTERFACE("5c997f38-a35a-4516-8124-acec5a4e2d5a")
IWaitableCallback : public IFabricAsyncOperationCallback {
public:
  // blocking wait
  virtual void STDMETHODCALLTYPE Wait() = 0;
};

class waitable_callback
    : public winrt::implements<waitable_callback, IWaitableCallback> {
public:
  waitable_callback();
  void Invoke(/* [in] */ IFabricAsyncOperationContext *context) override;

  void Wait() override;

private:
  std::binary_semaphore bs_;
};

} // namespace fabricrpc