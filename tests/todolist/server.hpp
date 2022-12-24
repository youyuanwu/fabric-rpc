// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

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

#include "fabricrpc/exp/AsyncAnyContext.hpp"

#include <map>

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
    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>> ctxPtr(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>());
    ctxPtr->SetContent("dummyStr");
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status EndFind(IFabricAsyncOperationContext *context,
                            /*out*/ todolist::FindResponse *response) override {
    CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *ctx =
        dynamic_cast<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *>(
            context);
    std::string content = ctx->GetContent();
    assert(content == "dummyStr");

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

    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>>>
        ctxPtr(new CComObjectNoLock<
               fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>>());
    ctxPtr->SetContent(std::move(item));
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }

  fabricrpc::Status
  EndAddOne(IFabricAsyncOperationContext *context,
            /*out*/ todolist::AddOneResponse *response) override {
    CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>> *ctx =
        dynamic_cast<CComObjectNoLock<
            fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>> *>(context);
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

    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>>>
        ctxPtr(new CComObjectNoLock<
               fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>>());
    ctxPtr->SetContent(std::move(item));
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }

  fabricrpc::Status
  EndDeleteOne(IFabricAsyncOperationContext *context,
               /*out*/ todolist::DeleteOneResponse *response) override {
    CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>> *ctx =
        dynamic_cast<CComObjectNoLock<
            fabricrpc::AsyncAnyCtx<std::shared_ptr<todoItem>>> *>(context);
    std::shared_ptr<todoItem> content = ctx->GetContent();
    assert(content != nullptr);

    auto i = response->mutable_payload();
    content->FillProto(i);
    return fabricrpc::Status();
  }

private:
  std::map<int, std::shared_ptr<todoItem>> items_;
};