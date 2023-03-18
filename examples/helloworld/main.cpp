// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include "fabrictransport_.h"

#include "fabricrpc_tool/msg_disposer.hpp"
#include "fabricrpc_tool/tool_server_connection_handler.hpp"
#include "fabricrpc_tool/waitable_callback.hpp"
#include "winrt/base.h"

#ifdef SF_MANUAL
#include "helloworld.gen.h"
#else
#include "helloworld.fabricrpc.h"
#endif

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "helloworld_server.hpp"

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

  winrt::com_ptr<IFabricTransportMessageHandler> req_handler;
  helloworld::CreateFabricRPCRequestHandler({hello_svc}, req_handler.put());

  winrt::com_ptr<IFabricTransportConnectionHandler> conn_handler =
      winrt::make<fabricrpc::server_tool_connection_handler>();
  winrt::com_ptr<IFabricTransportMessageDisposer> msg_disposer =
      winrt::make<fabricrpc::msg_disposer>();
  winrt::com_ptr<IFabricTransportListener> listener;

  // create listener
  HRESULT hr = CreateFabricTransportListener(
      IID_IFabricTransportListener, &settings, &addr, req_handler.get(),
      conn_handler.get(), msg_disposer.get(), listener.put());

  if (hr != S_OK) {
    return EXIT_FAILURE;
  }
  // open listener
  winrt::com_ptr<IFabricStringResult> addr_str;
  {
    winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
        winrt::make<fabricrpc::waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = listener->BeginOpen(callback.get(), ctx.put());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = listener->EndOpen(ctx.get(), addr_str.put());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
  }
  std::wcout << L"Listening on: " << std::wstring(addr_str->get_String());

  // wait for ctl-c TODO
  std::this_thread::sleep_for(std::chrono::seconds(15));

  // close listener
  {
    winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
        winrt::make<fabricrpc::waitable_callback>();
    winrt::com_ptr<IFabricAsyncOperationContext> ctx;
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