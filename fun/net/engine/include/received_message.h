#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
TODO
*/
class ReceivedMessage {
 public:
  MessageIn unsafe_message;
  HostId remote_id;
  InetAddress remote_addr_udp_only;
  bool relayed;
  //double action_time; //@deprecated

 public:
  inline ReceivedMessage()
    : relayed(false)
    , remote_id(HostId_None)
    , remote_addr_udp_only(InetAddress::None)
    //, action_time(0)
  {
  }
};

/*
class ReceivedMessageList
  //NOTE InlineAllocator를 사용하면 문제가 있는듯 싶은데??  일단 확인을 위해서, 막음.
  //: public Array<ReceivedMessage, InlineAllocator<128>>
  : public Array<ReceivedMessage>
  , public Noncopyable //WARNING: 덩치가 상당하므로, 복사는 허용하지 않음.
{
 public:
  inline ReceivedMessageList()
  {
    //TODO MinCapacity는 해주는 편이 좋지 아니할까???
    //Reserve 보다는 Minimum capacity를 지정할 수 있도록 하여, 하한선을 그을 수 있도록 하는게 좋을듯...
    //Reserve(1024);

    //SetMinCapacity(1024); //@note Allocate pre-allocated space for efficiency
    //SetGrowPolicy(GrowBufferPolicy::HighSpeed);
  }
};
*/
typedef Array<ReceivedMessage, InlineAllocator<128>> ReceivedMessageList;

} // namespace net
} // namespace fun
