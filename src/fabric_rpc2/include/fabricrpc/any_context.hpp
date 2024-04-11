#pragma once

#include <fabrictransport_.h>
#include <winrt/base.h>

namespace fabricrpc {

// allow T to be passed in this context
template <typename T>
class any_context
    : public winrt::implements<any_context<T>, IFabricAsyncOperationContext> {
public:
  any_context(IFabricAsyncOperationCallback *callback) : is_completed_(false) {
    assert(callback != nullptr);
    callback_.copy_from(callback);
  }

  // takes ownership
  void set_content(T &&content) { content_ = std::move(content); }

  T &get_content() { return content_; }

  const T &get_content_view() const { return content_; }

  T &&move_content() { return std::move(content_); }

  // invokes the callback.
  void complete() {
    // TODO: maybe detect thread id.
    if (is_completed_) {
      return;
    }
    is_completed_ = true;
    callback_->Invoke(this);
  }

  // impl of internface
  // TODO: completed etc api return values needs to be fixed.
  BOOLEAN STDMETHODCALLTYPE IsCompleted() override { return is_completed_; }
  BOOLEAN STDMETHODCALLTYPE CompletedSynchronously() override {
    return false; // we never complete synchrously
  }
  HRESULT STDMETHODCALLTYPE get_Callback(
      /* [retval][out] */ IFabricAsyncOperationCallback **callback) override {
    assert(callback != nullptr);
    callback_.copy_to(callback);
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Cancel() override { return S_OK; }

private:
  winrt::com_ptr<IFabricAsyncOperationCallback> callback_;
  T content_;
  bool is_completed_;
};

} // namespace fabricrpc