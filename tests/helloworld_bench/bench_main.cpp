// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#define BOOST_TEST_MODULE bench_test
#include <boost/test/unit_test.hpp>

#include "fabricrpc/exp/AsyncAnyContext.hpp"
#include "helloworld.fabricrpc.h"

#include "fabricrpc_test_helpers.hpp"

#include "servicefabric/asio_callback.hpp"

#include <boost/program_options.hpp>

class Service_Impl : public helloworld::FabricHello::Service {
public:
  fabricrpc::Status
  BeginSayHello(const ::helloworld::FabricRequest *request,
                IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) override {
    std::string msg = "hello " + request->fabricname();

    CComPtr<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>> ctxPtr(
        new CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>>());
    ctxPtr->SetContent(std::move(msg));
    ctxPtr->Initialize(callback);

    *context = ctxPtr.Detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status
  EndSayHello(IFabricAsyncOperationContext *context,
              /*out*/ ::helloworld::FabricResponse *response) override {
    CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *ctx =
        dynamic_cast<CComObjectNoLock<fabricrpc::AsyncAnyCtx<std::string>> *>(
            context);
    std::string content = ctx->GetContent();
    response->set_fabricmessage(std::move(content));
    return fabricrpc::Status();
  }
};

namespace po = boost::program_options;
// global fixture to tear down protobuf
// protobuf has internal memories that needs to be freed before exit program
struct MyGlobalFixture {
  MyGlobalFixture() { BOOST_TEST_MESSAGE("ctor fixture"); }
  void setup() {
    BOOST_TEST_MESSAGE("setup fixture: parsing cmd args");
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "concurrency", po::value(&glb.concurrency)->default_value(2),
        "number concurrent request per client")(
        "connections", po::value(&glb.connections)->default_value(2),
        "number of client connection")(
        "test_sec", po::value(&glb.test_sec)->default_value(1),
        "number of seconds to run");

    po::variables_map vm;
    po::store(po::parse_command_line(
                  boost::unit_test::framework::master_test_suite().argc,
                  boost::unit_test::framework::master_test_suite().argv, desc),
              vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::stringstream ss;
      ss << std::endl;
      desc.print(ss);
      BOOST_REQUIRE_MESSAGE(false, ss.str());
    }
  }
  void teardown() { BOOST_TEST_MESSAGE("teardown fixture"); }
  ~MyGlobalFixture() {
    google::protobuf::ShutdownProtobufLibrary();
    BOOST_TEST_MESSAGE("dtor fixture");
  }
  static MyGlobalFixture glb;

  int concurrency;
  int connections;
  int test_sec;
};

MyGlobalFixture MyGlobalFixture::glb;

BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

BOOST_AUTO_TEST_SUITE(bench_suite)

namespace sf = servicefabric;
namespace net = boost::asio;

// concurrency determines how many requests to launch from one client at a time
// connections determines how many clients in total

void start_one_client(std::atomic<int> &successcount,
                      std::shared_ptr<myclient> c, std::size_t concurrency,
                      std::stop_token st) {
  net::io_context io_context;
  HRESULT hr = S_OK;

  std::atomic<int> subSuccessCount = 0;

  helloworld::FabricHelloClient hc(c->GetClient());
  // need to hammer the server as much as possible
  // try use asio ptr
  boost::system::error_code ec;

  // single thread, no lock needed.
  std::map<IFabricAsyncOperationCallback *,
           belt::com::com_ptr<IFabricAsyncOperationCallback>>
      callbacks;
  std::map<IFabricAsyncOperationContext *,
           belt::com::com_ptr<IFabricAsyncOperationContext>>
      ctxs;
  std::map<IFabricAsyncOperationContext *, IFabricAsyncOperationCallback *>
      ctx2callback;

  std::function<void(void)> startfunc;

  auto lamda_callback = [&](IFabricAsyncOperationContext *ctx) {
    helloworld::FabricResponse resp;
    HRESULT hr = hc.EndSayHello(ctx, &resp);
    if (hr != S_OK) {
      return;
    }
    std::string reply = resp.fabricmessage();
    assert(reply == "hello myname");

    subSuccessCount++;

    // loop to next run
    // it is safe to post first because this is single thread.
    if (!st.stop_requested()) {
      // loop forever until stop requested
      assert(startfunc != nullptr);
      io_context.post(startfunc);
    }
    // remove callbacks and ctxs from map
    // copy the callback and ctx to prevent deletion because the lambda is
    // inside the ctx.
    assert(ctx2callback.contains(ctx));
    IFabricAsyncOperationCallback *callbackPtr = ctx2callback.at(ctx);
    belt::com::com_ptr<IFabricAsyncOperationContext> ctxtemp = ctxs.at(ctx);
    belt::com::com_ptr<IFabricAsyncOperationCallback> callbacktemp =
        callbacks.at(callbackPtr);
    assert(1 == ctx2callback.erase(ctx));
    assert(1 == ctxs.erase(ctxtemp.get()));
    assert(1 == callbacks.erase(callbackPtr));
  };

  startfunc = [&]() {
    // callback needs to be persisted until operation ends.
    // ctx too
    belt::com::com_ptr<IFabricAsyncOperationCallback> callback =
        sf::AsioCallback::create_instance(lamda_callback,
                                          io_context.get_executor())
            .to_ptr();
    callbacks.emplace(callback.get(), callback);

    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    helloworld::FabricRequest req;
    req.set_fabricname("myname");
    hr = hc.BeginSayHello(&req, callback.get(), ctx.put());
    if (hr != S_OK) {
      BOOST_TEST_MESSAGE("BeginSayHello Error: " +
                         sf::get_fabric_error_str(hr));
    }
    assert(hr == S_OK);
    ctxs.emplace(ctx.get(), ctx);
    ctx2callback.emplace(ctx.get(), callback.get());
  };

  for (size_t i = 0; i < concurrency; i++) {
    io_context.post(startfunc);
  }

  io_context.run();
  // BOOST_CHECK_EQUAL(concurrency, subSuccessCount.load());
  successcount += subSuccessCount;
}

BOOST_AUTO_TEST_CASE(test_1) {

  std::shared_ptr<fabricrpc::MiddleWare> svc = std::make_shared<Service_Impl>();

  belt::com::details::com_ptr<IFabricTransportMessageHandler> handler;
  helloworld::CreateFabricRPCRequestHandler({svc}, handler.put());

  HRESULT hr = S_OK;

  std::atomic<int> failcount = 0;
  std::atomic<int> successcount = 0;

  myserver s;
  hr = s.StartServer(handler);
  BOOST_REQUIRE_EQUAL(hr, S_OK);

  // open all clients
  std::size_t connection = MyGlobalFixture::glb.connections;
  std::vector<std::shared_ptr<myclient>> clients;
  for (std::size_t i = 0; i < connection; i++) {
    std::shared_ptr<myclient> c = std::make_shared<myclient>();
    hr = c->Open(s.GetAddr());
    BOOST_REQUIRE_EQUAL(hr, S_OK);
    clients.push_back(c);
  }

  std::stop_source ss;

  std::size_t concurrency = MyGlobalFixture::glb.concurrency;

  // each thread runs one client
  std::vector<std::jthread> threads;

  for (int i = 0; i < connection; i++) {
    std::jthread th(start_one_client, std::ref(successcount), clients[i],
                    concurrency, ss.get_token());
    threads.emplace_back(std::move(th));
  }

  int test_duration_sec = MyGlobalFixture::glb.test_sec;

  std::this_thread::sleep_for(std::chrono::seconds(test_duration_sec));

  ss.request_stop();

  for (int i = 0; i < connection; i++) {
    threads[i].join();
  }

  // close all clients
  for (std::size_t i = 0; i < connection; i++) {
    std::shared_ptr<myclient> c = clients.at(i);
    hr = c->Close();
    BOOST_REQUIRE_EQUAL(hr, S_OK);
  }
  hr = s.CloseServer();
  BOOST_REQUIRE_EQUAL(hr, S_OK);
  std::cout << "=========" << std::endl;
  std::cout << "config: concurrency " << concurrency << " connections "
            << connection << " test_sec " << test_duration_sec << std::endl;
  std::cout << "total success count: " + std::to_string(successcount.load())
            << std::endl;
  std::cout << "req/s: "
            << std::to_string(successcount.load() / test_duration_sec)
            << std::endl;
  std::cout << "=========" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
