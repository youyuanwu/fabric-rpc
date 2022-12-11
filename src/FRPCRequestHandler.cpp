// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "fabricrpc/FRPCRequestHandler.h"
#include "fabricrpc/Operation.h"
#include "fabricrpc/FRPCTransportMessage.h"

#include <fabricrpc.pb.h>

#include <cassert>
#include <memory>

namespace fabricrpc{

// The synchronous implementation of async context
// it shuold executes callbacks synchronously.
// User needs to manually invoke callback out side the ctx.
// This is used in fabric rpc to bailout failures.
class SyncAsyncOperationContext : 
  public CComObjectRootEx<CComSingleThreadModel>,
  public IFabricAsyncOperationContext
{

BEGIN_COM_MAP(SyncAsyncOperationContext)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationContext)
END_COM_MAP()

public:
  IFabricAsyncOperationContext * Initialize(IFabricAsyncOperationCallback *callback){
    callback->AddRef();
    callback_ = callback;
    // Note: callback is not invoked here.
    return this;
  }

  STDMETHOD_(BOOLEAN, IsCompleted)() override
  {
    return true;
  }

  STDMETHOD_(BOOLEAN, CompletedSynchronously)() override{
    return true;
  }

  STDMETHOD(get_Callback)(
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override{
    *callback = callback_;
    return S_OK;
  }

  STDMETHOD(Cancel)() override{
    return S_OK;
  }
private:
  CComPtr<IFabricAsyncOperationCallback> callback_;
};

// wraps endoperation
class AsyncEndOperationContext : 
  public CComObjectRootEx<CComSingleThreadModel>,
  public IFabricAsyncOperationContext
{

BEGIN_COM_MAP(AsyncEndOperationContext)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationContext)
END_COM_MAP()

public:
  IFabricAsyncOperationContext * Initialize(IFabricAsyncOperationContext* innerCtx, std::unique_ptr<IEndOperation> && op, Status beginStatus){
    innerCtx->AddRef();
    innerCtx_ = innerCtx;
    op_ = std::move(op);
    beginStatus_ = std::move(beginStatus);
    return this;
  }

  STDMETHOD_(BOOLEAN, IsCompleted)() override
  {
    return innerCtx_->IsCompleted();
  }

  STDMETHOD_(BOOLEAN, CompletedSynchronously)() override{
    return innerCtx_->CompletedSynchronously();
  }

  STDMETHOD(get_Callback)(
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override{
    return innerCtx_->get_Callback(callback);  
  }

  STDMETHOD(Cancel)() override{
    return innerCtx_->Cancel();
  }

  // custom function
  const std::unique_ptr<IEndOperation> & get_EndOperation(){
    return op_;
  }

  IFabricAsyncOperationContext * get_InnerCtx(){
    // get a view of the ctx
    CComPtr<IFabricAsyncOperationContext> innerCtxCopy = innerCtx_;
    return innerCtxCopy.Detach();
  }

  const Status & get_BeginStatus() const {
    return beginStatus_;
  }
private:
  //std::unique_ptr<void*> data_;
  CComPtr<IFabricAsyncOperationContext> innerCtx_;
  std::unique_ptr<IEndOperation> op_;
  Status beginStatus_;
};

  FRPCRequestHandler::FRPCRequestHandler(): svc_() {}
  void FRPCRequestHandler::Initialize(std::shared_ptr<MiddleWare> svc){
    assert(svc != nullptr);
    svc_ = svc;
  }

  HRESULT STDMETHODCALLTYPE FRPCRequestHandler::BeginProcessRequest(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback *callback,
      /* [retval][out] */ IFabricAsyncOperationContext **context) {
    
    assert(svc_ != nullptr); // must initialize

    // copy the message to fabric rpc implementation
    CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(new CComObjectNoLock<FRPCTransportMessage>());
    msgPtr->CopyFrom(message);

    const std::string & body = msgPtr->GetBody();
    const std::string & header = msgPtr->GetHeader();

    Status err; // The error to be sent back to client
    CComPtr<IFabricAsyncOperationContext> ctx; // context to be returned by the begin operation
    // begin and end ops to be looked up by routing.
    std::unique_ptr<IBeginOperation> beginOp;
    std::unique_ptr<IEndOperation> endOp;
    fabricrpc::request_header pbHeader;
    if(header.size() == 0){
      err = Status(StatusCode::INVALID_ARGUMENT, "fabric rpc header is empty");
    }else {
      bool ok = pbHeader.ParseFromArray(header.data(), static_cast<int>(header.size()));
      if (!ok) {
        err = Status(StatusCode::INVALID_ARGUMENT, "Cannot parse fabric rpc header");
      }else{
        std::string url = pbHeader.url();
        err = svc_->Route(url, beginOp, endOp);
        if(!err){
          // invoke begin op
          // TODO: investigate callback passed here that is invoked synchronously
          // maybe the ctx to invoke this callback should be the wrapped ctx
          err = beginOp->Invoke(body, callback, &ctx);
        }
      }
    }

    if (err) {
      // begin op failure should not return a context.
      assert(ctx == nullptr);
      // ctx is not set by begin op, need to fill up a ctx here.
      // callback should not have been called.
      // fill a dummy sync ctx.
      // callback is not invoked by this ctx here but later.
      CComPtr<CComObjectNoLock<SyncAsyncOperationContext>> errCtx(new CComObjectNoLock<SyncAsyncOperationContext>());
      errCtx->Initialize(callback);
      ctx = errCtx.Detach(); // transfer ownership.
    }

    // pass user end op and error
    CComPtr<CComObjectNoLock<AsyncEndOperationContext>> ctx_wrap2(new CComObjectNoLock<AsyncEndOperationContext>());
    ctx_wrap2->Initialize(ctx, std::move(endOp), err);

    if (err) {
      // invoke the callback with the wrapped ctx.
      callback->Invoke(ctx_wrap2);
    }

    *context = ctx_wrap2.Detach();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE FRPCRequestHandler::EndProcessRequest(
      /* [in] */ IFabricAsyncOperationContext *context,
      /* [retval][out] */ IFabricTransportMessage **reply) {
    // get the packed additional info
    CComObjectNoLock<AsyncEndOperationContext> * ctx_wrap2 = dynamic_cast<CComObjectNoLock<AsyncEndOperationContext> *>(context);
    
    fabricrpc::reply_header h_reply;
    std::string reply_str;

    Status const & beginErr = ctx_wrap2->get_BeginStatus();
    if(beginErr){
      // we send the err back to client in header while body is empty.
      h_reply.set_status_code(beginErr.GetErrorCode());
      h_reply.set_status_message(beginErr.GetErrorMessage());
    }else{
      Status err;
      // invoke user end operation
      const std::unique_ptr<IEndOperation> & end = ctx_wrap2->get_EndOperation();
      err = end->Invoke(ctx_wrap2->get_InnerCtx(), reply_str);
      if(err){
          reply_str.clear();
      }
      assert(err.GetErrorMessage().size() != 0); // message must not be empty, or proto serialization fails.
      // prepare header
      h_reply.set_status_code(err.GetErrorCode());
      h_reply.set_status_message(err.GetErrorMessage());
    }
    std::string h_response_str;
    bool ok = h_reply.SerializeToString(&h_response_str);
    assert(ok);

    // create com msg
    CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(new CComObjectNoLock<FRPCTransportMessage>());
    msgPtr->Initialize(std::move(h_response_str), std::move(reply_str));
    *reply = msgPtr.Detach();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE FRPCRequestHandler::HandleOneWay(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message) {
    //BOOST_LOG_TRIVIAL(debug) << "request_handler::HandleOneWay";
    return S_OK;
  }
} // namespace fabricrpc