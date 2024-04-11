#include "fabricrpc/endpoint.hpp"

namespace fabricrpc {

endpoint::endpoint(std::wstring host, std::uint32_t port)
    : host_(host), port_(port), settings_(), creds_(), addr_() {

  creds_.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;
  settings_.KeepAliveTimeoutInSeconds = 10;
  settings_.MaxConcurrentCalls = 10;
  settings_.MaxMessageSize = 100;
  settings_.MaxQueueSize = 100;
  settings_.OperationTimeoutInSeconds = 30;
  settings_.SecurityCredentials = &creds_;

  addr_.IPAddressOrFQDN = host_.c_str();
  addr_.Port = port_;
  addr_.Path = L"/";
}

const FABRIC_TRANSPORT_LISTEN_ADDRESS *endpoint::get_addr() const {
  return &this->addr_;
}

const FABRIC_TRANSPORT_SETTINGS *endpoint::get_settings() const {
  return &this->settings_;
}

std::wstring endpoint::get_url() const {
  return host_ + L":" + std::to_wstring(port_) + L"+/";
}

} // namespace fabricrpc