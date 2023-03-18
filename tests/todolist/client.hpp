// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "todolist.fabricrpc.h"

#include <fabricrpc_tool/waitable_callback.hpp>
#include <winrt/base.h>

fabricrpc::Status AddItem(todolist::TodoClient &c, int id) {
  winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
      winrt::make<fabricrpc::waitable_callback>();
  winrt::com_ptr<IFabricAsyncOperationContext> ctx;
  std::string desc = "myitem" + std::to_string(id);

  todolist::AddOneRequest req;
  auto i = req.mutable_payload();
  i->set_completed(false);
  i->set_description(desc);
  i->set_id(id);
  fabricrpc::Status ec = c.BeginAddOne(&req, 1000, callback.get(), ctx.put());
  if (ec) {
    return ec;
  }
  callback->Wait();
  todolist::AddOneResponse resp;
  ec = c.EndAddOne(ctx.get(), &resp);
  if (ec) {
    return ec;
  }
  auto ri = resp.payload();
  BOOST_CHECK_EQUAL(ri.id(), i->id());
  BOOST_CHECK_EQUAL(ri.description(), i->description());
  BOOST_CHECK_EQUAL(ri.completed(), i->completed());
  return ec;
}

fabricrpc::Status FindAll(todolist::TodoClient &c, int count) {
  winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
      winrt::make<fabricrpc::waitable_callback>();
  winrt::com_ptr<IFabricAsyncOperationContext> ctx;

  todolist::FindRequest req;
  fabricrpc::Status ec = c.BeginFind(&req, 1000, callback.get(), ctx.put());
  if (ec) {
    return ec;
  }
  callback->Wait();
  todolist::FindResponse resp;
  ec = c.EndFind(ctx.get(), &resp);
  if (ec) {
    return ec;
  }
  BOOST_CHECK_EQUAL(resp.items_size(), count);
  return ec;
}

fabricrpc::Status DeleteOne(todolist::TodoClient &c, int id) {
  winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
      winrt::make<fabricrpc::waitable_callback>();
  winrt::com_ptr<IFabricAsyncOperationContext> ctx;
  todolist::DeleteOneRequest req;
  req.set_id(id);
  fabricrpc::Status ec =
      c.BeginDeleteOne(&req, 1000, callback.get(), ctx.put());
  if (ec) {
    return ec;
  }
  callback->Wait();
  todolist::DeleteOneResponse resp;
  ec = c.EndDeleteOne(ctx.get(), &resp);
  if (ec) {
    return ec;
  }
  auto ri = resp.payload();
  BOOST_CHECK_EQUAL(ri.id(), id);
  return ec;
}