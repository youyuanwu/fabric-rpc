// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

// client cli exe
#include "fabrictransport_.h"

#include <moderncom/interfaces.h>

#include "servicefabric/async_context.hpp"
#include "servicefabric/fabric_error.hpp"
#include "servicefabric/transport_dummy_client_conn_handler.hpp"
#include "servicefabric/transport_dummy_client_notification_handler.hpp"
#include "servicefabric/transport_dummy_msg_disposer.hpp"
#include "servicefabric/transport_dummy_server_conn_handler.hpp"
#include "servicefabric/transport_message.hpp"
#include "servicefabric/waitable_callback.hpp"

#include "helloworld.pb.h"

#include <boost/log/trivial.hpp>

#ifdef SF_MANUAL
#include "helloworld.gen.h"
#else
#include "helloworld.fabricrpc.h"
#endif

namespace sf = servicefabric;

int main() {
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

  std::wstring addr_str = L"localhost:12345+/";

  belt::com::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
      sf::transport_dummy_client_notification_handler::create_instance()
          .to_ptr();

  belt::com::com_ptr<IFabricTransportClientEventHandler> client_event_h =
      sf::transport_dummy_client_conn_handler::create_instance().to_ptr();
  belt::com::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
      sf::transport_dummy_msg_disposer::create_instance().to_ptr();
  ;
  belt::com::com_ptr<IFabricTransportClient> client;

  // open client
  hr = CreateFabricTransportClient(
      /* [in] */ IID_IFabricTransportClient,
      /* [in] */ &settings,
      /* [in] */ addr_str.c_str(),
      /* [in] */ client_notify_h.get(),
      /* [in] */ client_event_h.get(),
      /* [in] */ client_msg_disposer.get(),
      /* [retval][out] */ client.put());
  if (hr != S_OK) {
    BOOST_LOG_TRIVIAL(debug) << "cannot create client " << hr;
    return EXIT_FAILURE;
  }

  // open client
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = client->BeginOpen(1000, callback.get(), ctx.put());
    if (hr != S_OK) {
      BOOST_LOG_TRIVIAL(debug) << "cannot begin open " << hr;
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = client->EndOpen(ctx.get());
    if (hr != S_OK) {
      BOOST_LOG_TRIVIAL(debug) << "cannot end open " << hr;
      return EXIT_FAILURE;
    }
  }

  // make request
  helloworld::FabricHelloClient h_client(client.get());

  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

    helloworld::FabricRequest request;
    request.set_fabricname("world");

    fabricrpc::Status err;

    err = h_client.BeginSayHello(&request, callback.get(), ctx.put());
    if (err) {
      BOOST_LOG_TRIVIAL(debug) << "BeginSayHello Failed " << err;
      return EXIT_FAILURE;
    }
    callback->Wait();

    helloworld::FabricResponse response;
    err = h_client.EndSayHello(ctx.get(), &response);
    if (err) {
      BOOST_LOG_TRIVIAL(debug) << "SaySayHelloEnd Failed " << err;
      return EXIT_FAILURE;
    }

    BOOST_LOG_TRIVIAL(info) << "Reply: " << response.fabricmessage();
  }
}