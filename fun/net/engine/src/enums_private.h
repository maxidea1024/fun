#pragma once

namespace fun {
namespace net {

/**
 * Internal Engine Layer Message Types.
 */
enum class MessageType {
  None = 0,

  //
  // RPC / Freeform
  //

  /**
   * RPC(Remote Procedure Call) 메시지입니다.
   */
  RPC = 1,

  /**
   * RPC가 아닌 사용자 정의 메시지입니다.
   */
  FreeformMessage = 2,

  //
  // 접속 / 인증
  //

  /**
   * S2C (TCP)
   *
   * 서버에 물리적으로 접속된 상태에서, 일정시간 동안 인증이 이루어지지
   * 않을 경우, 접속을 해제하는 절차를 밟게 되는되는데 이때 서버에서
   * 클라이언트로 보내지는 메시지입니다.
   */
  ConnectServerTimedout = 3,

  /**
   * S2C (TCP)
   *
   * 클라이언트에서 NotifyServerConnectionHint 메시지를 받았을때, 서버에서
   * 클라이언트로 보내지는 메시지입니다.
   *
   * 클라이언트에서 서버로 접속하기 위한 기본 셋팅 정보를 클라이언트에게
   * 보내며, 공개키등이 보내집니다.
   *
   * payload:
   *   bool intra_logging_on - 내부 엔진 로깅 여부
   *   NetSettings settings - 클라이언트에서 서버로 접속하기 위한 네트워크 설정
   * 값들 ByteArray public_key_blob - 공개키 Blob
   */
  NotifyServerConnectionHint = 4,

  /**
   * C2S (TCP)
   *
   * 클라이언트가 서버에서 NotifyServerConnectionHint 메시지를 받게 되면,
   * 클라이언트가 서버로 보내는 메시지입니다.
   *
   * payload:
   *   ByteArray encrypted_aes_key_blob
   *   ByteArray encrypted_rc4_key
   */
  NotifyCSEncryptedSessionKey = 5,

  /**
   * S2C (TCP)
   *
   * 클라이언트가 NotifyCSEncryptedSessionKey 메시지를 보내면, 이를 받은
   * 서버는 받은 보안 키가 정상인지 여부를 판단해서 정상일 경우,
   * 클라이언트에게 보내지는 메시지입니다.
   *
   * 정상이 아닐 경우에는, 일정 시간후에 타임아웃이 걸려서,
   * ConnectServerTimedout 메시지를 클라이언트로 보내지며, 클라이언트의
   * 연결이 끊어지게 됩니다.
   *
   * payload:
   *   없음
   */
  NotifyCSSessionKeySuccess = 6,

  /**
   * C2S (TCP)
   *
   * 클라이언트가 서버에서 NotifyCSSessionKeySuccess 메시지를 받았을
   * 경우 서버로 보내는 메시지입니다.
   *
   * 내부 프로토콜 버젼정보등을 함께 보냅니다.
   *
   * payload:
   *   ByteArray user_data
   *   Uuid protocol_version
   *   uint32 internal_version
   */
  NotifyServerConnectionRequestData = 7,

  /**
   * S2C (TCP)
   *
   * 서버가 클라이언트가 보낸 NotifyServerConnectionRequestData 메시지를
   * 받아서 프로토콜 버젼을 확인한 결과 지원할 수 없는 경우이거나,
   * 오류가 있을 경우 클라이언트로 보내지는 메시지입니다.
   *
   * payload:
   *   없음
   */
  NotifyProtocolVersionMismatch = 8,

  /**
   * S2C (TCP)
   *
   * 서버가 클라이언트가 보낸 NotifyServerConnectionRequestData 메시지를
   * 받아서 확인해 본 후 암호화 키를 받은 적이 없을 경우 이 메시지를
   * 클라이언트로 보내고 접속을 해제합니다.
   *
   * 아직 까지 클라이언트는 미인증 상태이므로, 미인증 상태가 한동안 지속되면
   * 서버는 접속을 해제합니다.
   *
   * TODO NotifyProtocolVersionMismatch와 통합 시키는게 좋을듯 싶음.
   */
  NotifyServerDeniedConnection = 9,

  /**
   * S2C (TCP)
   *
   * 접속한 클라이언트가 최종적으로 접속이 승인되어, 후보에서
   * 인증된 클라이언트로 변경될 경우.
   *
   * 즉, 최종 인증성공 후 클라이언트로 보내지는 메시지입니다.
   *
   * payload:
   *   HostId host_id - 서버에서 발급받은 Host ID
   *   Uuid instance_tag_ - 서버의 Instance UUID (접속된 서버를 구분하기 위한
   * 용도) ByteArray response - 서버에서 필요시 보낸 응답메시지
   *         (INetServerCallbacks::OnConnectionRequest() 함수에서 채워집니다.)
   *   NamedInetAddress remote_addr - 접속한 클라이언트의 서버에서 인식한
   * 주소(public ip)
   */
  NotifyServerConnectSuccess = 10,

  //
  // Server Holepunching
  //

  /**
   * S2C (TCP)
   *
   * 클라이언트에게 서버와의 UDP 홀펀치을 시작하라고 지시합니다.
   *
   * payload:
   *   Uuid holepunch_tag - 해당 홀펀칭 동작의 매칭 여부를 확인하기 위해
   * 사용되는 UUID
   */
  RequestStartServerHolepunch = 11,

  /**
   * C2S (UDP)
   *
   * 서버와의 홀펀칭시 클라이언트가 서버에 보내는 메시지입니다.
   * UDP transport를 통해서 보내집니다.
   *
   * payload:
   *   Uuid holepunch_tag - 해당 홀펀칭 동작의 매칭 여부를 확인하기 위해
   * 사용되는 UUID (이 값은 서버에서 RequestStartServerHolepunch 메시지를 통해서
   * 받아두었던 값입니다.)
   */
  ServerHolepunch = 12,

  /**
   * S2C (UDP)
   *
   * 서버가 클라이언트에서 UDP transport를 통해서 ServerHolepunch 메시지를
   * 받았을 경우, 클라이언트로 보내는 메시지입니다.
   *
   * payload:
   *   Uuid holepunch_tag - 해당 홀펀칭 동작의 매칭 여부를 확인하기 위해
   * 사용되는 UUID InetAddress from - 서버의 주소 (클라이언트에서 확인을 위해.
   * 그러나 실제로는 필요치 않아보인다. 왜냐면 위의 holepunch_tag 값이 같다면,
   * 실질적으로 잘못 올 확률은 없을터... 뭐 여러 머신에서 한다고 쳐도 겹치지는
   * 않을듯 싶은데... UUID가 random에 기반한거라 좀 불안하기 할려나??)
   */
  ServerHolepunchAck = 13,

  /**
   * C2S (TCP)
   *
   * 클라이언트가 서버에서 ServerHolepunchAck 메시지를 받으면, 최종적으로
   * 서버와의 홀펀칭이 성공한걸로 간주하고 이 메시지를 서버에게 보냅니다.
   *
   * 최종 성공은 아니고 일단은 클라이언트에서만 성공인 상태입니다.
   *
   * payload:
   *   Uuid holepunch_tag - 해당 홀펀칭 동작의 매칭 여부를 확인하기 위해
   * 사용되는 UUID InetAddress udp_socket_local_addr - UDP 소켓의 로컬
   * 주소입니다. InetAddress here_addr_at_server - 서버에서 인식한 주소입니다.
   * (external IP)
   */
  NotifyHolepunchSuccess = 14,

  /**
   * S2C (TCP)
   *
   * 클라이언트가 보낸 NotifyHolepunchSuccess 메시지를 받은 서버는
   * 최종 서버와의 홀펀칭 성공을 알리기 위해서 이 메시지를
   * 클라이언트로 보내게 됩니다.
   *
   * payload:
   *   Uuid holepunch_tag - 해당 홀펀칭 동작의 매칭 여부를 확인하기 위해
   * 사용되는 UUID
   */
  NotifyClientServerUdpMatched = 15,

  //
  // Peer Server Holepunching
  //

  /**
   * C2S (UDP)
   *
   * 문서화
   *
   * payload:
   *   Uuid holepunch_tag - 홀펀칭시에 구분하기 위한 Tag입니다.
   *   HostId host_id
   */
  PeerUdp_ServerHolepunch = 16,

  /**
   * S2C (UDP)
   *
   */
  PeerUdp_ServerHolepunchAck = 17,

  /**
   * C2S (TCP)
   *
   */
  PeerUdp_NotifyHolepunchSuccess = 18,

  //
  // Reliable UDP
  //

  /**
   * P2P or S2C(by relay layer, fake)
   *
   * Reliable UDP frame
   */
  RUdp_Frame = 19,

  //
  // Relaying Messages
  //

  /**
   * C2S (TCP)
   *
   * 릴레이를 통해서 메시지를 서버로 보내야할 경우에 사용되는 메시지입니다.
   *
   * payload:
   *   RelayDestList relay_dest_list
   *   OptimalCounter32 frame_length
   *   ByteArray long_frame
   */
  ReliableRelay1 = 20,

  /**
   * C2S (TCP)
   *
   * Unreliable 메시지를 서버 릴레이를 통해서 보내야할 경우에
   * 사용되는 메시지입니다.
   *
   * payload:
   *   MessagePriority priority
   *   uint64 unique_id
   *   Array<HostId> relay_dest_list
   *   ByteArray payload
   */
  UnreliableRelay1 = 21,

  /**
   * C2S (TCP)
   *
   * UnreliableRelay1 메시지와 같지만, host_id 목록을 압축해서 보냅니다.
   *
   * payload:
   *   MessagePriority priority
   *   uint64 unique_id
   *   Array<HostId> includee_host_id_list - 그룹에 들어가 있지 않은 호스트들의
   * 릴레이 목록 OptimalCount32 group_list_count
   *     + HostId group_id
   *     + Array<HostId> excludee_host_id_list
   *   ByteArray payload
   */
  UnreliableRelay1_RelayDestListCompressed = 22,

  /**
   * C2S (TCP)
   *
   * ReliableRelay1은 브로드캐스트를 하지만 이건 이미
   * reliable UDP sender window에 릴레이 모드 전환 전에
   * 이미 있던 것들을 뒤늦게라도 릴레이로 상대에게 전송하는 역할을 합니다.
   *
   * 참고: reliable하게 가는거니까 ack는 굳이 보내지 않아도 됩니다.
   *
   * payload:
   *   HostId host_id
   *   FrameNumber frame_number
   *   ByteArray frame_data
   */
  LingerDataFrame1 = 23,

  /**
   * S2C (TCP)
   *
   * 서버는 ReliableRelay1 메시지를 받으면, 각 목적지에 해당하는 호스트들에게
   * 메시지를 릴레이 시켜줍니다.
   *
   * 이때 클라이언트로 전송되는 메시지입니다.
   *
   * payload:
   *   HostId remote_id - 메시지를 보낸 호스트의 ID입니다.
   *   FrameNumber frame_number - 메시지 프레임 번호입니다.
   *   ByteArray frame_data - 실제 메시지 데이터입니다.
   */
  ReliableRelay2 = 24,

  /**
   * S2C (UDP or TCP)
   *
   * 서버가 Unreliable1 메시지를 받으면, 해당 호스트들에게 메시지를
   * 릴레이시켜줍니다.
   *
   * 이때 클라이언트로 보내지는 메시지입니다.
   *
   * payload:
   *   HostId remote_id - 메시지를 보낸 호스트의 ID입니다.
   *   ByteArray payload - 실제 메시지 데이터입니다.
   */
  UnreliableRelay2 = 25,

  /**
   * S2C (TCP)
   *
   * LingerDataFrame1 메시지에 대한 응답 메시지입니다.
   *
   * payload:
   *   HostId remote
   *   FrameNumber frame_number
   *   ByteArray frame_data
   */
  LingerDataFrame2 = 26,

  //
  // Ping / Time
  //

  /**
   * C2S (UDP or TCP)
   *
   * 서버에 클라이언트의 시간을 알리는 역활을 하고 동시에 연결 유지를 위한
   * keep-alive 역활도 하는 메시지입니다.
   *
   * payload:
   *   double client_time
   *   double server_udp_recent_ping
   *
   * 이 메시지를 받은 서버는 시간 값들을 갱신한 후 ReplyServerTime 메시지를
   * 클라이언트에게 보내줍니다.
   */
  RequestServerTimeAndKeepAlive = 27,

  /**
   * C2S (UDP or TCP(fallback))
   *
   * 스피드핵 검출 기능이 켜져있을 경우 클라이언트는 이 메시지를
   * 서버로 보내게 됩니다.
   *
   * 일정한 주기마다 클라이언트는 이 메시지를 서버로 보내게 되는데,
   * 서버는 이 메시지를 받는 주기를 살펴본 후 일정 범위 이상으로 바쁘게 오면
   * 클라이언트의 타이머(클럭)가 정상이 아니라고 판단할 수 있습니다.
   *
   * 이는 의심사례로 판단할 수 있으며, 랙등으로 인해서 타이밍이 밀려서
   * 올수도 있으므로, 보수적으로 판단해야합니다.
   */
  SpeedHackDetectorPing = 28,

  /**
   * S2C (UDP or TCP(fallback))
   *
   * 클라이언트에서 RequestServerTimeAndKeepAlive 메시지를 받게되면,
   * 서버는 이 메시지를 클라이언트로 보냅니다.
   *
   * payload:
   *   double client_local_time - 클라이언트가 이전에 보냈던 클라이언트의
   * 시간(시간차를 구하기 위해서 echo) double server_time - 서버의 현재 시각
   */
  ReplyServerTime = 29,

  //
  // Peer Direct Holepunching
  //

  /**
   * S2C (UDP or TCP(fallback))
   *
   * 홀펀칭 상태를 유지하기 위해서, 서버에서 클라이언트로 보내는
   * 단방향 메시지입니다.
   *
   * 이 메시지를 받은 클라이언트에서는 무시되며(처리된걸로 간주)
   * 아무런 처리도 하지 않습니다.
   *
   * 이 메시지 송신이 필요한 이유는 대부분의 공유기(NAT)에서는
   * 일정 시간동안 메시지 수신이 없는경우 주소 맵핑이 삭제되어, UDP 홀펀칭이
   * 닫히게 됩니다.
   *
   * 이를 강제로 계속 열린 상태로 유지하기 위해서 이와 같이
   * 더미 패킷을 보내게 됩니다.
   *
   * 실제 UDP transport를 통해서 전송할때에 헤더가 덧붙여져서 보내지는데
   * 젤 처음 4바이트는 값이 계속 바뀌는 형태로 보내지게 됩니다.
   *
   * 이는 특정 공유기의 특성을 피하기 위함인데, UDP 프레임 처음의 값이
   * 지속적으로 같으면 차단되는 경우가 있어서, 이를 피하기 위해서 헤더 부분의
   * 값을 매번 같은 값이 아니도록 처리를 해서 보냅니다.
   */
  ArbitaryTouch = 30,

  /**
   * P2P (UDP)
   *
   * Peer와의 홀펀칭을 위해서 해당 Peer에게 보내지는 메시입니다.
   *
   * payload:
   *   HostId local_host_id - 메시지를 보내는 호스트의 ID입니다.
   *   Uuid holepunch_tag - 여러개의 홀펀칭이 이루어지고 있을 경우 구분하기
   * 위해서 사용되는 Tag입니다. Uuid server_instance_tag_ - 서버 인스턴스가
   * 여러개 띄워져 있을 경우에 해당하는 서버를 구분하기 위해서 사용됩니다.
   *   InetAddress a2b_send_addr - 상대편에서 이쪽으로 메시지를 보낼때 사용할
   * 주소입니다. (즉, 상대방에게 메시지를 보내려거든 이 주소로 보내면
   * 받겠다입니다.)
   */
  PeerUdp_PeerHolepunch = 31,

  /**
   * P2P (UDP)
   *
   * PeerUdp_PeerHolepunch 메시지에 대한 응답 메시지로, Peer 와의
   * 홀펀칭에 사용되는 메시지입니다.
   *
   * payload:
   *   Uuid holepunch_tag - 홀펀칭 상태를 구분하기 위한 Tag입니다.
   *   HostId host_id - 호스트 ID입니다.
   *   InetAddress a2b_send_addr - 상대편에서 이쪽으로 메시지를 보낼때 사용되는
   * 주소입니다. InetAddress a2b_recv_addr - 상대편의 메시지를 수신받을
   * 주소입니다. InetAddress b2a_send_addr - 상대편으로 메시지를 보낼
   * 주소입니다.
   */
  PeerUdp_PeerHolepunchAck = 32,

  /**
   * P2P (UDP)
   *
   * Peer들끼리의 시간을 동기화를 위한 메시지입니다.
   *
   * payload:
   *   double client_time - 보낸 시점에서의 클라이언트의 절대 시간입니다. (일반
   * stepped time보다 정밀해야함?)
   */
  P2PIndirectServerTimeAndPing = 33,

  /**
   * P2P (UDP)
   *
   * P2PIndirectServerTimeAndPing 메시지에 대한 응답 메시지입니다.
   *
   * payload:
   *   double client_time - P2PIndirectServerTimeAndPing 보냈던 값을 echo
   * (시간차를 구하기 위해서)
   */
  P2PIndirectServerTimeAndPong = 34,

  /**
   * S2C
   *
   * 서버로 부터의 브로드캐스팅 메세지를 Peer간의 릴레이를
   * 통해서 해결하기 위해 추가 서버의 브로드 캐스팅 메세지를 릴레이를
   * 해줄 피어가 받는 메세지 타입
   *
   * payload:
   *   MessagePriority priority
   *   uint64 unique_id
   *   Array<HostId> p2p_route_list
   *   OptimalCounter payload_length
   *   byte[] payload
   */
  S2CRoutedMulticast1 = 35,

  /**
   * C2C
   *
   * 피어를 통해 릴레이된 메세지를 받는다는 의미를 가진 메세지 타입
   *
   * payload:
   *   ByteArray content - 전송할 메시지입니다.
   */
  S2CRoutedMulticast2 = 36,

  /**
   * C2S, S2C, C2C (TCP / UDP)
   *
   * 암호화 처리된 메시지입니다.  Reliable하게 보내집니다.
   *
   * payload:
   *   EncryptionMode encryption_mode - 적용된 암호화 모드입니다.
   *   ByteArray encrypted_payload - 암호화된 메시지 데이터입니다.
   */
  Encrypted_Reliable = 37,

  /**
   * C2S, S2C, C2C (TCP / UDP)
   *
   * 암호화 처리된 메시지입니다.  Unreliable하게 보내집니다.
   *
   * payload:
   *   EncryptionMode encryption_mode - 적용된 암호화 모드입니다.
   *   ByteArray encrypted_payload - 암호화된 메시지 데이터입니다.
   */
  Encrypted_Unreliable = 38,

  /**
   * C2S, S2C, C2C (TCP / UDP)
   *
   * 압축된 메시지입니다.
   *
   * payload:
   *   CompressionMode compression_mode - 압축에 사용된 모드입니다.
   *   OptimalCounter32 compressed_message_length - 압축된 메시지의 길이입니다.
   *   byte[] compressed_message - 압축된 메시지 데이터입니다.
   */
  Compressed = 39,

  //
  // 송/수신 속도 측정을 위한 메시지입니다.
  //

  /**
   * C2S, S2C, C2C
   *
   * 수신측에서 계속 산출해왔던 '수신 속도'를 송신측에서 받고자 함 및 리플.
   */
  RequestReceiveSpeedAtReceiverSide_NoRelay = 40,

  /**
   * C2S, S2C, C2C
   *
   * RequestReceiveSpeedAtReceiverSide_NoRelay 메시지에 대한
   * 응답 메시지입니다.
   */
  ReplyReceiveSpeedAtReceiverSide_NoRelay = 41,

  //
  // 이하는 LanClient 전용 메시지입니다.
  //

  /**
   * [LAN] C2C (TCP)
   *
   * P2P 연결된 Peer에게 인증관련 데이터를 보낸다.
   *
   * LanClient에서만 사용되며, Peer에게 보내는 메시지입니다.
   *
   * 각 peer간 검증을 하기 위해서 보내지는 메시지입니다.
   *
   * 한 머신에서 여러개의 인스턴스를 띄울 경우, 서로의 존재를 인식하는데
   * 다소 정확하지 않을 수 있는데,
   * 이 메시지를 보내서 서로의 존재를 확인할 수 있도록 합니다.
   *
   * payload:
   *   HostId local_host_id - 로컬 호스트 ID입니다.
   *   Uuid server_instance_tag_ - 서버의 인스턴스 Tag입니다.
   *   int32 internal_version - 엔진 내부버젼 넘버입니다.
   *   Uuid holepunch_tag - 홀펀칭시에 구분하기 위한 Tag입니다.
   */
  NotifyConnectionPeerRequestData = 42,

  /**
   * [LAN] C2C (TCP)
   *
   * 서버에게 현재 LAN client에서 해당 peer의 접속이
   * 끊겼음을 알리는 메시지입니다.
   *
   * payload:
   *   ResultCode reason - 접속이 끊긴 이유입니다.
   *   HostId host_id - 접속이 끊긴 peer의 host id입니다.
   */
  NotifyCSP2PDisconnected = 43,

  /**
   * [LAN] C2C (TCP)
   *
   * NotifyConnectionPeerRequestData 메시지를 수신받은 후 체크 결과
   * 정상적으로 인식된 경우에 해당 peer에게 보내지는 메시지입니다.
   *
   * payload:
   *   Uuid holepunch_tag - 홀펀칭시에 구분하기 위한 Tag입니다.
   */
  NotifyConnectPeerRequestDataSucess = 44,

  /**
   * [LAN] C2S (TCP)
   *
   * 최종적으로 해당 peer에 연결이 정상적으로 이루어졌음으로,
   * 서버에게 알릴때 사용되는 메시지입니다.
   *
   * payload:
   *   HostId peer_id
   */
  NotifyCSConnectionPeerSuccess = 45,

  //
  // ETC.
  //

  /**
   * 수신측은 받아도 그냥 버리는 메시지
   * Windows 2003은 서버에서 한번 보낸 적이 있는 곳으로부터 오는 패킷만 받는다.
   * 따라서 이걸로 구멍을 내준다.
   *
   * 주의:
   * 이 메시지는 사용하면 안됨.  현재 윈도우즈 서버 2008에서 전송 포트가
   * 모두 차단되는 현상이 있음.
   */
  Ignore = 46,

  /**
   * C2S (TCP)
   *
   * 클라이언트가 물리적으로 TCP 접속을 한 직후 처음으로
   * 서버로 보내는 메시지입니다.
   *
   * 서버에게 어떻게 연결해야하는지 질의하는 과정입니다.
   *
   * payload:
   *   RuntimePlatform runtime_platform - 실행중인 플랫폼 타입입니다. (Windows,
   * Linux, ...)
   */
  RequestServerConnectionHint = 47,
};

TextStream& operator<<(TextStream& stream, const MessageType v);

/**
 * 로컬(내부) 이벤트 타입들.
 */
enum class LocalEventType {
  /**
   * 없음.
   */
  None = 0,

  //
  // CS
  //

  /**
   * 클라이언트에서 사용되며, 최종적으로 서버에 접속이 성공적으로
   * 끝났을 경우에 발생하는 이벤트.
   */
  ConnectServerSuccess = 0,

  /**
   * 클라이언트에서 사용되며, 서버에 접속하지 못했을 경우에 발생하는 이벤트.
   */
  ConnectServerFail = 1,

  /**
   * C/S 연결이 해제됐음을 의미
   */
  ClientServerDisconnect = 2,

  /**
   * 서버에서 사용되며, 연결은 되었지만 승인 대기중일때 발생하는 이벤트.
   */
  ClientJoinCandidate = 3,

  /**
   * 서버에서 사용되며, 연결 후 최종 승인까지 완료 직후 발생하는 이벤트.
   */
  ClientJoinApproved = 4,

  /**
   * 서버에 연결되어있던 클라이언트 1개가 나갔음.
   */
  ClientLeaveAfterDispose = 5,

  //
  // P2P
  //

  /**
   * P2P 그룹의 참여원이 정상적으로 참여 했다고 응답을 서버에 주게 되는데
   * 이때 발생하는 이벤트입니다.
   *
   * 즉, 참여원이 일련의 준비동작을 마무리하고, 해당 그룹에 정상적으로
   * 참여 했을때 이 이벤트가 발생합니다.
   */
  P2PAddMemberAckComplete = 6,

  /**
   * 클라이언트에서만 발생하는 이벤트로, 현재 내가 참여하고 있는
   * 그룹에 새 참가자가 참여 했을 경우에
   * 발생하는 이벤트입니다.
   */
  P2PAddMember = 7,

  /**
   * 클라이언트에서만 발생하는 이벤트로, 현재 내가 참여하고 있는
   * 그룹에서 멤버하나가 나갔을 경우에 발생하는
   * 이벤트입니다.
   */
  P2PLeaveMember = 8,

  /**
   * 클라이언트에서 발생하는 이벤트로, RelayedP2P에서 DirectP2P로 전활될때
   * 발생하는 이벤트입니다.
   *
   * 즉, UDP 홀펀칭이 성공적으로 이루어져, UDP끼리 직접적으로 통신이 가능한
   * 상황이 될때 이 이벤트가 발생합니다.
   */
  DirectP2PEnabled = 9,

  /**
   * 클라이언트에서 발생하는 이벤트로, DirectP2P에서 RelayedP2P로 전환될때
   * 발생하는 이벤트입니다.
   *
   * UDP 홀펀칭이 차단되거나, 릴레이 대비 효율이 좋지 못한 상황에서
   * 강제로 릴레이 모드로 전환되는데 이때 이 이벤트가 발생합니다.
   */
  RelayP2PEnabled = 10,

  /**
   * Lan 클섭에서 한 그룹의 P2P connection이 모두완료되면 뜨는 이벤트
   */
  GroupP2PEnabled = 11,

  /**
   * 서버와의 UDP punchthrough가 성공했거나 실패했을 경우에 발생하는
   * 이벤트입니다.
   */
  ServerUdpChanged = 12,

  /**
   * 클라이언트에서만 발생하는 이벤트로 서버와의 시간을 동기화할수 있는
   * 타이밍에 발생하는 이벤트입니다.
   *
   * 서버에서 ReplyServerTime 메시지를 받는 시점에서 호출됩니다.
   */
  SynchronizeServerTime = 13,

  /**
   * 해킹 의심 사례(스프디핵, 패킷변조)가 검출 되었을때 발생하는 이벤트입니다.
   */
  HackSuspected = 14,

  /**
   * 사용되고 있지 않습니다.  검토 후 제거해도 좋을듯...
   */
  TcpListenFail = 15,

  /**
   * P2P 그룹이 파괴 되었을 경우에 발생하는 이벤트입니다.
   */
  P2PGroupRemoved = 16,

  /**
   * P2P 연결이 실패되었음
   *
   * LAN 서버에서만 사용됨.
   */
  P2PDisconnected = 17,

  //
  // ETC
  //

  ///**
  //*/
  // UnitTestFail = 18,

  /**
   * 오류가 발생했을 경우에, 발생하는 이벤트입니다.
   */
  Error = 19,

  /**
   * 경고성 메시지가 발생했을 경우, 발생하는 이벤트입니다.
   */
  Warning = 20,
};

String ToString(const LocalEventType value);

/**
 * Reliable UDP 프레임 타입.
 */
enum class RUdpFrameType {
  /**
   * None
   */
  None = 0,

  /**
   * 데이터 프레임
   */
  Data = 1,

  /**
   * 응답 프레임
   */
  Ack = 2,

  // Disconnect = 3,
};

TextStream& operator<<(TextStream& stream, const RUdpFrameType v);

/**
 * http://msdn.microsoft.com/en-us/library/aa366912(VS.85).aspx 에 의하면
 * 사용자가 할당할 수 없는 메모리 공간 주소로서의 값이 이걸로 쓰여야 한다.
 * 왜냐하면 OVERLAPPED 주소 값이 들어갈 곳에 이 값이 쓰이기 때문이다.
 */
enum class IocpCustomValue {
  /**
   * IOCP에 넣는 custom value는 overlapped 포인터값이다. NULL이어서는 안된다.
   * GQCS쪽에서 T/F 체크대신 overlapped를 체크하니까.
   */
  NewClient = -1,

  /**
   * 이걸 enque할때는 뒷북 신드롬을 피하기 위해 NetServer혹은 LanServer등을
   * 넣어야.
   */
  SendEnqueued = -2,

  /**
   * 새로운 peer가 접속했다.
   */
  NewPeerAccepted = -3,

  /**
   * Heartbeat
   */
  Heartbeat = -4,

  /**
   * Tick
   */
  OnTick = -5,

  /**
   * User task
   */
  DoUserTask = -6,

  /**
   * 호스트 종료. completion에 종료를 알린다.
   */
  End = -7
};

inline bool IocpCustomValueInRange(INT_PTR value) { return value < 0; }

}  // namespace net
}  // namespace fun
