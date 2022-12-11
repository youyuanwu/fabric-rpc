// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

module;
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/io_win32.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <fstream>

export module gen;

namespace pb = google::protobuf;

// helper functions
std::string getProtoNameNoExt(std::string const & filename){
  std::string proto_name = filename;
  std::size_t dot = proto_name.rfind(".proto");
  if (dot != std::string::npos)
  {
      proto_name.resize(dot);
  }
  return proto_name;
}

class printer{
public:
  printer(std::string &output): indent_(0), output_(output) {}

  void Add(const std::string & lines) {
    //
    // use 2 space indent
    std::string indent_str;
    for(std::size_t i = 0 ; i < indent_; i++)
    {
      indent_str += " ";
      indent_str += " ";
    }

    std::vector<std::string> strs;
    boost::split(strs,lines,boost::is_any_of("\n"));
    std::string indented_lines;
    for(auto str : strs){
      if(str.empty()){
        continue;
      }
      indented_lines += indent_str + str + "\n";
    }
    output_ += indented_lines;
  }

  
  void AddLn(const std::string & lines){
    Add(lines + "\n");
  }

  void Add(const std::map<std::string, std::string> &vars, const std::string & lines){
    // replace all occurances of $var with value in lines before add to output
    std::string replaced_lines = lines;
    for (auto kv : vars){
      std::string key = "$" + kv.first + "$";
      const std::string & val = kv.second;
      boost::replace_all(replaced_lines, key, val);
    }
    Add(replaced_lines);
  }

  
  void AddLn(const std::map<std::string, std::string> &vars, const std::string & lines){
    Add(vars, lines + "\n");
  }

  void Indent(){
    indent_++;
  }

  void Outdent(){
    assert(indent_ >0);
    indent_--;
  }

  std::string GetOutput(){
    return output_;
  }

private:
  std::size_t indent_;
  std::string & output_;
};

// generator for a single pb file.
class pbGen{
public:
  pbGen(const pb::FileDescriptor* file): file_(file) {}

  // Header generation methods

  std::string GenerateHeaderContent(){
    
    std::map<std::string, std::string> vars;
    vars["filename"] = file_->name();
    vars["filename_base"] = getProtoNameNoExt(file_->name());
    vars["message_header_ext"] = ".pb.h";

    std::string output;
    printer p(output);
    p.AddLn("// Generated by fabric_rpc_cpp_plugin. Do not Edit.");

    p.AddLn(vars, "// source: $filename$");

    p.AddLn(vars, "#pragma once");

    // protobuf header
    p.AddLn(vars, "#include \"$filename_base$$message_header_ext$\"");
    
    // fabric rpc required headers
    p.Add(
      "#include \"fabrictransport_.h\"\n"
      "#include <atlbase.h>\n"
      "#include <atlcom.h>\n"
      "#include \"fabricrpc/Operation.h\"\n"
    );

    std::string services_code = GetHeaderServices(file_);

    return p.GetOutput() + services_code + GetHeaderClients(file_);
  }
    // add services code
  std::string GetHeaderServices(const pb::FileDescriptor* file) {
    std::string output;
    printer p(output);
    std::map<std::string, std::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    vars["Namespace"] = file->package();
    if(vars["Namespace"].empty()){
      vars["Namespace"] = "fabricrpc";
    }

    p.AddLn(vars,"// Service pkg $Package$");

    p.AddLn(vars,"namespace $Namespace$ {");

    for (int i = 0; i < file->service_count(); ++i) {
      PrintHeaderService(p, file->service(i), vars);
      //printer->Print("\n");
    }

    // create the request handler
    p.AddLn("void CreateFabricRPCRequestHandler(std::shared_ptr<fabricrpc::MiddleWare> svc, IFabricTransportMessageHandler ** handler);");

    p.AddLn(vars,"} // namespace $Namespace$");
    return p.GetOutput();
  }

  void PrintHeaderService(printer& p, const google::protobuf::ServiceDescriptor *service, std::map<std::string, std::string> vars) {
      vars["Service"] = service->name();

    p.Add(vars,
            "class $Service$ final {\n"
            "  public:\n");
    p.Indent();

    // Service metadata
    p.Add(vars,
        "static constexpr char const* service_full_name() {\n"
        "  return \"$Package$$Service$\";\n"
        "}\n");

    // Server side - base
    p.Add(
        "class Service : public fabricrpc::MiddleWare {\n"
        "  public:\n");
    p.Indent();
    p.Add("Service() {};\n");
    p.Add("virtual ~Service() {};\n");

    // methods that user needs to implement
    for (int i = 0; i < service->method_count(); ++i) {
      PrintHeaderServerMethodSync(p, service->method(i), vars);
    }

    // generated Routing method.
    p.AddLn(vars, "virtual fabricrpc::Status Route(const std::string & url, std::unique_ptr<fabricrpc::IBeginOperation> & beginOp, std::unique_ptr<fabricrpc::IEndOperation> & endOp) override;");

    p.Outdent();
    p.Add("};\n");

    p.Outdent();
    p.AddLn("};");
  }

  void PrintHeaderServerMethodSync(printer& p, const google::protobuf::MethodDescriptor * method, std::map<std::string, std::string> &vars){
    vars["Method"] = method->name();
    vars["Request"] = method->input_type()->name();
    vars["Response"] = method->output_type()->name();
    bool no_streaming = !(method->client_streaming() || method->server_streaming());
    if(no_streaming){
      p.AddLn(vars,
          "virtual fabricrpc::Status Begin$Method$("
          "const $Request$* request, "
          "IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext **context) = 0;");
      p.AddLn(vars,
          "virtual fabricrpc::Status End$Method$("
          "IFabricAsyncOperationContext *context, /*out*/$Response$* response) = 0;");
    }else{
      p.AddLn("// Streamingfor method $Method$ request $Request$ response $Response$ not supported ");
    }
  }

  // CC generation methods

  void PrintCCService(printer& p, const google::protobuf::ServiceDescriptor *service, std::map<std::string, std::string> vars){
    // generate routing code.
    vars["Service"] = service->name();

    p.AddLn(vars, "fabricrpc::Status $Service$::Service::Route(const std::string & url, std::unique_ptr<fabricrpc::IBeginOperation> & beginOp, std::unique_ptr<fabricrpc::IEndOperation> & endOp) {");
    p.Indent();
    for (int i = 0; i < service->method_count(); ++i) {
      const google::protobuf::MethodDescriptor * method = service->method(i);
      vars["Method"] = method->name();
      vars["Request"] = method->input_type()->name();
      vars["Response"] = method->output_type()->name();
      p.Add(vars, "if (url == \"/$Package$$Service$/$Method$\") {\n");
      p.Indent();
      p.Add(vars,
        "auto bo = std::bind(&Service::Begin$Method$, this, std::placeholders::_1,\n"
        "              std::placeholders::_2, std::placeholders::_3);\n"
        "auto eo = std::bind(&Service::End$Method$, this, std::placeholders::_1,\n"
        "              std::placeholders::_2);\n"
        "beginOp = std::make_unique<fabricrpc::BeginOperation<$Request$>>(bo);\n"
        "endOp = std::make_unique<fabricrpc::EndOperation<$Response$>>(eo);\n");
      p.Outdent();
    }
    p.Add("} else {\n"
        "  return fabricrpc::Status(fabricrpc::StatusCode::NOT_FOUND, \"method not found: \" + url);\n"
        "}\n"
    );

    p.AddLn("return fabricrpc::Status();");
    p.Outdent();
    p.AddLn("}");

  }

  std::string GetCCServices(const pb::FileDescriptor* file){
    std::string output;
    printer p(output);
    std::map<std::string, std::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    vars["Namespace"] = file->package();
    if(vars["Namespace"].empty()){
      vars["Namespace"] = "fabricrpc";
    }

    p.AddLn(vars,"// Service pkg $Package$");

    p.AddLn(vars,"namespace $Namespace$ {");

    //printServiceRoutingCode(p, vars);
    for (int i = 0; i < file->service_count(); ++i) {
      PrintCCService(p, file->service(i), vars);
    }

    // create the request handler
    p.AddLn("void CreateFabricRPCRequestHandler(std::shared_ptr<fabricrpc::MiddleWare> svc,\n"
            "                                    IFabricTransportMessageHandler **handler) {");
    p.Indent();
    p.AddLn("CComPtr<CComObjectNoLock<fabricrpc::FRPCRequestHandler>> msgHandlerPtr(\n"
            "  new CComObjectNoLock<fabricrpc::FRPCRequestHandler>());\n"
            "msgHandlerPtr->Initialize(svc);\n"
            "*handler = msgHandlerPtr.Detach();\n"
    );
    p.Outdent();
    p.AddLn("}");

    p.AddLn(vars,"} // namespace $Namespace$");
    return p.GetOutput();
  }

  std::string GenerateCCContent(){
    std::map<std::string, std::string> vars;
    vars["filename"] = file_->name();
    vars["filename_base"] = getProtoNameNoExt(file_->name());
    vars["message_header_ext"] = ".pb.h";

    std::string output;
    printer p(output);
    p.AddLn("// Generated by fabric_rpc_cpp_plugin. Do not Edit.");

    p.AddLn(vars, "// source: $filename$");

    // include generated header
    p.AddLn(vars, "#include \"$filename_base$.fabricrpc.h\"");
    
    // fabric rpc required headers
    p.Add(
      "#include <functional>\n"
      "#include \"fabricrpc/ClientHelpers.h\"\n"
      "#include \"fabricrpc/FRPCRequestHandler.h\"\n"
    );
    
    std::string services_cc_code = GetCCServices(file_);

    return p.GetOutput() + services_cc_code + GetCCClientServices(file_);
  }

  // client code generation block
  void PrintHeaderClientMethodSync(printer& p, const google::protobuf::MethodDescriptor * method, std::map<std::string, std::string> &vars){
    vars["Method"] = method->name();
    vars["Request"] = method->input_type()->name();
    vars["Response"] = method->output_type()->name();
    bool no_streaming = !(method->client_streaming() || method->server_streaming());
    if(no_streaming){
      p.AddLn(vars,
          "fabricrpc::Status Begin$Method$("
          "const $Request$* request, "
          "IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext **context);");
      p.AddLn(vars,
          "fabricrpc::Status End$Method$("
          "IFabricAsyncOperationContext *context, /*out*/$Response$* response);");
    }else{
      p.AddLn("// Streamingfor method $Method$ request $Request$ response $Response$ not supported ");
    }
  }

  void PrintHeaderClientService(printer& p, const google::protobuf::ServiceDescriptor *service, std::map<std::string, std::string> vars){
    vars["Service"] = service->name();
    p.Add(vars, 
      "class $Service$Client{\n"
      "public:\n");
    p.Indent();
    p.AddLn(vars, "$Service$Client(IFabricTransportClient * client);");
    for (int i = 0; i < service->method_count(); ++i) {
      PrintHeaderClientMethodSync(p, service->method(i), vars);
    }
    p.Outdent();
    p.Add("private:\n");
    p.Add("  CComPtr<IFabricTransportClient> client_;");
    p.Add("};\n");
  }

  std::string GetHeaderClients(const pb::FileDescriptor* file) {
    std::string output;
    printer p(output);
    std::map<std::string, std::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    vars["Namespace"] = file->package();
    if(vars["Namespace"].empty()){
      vars["Namespace"] = "fabricrpc";
    }

    p.AddLn(vars,"// Client code");

    p.AddLn(vars,"namespace $Namespace$ {");

    for (int i = 0; i < file->service_count(); ++i) {
      PrintHeaderClientService(p, file->service(i), vars);
    }

    // create the request handler

    p.AddLn(vars,"} // namespace $Namespace$");
    return p.GetOutput();
  }

  void PrintCCClientService(printer& p, const google::protobuf::ServiceDescriptor *service, std::map<std::string, std::string> vars){
    // generate routing code.
    vars["Service"] = service->name();
    p.Add(vars,
      "$Service$Client::$Service$Client(IFabricTransportClient *client)\n"
      "  : client_() {\n"
      "  client->AddRef();\n"
      "  client_.Attach(client);\n"
      "}\n"
    );
    for (int i = 0; i < service->method_count(); ++i) {
      const google::protobuf::MethodDescriptor * method = service->method(i);
      vars["Method"] = method->name();
      vars["Request"] = method->input_type()->name();
      vars["Response"] = method->output_type()->name();
      bool no_streaming = !(method->client_streaming() || method->server_streaming());
      if(no_streaming){
        p.AddLn(vars,
            "fabricrpc::Status $Service$Client::Begin$Method$("
            "const $Request$* request, "
            "IFabricAsyncOperationCallback *callback, /*out*/ IFabricAsyncOperationContext **context){\n"
            "  return fabricrpc::ExecClientBegin(client_, \"/$Package$$Service$/$Method$\", request,\n"
            "             callback, context);"
            "}\n"
        );
        p.AddLn(vars,
            "fabricrpc::Status $Service$Client::End$Method$("
            "IFabricAsyncOperationContext *context, /*out*/$Response$* response){\n"
            "  return fabricrpc::ExecClientEnd(client_, context, response);"
            "}\n"
        );
      }else{
        p.AddLn("// Streamingfor method $Method$ request $Request$ response $Response$ not supported ");
      }
    }
  }

  std::string GetCCClientServices(const pb::FileDescriptor* file){
    std::string output;
    printer p(output);
    std::map<std::string, std::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    vars["Namespace"] = file->package();
    if(vars["Namespace"].empty()){
      vars["Namespace"] = "fabricrpc";
    }

    p.AddLn(vars,"// Client code");

    p.AddLn(vars,"namespace $Namespace$ {");

    for (int i = 0; i < file->service_count(); ++i) {
      PrintCCClientService(p, file->service(i), vars);
    }

    p.AddLn(vars,"} // namespace $Namespace$");
    return p.GetOutput();
  }

private:

  const pb::FileDescriptor* file_;
};

export class CodeGenerator{
public:
  bool GenerateAll(std::vector<const pb::FileDescriptor*> parsed_files
                                      , std::string & error) const {

    for(auto f : parsed_files){
      if(!Generate(f)){
        error = "Generate failed";
        return false;
      }
    }
    return true;
  }

  bool Generate(const pb::FileDescriptor* file) const {
    // usually input file name is myapp.proto
    // out file should be myapp.fabricrpc.h
    // and myapp.fabricrpc.cc
    std::string proto_name = getProtoNameNoExt(file->name());
    // generate header
    std::string out_header_file_name = proto_name + ".fabricrpc.h";
    std::ofstream o_header(out_header_file_name.c_str());

    pbGen gen(file);

    o_header << gen.GenerateHeaderContent();
    o_header.close();

    // generate cc
    std::string out_cc_file_name = proto_name + ".fabricrpc.cc";
    std::ofstream o_cc(out_cc_file_name.c_str());

    o_cc << gen.GenerateCCContent();
    o_cc.close();

    return true;
  }

};

