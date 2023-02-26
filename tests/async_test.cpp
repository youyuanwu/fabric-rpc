// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include <boost/test/unit_test.hpp>

#include <FabricCommon.h>
#include <atlbase.h>
#include <atlcom.h>

#include <fabricrpc/exp/AsyncAnyContext.hpp>
#include <servicefabric/waitable_callback.hpp>

#include <chrono>
#include <future> // for async
#include <thread> // for thread sleep

namespace sf = servicefabric;

// payload to carry around between proccess request begin and end.
struct ctxPayload {
  // User's end operation
  // std::unique_ptr<IEndOperation> endOp;
  // User's begin operation status or fabric rpc parsing status
  // if failed the end operation will send a default error msg back to client.
  HRESULT beginStatus;
  // User's ctx to be passed in to user's end operation.
  CComPtr<IFabricAsyncOperationContext> innerCtx;
  std::mutex mtx_;

  // set innerCtx thread safe
  // This can be called on BeginProcessRequest thread and
  // the thread user choose to invoke the callback.
  void SetInnerCtx(IFabricAsyncOperationContext *context) {
    std::lock_guard<std::mutex> l(mtx_);
    if (innerCtx == nullptr) {
      context->AddRef();
      innerCtx.Attach(context);
    } else {
      assert(innerCtx == context);
    }
  }
};

// This is the ctx type passed from transport begin process request
// to end process request.
using composeCtx = fabricrpc::AsyncAnyCtx<std::unique_ptr<ctxPayload>>;

// This is the callback to be invoked by user's begin operation.
//
class ComposeCallback : public CComObjectRootEx<CComMultiThreadModel>,
                        public IFabricAsyncOperationCallback {

  BEGIN_COM_MAP(ComposeCallback)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationCallback)
  END_COM_MAP()
public:
  // save the callback from transport begine process request.
  // wrapCtx is the ctx to invoke the transport callback.
  void Initialize(IFabricAsyncOperationCallback *callback,
                  CComObjectNoLock<composeCtx> *wrapCtx) {
    assert(callback != nullptr);
    assert(wrapCtx != nullptr);

    callback->AddRef();
    outerCallback_.Attach(callback);
    wrapCtx->AddRef();
    wrapCtx_.Attach(wrapCtx);
  }

  // User will invoke this callback here, but we just through and invoke the
  // transport one.
  void STDMETHODCALLTYPE Invoke(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    assert(context != nullptr);
    assert(outerCallback_ != nullptr);
    assert(wrapCtx_ != nullptr);
    wrapCtx_->GetContent()->SetInnerCtx(context);
    outerCallback_->Invoke(wrapCtx_);
    // User may have this callback in the context and form a circular refcount.
    // so release the wrapCtx to break it.
    wrapCtx_.Release();
  }

private:
  CComPtr<IFabricAsyncOperationCallback> outerCallback_;
  CComPtr<CComObjectNoLock<composeCtx>> wrapCtx_;
};

// an attempt to compose async ops

// This async op copies request to response and goes through async framework.
template <typename T> class async_op {
public:
  HRESULT Begin(const T *request, IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) {
    UNREFERENCED_PARAMETER(request);
    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<T>>> dummyCtx(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<T>>());
    dummyCtx->Initialize(callback);

    T content = *request;
    dummyCtx->SetContent(std::move(content));

    // invoke callback in another thread.
    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<T>>> ctxCpy;
    dummyCtx.CopyTo(&ctxCpy);
    std::thread t([cpy = std::move(ctxCpy)] {
      // todo: ramdomize
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      // get_callback will not add ref
      IFabricAsyncOperationCallback *ctxCallback;
      cpy->get_Callback(&ctxCallback);
      assert(ctxCallback != nullptr);
      ctxCallback->Invoke(cpy);
    });
    t.detach();

    *context = dummyCtx.Detach();
    return S_OK;
  }
  HRESULT End(IFabricAsyncOperationContext *context, /*out*/ T *response) {
    CComObjectNoLock<fabricrpc::AsyncAnyCtx<T>> *ctxCast =
        dynamic_cast<CComObjectNoLock<fabricrpc::AsyncAnyCtx<T>> *>(context);
    assert(ctxCast != nullptr);

    bool content = ctxCast->GetContent();
    *response = content;
    return S_OK;
  }
};

using async_bool_op = async_op<bool>;
using async_string_op = async_op<std::string>;
using async_int_op = async_op<int>;

// try to compose an operation that takes a bool and returns the string of the
// bool.

/*
Begin1
Begin2
Begin3
End3
End2
End1
*/

// T is input type, V is output type
template <typename T, typename V> class async_compose_op2 {
public:
  async_compose_op2(
      std::function<HRESULT(const T *, IFabricAsyncOperationCallback *,
                            /*out*/ IFabricAsyncOperationContext **)>
          beginOp,
      std::function<HRESULT(IFabricAsyncOperationContext *,
                            /*out*/ V *)>
          endOp)
      : beginOp_(beginOp), endOp_(endOp) {}

  HRESULT Begin(const T *request, IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) noexcept {
    // context to be returned by the inner begin operation
    CComPtr<IFabricAsyncOperationContext> ctx;
    // ctx to be returned to caller
    CComPtr<CComObjectNoLock<composeCtx>> retCtx(
        new CComObjectNoLock<composeCtx>());
    retCtx->SetContent(std::make_unique<ctxPayload>());

    // compose/wrapper callback to be passed in inner begin
    CComPtr<CComObjectNoLock<ComposeCallback>> cCb(
        new CComObjectNoLock<ComposeCallback>());
    cCb->Initialize(callback, retCtx);

    HRESULT hr = beginOp_(request, cCb, &ctx);

    // inner begin success then just return the ctx.
    if (hr == S_OK) {
      retCtx->GetContent()->SetInnerCtx(ctx);
      *context = retCtx.Detach();
      return S_OK;
    }

    // set the error in the wrapper ctx and pass to the end fn.
    retCtx->GetContent()->beginStatus = hr;
    // invoke callback
    // make a dummy sync innerctx and invoke callback, so that the end op is
    // synchronously triggered
    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>> dummyCtx(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>());
    dummyCtx->Initialize(cCb);
    cCb->Invoke(dummyCtx);

    *context = retCtx.Detach();
    return S_OK;
  }

  HRESULT End(IFabricAsyncOperationContext *context,
              /*out*/ V *response) {
    CComObjectNoLock<composeCtx> *ctxCast =
        dynamic_cast<CComObjectNoLock<composeCtx> *>(context);
    assert(ctxCast != nullptr);

    const std::unique_ptr<ctxPayload> &ctxPayload = ctxCast->GetContent();

    if (ctxPayload->beginStatus != S_OK) {
      return ctxPayload->beginStatus;
    } else {
      bool resp = {};
      HRESULT hr = endOp_(ctxPayload->innerCtx, &resp);
      if (hr != S_OK) {
        return hr;
      }
      *response = resp ? "true" : "false";
      return S_OK;
    }
  }

private:
  async_bool_op bool_op_;
  std::function<HRESULT(const T *, IFabricAsyncOperationCallback *,
                        /*out*/ IFabricAsyncOperationContext **)>
      beginOp_;
  std::function<HRESULT(IFabricAsyncOperationContext *,
                        /*out*/ V *)>
      endOp_;
};

class async_compose_op {
public:
  HRESULT Begin(const bool *request, IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) noexcept {
    // context to be returned by the inner begin operation
    CComPtr<IFabricAsyncOperationContext> ctx;
    // ctx to be returned to caller
    CComPtr<CComObjectNoLock<composeCtx>> retCtx(
        new CComObjectNoLock<composeCtx>());
    retCtx->SetContent(std::make_unique<ctxPayload>());

    // compose/wrapper callback to be passed in inner begin
    CComPtr<CComObjectNoLock<ComposeCallback>> cCb(
        new CComObjectNoLock<ComposeCallback>());
    cCb->Initialize(callback, retCtx);

    HRESULT hr = bool_op_.Begin(request, cCb, &ctx);

    // inner begin success then just return the ctx.
    if (hr == S_OK) {
      retCtx->GetContent()->SetInnerCtx(ctx);
      *context = retCtx.Detach();
      return S_OK;
    }

    // set the error in the wrapper ctx and pass to the end fn.
    retCtx->GetContent()->beginStatus = hr;
    // invoke callback
    // make a dummy sync innerctx and invoke callback, so that the end op is
    // synchronously triggered
    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>> dummyCtx(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>());
    dummyCtx->Initialize(cCb);
    cCb->Invoke(dummyCtx);

    *context = retCtx.Detach();
    return S_OK;
  }

  HRESULT End(IFabricAsyncOperationContext *context,
              /*out*/ std::string *response) {
    CComObjectNoLock<composeCtx> *ctxCast =
        dynamic_cast<CComObjectNoLock<composeCtx> *>(context);
    assert(ctxCast != nullptr);

    const std::unique_ptr<ctxPayload> &ctxPayload = ctxCast->GetContent();

    if (ctxPayload->beginStatus != S_OK) {
      return ctxPayload->beginStatus;
    } else {
      bool resp = {};
      HRESULT hr = bool_op_.End(ctxPayload->innerCtx, &resp);
      if (hr != S_OK) {
        return hr;
      }
      *response = resp ? "true" : "false";
      return S_OK;
    }
  }

private:
  async_bool_op bool_op_;
  async_string_op string_op_;
};

BOOST_AUTO_TEST_SUITE(test_fabric_async)

BOOST_AUTO_TEST_CASE(async_test) {
  HRESULT hr = S_OK;
  {
    async_bool_op bool_op;

    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

    bool request = true;
    hr = bool_op.Begin(&request, callback.get(), ctx.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    callback->Wait();

    bool response = false;
    hr = bool_op.End(ctx.get(), &response);
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    BOOST_CHECK_EQUAL(request, response);
  }

  {
    async_compose_op bool_op;

    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

    bool request = true;
    hr = bool_op.Begin(&request, callback.get(), ctx.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    callback->Wait();

    std::string response;
    hr = bool_op.End(ctx.get(), &response);
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    BOOST_CHECK_EQUAL("true", response);
  }
}

BOOST_AUTO_TEST_SUITE_END()
