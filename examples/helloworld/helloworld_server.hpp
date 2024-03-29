// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "FabricCommon.h"
#include <winrt/base.h>

#ifdef SF_MANUAL
#include "helloworld.gen.h"
#else
#include "helloworld.fabricrpc.h"
#endif

#include "helloworld.pb.h"
#include <any>

// this hosts tools to support helloworld impl.
// may eventually be migrated to service-fabric-cpp repo

// any_context allow user to store any in the ctx.
// std::any is owned by this ctx.
// User needs to invoke the callback.
class async_any_context
    : public winrt::implements<async_any_context,
                               IFabricAsyncOperationContext> {
public:
  async_any_context(IFabricAsyncOperationCallback *callback, std::any &&a)
      : callback_(), a_(std::move(a)) {
    callback_.copy_from(callback);
    // we cannot not invoke callback in the constructor, because
    // this ctx might be invoked with a callback synchronously but the ctx has
    // not finished construction.
    // This is specifically for belt::com lib. We first construct object in a
    // holder and not yet created the com ptr, so calling callback->Invoke(this)
    // is undefined.
  }
  /*IFabricAsyncOperationContext members*/
  BOOLEAN STDMETHODCALLTYPE IsCompleted() override { return true; }
  BOOLEAN STDMETHODCALLTYPE CompletedSynchronously() override { return true; }
  HRESULT STDMETHODCALLTYPE get_Callback(
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override {
    // TODO this might not be right.
    *callback = callback_.get();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Cancel() override { return S_OK; }

  // custom functions
  std::any &get_any() { return a_; }

private:
  winrt::com_ptr<IFabricAsyncOperationCallback> callback_;
  std::any a_;
};

class Service_Impl : public helloworld::FabricHello::Service {
public:
  fabricrpc::Status
  BeginSayHello(const ::helloworld::FabricRequest *request,
                DWORD timeoutMilliseconds,
                IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) override {
    UNREFERENCED_PARAMETER(timeoutMilliseconds);
    // std::cout << timeoutMilliseconds << std::endl;
    std::string msg = "hello " + request->fabricname();
    std::any a = std::move(msg);
    winrt::com_ptr<IFabricAsyncOperationContext> ctx =
        winrt::make<async_any_context>(callback, std::move(a));
    callback->Invoke(ctx.get());
    *context = ctx.detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status
  EndSayHello(IFabricAsyncOperationContext *context,
              /*out*/ ::helloworld::FabricResponse *response) override {
    async_any_context *ctx = dynamic_cast<async_any_context *>(context);
    std::any &a = ctx->get_any();
    std::string message = std::any_cast<std::string>(a);
    response->set_fabricmessage(std::move(message));
    return fabricrpc::Status();
  }
};