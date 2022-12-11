# Fabric RPC protocol specification

## Fabric Transport Layer
For each request and response fabric transport layer can deliver one header bytes blob and multiple content(body) bytes blob. The blob content needs to be present all at once, i.e. there is no streaming support. Fabric RPC will use header blob to send metadata and only one content blob to send payload body. 

## Fabric RPC Specification File
Fabric RPC uses protobuf specification to specify RPC contract, and only supports unary operation, because streaming in not supported in transport layer.
The developer experience is aimed to be comparable to grpc.

## RPC content layout
Header and content blobs will be encoded protobuf bytes.
See [fabricrpc.proto](../protos/fabricrpc.proto) for header definitions.
The content is specified as follows:
### Request header
Request header contains the following information:
* Method url string. The same as the http url in grpc sepcification. It is the service name plus the method name.
### Request body
Request body is any protobuf payload.
### Response header
Response header contains the following information:
* Status code. Similar to grpc status code. 0 for success etc.
* Status message. Similar to grpc status message. (optional)
### Response payload
Response payload is any protobuf payload in the Fabric RPC proto file.



