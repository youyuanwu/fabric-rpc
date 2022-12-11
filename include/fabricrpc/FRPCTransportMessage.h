// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <atlbase.h>
#include <atlcom.h>

#include "fabrictransport_.h"
#include <string>

namespace fabricrpc{

// atl implementation of transport message
class FRPCTransportMessage : 
  public CComObjectRootEx<CComSingleThreadModel>,
  public IFabricTransportMessage
{

BEGIN_COM_MAP(FRPCTransportMessage)
  COM_INTERFACE_ENTRY(IFabricTransportMessage)
END_COM_MAP()


// helper functions to extract content from message

std::string get_header(IFabricTransportMessage *message);
// concat all body chunks to one
std::string get_body(IFabricTransportMessage *message);

public:
  FRPCTransportMessage();

  void Initialize(std::string header, std::string body);

  // copy content from another msg
  // if msg blob has multiple parts, this will concat all msg blobs into one
  void CopyFrom(IFabricTransportMessage * other);

  const std::string & GetHeader();
  const std::string & GetBody();

  // IFabricTransportMessage impl

  STDMETHOD_(void,GetHeaderAndBodyBuffer)      
    (/* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **headerBuffer,
      /* [out] */ ULONG *msgBufferCount,
      /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **MsgBuffers) override;

  STDMETHOD_(void,Dispose)(void) override;

private:
  std::string body_;
  FABRIC_TRANSPORT_MESSAGE_BUFFER body_ret_;
  std::string header_;
  FABRIC_TRANSPORT_MESSAGE_BUFFER header_ret_;
};

} // namespace fabricrpc