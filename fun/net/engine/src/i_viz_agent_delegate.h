#pragma once

namespace fun {
namespace net {

class NetClientImpl;
class NetServerImpl;

class IVizAgentDelegate {
 public:
  virtual ~IVizAgentDelegate() {}

  virtual double GetAbsoluteTime() = 0;
  virtual NetClientImpl* QueryNetClient() = 0;
  virtual NetServerImpl* QueryNetServer() = 0;
};

} // namespace net
} // namespace fun
