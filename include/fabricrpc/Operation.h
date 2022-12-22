// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "fabricrpc/Status.h"
#include <functional>
#include <memory>

namespace fabricrpc{

class IBeginOperation{
public:
    virtual Status Invoke(std::string body, IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext**context) = 0;
    virtual ~IBeginOperation() = default;
};

template<typename T>
class BeginOperation : public IBeginOperation {
public:
    BeginOperation(std::function<Status(const T*, IFabricAsyncOperationCallback *, /*out*/ IFabricAsyncOperationContext**)> op)
        :op_(op){}

    Status Invoke(std::string body, IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext**context) override {
        T req;
        // deserialize
        bool ok = req.ParseFromString(body);
        if (!ok) {
            return Status(StatusCode::INVALID_ARGUMENT, "cannot parse body");
        }
        return op_(&req, callback, context);
    }
private:
    std::function<Status(const T*, IFabricAsyncOperationCallback *, /*out*/ IFabricAsyncOperationContext**)> op_;
};

class IEndOperation{
public:
    virtual Status Invoke(IFabricAsyncOperationContext* context, std::string & reply) = 0;
    virtual ~IEndOperation() = default;
};

template<typename T>
class EndOperation : public IEndOperation{
public:
    EndOperation(std::function<Status(IFabricAsyncOperationContext *context, /*out*/T*)> op)
        :op_(op){}

    Status Invoke(IFabricAsyncOperationContext* context, std::string & reply) override{
        T proto;
        Status err = op_(context, &proto);
        if(err){
            return err;
        }
        // serialize to reply
        bool ok = proto.SerializeToString(&reply);
        assert(ok); // This only happens in dbg mode
        if (!ok) {
            return Status(StatusCode::INTERNAL, "Server cannot serialize body.");
        }
        return Status();
    }
private:
    std::function<Status(IFabricAsyncOperationContext *context, /*out*/T*)> op_;
};

class MiddleWare {
public:
    virtual Status Route(const std::string & url, std::unique_ptr<IBeginOperation> & beginOp, std::unique_ptr<IEndOperation> & endOp) = 0;
};

} // namespace fabricrpc