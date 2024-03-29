# Fabric RPC
![ci](https://github.com/youyuanwu/fabric-rpc/actions/workflows/build.yaml/badge.svg)
[![codecov](https://codecov.io/github/youyuanwu/fabric-rpc/branch/main/graph/badge.svg?token=RE9WQSS0MU)](https://codecov.io/github/youyuanwu/fabric-rpc)

RPC framework based on FabricTransport in Service Fabric.

Generate server and client code from proto file, similar to grpc.

# Dependencies
Required:
* service fabric runtime installation. See [get-started](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-get-started)

Auto downloaded by cmake:
* [fabric-metadata](https://github.com/youyuanwu/fabric-metadata). Service fabric cpp COM headers.
* [protobuf](https://github.com/protocolbuffers/protobuf)

The fabric_rpc.lib and all support headers do not depend on protobuf.lib and header, so user can intergrate fabricrpc with any protobuf version.
The generated fabricrpc code depends on protobuf lib and headers.
User needs to generate protobuf cpp code for [fabricrpc.proto](../protos/fabricrpc.proto) file in order to compile the generated fabricrpc code.
See the examples for details.

# Build
```ps1
cmake . -B build
cmake --build build
```

# Tutorial
See [Tutorial](./docs/Tutorial.md).

# Details
The spec for networking protocol on top of FabricTransport is here [ProtocolSpec](./docs/ProtocolSpec.md)

# Packaging Distribution
Only build from source is supported

# License
The Fabric RPC is covered by MIT license.

Code generated by the fabric_rpc_cpp_plugin is owned by the owner of the input file used when generating it.
But the generated code requires linking with the Fabric RPC support library, which is covered by MIT license,
and requires linking with protobuf with google's license.