// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <atlbase.h>
#include <atlcom.h>

#include "fabrictransport_.h"

#include "fabricrpc/Operation.h"

#include <memory>

namespace fabricrpc{

class FRPCRequestHandler
  : public CComObjectRootEx<CComSingleThreadModel>,
    public IFabricTransportMessageHandler {
BEGIN_COM_MAP(FRPCRequestHandler)
  COM_INTERFACE_ENTRY(IFabricTransportMessageHandler)
END_COM_MAP()

public:
  FRPCRequestHandler();
  void Initialize(std::shared_ptr<MiddleWare> svc);

  HRESULT STDMETHODCALLTYPE BeginProcessRequest(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback *callback,
      /* [retval][out] */ IFabricAsyncOperationContext **context) override;

  HRESULT STDMETHODCALLTYPE EndProcessRequest(
      /* [in] */ IFabricAsyncOperationContext *context,
      /* [retval][out] */ IFabricTransportMessage **reply) override;

  HRESULT STDMETHODCALLTYPE HandleOneWay(
      /* [in] */ COMMUNICATION_CLIENT_ID clientId,
      /* [in] */ IFabricTransportMessage *message) override;
private:
    std::shared_ptr<MiddleWare> svc_;
};

} // namespace fabricrpc