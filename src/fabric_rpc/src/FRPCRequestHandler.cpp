// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabricrpc/FRPCRequestHandler.hpp"
#include "fabricrpc/FRPCTransportMessage.hpp"
#include "fabricrpc/Operation.hpp"
#include "fabricrpc/exp/AsyncAnyContext.hpp"

#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

namespace fabricrpc {

// router holds all svcs
class FRPCRouter : public MiddleWare {
public:
  FRPCRouter() : svcList_() {}
  void AddSvc(std::shared_ptr<MiddleWare> svc) { svcList_.push_back(svc); }

  fabricrpc::Status
  Route(const std::string &url,
        std::unique_ptr<fabricrpc::IBeginOperation> &beginOp,
        std::unique_ptr<fabricrpc::IEndOperation> &endOp) override {
    for (auto &svc : svcList_) {
      // TODO: can change return type of inner route
      fabricrpc::Status ec = svc->Route(url, beginOp, endOp);
      if (!ec) {
        return fabricrpc::Status();
      }
    }
    // no route found
    return fabricrpc::Status(fabricrpc::StatusCode::NOT_FOUND,
                             "method not found: " + url);
  }

private:
  std::vector<std::shared_ptr<MiddleWare>> svcList_;
};

// payload to carry around between proccess request begin and end.
struct ctxPayload {
  // User's end operation
  std::unique_ptr<IEndOperation> endOp;
  // User's begin operation status or fabric rpc parsing status
  // if failed the end operation will send a default error msg back to client.
  fabricrpc::Status beginStatus;
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
using trCtx = fabricrpc::AsyncAnyCtx<std::unique_ptr<ctxPayload>>;

// This is the callback to be invoked by user's begin operation.
// It contains the Processing request callback from transport, and will trigger
// it in a chain. This callback is a proxy for the transport's callback.
class FRPCOperationCallback : public CComObjectRootEx<CComMultiThreadModel>,
                              public IFabricAsyncOperationCallback {

  BEGIN_COM_MAP(FRPCOperationCallback)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationCallback)
  END_COM_MAP()
public:
  // save the callback from transport begine process request.
  // wrapCtx is the ctx to invoke the transport callback.
  void Initialize(IFabricAsyncOperationCallback *callback,
                  CComObjectNoLock<trCtx> *wrapCtx) {
    assert(callback != nullptr);
    assert(wrapCtx != nullptr);

    callback->AddRef();
    transportCallback_.Attach(callback);
    wrapCtx->AddRef();
    wrapCtx_.Attach(wrapCtx);
  }

  // User will invoke this callback here, but we just through and invoke the
  // transport one.
  void STDMETHODCALLTYPE Invoke(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    assert(context != nullptr);
    assert(transportCallback_ != nullptr);
    assert(wrapCtx_ != nullptr);
    wrapCtx_->GetContent()->SetInnerCtx(context);
    transportCallback_->Invoke(wrapCtx_);
    // User may have this callback in the context and form a circular refcount.
    // so release the wrapCtx to break it.
    wrapCtx_.Release();
  }

private:
  CComPtr<IFabricAsyncOperationCallback> transportCallback_;
  CComPtr<CComObjectNoLock<trCtx>> wrapCtx_;
};

FRPCRequestHandler::FRPCRequestHandler() : svc_(), cv_() {}

void FRPCRequestHandler::Initialize(
    const std::vector<std::shared_ptr<MiddleWare>> &svcList,
    std::shared_ptr<IFabricRPCHeaderProtoConverter> cv) {
  assert(svcList.size() != 0);
  assert(cv != nullptr);
  std::shared_ptr<FRPCRouter> router = std::make_shared<FRPCRouter>();
  for (auto svc : svcList) {
    router->AddSvc(svc);
  }
  svc_ = router;
  cv_ = cv;
}

HRESULT STDMETHODCALLTYPE FRPCRequestHandler::BeginProcessRequest(
    /* [in] */ COMMUNICATION_CLIENT_ID clientId,
    /* [in] */ IFabricTransportMessage *message,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) {
  UNREFERENCED_PARAMETER(clientId);

  assert(svc_ != nullptr); // must initialize

  // calculate time spent parsing headers, and substract from timeout passed to
  // user handlers.
  auto starttime = std::chrono::steady_clock::now();

  // copy the message to fabric rpc implementation
  CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(
      new CComObjectNoLock<FRPCTransportMessage>());
  msgPtr->CopyMsg(message);

  const std::string &body = msgPtr->GetBody();
  const std::string &header = msgPtr->GetHeader();

  Status err; // The error to be sent back to client
  // context to be returned by the begin operation
  CComPtr<IFabricAsyncOperationContext> ctx;
  // begin and end ops to be looked up by routing.
  std::unique_ptr<IBeginOperation> beginOp;
  std::unique_ptr<IEndOperation> endOp;

  fabricrpc::FabricRPCRequestHeader fRequestHeader;

  // ctx to be returned to caller
  CComPtr<CComObjectNoLock<trCtx>> retCtx(new CComObjectNoLock<trCtx>());
  retCtx->SetContent(std::make_unique<ctxPayload>());

  CComPtr<CComObjectNoLock<FRPCOperationCallback>> frpcCallback(
      new CComObjectNoLock<FRPCOperationCallback>());
  frpcCallback->Initialize(callback, retCtx);

  if (header.size() == 0) {
    err = Status(StatusCode::INVALID_ARGUMENT, "fabric rpc header is empty");
  } else {
    bool ok = cv_->DeserializeRequestHeader(&header, &fRequestHeader);
    if (!ok) {
      err = Status(StatusCode::INVALID_ARGUMENT,
                   "Cannot parse fabric rpc header");
    } else {
      std::string url = fRequestHeader.GetUrl();
      err = svc_->Route(url, beginOp, endOp);
      if (!err) {
        assert(endOp != nullptr);
        retCtx->GetContent()->endOp = std::move(endOp);

        // prepare timeout value
        auto endtime = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      endtime - starttime)
                      .count();
        DWORD newTimeout = {};
        if (timeoutMilliseconds > ms) {
          newTimeout = timeoutMilliseconds - static_cast<DWORD>(ms);
        }
        //  invoke begin op
        err = beginOp->Invoke(body, newTimeout, frpcCallback, &ctx);
        if (!err) {
          retCtx->GetContent()->SetInnerCtx(ctx);
          // return a ctx and done.
          *context = retCtx.Detach();
          return S_OK;
        }
      }
    }
  }

  // some steps failed above. There is no ctx and frpcCallback is not invoked.
  assert(ctx == nullptr);
  assert(err);
  retCtx->GetContent()->beginStatus = err;

  // make a dummy sync innerctx and invoke callback.
  CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>> dummyCtx(
      new CComObjectNoLock<fabricrpc::AsyncAnyCtx<bool>>());
  dummyCtx->Initialize(frpcCallback);
  frpcCallback->Invoke(dummyCtx);

  *context = retCtx.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE FRPCRequestHandler::EndProcessRequest(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricTransportMessage **reply) {

  // get the packed additional info
  CComObjectNoLock<trCtx> *ctxWrap =
      dynamic_cast<CComObjectNoLock<trCtx> *>(context);
  assert(ctxWrap != nullptr);

  const std::unique_ptr<fabricrpc::ctxPayload> &ctxPayload =
      ctxWrap->GetContent();

  fabricrpc::FabricRPCReplyHeader fReplyHeader;
  std::string reply_str;

  Status const &beginErr = ctxPayload->beginStatus;
  std::unique_ptr<IEndOperation> const &end = ctxPayload->endOp;
  CComPtr<IFabricAsyncOperationContext> &innerCtx = ctxPayload->innerCtx;
  if (beginErr) {
    // we send the err back to client in header while body is empty.
    fReplyHeader.SetStatusCode(beginErr.GetErrorCode());
    fReplyHeader.SetStatusMessage(beginErr.GetErrorMessage());
  } else {
    assert(end != nullptr);
    assert(innerCtx != nullptr);
    Status err;
    // invoke user end operation
    err = end->Invoke(innerCtx, reply_str);
    if (err) {
      reply_str.clear();
    }
    assert(err.GetErrorMessage().size() !=
           0); // message must not be empty, or proto serialization fails.
    // prepare header
    fReplyHeader.SetStatusCode(err.GetErrorCode());
    fReplyHeader.SetStatusMessage(err.GetErrorMessage());
  }
  std::string h_response_str;
  bool ok = cv_->SerializeReplyHeader(&fReplyHeader, &h_response_str);
  assert(ok);
  DBG_UNREFERENCED_LOCAL_VARIABLE(ok);

  // create com msg
  CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(
      new CComObjectNoLock<FRPCTransportMessage>());
  msgPtr->Initialize(std::move(h_response_str), std::move(reply_str));
  *reply = msgPtr.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE FRPCRequestHandler::HandleOneWay(
    /* [in] */ COMMUNICATION_CLIENT_ID clientId,
    /* [in] */ IFabricTransportMessage *message) {
  UNREFERENCED_PARAMETER(clientId);
  UNREFERENCED_PARAMETER(message);
  return S_OK;
}
} // namespace fabricrpc