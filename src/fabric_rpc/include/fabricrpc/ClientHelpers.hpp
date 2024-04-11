// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

#pragma once

#include "fabricrpc/FRPCHeader.hpp"
#include "fabricrpc/FRPCTransportMessage.hpp"
#include "fabricrpc/Status.hpp"

#include <chrono>

// some helpers for generated client code
namespace fabricrpc {

template <typename ProtoReq>
Status ExecClientBegin(IFabricTransportClient *client,
                       std::shared_ptr<IFabricRPCHeaderProtoConverter> cv,
                       DWORD timeoutMilliseconds, std::string const &url,
                       const ProtoReq *request,
                       IFabricAsyncOperationCallback *callback,
                       /*out*/ IFabricAsyncOperationContext **context) {
  HRESULT hr = S_OK;

  // calculate new timeout. Parsing may take some time if payload is big.
  auto starttime = std::chrono::steady_clock::now();

  fabricrpc::FabricRPCRequestHeader fRequestHeader;
  // prepare header
  fRequestHeader.SetUrl(url);

  std::string header_str;
  bool ok = cv->SerializeRequestHeader(&fRequestHeader, &header_str);
  assert(ok);
  if (!ok) {
    return Status(StatusCode::INTERNAL,
                  "Client cannot serialize request header.");
  }
  // prepare body
  std::string body_str;
  ok = request->SerializeToString(&body_str);
  assert(ok);
  if (!ok) {
    return Status(StatusCode::INTERNAL,
                  "Client cannot serialize request body.");
  }

  CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(
      new CComObjectNoLock<FRPCTransportMessage>());
  msgPtr->Initialize(std::move(header_str), std::move(body_str));

  // prepare timeout value
  auto endtime = std::chrono::steady_clock::now();
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime)
          .count();
  DWORD newTimeout = {};
  if (timeoutMilliseconds > ms) {
    newTimeout = timeoutMilliseconds - static_cast<DWORD>(ms);
  }

  // send request
  hr = client->BeginRequest(msgPtr, newTimeout, callback, context);
  if (FAILED(hr)) {
    return Status(StatusCode::FABRIC_TRANSPORT_ERROR, "BeginRequest failed",
                  hr);
  }
  return Status();
}

template <typename ResponseProto>
Status ExecClientEnd(IFabricTransportClient *client,
                     std::shared_ptr<IFabricRPCHeaderProtoConverter> cv,
                     IFabricAsyncOperationContext *context,
                     /*out*/ ResponseProto *response) {
  HRESULT hr = S_OK;
  CComPtr<IFabricTransportMessage> reply;
  hr = client->EndRequest(context, &reply);
  if (hr != S_OK) {
    return Status(StatusCode::FABRIC_TRANSPORT_ERROR, "EndRequest failed", hr);
  }

  // copy request reply to fabric rpc impl
  CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(
      new CComObjectNoLock<FRPCTransportMessage>());
  msgPtr->CopyMsg(reply);

  std::string const &header_str = msgPtr->GetHeader();
  if (header_str.size() == 0) {
    return Status(StatusCode::UNKNOWN, "Server returned empty header");
  }

  fabricrpc::FabricRPCReplyHeader fReplyHeader;

  if (!cv->DeserializeReplyHeader(&header_str, &fReplyHeader)) {
    return Status(StatusCode::UNKNOWN, "Server returned bad header");
  }
  if (fReplyHeader.GetStatusCode() != 0) {
    return Status(StatusCode(fReplyHeader.GetStatusCode()),
                  fReplyHeader.GetStatusMessage());
  }
  // parse response
  auto data = msgPtr->GetBody();
  if (!response->ParseFromArray(data.c_str(), static_cast<int>(data.size()))) {
    return Status(StatusCode::UNKNOWN, "Server returned bad body");
  }
  return Status();
}

} // namespace fabricrpc