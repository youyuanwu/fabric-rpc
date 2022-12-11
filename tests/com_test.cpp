// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define BOOST_TEST_MODULE com_test
#include <boost/test/unit_test.hpp>

#include <FabricCommon.h>
#include <atlbase.h>
#include <atlcom.h>

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
    return S_OK;
  }

  STDMETHOD(Cancel)() override { return S_OK; }
};

BOOST_AUTO_TEST_SUITE(test_fabric_transport)

BOOST_AUTO_TEST_CASE(test_1) {

  CComPtr<IFabricAsyncOperationContext> ctx(
      new CComObjectNoLock<async_wapping_context2>());

  BOOST_CHECK_EQUAL(true, ctx->CompletedSynchronously());
}

BOOST_AUTO_TEST_SUITE_END()