// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <FabricCommon.h>
#include <atlbase.h>
#include <atlcom.h>

namespace fabricrpc {

// template any context using atl
// it is synchronous
template <typename T>
class AsyncAnyCtx : public CComObjectRootEx<CComMultiThreadModel>,
                    public IFabricAsyncOperationContext {
  BEGIN_COM_MAP(AsyncAnyCtx)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationContext)
  END_COM_MAP()

public:
  AsyncAnyCtx() {}

  void Initialize(IFabricAsyncOperationCallback *callback) {
    callback->AddRef();
    callback_.Attach(callback);

    // invoke callback
    callback->Invoke(this);
  }

  // takes ownership
  void SetContent(T &&content) { content_ = std::move(content); }

  const T &GetContent() { return content_; }

  // impl
  BOOLEAN STDMETHODCALLTYPE IsCompleted() override { return true; }
  BOOLEAN STDMETHODCALLTYPE CompletedSynchronously() override { return true; }
  HRESULT STDMETHODCALLTYPE get_Callback(
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override {
    // add ref???
    *callback = callback_;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Cancel() override { return S_OK; }

private:
  CComPtr<IFabricAsyncOperationCallback> callback_;
  T content_;
};

} // namespace fabricrpc