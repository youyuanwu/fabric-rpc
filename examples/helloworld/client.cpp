// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

// client cli exe
#include "fabrictransport_.h"

#include "fabricrpc_tool/msg_disposer.hpp"
#include "fabricrpc_tool/tool_client_connection_handler.hpp"
#include "fabricrpc_tool/tool_client_notification_handler.hpp"
#include "fabricrpc_tool/waitable_callback.hpp"
#include <winrt/base.h>

#include "helloworld.pb.h"

#ifdef SF_MANUAL
#include "helloworld.gen.h"
#else
#include "helloworld.fabricrpc.h"
#endif

#include <iostream>

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

  winrt::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
      winrt::make<fabricrpc::tool_client_notification_handler>();

  winrt::com_ptr<IFabricTransportClientEventHandler> client_event_h =
      winrt::make<fabricrpc::tool_client_connection_handler>();
  winrt::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
      winrt::make<fabricrpc::msg_disposer>();

  winrt::com_ptr<IFabricTransportClient> client;

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
    std::cerr << "cannot create client " << hr << std::endl;
    return EXIT_FAILURE;
  }

  // open client
  {
    winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
        winrt::make<fabricrpc::waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;

    hr = client->BeginOpen(1000, callback.get(), ctx.put());
    if (hr != S_OK) {
      std::cerr << "cannot begin open " << hr << std::endl;
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = client->EndOpen(ctx.get());
    if (hr != S_OK) {
      std::cerr << "cannot end open " << hr << std::endl;
      return EXIT_FAILURE;
    }
  }

  // make request
  helloworld::FabricHelloClient h_client(client.get());

  {
    winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
        winrt::make<fabricrpc::waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;

    helloworld::FabricRequest request;
    request.set_fabricname("world");

    fabricrpc::Status err;

    err = h_client.BeginSayHello(&request, 1000, callback.get(), ctx.put());
    if (err) {
      std::cerr << "BeginSayHello Failed " << err << std::endl;
      return EXIT_FAILURE;
    }
    callback->Wait();

    helloworld::FabricResponse response;
    err = h_client.EndSayHello(ctx.get(), &response);
    if (err) {
      std::cerr << "SaySayHelloEnd Failed " << err << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "Reply: " << response.fabricmessage() << std::endl;
  }
}