// ------------------------------------------------------------
// Copyright 2022 Youyuan Wu
// Licensed under the MIT License (MIT). See License.txt in the repo root for
// license information.
// ------------------------------------------------------------

// The code is based on protobuf generator code with the following copyright:

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <fcntl.h>
#include <io.h> // setmode

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/io_win32.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/logging.h>

#include <iostream>

import gen;

namespace pb = google::protobuf;
namespace pbc = pb::compiler;

bool GenerateCode(const pbc::CodeGeneratorRequest &request,
                  const CodeGenerator &generator,
                  pbc::CodeGeneratorResponse *response,
                  std::string *error_msg) {
  pb::DescriptorPool pool;
  for (int i = 0; i < request.proto_file_size(); i++) {
    const pb::FileDescriptor *file = pool.BuildFile(request.proto_file(i));
    if (file == nullptr) {
      // BuildFile() already wrote an error message.
      return false;
    }
  }

  std::vector<const pb::FileDescriptor *> parsed_files;
  for (int i = 0; i < request.file_to_generate_size(); i++) {
    parsed_files.push_back(pool.FindFileByName(request.file_to_generate(i)));
    if (parsed_files.back() == nullptr) {
      *error_msg = "protoc asked plugin to generate a file but "
                   "did not provide a descriptor for the file: " +
                   request.file_to_generate(i);
      return false;
    }
  }

  std::string error;
  bool succeeded = generator.GenerateAll(parsed_files, error);
  // currently the generator always return success.
  if (!succeeded && error.empty()) {
    error = "Code generator returned false but provided no error description.";
  }
  if (!error.empty()) {
    response->set_error(error);
  }
  return true;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    // TODO: support options.
    std::cerr << argv[0] << ": Unknown option: " << argv[1] << std::endl;
    return EXIT_FAILURE;
  }

#ifdef _WIN32
  // On windows protoc will send proto binary request from stdin.
  // we need to change stdin and stdout in binary mode.
  ::_setmode(_fileno(stdin), _O_BINARY);
  ::_setmode(_fileno(stdout), _O_BINARY);
#endif

  pbc::CodeGeneratorRequest request;

  if (!request.ParseFromIstream(&std::cin)) {
    std::cerr << argv[0] << ": protoc sent unparseable request to plugin."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::string error_msg;
  pbc::CodeGeneratorResponse response;

  CodeGenerator generator;

  if (GenerateCode(request, generator, &response, &error_msg)) {
    if (!response.SerializeToOstream(&std::cout)) {
      std::cerr << argv[0] << ": Error writing to stdout." << std::endl;
      return EXIT_FAILURE;
    }
  } else {
    if (!error_msg.empty()) {
      std::cerr << argv[0] << ": " << error_msg << std::endl;
    }
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}