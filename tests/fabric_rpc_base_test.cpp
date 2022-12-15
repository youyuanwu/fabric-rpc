#define BOOST_TEST_MODULE fabric_rpc_test
#include <boost/test/unit_test.hpp>

#include "fabricrpc.pb.h"
#include "fabricrpc/FRPCHeader.h"
#include <memory>

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