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

#ifdef SF_MANUAL
#include "helloworld.gen.h"
#else
#include "helloworld.fabricrpc.h"
#endif

#include <any>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

import helloworld.tools;

namespace sf = servicefabric;

int main() {

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
  addr.Port = 12345;

  std::shared_ptr<fabricrpc::MiddleWare> hello_svc =
      std::make_shared<Service_Impl>();

  belt::com::com_ptr<IFabricTransportMessageHandler> req_handler;
  helloworld::CreateFabricRPCRequestHandler({hello_svc}, req_handler.put());

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
    return EXIT_FAILURE;
  }
  // open listener
  belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
      sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
  belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
  hr = listener->BeginOpen(callback.get(), ctx.put());
  if (hr != S_OK) {
    return EXIT_FAILURE;
  }
  callback->Wait();
  belt::com::com_ptr<IFabricStringResult> addr_str;
  hr = listener->EndOpen(ctx.get(), addr_str.put());
  if (hr != S_OK) {
    return EXIT_FAILURE;
  }

  std::wcout << L"Listening on: " << std::wstring(addr_str->get_String());

  // wait for ctl-c TODO
  std::this_thread::sleep_for(std::chrono::seconds(15));

  // close listener
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = listener->BeginClose(callback.get(), ctx.put());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = listener->EndClose(ctx.get());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}