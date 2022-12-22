// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#define BOOST_TEST_MODULE todolist_test
#include <boost/test/unit_test.hpp>

// #include <FabricCommon.h>
// #include <atlbase.h>
// #include <atlcom.h>

#include "client.hpp"
#include "fabricrpc.pb.h"
#include "server.hpp"
#include "servicefabric/fabric_error.hpp"
#include "todolist.fabricrpc.h"

// global fixture to tear down protobuf
// protobuf has internal memories that needs to be freed before exit program
struct MyGlobalFixture {
  MyGlobalFixture() { BOOST_TEST_MESSAGE("ctor fixture"); }
  void setup() { BOOST_TEST_MESSAGE("setup fixture"); }
  void teardown() { BOOST_TEST_MESSAGE("teardown fixture"); }
  ~MyGlobalFixture() {
    google::protobuf::ShutdownProtobufLibrary();
    BOOST_TEST_MESSAGE("dtor fixture");
  }
};
BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

BOOST_AUTO_TEST_SUITE(test_todolist)

// BOOST_AUTO_TEST_CASE(ServerBoot){
//   myserver s;
//   HRESULT hr = s.StartServer();
//   BOOST_REQUIRE_EQUAL(hr, S_OK);
//   hr = s.CloseServer();
//   BOOST_REQUIRE_EQUAL(hr, S_OK);
// }

BOOST_AUTO_TEST_CASE(test_1) {
  myserver s;
  HRESULT hr = s.StartServer();
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  myclient c;
  hr = c.Open(s.GetAddr());
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // test logic
  todolist::TodoClient todoClient(c.GetClient());

  fabricrpc::Status ec;
  // add item
  ec = AddItem(todoClient, 1);
  BOOST_REQUIRE(!ec);
  ec = AddItem(todoClient, 2);
  BOOST_REQUIRE(!ec);
  ec = AddItem(todoClient, 3);
  BOOST_REQUIRE(!ec);
  // find item
  ec = FindAll(todoClient, 3);
  BOOST_REQUIRE(!ec);

  // add again should fail
  ec = AddItem(todoClient, 1);
  BOOST_REQUIRE(ec);
  BOOST_CHECK_EQUAL(ec.GetErrorCode(), fabricrpc::StatusCode::INVALID_ARGUMENT);
  BOOST_CHECK_EQUAL(ec.GetErrorMessage(), "id already exist: 1");

  // delete non existing fail
  ec = fabricrpc::Status();
  ec = DeleteOne(todoClient, 100);
  BOOST_REQUIRE(ec);
  BOOST_CHECK_EQUAL(ec.GetErrorCode(), fabricrpc::StatusCode::INVALID_ARGUMENT);
  BOOST_CHECK_EQUAL(ec.GetErrorMessage(), "id not found: 100");

  // delete success
  ec = DeleteOne(todoClient, 1);
  BOOST_REQUIRE(!ec);

  ec = FindAll(todoClient, 2);
  BOOST_REQUIRE(!ec);

  // test special error cases. TODO: more error cases

  // Send some bad header to server
  // server should reply with payload with detailed error info
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    belt::com::com_ptr<IFabricTransportMessage> msg =
        sf::transport_message::create_instance("mybody", "myheader").to_ptr();
    hr =
        c.GetClient()->BeginRequest(msg.get(), 1000, callback.get(), ctx.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    callback->Wait();
    belt::com::com_ptr<IFabricTransportMessage> reply;
    hr = c.GetClient()->EndRequest(ctx.get(), reply.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);

    std::string body = sf::get_body(reply.get());
    std::string headers = sf::get_header(reply.get());
    fabricrpc::reply_header h_reply;
    bool ok = h_reply.ParseFromString(headers);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(h_reply.status_code(),
                      fabricrpc::StatusCode::INVALID_ARGUMENT);
    BOOST_CHECK_EQUAL(h_reply.status_message(),
                      "Cannot parse fabric rpc header");
    BOOST_CHECK_EQUAL(body.size(), 0);
  }

  // close client
  hr = c.Close();
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // close server
  hr = s.CloseServer();
  BOOST_TEST_MESSAGE("error:" + sf::get_fabric_error_str(hr));
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // sleep does not help with mem leak cleanups.
  // std::this_thread::sleep_for(std::chrono::seconds(5));
}

// Test callback invokes the end process request operation.
class MyTestCallback : public CComObjectRootEx<CComMultiThreadModel>,
                       public IFabricAsyncOperationCallback {

  BEGIN_COM_MAP(MyTestCallback)
  COM_INTERFACE_ENTRY(IFabricAsyncOperationCallback)
  END_COM_MAP()

public:
  void Initialize(IFabricTransportMessageHandler *handler) {
    handler->AddRef();
    handler_.Attach(handler);
  }
  // impl
  void STDMETHODCALLTYPE Invoke(
      /* [in] */ IFabricAsyncOperationContext *context) override {
    belt::com::com_ptr<IFabricTransportMessage> reply_msg;
    HRESULT hr = handler_->EndProcessRequest(context, reply_msg.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
  }

private:
  CComPtr<IFabricTransportMessageHandler> handler_;
};

BOOST_AUTO_TEST_CASE(test_sync_invoke) {
  std::shared_ptr<fabricrpc::MiddleWare> todo_svc =
      std::make_shared<TodoList_Impl>();

  belt::com::com_ptr<IFabricTransportMessageHandler> req_handler;
  todolist::CreateFabricRPCRequestHandler({todo_svc}, req_handler.put());

  // send a dummy message using wait
  COMMUNICATION_CLIENT_ID id = L"Dummy";
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

    fabricrpc::request_header header;
    todolist::FindRequest body;

    header.set_url("/todolist.Todo/Find");

    belt::com::com_ptr<IFabricTransportMessage> msg =
        sf::transport_message::create_instance(body.SerializeAsString(),
                                               header.SerializeAsString())
            .to_ptr();
    HRESULT hr = req_handler->BeginProcessRequest(id, msg.get(), 1000,
                                                  callback.get(), ctx.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    callback->Wait();
    belt::com::com_ptr<IFabricTransportMessage> reply_msg;
    hr = req_handler->EndProcessRequest(ctx.get(), reply_msg.put());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
  }

  // This block is disabled.
  // send dummy message using proper callback.
  // {
  //   CComPtr<CComObjectNoLock<MyTestCallback>> callback(new
  //   CComObjectNoLock<MyTestCallback>());
  //   callback->Initialize(req_handler.get());

  //   belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

  //   fabricrpc::request_header header;
  //   todolist::FindRequest body;

  //   header.set_url("/todolist.Todo/Find");

  //   // This is a limitation of the fabric rpc impl.
  //   // This will hit the assert.
  //   // The callback here is executed on the user ctx in svc impl
  //   // and the ctx is sync so the unwrapped ctx gets passed into this
  //   callback
  //   // and fabricrpc has no way to get the additional routing from the wrap.

  //   belt::com::com_ptr<IFabricTransportMessage> msg =
  //       sf::transport_message::create_instance(body.SerializeAsString(),
  //       header.SerializeAsString()).to_ptr();
  //   HRESULT hr = req_handler->BeginProcessRequest(id, msg.get(), 1000,
  //   callback, ctx.put()); BOOST_REQUIRE_EQUAL(hr, S_OK);
  // }
}

// TODO: mem leak due to msg disposer not freed.
// BOOST_AUTO_TEST_CASE(client_mem_leak){

//     HRESULT hr = S_OK;

//     myserver s;
//     hr = s.StartServer();
//     BOOST_REQUIRE_EQUAL(hr, S_OK);

//     FABRIC_SECURITY_CREDENTIALS cred = {};
//     cred.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;

//     FABRIC_TRANSPORT_SETTINGS settings = {};
//     settings.KeepAliveTimeoutInSeconds = 10;
//     settings.MaxConcurrentCalls = 10;
//     settings.MaxMessageSize = 100;
//     settings.MaxQueueSize = 100;
//     settings.OperationTimeoutInSeconds = 30;
//     settings.SecurityCredentials = &cred;

//     belt::com::com_ptr<IFabricTransportCallbackMessageHandler>
//     client_notify_h =
//         sf::transport_dummy_client_notification_handler::create_instance()
//             .to_ptr();

//     belt::com::com_ptr<IFabricTransportClientEventHandler> client_event_h =
//         sf::transport_dummy_client_conn_handler::create_instance().to_ptr();
//     // This is a bug in transport. the disposer is never freed.
//     belt::com::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
//         sf::transport_dummy_msg_disposer::create_instance().to_ptr();

//     belt::com::com_ptr<IFabricTransportClient> client;

//     // open client
//     hr = CreateFabricTransportClient(
//         /* [in] */ IID_IFabricTransportClient,
//         /* [in] */ &settings,
//         /* [in] */ s.GetAddr().c_str(),
//         /* [in] */ client_notify_h.get(),
//         /* [in] */ client_event_h.get(),
//         /* [in] */ client_msg_disposer.get(),
//         /* [retval][out] */ client.put());

//     BOOST_REQUIRE_EQUAL(hr, S_OK);

//     hr = s.CloseServer();
//     BOOST_REQUIRE_EQUAL(hr, S_OK);
// }

BOOST_AUTO_TEST_SUITE_END()