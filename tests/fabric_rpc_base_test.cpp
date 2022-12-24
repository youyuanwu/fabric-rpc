// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#define BOOST_TEST_MODULE fabric_rpc_test
#include <boost/test/unit_test.hpp>

#include "fabricrpc.pb.h"
#include "fabricrpc/FRPCHeader.hpp"
#include <memory>

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

using testconverter =
    fabricrpc::FabricRPCHeaderProtoConverter<fabricrpc::request_header,
                                             fabricrpc::reply_header>;

BOOST_AUTO_TEST_SUITE(test_fabric_rpc)

BOOST_AUTO_TEST_CASE(test_request_header_convert) {
  fabricrpc::FabricRPCRequestHeader req("myurl");

  std::unique_ptr<fabricrpc::IFabricRPCHeaderProtoConverter> cv =
      std::make_unique<testconverter>();

  std::string data;
  BOOST_REQUIRE(cv->SerializeRequestHeader(&req, &data));

  fabricrpc::FabricRPCRequestHeader req2;
  BOOST_REQUIRE(cv->DeserializeRequestHeader(&data, &req2));
  BOOST_CHECK_EQUAL(req2.GetUrl(), "myurl");
}

BOOST_AUTO_TEST_CASE(test_reply_header_convert) {
  fabricrpc::FabricRPCReplyHeader reply(5, "mymessage");
  testconverter cv;

  std::string data;
  BOOST_REQUIRE(cv.SerializeReplyHeader(&reply, &data));

  fabricrpc::FabricRPCReplyHeader reply2;
  BOOST_REQUIRE(cv.DeserializeReplyHeader(&data, &reply2));
  BOOST_CHECK_EQUAL(reply2.GetStatusCode(), 5);
  BOOST_CHECK_EQUAL(reply2.GetStatusMessage(), "mymessage");
}

BOOST_AUTO_TEST_SUITE_END()