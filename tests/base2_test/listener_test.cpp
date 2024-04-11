#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/test/unit_test.hpp>
#include <fabricrpc/fabricrpc2.hpp>
#include <fabricrpc_tool/tool_client_connection_handler.hpp>
#include <fabricrpc_tool/tool_client_notification_handler.hpp>
#include <fabricrpc_tool/tool_transport_msg.hpp>

#include <latch>

namespace net = boost::asio;

BOOST_AUTO_TEST_SUITE(listener_test)

BOOST_AUTO_TEST_CASE(event_test) {

  net::io_context ioc;

  fabricrpc::basic_event<net::io_context::executor_type> ev(ioc.get_executor());

  std::atomic_int count;

  ev.async_wait([&](boost::system::error_code ec) {
    BOOST_REQUIRE(!ec.failed());
    count = 11;
  });

  // set event in another thread.
  std::thread th([&]() { ev.set(); });

  ioc.run();

  BOOST_REQUIRE_EQUAL(11, count.load());

  th.join();
}

BOOST_AUTO_TEST_CASE(msg_queue_test) {

  net::io_context ioc;

  typedef std::unique_ptr<int> payload_t;
  typedef fabricrpc::basic_event<net::io_context::executor_type> event_t;

  fabricrpc::basic_item_queue<payload_t, net::io_context::executor_type> queue;

  // msg pushed before pop.
  {
    auto pl1 = std::make_unique<int>(1);
    auto ev1 = std::make_shared<event_t>(ioc.get_executor());

    queue.push(std::move(pl1));

    payload_t res_pl1;
    queue.async_pop(ev1, &res_pl1);

    std::latch lch{1};

    ev1->async_wait([&](boost::system::error_code ec) {
      BOOST_REQUIRE(!ec);
      lch.count_down();
    });

    ioc.run();

    lch.wait();

    BOOST_REQUIRE(res_pl1);
    BOOST_REQUIRE_EQUAL(*res_pl1.get(), 1);
  }

  ioc.reset();

  // msg poped before push
  {
    auto ev1 = std::make_shared<event_t>(ioc.get_executor());

    payload_t res_pl1;
    queue.async_pop(ev1, &res_pl1);

    std::latch lch{1};
    ev1->async_wait([&](boost::system::error_code ec) {
      BOOST_REQUIRE(!ec);
      lch.count_down();
    });

    auto pl1 = std::make_unique<int>(1);
    queue.push(std::move(pl1));

    ioc.run();

    lch.wait();

    BOOST_REQUIRE(res_pl1);
    BOOST_REQUIRE_EQUAL(*res_pl1.get(), 1);
  }
}

// void make_request2() {
//   HRESULT hr = S_OK;
//   FABRIC_SECURITY_CREDENTIALS cred = {};
//   cred.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;

//   FABRIC_TRANSPORT_SETTINGS settings = {};
//   settings.KeepAliveTimeoutInSeconds = 10;
//   settings.MaxConcurrentCalls = 10;
//   settings.MaxMessageSize = 100;
//   settings.MaxQueueSize = 100;
//   settings.OperationTimeoutInSeconds = 30;
//   settings.SecurityCredentials = &cred;

//   std::wstring addr_str = L"localhost:12345+/";

//   winrt::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
//       winrt::make<fabricrpc::tool_client_notification_handler>();

//   winrt::com_ptr<IFabricTransportClientEventHandler> client_event_h =
//       winrt::make<fabricrpc::tool_client_connection_handler>();
//   winrt::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
//       winrt::make<fabricrpc::msg_disposer>();

//   winrt::com_ptr<IFabricTransportClient> client;

//   // open client
//   hr = CreateFabricTransportClient(
//       /* [in] */ IID_IFabricTransportClient,
//       /* [in] */ &settings,
//       /* [in] */ addr_str.c_str(),
//       /* [in] */ client_notify_h.get(),
//       /* [in] */ client_event_h.get(),
//       /* [in] */ client_msg_disposer.get(),
//       /* [retval][out] */ client.put());
//   BOOST_REQUIRE_EQUAL(hr, S_OK);

//   // open client
//   {
//     winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
//         winrt::make<fabricrpc::waitable_callback>();
//     winrt::com_ptr<IFabricAsyncOperationContext> ctx;

//     hr = client->BeginOpen(1000, callback.get(), ctx.put());
//     BOOST_REQUIRE_EQUAL(hr, S_OK);
//     callback->Wait();
//     hr = client->EndOpen(ctx.get());
//     BOOST_REQUIRE_EQUAL(hr, S_OK);
//   }

//   // send requset
//   {
//     winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
//         winrt::make<fabricrpc::waitable_callback>();
//     winrt::com_ptr<IFabricAsyncOperationContext> ctx;

//     winrt::com_ptr<IFabricTransportMessage> req =
//         winrt::make<fabricrpc::tool_transport_msg>("mybody", "myheader");
//     hr = client->BeginRequest(req.get(), 1000, callback.get(), ctx.put());
//     BOOST_REQUIRE_EQUAL(hr, S_OK);
//     callback->Wait();
//     winrt::com_ptr<IFabricTransportMessage> reply;
//     hr = client->EndRequest(ctx.get(), reply.put());
//     BOOST_REQUIRE_EQUAL(hr, S_OK);

//     std::string body = fabricrpc::get_body(reply.get());
//     std::string header = fabricrpc::get_header(reply.get());
//     BOOST_REQUIRE_EQUAL(body, "hello mybody");
//     BOOST_REQUIRE_EQUAL(header, "hello myheader");
//   }
// }

// TODO: replace with async version
void make_request() {
  boost::system::error_code ec = {};
  net::io_context ioc;
  fabricrpc::endpoint ep(L"localhost", 12345);

  fabricrpc::basic_client_connection<net::io_context::executor_type> conn(
      ioc.get_executor());

  BOOST_REQUIRE(ep.get_url() == L"localhost:12345+/");

  ec = conn.open(ep);
  BOOST_REQUIRE(!ec.failed());

  // send requset
  auto f = [&]() -> net::awaitable<void> {
    try {
      winrt::com_ptr<IFabricTransportMessage> req =
          winrt::make<fabricrpc::tool_transport_msg>("mybody", "myheader");
      auto reply = co_await conn.async_send(req, net::use_awaitable);
      std::string body = fabricrpc::get_body(reply.get());
      std::string header = fabricrpc::get_header(reply.get());
      BOOST_REQUIRE_EQUAL(body, "hello mybody");
      BOOST_REQUIRE_EQUAL(header, "hello myheader");
    } catch (const std::exception &e) {
      BOOST_REQUIRE_MESSAGE(false,
                            std::string("coro has exception ") + e.what());
    }
  };

  net::co_spawn(ioc, f, net::detached);
  net::co_spawn(ioc, f, net::detached);

  ioc.run();
}

BOOST_AUTO_TEST_CASE(acceptor_test) {
  net::io_context ioc;
  fabricrpc::endpoint ep(L"localhost", 12345);
  fabricrpc::basic_acceptor<net::io_context::executor_type> acceptor(
      ioc.get_executor(), ep);
  boost::system::error_code ec = {};
  std::wstring addr;
  ec = acceptor.open(&addr);
  BOOST_REQUIRE(!ec.failed());
  BOOST_CHECK(addr == L"localhost:12345+/");

  auto listener = [&]() -> net::awaitable<void> {
    auto executor = co_await net::this_coro::executor;
    for (;;) {
      // BOOST_TEST_MESSAGE("acceptor.async_accept");

      fabricrpc::basic_server_connection<net::io_context::executor_type> conn =
          co_await acceptor.async_accept_conn(net::use_awaitable);

      auto conn_f = [c = std::move(conn)]() mutable -> net::awaitable<void> {
        for (;;) {
          // accept request in loop
          fabricrpc::p_request_t pl =
              co_await c.async_accept(net::use_awaitable);
          BOOST_TEST_MESSAGE("acceptor.async_accept finish");
          winrt::com_ptr<IFabricTransportMessage> req;
          pl->get_request_msg(req.put());
          std::string body = fabricrpc::get_body(req.get());
          std::string header = fabricrpc::get_header(req.get());
          winrt::com_ptr<IFabricTransportMessage> reply =
              winrt::make<fabricrpc::tool_transport_msg>("hello " + body,
                                                         "hello " + header);
          pl->complete(S_OK, reply);
        }
      };
      net::co_spawn(executor, std::move(conn_f), net::detached);
    }
  };

  net::co_spawn(ioc, listener, net::detached);

  // run the server in another thread
  std::thread th([&]() { ioc.run(); });

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // make a request
  make_request();
  // make_request2();

  ioc.stop();

  th.join();
  ec = acceptor.close();
  BOOST_REQUIRE(!ec.failed());
}

BOOST_AUTO_TEST_SUITE_END()