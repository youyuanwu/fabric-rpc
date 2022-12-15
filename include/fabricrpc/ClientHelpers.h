// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "fabricrpc/Status.h"
#include "fabricrpc/FRPCTransportMessage.h"
#include "fabricrpc/FRPCHeader.h"

// some helpers for generated client code
namespace fabricrpc {

template<typename ProtoReq>
Status ExecClientBegin(IFabricTransportClient * client,
    std::shared_ptr<IFabricRPCHeaderProtoConverter> cv,
    std::string const & url,
    const ProtoReq* request,
    IFabricAsyncOperationCallback *callback, 
    /*out*/ IFabricAsyncOperationContext **context){
  HRESULT hr = S_OK;

  fabricrpc::FabricRPCRequestHeader fRequestHeader;
  // prepare header
  fRequestHeader.SetUrl(url);

  std::string header_str;
  bool ok = cv->SerializeRequestHeader(&fRequestHeader, &header_str);
  assert(ok);
  // prepare body
  std::string body_str = request->SerializeAsString();

  CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(new CComObjectNoLock<FRPCTransportMessage>());
  msgPtr->Initialize(std::move(header_str), std::move(body_str));
  
  // send request
  hr = client->BeginRequest(msgPtr, 1000, callback, context);
  if(FAILED(hr)){
      return Status(StatusCode::FABRIC_TRANSPORT_ERROR, "BeginRequest failed", hr);
  }
  return Status();
}

template<typename ResponseProto>
Status ExecClientEnd(IFabricTransportClient * client,
    std::shared_ptr<IFabricRPCHeaderProtoConverter> cv,
    IFabricAsyncOperationContext *context, /*out*/ResponseProto* response){
    HRESULT hr = S_OK;
    CComPtr<IFabricTransportMessage> reply;
    hr = client->EndRequest(context, &reply);
    if(hr != S_OK){
        return Status(StatusCode::FABRIC_TRANSPORT_ERROR, "EndRequest failed", hr);
    }

    // copy request reply to fabric rpc impl
    CComPtr<CComObjectNoLock<FRPCTransportMessage>> msgPtr(new CComObjectNoLock<FRPCTransportMessage>());
    msgPtr->CopyFrom(reply);

    std::string const & header_str = msgPtr->GetHeader();
    if(header_str.size() == 0){
        return Status(StatusCode::UNKNOWN, "Server returned empty header");
    }

    fabricrpc::FabricRPCReplyHeader fReplyHeader;

    if(!cv->DeserializeReplyHeader(&header_str, &fReplyHeader)){
        return Status(StatusCode::UNKNOWN, "Server returned bad header");
    }
    if(fReplyHeader.GetStatusCode() != 0){
        return Status(StatusCode(fReplyHeader.GetStatusCode()), fReplyHeader.GetStatusMessage());
    }
    // parse response
    if(!response->ParseFromString(msgPtr->GetBody())){
        return Status(StatusCode::UNKNOWN, "Server returned bad body");
    }
    return Status();
}

} // namespace fabricrpc