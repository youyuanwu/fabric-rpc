# Fabric RPC
RPC framework based on FabricTransport in Service Fabric.

Generate server and client code from proto file, similar to grpc.

# Dependencies
Required:
* service fabric runtime installation. See [get-started](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-get-started)

Auto downloaded by cmake:
* [service-fabric-cpp](https://github.com/youyuanwu/service-fabric-cpp)
    The generated code has no dependency to service-fabric-cpp.
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
## Nuget
Nuget is publish with windows MSVC build. Since MSVC is ABI compatible, the binary can be used on windows platform.
[FabricRpc](https://www.nuget.org/packages/FabricRpc)

# License
The Fabric RPC is covered by MIT license.

Code generated by the fabric_rpc_cpp_plugin is owned by the owner of the input file used when generating it.
But the generated code requires linking with the Fabric RPC support library, which is covered by MIT license,
and requires linking with protobuf with google's license.