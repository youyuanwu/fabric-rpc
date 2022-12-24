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

// common functions

// open a server and client

namespace sf = servicefabric;

class myserver {
public:
  HRESULT
  StartServer(belt::com::com_ptr<IFabricTransportMessageHandler> req_handler) {
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