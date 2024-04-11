#include "fabricrpc/request.hpp"

#include "fabricrpc.pb.h"
#include "fabricrpc_tool/tool_transport_msg.hpp"

namespace fabricrpc {

request::request(std::wstring id, winrt::com_ptr<IFabricTransportMessage> msg,
                 winrt::com_ptr<IFabricAsyncOperationCallback> callback,
                 winrt::com_ptr<IFabricAsyncOperationContext> ctx)
    : id_(id), msg_(msg), callback_(callback), ctx_(ctx) {}

request_context *request::get_request_context() {
  request_context *res = dynamic_cast<request_context *>(this->ctx_.get());
  assert(res != nullptr);
  return res;
}

// raw complete. lowest level
void request::complete(HRESULT hr,
                       winrt::com_ptr<IFabricTransportMessage> reply_msg) {
  if (hr == S_OK) {
    assert(reply_msg);
  }
  // pass the result into context.
  //
  request_context *ctx = this->get_request_context();
  context_content &content = ctx->get_content();
  content.hr = hr;
  content.reply_msg.copy_from(reply_msg.get());

  // invoke callback. This notifies that reply msg is ready.
  // EndProcessRequest will be invoked by transport and msg will be extracted
  // there.
  ctx->complete();
}

void request::complete_rpc_error(absl::Status st) {
  // make the transport msg
  fabricrpc::reply_header header;
  header.set_status_code(st.raw_code());
  header.set_status_message(st.message());
  winrt::com_ptr<IFabricTransportMessage> msg =
      winrt::make<fabricrpc::tool_transport_msg>("",
                                                 header.SerializeAsString());
  this->complete(S_OK, msg);
}

// void complete_rpc_ok(winrt::com_ptr<IFabricTransportMessage> reply_msg){

// }

void request::get_request_msg(IFabricTransportMessage **msgout) {
  this->msg_.copy_to(msgout);
}

} // namespace fabricrpc