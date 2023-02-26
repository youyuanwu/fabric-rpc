// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "servicefabric/async_context.hpp"
#include "servicefabric/fabric_error.hpp"
#include "servicefabric/transport_dummy_client_conn_handler.hpp"
#include "servicefabric/transport_dummy_client_notification_handler.hpp"
#include "servicefabric/transport_dummy_msg_disposer.hpp"
#include "servicefabric/transport_dummy_server_conn_handler.hpp"
#include "servicefabric/transport_message.hpp"
#include "servicefabric/waitable_callback.hpp"
#include "todolist.fabricrpc.h"

namespace sf = servicefabric;

fabricrpc::Status AddItem(todolist::TodoClient &c, int id) {
  belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
      sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
  belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

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
  belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
      sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
  belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

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
  belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
      sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
  belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
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