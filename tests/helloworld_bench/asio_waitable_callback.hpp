// ------------------------------------------------------------
// Copyright 2023 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

#include <boost/asio/windows/object_handle.hpp>
#include <functional>

// implementation that allows sf callback to be run on the io_context

class asio_waitable_callback
    : public winrt::implements<asio_waitable_callback,
                               IFabricAsyncOperationCallback> {
public:
  template <typename Executor>
  asio_waitable_callback(
      std::function<void(IFabricAsyncOperationContext *)> token, Executor ex)
      : oh_(ex), ctx_(nullptr), token_(token) {
    HANDLE ev = CreateEvent(NULL,  // default security attributes
                            TRUE,  // manual-reset event
                            FALSE, // initial state is nonsignaled
                            NULL   // object name
    );
    assert(ev != nullptr);
    this->oh_.assign(ev);
    // start the async wait on the event
    oh_.async_wait([this]([[maybe_unused]] boost::system::error_code ec) {
      assert(!ec.failed());
      assert(this->ctx_ != nullptr);
      this->token_(this->ctx_);
    });
  }

  void Invoke(/* [in] */ IFabricAsyncOperationContext *context) override {
    assert(context != nullptr);
    this->ctx_ = context;
    HANDLE h = oh_.native_handle();
    [[maybe_unused]] bool ok = SetEvent(h);
    assert(ok);
  }

private:
  boost::asio::windows::object_handle oh_;
  IFabricAsyncOperationContext *ctx_;
  std::function<void(IFabricAsyncOperationContext *)> token_;
};