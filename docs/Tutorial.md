# How to use FabricRpc to write server and client

## Code Generation
Generated code is for server and client together.
* Generate fabric rpc protobuf code. [fabricrpc.proto](../protos/fabricrpc.proto) This proto is used to parse fabricrpc headers.
```cmake
#CMake
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS fabricrpc.proto)
```
(You can avoid generate this by linking with fabric_rpc_proto target in this repo.)
Generated code includes `fabricrpc.pb.cc` and `fabricrpc.pb.h`.

* Generate protobuf cpp code. The same as grpc. [helloworld.proto](../examples/helloworld/helloworld.proto)
```cmake
#CMake
include(FindProtobuf)
set(_proto_file_path helloworld.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${_proto_file_path})
```
This effectively runs the protoc.exe of the following command:
```ps1
protoc.exe `
  --cpp_out ${out_dir} `
  -I ${dir_of_proto_file} `
  ${proto_file_full_path}
```
Generated file includes `helloworld.pb.cc` and `helloworld.pb.h`. 

* Generate fabric rpc cpp code
```cmake
#CMake
protobuf_generate(LANGUAGE grpc
    PLUGIN "protoc-gen-grpc=${path_to_fabric_rpc_cpp_plugin}"
    OUT_VAR FABRIC_RPC_SRCS
    APPEND_PATH
    GENERATE_EXTENSIONS
        .fabricrpc.h
        .fabricrpc.cc
    PROTOS ${_proto_file_path}
)
```
This effectively executes the following command:
```ps1
protoc.exe `
  --grpc_out ${out_dir}  `
  --plugin=protoc-gen-grpc=${path_to_plugin}\fabric_rpc_cpp_plugin.exe `
  -I ${dir_of_proto_file} `
  ${proto_file_full_path}
```
Generated file includes `helloworld.fabricrpc.cc` and `helloworld.fabricrpc.h`.

## Implement Server
Generated service is a virtual class. Function signature is in classic service fabric async framework style.
The implementation for Route() function is generated and user does not need to implment it; it handles the routing for each Begin and End operation pair, and the generated code use it to hook into the FabricTransport library.
```cpp
namespace helloworld {
class FabricHello final {
  public:
  static constexpr char const* service_full_name() {
    return "helloworld.FabricHello";
  }
  class Service : public fabricrpc::MiddleWare {
    public:
    Service() {};
    virtual ~Service() {};
    virtual fabricrpc::Status BeginSayHello(const FabricRequest* request, IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext **context) = 0;
    virtual fabricrpc::Status EndSayHello(IFabricAsyncOperationContext *context, /*out*/FabricResponse* response) = 0;
    virtual fabricrpc::Status Route(const std::string & url, std::unique_ptr<fabricrpc::IBeginOperation> & beginOp, std::unique_ptr<fabricrpc::IEndOperation> & endOp) override;
  };
};

```
User needs to subclass the generated virtual class. The com object implementation of returned context can be anything that satisfies `IFabricAsyncOperationContext` signature. In this example the `ctx` object carries the return string of the hello response, and it is passed from the begin function to the end function.
```cpp
#include "helloworld.fabricrpc.h"

class Service_Impl : public helloworld::FabricHello::Service {
public:
  fabricrpc::Status
  BeginSayHello(const ::helloworld::FabricRequest *request,
                IFabricAsyncOperationCallback *callback,
                /*out*/ IFabricAsyncOperationContext **context) override {
    std::string msg = "hello " + request->fabricname();
    std::any a = std::move(msg);
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx =
        async_any_context::create_instance(callback, std::move(a)).to_ptr();
    *context = ctx.detach();
    return fabricrpc::Status();
  }
  fabricrpc::Status
  EndSayHello(IFabricAsyncOperationContext *context,
              /*out*/ ::helloworld::FabricResponse *response) override {
    async_any_context *ctx = dynamic_cast<async_any_context *>(context);
    std::any &a = ctx->get_any();
    std::string message = std::any_cast<std::string>(a);
    response->set_fabricmessage(std::move(message));
    return fabricrpc::Status();
  }
};
```
## Open the server
FabricTransport handles accepting and dispatching network request to a IFabricTransportMessageHandler implementation.
FabricRPC wraps each generated Service class (subclass of MiddleWare) in a IFabricTransportMessageHandler.
Use generated `helloworld::CreateFabricRPCRequestHandler(const std::vector<std::shared_ptr<fabricrpc::MiddleWare>> & svcList, IFabricTransportMessageHandler **handler)` function to obtain a IFabricTransportMessageHandler, passing in the implementation of the generated virtual class.
Then FabricTransport function `CreateFabricTransportListener` with IFabricTransportMessageHandler to obtain a IFabricTransportListener.
Once IFabricTransportListener is opened, the server is running and handling requests.
```cpp
  // ... Some setup code omitted 

  std::shared_ptr<fabricrpc::MiddleWare> hello_svc =
      std::make_shared<Service_Impl>();

  belt::com::com_ptr<IFabricTransportMessageHandler> req_handler;
  helloworld::CreateFabricRPCRequestHandler({hello_svc}, req_handler.put());

  belt::com::com_ptr<IFabricTransportConnectionHandler> conn_handler =
      sf::transport_dummy_server_conn_handler::create_instance().to_ptr();
  belt::com::com_ptr<IFabricTransportMessageDisposer> msg_disposer =
      sf::transport_dummy_msg_disposer::create_instance().to_ptr();
  belt::com::com_ptr<IFabricTransportListener> listener;

  // create listener
  HRESULT hr = CreateFabricTransportListener(
      IID_IFabricTransportListener, &settings, &addr, req_handler.get(),
      conn_handler.get(), msg_disposer.get(), listener.put());

  if (hr != S_OK) {
    return EXIT_FAILURE;
  }
  // open listener
  belt::com::com_ptr<IFabricStringResult> addr_str;
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = listener->BeginOpen(callback.get(), ctx.put());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = listener->EndOpen(ctx.get(), addr_str.put());
    if (hr != S_OK) {
      return EXIT_FAILURE;
    }
  }
  std::wcout << L"Listening on: " << std::wstring(addr_str->get_String());
```

## Create and open FabricTransportClient
This step is standard FabricTransport code, no FabricRpc involved.
```cpp
std::wstring addr_str = L"localhost:12345+/";

  belt::com::com_ptr<IFabricTransportCallbackMessageHandler> client_notify_h =
      sf::transport_dummy_client_notification_handler::create_instance()
          .to_ptr();

  belt::com::com_ptr<IFabricTransportClientEventHandler> client_event_h =
      sf::transport_dummy_client_conn_handler::create_instance().to_ptr();
  belt::com::com_ptr<IFabricTransportMessageDisposer> client_msg_disposer =
      sf::transport_dummy_msg_disposer::create_instance().to_ptr();
  ;
  belt::com::com_ptr<IFabricTransportClient> client;

  // open client
  hr = CreateFabricTransportClient(
      /* [in] */ IID_IFabricTransportClient,
      /* [in] */ &settings,
      /* [in] */ addr_str.c_str(),
      /* [in] */ client_notify_h.get(),
      /* [in] */ client_event_h.get(),
      /* [in] */ client_msg_disposer.get(),
      /* [retval][out] */ client.put());
  if (hr != S_OK) {
    BOOST_LOG_TRIVIAL(debug) << "cannot create client " << hr;
    return EXIT_FAILURE;
  }

  // open client
  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;
    hr = client->BeginOpen(1000, callback.get(), ctx.put());
    if (hr != S_OK) {
      BOOST_LOG_TRIVIAL(debug) << "cannot begin open " << hr;
      return EXIT_FAILURE;
    }
    callback->Wait();
    hr = client->EndOpen(ctx.get());
    if (hr != S_OK) {
      BOOST_LOG_TRIVIAL(debug) << "cannot end open " << hr;
      return EXIT_FAILURE;
    }
  }
```
## Use the generated client
Generated code wraps around IFabricTransportClient and use it to send messages.
```cpp
class FabricHelloClient{
public:
  FabricHelloClient(IFabricTransportClient * client);
  fabricrpc::Status BeginSayHello(const FabricRequest* request, IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext **context);
  fabricrpc::Status EndSayHello(IFabricAsyncOperationContext *context, /*out*/FabricResponse* response);
private:
  CComPtr<IFabricTransportClient> client_;
  std::shared_ptr<fabricrpc::IFabricRPCHeaderProtoConverter> cv_;
};
```
Once the IFabricTransportClient is opened create the `helloworld::FabricHelloClient` and then send requests.
```cpp
  // make request
  helloworld::FabricHelloClient h_client(client.get());

  {
    belt::com::com_ptr<sf::IFabricAsyncOperationWaitableCallback> callback =
        sf::FabricAsyncOperationWaitableCallback::create_instance().to_ptr();
    belt::com::com_ptr<IFabricAsyncOperationContext> ctx;

    helloworld::FabricRequest request;
    request.set_fabricname("world");

    fabricrpc::Status err;

    err = h_client.BeginSayHello(&request, callback.get(), ctx.put());
    if (err) {
      BOOST_LOG_TRIVIAL(debug) << "BeginSayHello Failed " << err;
      return EXIT_FAILURE;
    }
    callback->Wait();

    helloworld::FabricResponse response;
    err = h_client.EndSayHello(ctx.get(), &response);
    if (err) {
      BOOST_LOG_TRIVIAL(debug) << "SaySayHelloEnd Failed " << err;
      return EXIT_FAILURE;
    }

    BOOST_LOG_TRIVIAL(info) << "Reply: " << response.fabricmessage();
  }
```

## Build and link this example
Generated h and cc files are needed for build.
```cmake
add_executable(hello_server
  main.cpp
  fabricrpc.pb.cc
  fabricrpc.pb.h
  helloworld.pb.cc
  helloworld.pb.h
  helloworld.fabricrpc.cc
  helloworld.fabricrpc.h
)
target_link_libraries(hello_server 
  PRIVATE 
  fabric_rpc # lib of fabricrpc with src code in src/ of this repo
  FabricTransport.lib # support lib for FabricTransport.dll
)
target_include_directories(hello_server
  PRIVATE
  ${Include_dir_for_fabric_rpc} # header files in include/ of this repo
  ${Include_dir_for_fabrictransport_.h} # generated from sf idl file.
)
```
Remarks:

`fabric_rpc` lib is compiled from [src](../src/). It contains the internal implementation of IFabricTransportMessageHandler for fabric rpc etc. It is a counter part of `grpc++.lib` in grpc where its generated code depends on.
One can separate fabricrpc.ph.cc and/or helloworld.fabricrpc.cc etc in a seperate target from the main executable, as is done the the full example in this repo.

## Full working code
See the full working code in [helloworld](../examples/helloworld/)
After build this project the following cmd can be used to test the example is working.
```ps1
# Start server
.\build\examples\helloworld\Debug\hello_server.exe
```
In another shell
```ps1
# Make request using client
.\build\examples\helloworld\Debug\hello_client.exe
```