#pragma once

#include "todolist.fabricrpc.h"
#include <atlbase.h>
#include <atlcom.h>

#include "servicefabric/async_context.hpp"
#include "servicefabric/fabric_error.hpp"
#include "servicefabric/transport_dummy_client_conn_handler.hpp"
#include "servicefabric/transport_dummy_client_notification_handler.hpp"
#include "servicefabric/transport_dummy_msg_disposer.hpp"
#include "servicefabric/transport_dummy_server_conn_handler.hpp"
#include "servicefabric/transport_message.hpp"
#include "servicefabric/waitable_callback.hpp"

#include <map>

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
    *callback = callback_;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Cancel() override { return S_OK; }

private:
  CComPtr<IFabricAsyncOperationCallback> callback_;
  T content_;
};

struct todoItem {
  int id;
  std::string description;
  bool completed;

  void FillProto(todolist::Item *p) const {
    p->set_id(id);
    p->set_description(description);
    p->set_completed(completed);
  }

  void FromProto(const todolist::Item *p) {
    id = p->id();
    description = p->description();
    completed = p->completed();
  }
};
// implement server

class TodoList_Impl : public todolist::Todo::Service {
public:
  TodoList_Impl() : items_() {}

  fabricrpc::Status
  BeginFind(const todolist::FindRequest *request,
            IFabricAsyncOperationCallback *callback,
            /*out*/ IFabricAsyncOperationContext **context) override {
    UNREFERENCED_PARAMETER(request);

    // there is no content passed to the end needed. pass a string for testing
    CComPtr<CComObjectNoLock<AsyncAnyCtx<std::string>>> ctxPtr(
        new CComObjectNoLock<AsyncAnyCtx<std::string>>());
    ctxPtr->SetContent("dummyStr");
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status EndFind(IFabricAsyncOperationContext *context,
                            /*out*/ todolist::FindResponse *response) override {
    CComObjectNoLock<AsyncAnyCtx<std::string>> *ctx =
        dynamic_cast<CComObjectNoLock<AsyncAnyCtx<std::string>> *>(context);
    std::string content = ctx->GetContent();
    assert(content == "dummyStr");
    UNREFERENCED_PARAMETER(response);

    // return a dummy item for now
    for (auto i : items_) {
      std::shared_ptr<todoItem> innerItem = i.second;
      auto it = response->add_items();
      innerItem->FillProto(it);
    }

    return fabricrpc::Status();
  }

  fabricrpc::Status
  BeginAddOne(const todolist::AddOneRequest *request,
              IFabricAsyncOperationCallback *callback,
              /*out*/ IFabricAsyncOperationContext **context) override {
    auto p = request->payload();
    int id = p.id();
    if (items_.count(id) > 0) {
      // already exist
      return fabricrpc::Status(fabricrpc::StatusCode::INVALID_ARGUMENT,
                               "id already exist: " + std::to_string(id));
    }

    std::shared_ptr<todoItem> item = std::make_shared<todoItem>();
    item->FromProto(&p);
    items_[id] = item;

    CComPtr<CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>>> ctxPtr(
        new CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>>());
    ctxPtr->SetContent(std::move(item));
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }

  fabricrpc::Status
  EndAddOne(IFabricAsyncOperationContext *context,
            /*out*/ todolist::AddOneResponse *response) override {
    CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>> *ctx =
        dynamic_cast<
            CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>> *>(
            context);
    std::shared_ptr<todoItem> content = ctx->GetContent();
    assert(content != nullptr);

    auto i = response->mutable_payload();
    content->FillProto(i);

    return fabricrpc::Status();
  }

  fabricrpc::Status
  BeginDeleteOne(const todolist::DeleteOneRequest *request,
                 IFabricAsyncOperationCallback *callback,
                 /*out*/ IFabricAsyncOperationContext **context) override {

    auto id = request->id();

    auto it = items_.find(id);
    if (it == items_.end()) {
      return fabricrpc::Status(fabricrpc::StatusCode::INVALID_ARGUMENT,
                               "id not found: " + std::to_string(id));
    }

    std::shared_ptr<todoItem> item = it->second;
    items_.erase(it);

    CComPtr<CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>>> ctxPtr(
        new CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>>());
    ctxPtr->SetContent(std::move(item));
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }

  fabricrpc::Status
  EndDeleteOne(IFabricAsyncOperationContext *context,
               /*out*/ todolist::DeleteOneResponse *response) override {
    CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>> *ctx =
        dynamic_cast<
            CComObjectNoLock<AsyncAnyCtx<std::shared_ptr<todoItem>>> *>(
            context);
    std::shared_ptr<todoItem> content = ctx->GetContent();
    assert(content != nullptr);

    auto i = response->mutable_payload();
    content->FillProto(i);
    return fabricrpc::Status();
  }

private:
  std::map<int, std::shared_ptr<todoItem>> items_;
};

namespace sf = servicefabric;

class myserver {
public:
  HRESULT StartServer() {
    if (listener_) {
      return ERROR_ALREADY_EXISTS;
    }

    FABRIC_SECURITY_CREDENTIALS cred = {};
    cred.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;

    FABRIC_TRANSPORT_SETTINGS settings = {};
    settings.KeepAliveTimeoutInSeconds = 10;
    settings.MaxConcurrentCalls = 10;
    settings.MaxMessageSize = 100;
    settings.MaxQueueSize = 100;
    settings.OperationTimeoutInSeconds = 30;
    settings.SecurityCredentials = &cred;

    FABRIC_TRANSPORT_LISTEN_ADDRESS addr = {};
    addr.IPAddressOrFQDN = L"localhost";
    addr.Path = L"/";
    addr.Port = 0; // os will pick an random port.

    std::shared_ptr<fabricrpc::MiddleWare> todo_svc =
        std::make_shared<TodoList_Impl>();

    belt::com::com_ptr<IFabricTransportMessageHandler> req_handler;
    todolist::CreateFabricRPCRequestHandler({todo_svc}, req_handler.put());

    belt::com::com_ptr<IFabricTransportConnectionHandler> conn_handler =
        sf::transport_dummy_server_conn_handler::create_instance().to_ptr();
    belt::com::com_ptr<IFabricTransportMessageDisposer> msg_disposer =
        sf::transport_dummy_msg_disposer::create_instance().to_ptr();
    belt::com::com_ptr<IFabricTransportListener> listener;

    // create listener
    HRESULT hr = CreateFabricTransportListener(
        IID_IFabricTransportListener, &settings, &addr, req_handler.get(),
        conn_handler.get(), msg_disposer.get(), listener.put());

    if (hr != S_OK) {
      return hr;
    }
    // open listener
    belt::com::com_ptr<IFabricStringResult> addr_str;
    {
      belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
          sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
      belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
      hr = listener->BeginOpen(callback.get(), ctx.put());
      if (hr != S_OK) {
        return hr;
      }
      callback->Wait();
      hr = listener->EndOpen(ctx.get(), addr_str.put());
      if (hr != S_OK) {
        return hr;
      }
    }
    addr_ = std::wstring(addr_str->get_String());
    listener_.Attach(listener.detach());
    return S_OK;
  }

  std::wstring GetAddr() { return this->addr_; }

  HRESULT CloseServer() {
    if (!listener_) {
      return ERROR_NOT_FOUND;
    }

    // cannot call this here otherwise not able to close.
    // listener_->Abort();

    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    HRESULT hr = listener_->BeginClose(callback.get(), ctx.put());
    if (hr != S_OK) {
      return hr;
    }
    callback->Wait();
    hr = listener_->EndClose(ctx.get());
    if (hr != S_OK) {
      return hr;
    }
    listener_.Release();
    return S_OK;
  }

private:
  CComPtr<IFabricTransportListener> listener_;
  std::wstring addr_;
};