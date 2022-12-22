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

class myclient {
public:
  HRESULT Open(const std::wstring &addr) {
    if (client_) {
      return ERROR_ALREADY_EXISTS;
    }

    HRESULT hr = S_OK;
    FABRIC_SECURITY_CREDENTIALS cred = {};
    cred.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;

    FABRIC_TRANSPORT_SETTINGS settings = {};
    settings.KeepAliveTimeoutInSeconds = 10;
    settings.MaxConcurrentCalls = 10;
    settings.MaxMessageSize = 100;
    settings.MaxQueueSize = 100;
    settings.OperationTimeoutInSeconds = 30;
    settings.SecurityCredentials = &cred;

    belt::com::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
        sf::transport_dummy_client_notification_handler::create_instance()
            .to_ptr();

    belt::com::com_ptr<IFabricTransportClientEventHandler> client_event_h =
        sf::transport_dummy_client_conn_handler::create_instance().to_ptr();
    // mem leak here. allo block 7929 in cmd or 7934 in debugger.
    belt::com::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
        sf::transport_dummy_msg_disposer::create_instance().to_ptr();
    ;
    belt::com::com_ptr<IFabricTransportClient> client;

    // open client
    hr = CreateFabricTransportClient(
        /* [in] */ IID_IFabricTransportClient,
        /* [in] */ &settings,
        /* [in] */ addr.c_str(),
        /* [in] */ client_notify_h.get(),
        /* [in] */ client_event_h.get(),
        /* [in] */ client_msg_disposer.get(),
        /* [retval][out] */ client.put());
    if (hr != S_OK) {
      // BOOST_LOG_TRIVIAL(debug) << "cannot create client " << hr;
      return hr;
    }

    // open client
    {
      belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
          sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
      belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
      hr = client->BeginOpen(1000, callback.get(), ctx.put());
      if (hr != S_OK) {
        // BOOST_LOG_TRIVIAL(debug) << "cannot begin open " << hr;
        // return EXIT_FAILURE;
        return hr;
      }
      callback->Wait();
      hr = client->EndOpen(ctx.get());
      if (hr != S_OK) {
        // BOOST_LOG_TRIVIAL(debug) << "cannot end open " << hr;
        // return EXIT_FAILURE;
        return hr;
      }
    }
    client_.Attach(client.detach());
    return S_OK;
  }

  // close
  HRESULT Close() {
    if (!client_) {
      return ERROR_NOT_FOUND;
    }

    client_->Abort();

    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    HRESULT hr = client_->BeginClose(1000, callback.get(), ctx.put());
    if (hr != S_OK) {
      // BOOST_LOG_TRIVIAL(debug) << "cannot begin open " << hr;
      // return EXIT_FAILURE;
      return hr;
    }
    callback->Wait();
    hr = client_->EndClose(ctx.get());
    if (hr != S_OK) {
      // BOOST_LOG_TRIVIAL(debug) << "cannot end open " << hr;
      // return EXIT_FAILURE;
      return hr;
    }
    client_.Release();
    return S_OK;
  }

  IFabricTransportClient *GetClient() { return client_; }

private:
  CComPtr<IFabricTransportClient> client_;
};

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
  fabricrpc::Status ec = c.BeginAddOne(&req, callback.get(), ctx.put());
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
  fabricrpc::Status ec = c.BeginFind(&req, callback.get(), ctx.put());
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
  fabricrpc::Status ec = c.BeginDeleteOne(&req, callback.get(), ctx.put());
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