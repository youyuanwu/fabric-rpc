// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#define BOOST_TEST_MODULE com_test
#include <boost/test/unit_test.hpp>

#include <FabricCommon.h>
#include <atlbase.h>
#include <atlcom.h>

#include "fabricrpc/FRPCTransportMessage.h"
#include "fabricrpc/Status.h"

class async_wapping_context2 : public CComObjectRootEx<CComSingleThreadModel>,
                               public IFabricAsyncOperationContext {

  BEGIN_COM_MAP(async_wapping_context2)
  // ...
  COM_INTERFACE_ENTRY(IFabricAsyncOperationContext)
  END_COM_MAP()

  // ...
  // DECLARE_NOT_AGGREGATABLE(async_wapping_context2)
  // DECLARE_GET_CONTROLLING_UNKNOWN()

public:
  STDMETHOD_(BOOLEAN, IsCompleted)() override { return true; }

  STDMETHOD_(BOOLEAN, CompletedSynchronously)() override { return true; }

  STDMETHOD(get_Callback)
  (
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override {
    UNREFERENCED_PARAMETER(callback);
    return S_OK;
  }

  STDMETHOD(Cancel)() override { return S_OK; }
};

BOOST_AUTO_TEST_SUITE(test_fabric_transport)

// BOOST_AUTO_TEST_CASE(test_1) {

//   CComPtr<IFabricAsyncOperationContext> ctx(
//       new CComObjectNoLock<async_wapping_context2>());

//   BOOST_CHECK_EQUAL(TRUE, ctx->CompletedSynchronously());
// }

// BOOST_AUTO_TEST_CASE(test_2) {

//   CComPtr<CComObjectNoLock<async_wapping_context2>> ctx(
//       new CComObjectNoLock<async_wapping_context2>());

//   BOOST_CHECK_EQUAL(TRUE, ctx->CompletedSynchronously());
// }

BOOST_AUTO_TEST_CASE(message) {
  CComPtr<CComObjectNoLock<fabricrpc::FRPCTransportMessage>> msgPtr(
      new CComObjectNoLock<fabricrpc::FRPCTransportMessage>());
  msgPtr->Initialize("myheader", "mybody");

  // copy into another msg
  CComPtr<CComObjectNoLock<fabricrpc::FRPCTransportMessage>> msgPtr2(
      new CComObjectNoLock<fabricrpc::FRPCTransportMessage>());
  msgPtr2->CopyMsg(msgPtr);

  BOOST_CHECK_EQUAL(msgPtr->GetBody(), msgPtr2->GetBody());
  BOOST_CHECK_EQUAL(msgPtr->GetHeader(), msgPtr2->GetHeader());

  // make sure no mem leak
  CComPtr<IFabricTransportMessage> msgPtr11(msgPtr); // This will add ref.

  // This will add ref in ptr12 nor release ptr
  // This will create mem leak because msgPtr does not do destruction.
  // The overload is hard to read. So do not use this syntax in prod.
  // CComPtr<IFabricTransportMessage> msgPtr12 = msgPtr.Detach();

  CComPtr<IFabricTransportMessage> msgPtr12;
  msgPtr12.Attach(msgPtr.Detach());
}

BOOST_AUTO_TEST_CASE(statustest) {
  fabricrpc::Status s;
  BOOST_CHECK_EQUAL(s.GetErrorCode(), fabricrpc::StatusCode::OK);
  BOOST_CHECK_EQUAL(s.GetErrorMessage(), "OK");

  fabricrpc::Status s2(s);

  fabricrpc::Status s3 = s;
  BOOST_CHECK_EQUAL(s3.GetErrorMessage(), "OK");

  auto f = []() { return fabricrpc::Status(); };
  fabricrpc::Status s4 = f();
  BOOST_CHECK_EQUAL(s4.GetErrorMessage(), "OK");

  s = fabricrpc::Status(fabricrpc::StatusCode::INTERNAL, "Internal");
  BOOST_CHECK_EQUAL(s.GetErrorCode(), fabricrpc::StatusCode::INTERNAL);
  BOOST_CHECK_EQUAL(s.GetErrorMessage(), "Internal");
}

BOOST_AUTO_TEST_SUITE_END()