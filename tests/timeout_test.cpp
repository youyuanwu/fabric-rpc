// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include <boost/test/unit_test.hpp>

#include <atlbase.h>
#include <atlcom.h>

#include <fabricrpc/exp/AsyncAnyContext.hpp>

#include "fabricrpc_test_helpers.hpp"
#include "helloworld.fabricrpc.h"

#include <chrono>

class Service_Impl_Timeout : public helloworld::FabricHello::Service {
public:
  fabricrpc::Status
  BeginSayHello(const ::helloworld::FabricRequest *request,
                DWORD timeoutMilliseconds,
                IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) override {
    BOOST_TEST_MESSAGE("BeginSayHello invoked with timeout: " +
                       std::to_string(timeoutMilliseconds) +
                       ", sleep: " + std::to_string(beginSleepMs_));
    if (timeoutMilliseconds <= 0) {
      return fabricrpc::Status(fabricrpc::StatusCode::DEADLINE_EXCEEDED,
                               "operation timedout with " +
                                   std::to_string(timeoutMilliseconds));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(beginSleepMs_));

    if (timeoutMilliseconds < beginSleepMs_) {
      return fabricrpc::Status(
          fabricrpc::StatusCode::DEADLINE_EXCEEDED,
          "operation timedout with " +
              std::to_string(timeoutMilliseconds - beginSleepMs_));
    }

    std::string msg = "hello " + request->fabricname();

    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>> ctxPtr(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>());
    ctxPtr->SetContent(std::move(msg));
    ctxPtr->Initialize(callback);
    callback->Invoke(ctxPtr);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status
  EndSayHello(IFabricAsyncOperationContext *context,
              /*out*/ ::helloworld::FabricResponse *response) override {
    BOOST_TEST_MESSAGE("EndSayHello invoked with sleep: " +
                       std::to_string(endSleepMs_));
    std::this_thread::sleep_for(std::chrono::milliseconds(endSleepMs_));

    CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *ctx =
        dynamic_cast<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *>(
            context);
    std::string content = ctx->GetContent();
    response->set_fabricmessage(std::move(content));
    return fabricrpc::Status();
  }

  void SetBeginSleepMs(DWORD ms) { beginSleepMs_ = ms; }

  void SetEndSleepMs(DWORD ms) { endSleepMs_ = ms; }

private:
  DWORD beginSleepMs_ = 0;
  DWORD endSleepMs_ = 0;
};

BOOST_AUTO_TEST_SUITE(test_timeout)

BOOST_AUTO_TEST_CASE(timeout1) {
  std::shared_ptr<Service_Impl_Timeout> hello_svc_raw =
      std::make_shared<Service_Impl_Timeout>();
  std::shared_ptr<fabricrpc::MiddleWare> hello_svc = hello_svc_raw;

  belt::com::com_ptr<IFabricTransportMessageHandler> req_handler;
  helloworld::CreateFabricRPCRequestHandler({hello_svc}, req_handler.put());

  myserver s;
  HRESULT hr = s.StartServer(req_handler);
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  myclient c;
  hr = c.Open(s.GetAddr());
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // test logic
  helloworld::FabricHelloClient hClient(c.GetClient());

  // make a normal call
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    helloworld::FabricRequest req;
    req.set_fabricname("fabric");
    fabricrpc::Status beginStatus =
        hClient.BeginSayHello(&req, 1000, callback.get(), ctx.put());
    BOOST_REQUIRE(!beginStatus);
    callback->Wait();
    helloworld::FabricResponse resp;
    fabricrpc::Status endStatus = hClient.EndSayHello(ctx.get(), &resp);
    BOOST_REQUIRE(!endStatus);
  }

  // make a timed out call
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    helloworld::FabricRequest req;
    req.set_fabricname("fabric");
    // 0 timeout
    fabricrpc::Status beginStatus =
        hClient.BeginSayHello(&req, 0, callback.get(), ctx.put());
    BOOST_REQUIRE(!beginStatus);
    callback->Wait();
    helloworld::FabricResponse resp;
    fabricrpc::Status endStatus = hClient.EndSayHello(ctx.get(), &resp);
    BOOST_REQUIRE(endStatus);
    BOOST_REQUIRE(endStatus.IsTransportError());
    BOOST_REQUIRE_EQUAL(endStatus.GetErrorCode(),
                        fabricrpc::StatusCode::FABRIC_TRANSPORT_ERROR);
    BOOST_TEST_MESSAGE(
        "transport error: " +
        sf::get_fabric_error_str(endStatus.GetTransportErrorCode()));
    BOOST_REQUIRE_EQUAL(endStatus.GetTransportErrorCode(),
                        FABRIC_E_CANNOT_CONNECT);
  }

  // make a call but the server timed out due to sleep
  hello_svc_raw->SetBeginSleepMs(1000);
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    helloworld::FabricRequest req;
    req.set_fabricname("fabric");
    // 0 timeout
    fabricrpc::Status beginStatus =
        hClient.BeginSayHello(&req, 1000, callback.get(), ctx.put());
    BOOST_REQUIRE(!beginStatus);
    callback->Wait();
    helloworld::FabricResponse resp;
    fabricrpc::Status endStatus = hClient.EndSayHello(ctx.get(), &resp);
    BOOST_REQUIRE(endStatus);
    BOOST_REQUIRE(endStatus.IsTransportError());
    BOOST_REQUIRE_EQUAL(endStatus.GetErrorCode(),
                        fabricrpc::StatusCode::FABRIC_TRANSPORT_ERROR);
    BOOST_TEST_MESSAGE(
        "transport error: " +
        sf::get_fabric_error_str(endStatus.GetTransportErrorCode()));
    BOOST_REQUIRE_EQUAL(endStatus.GetTransportErrorCode(), FABRIC_E_TIMEOUT);
  }

  // end sleep makes the request timeout.
  // This shows that even though the server successfully processed the request
  // and returned to transport its data, but the client get a timeout erorr.
  // This is typical phenomenon in networking.
  hello_svc_raw->SetBeginSleepMs(0);
  hello_svc_raw->SetEndSleepMs(1000);
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    helloworld::FabricRequest req;
    req.set_fabricname("fabric");
    // 0 timeout
    fabricrpc::Status beginStatus =
        hClient.BeginSayHello(&req, 1000, callback.get(), ctx.put());
    BOOST_REQUIRE(!beginStatus);
    callback->Wait();
    helloworld::FabricResponse resp;
    fabricrpc::Status endStatus = hClient.EndSayHello(ctx.get(), &resp);
    BOOST_REQUIRE(endStatus);
    BOOST_REQUIRE(endStatus.IsTransportError());
    BOOST_REQUIRE_EQUAL(endStatus.GetErrorCode(),
                        fabricrpc::StatusCode::FABRIC_TRANSPORT_ERROR);
    BOOST_TEST_MESSAGE(
        "transport error: " +
        sf::get_fabric_error_str(endStatus.GetTransportErrorCode()));
    BOOST_REQUIRE_EQUAL(endStatus.GetTransportErrorCode(), FABRIC_E_TIMEOUT);
  }

  // close client
  hr = c.Close();
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // close server
  hr = s.CloseServer();
  BOOST_TEST_MESSAGE("error:" + sf::get_fabric_error_str(hr));
  BOOST_REQUIRE_EQUAL(hr, S_OK);
}

BOOST_AUTO_TEST_SUITE_END()