#pragma once

#include "fabrictransport_.h"
#include <string>

namespace fabricrpc {

class endpoint {
public:
  endpoint(std::wstring host, std::uint32_t port);

  const FABRIC_TRANSPORT_SETTINGS *get_settings() const;
  const FABRIC_TRANSPORT_LISTEN_ADDRESS *get_addr() const;

  std::wstring get_url() const;

private:
  std::wstring host_;
  std::uint32_t port_;

  // for now setting is const
  FABRIC_TRANSPORT_SETTINGS settings_;
  FABRIC_SECURITY_CREDENTIALS creds_;
  FABRIC_TRANSPORT_LISTEN_ADDRESS addr_;
};

} // namespace fabricrpc