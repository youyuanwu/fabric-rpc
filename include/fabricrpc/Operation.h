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
};

template<typename T>
class BeginOperation : public IBeginOperation {
public:
    BeginOperation(std::function<Status(const T*, IFabricAsyncOperationCallback *, /*out*/ IFabricAsyncOperationContext**)> op)
        :op_(op){}

    Status Invoke(std::string body, IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext**context) override {
        T req;
        // deserialize
        bool ok = req.ParseFromArray(body.data(), static_cast<int>(body.size()));
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
        std::int32_t len = static_cast<std::int32_t>(proto.ByteSizeLong());
        reply.resize(len, 0);
        bool ok = proto.SerializeToArray(reply.data(), len);
        assert(ok);
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