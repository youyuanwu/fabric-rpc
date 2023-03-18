// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#include <boost/test/unit_test.hpp>

#include "fabrictransport_.h"
#include <winrt/base.h>

#include "fabricrpc_tool/sync_async_context.hpp"
#include "fabricrpc_tool/waitable_callback.hpp"

struct mystring : winrt::implements<mystring, IFabricStringResult> {
  IFACEMETHOD_(LPCWSTR, get_String)() { return L"OK"; }
};

BOOST_AUTO_TEST_SUITE(test_winrt)

BOOST_AUTO_TEST_CASE(basic) {
  winrt::com_ptr<IFabricStringResult> str = winrt::make<mystring>();
  BOOST_CHECK(std::wstring(L"OK") == str->get_String());

  HRESULT hr = S_OK;

  // basic test for callback
  winrt::com_ptr<fabricrpc::IWaitableCallback> callback =
      winrt::make<fabricrpc::waitable_callback>();

  // test async context
  winrt::com_ptr<IFabricAsyncOperationContext> ctx =
      winrt::make<fabricrpc::sync_async_context>(callback.get());
  BOOST_CHECK(ctx->IsCompleted());
  BOOST_CHECK(ctx->CompletedSynchronously());
  callback->Wait();
  winrt::com_ptr<IFabricAsyncOperationCallback> callbackRet;
  hr = ctx->get_Callback(callbackRet.put());
  BOOST_CHECK_EQUAL(hr, S_OK);

  BOOST_CHECK_EQUAL(callbackRet.get(), callback.get());
}

BOOST_AUTO_TEST_SUITE_END()