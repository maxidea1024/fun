#pragma once

namespace fun {
namespace net {

/**
 * NetClientInfoPtr, NetPeerInfoPtr 때문에 전방 참조가 불가능하다.
 * 이를 해소하려면, 정방 선언에 모두 모아두던지, include순서를 수정해야할듯
 * 싶다.
 *
 * 복사 연산자의 비용이 크므로 꼭 배열에 사용시 주의할 것
 */
class LocalEvent {
 public:
  /** 로컬 이벤트 타입 */
  LocalEventType type;

  /** 결과 정보 */
  SharedPtr<ResultInfo> result_info;

  /** 해킹 관련 타입 */
  HackType hack_type;

  /** 클라이언트 정보 */
  NetClientInfoPtr NetClientInfo;

  /** 피어정보 */
  NetPeerInfoPtr NetPeerInfo;

  /** 추가 코멘트 데이터 */
  ByteArray comment;

  /** 사용자 데이터 */
  ByteArray user_data;

  /** 그룹 ID */
  HostId group_id;

  /** 멤버 ID */
  HostId member_id;

  /** 호스트 ID */
  HostId remote_id;

  /** 추가 데이터 */
  ByteArray custom_field;

  /** 멤버 수 */
  int32 member_count;

  /** 원격지 주소 */
  InetAddress remote_addr;

  /** 소켓 오류 코드 */
  SocketErrorCode socket_error;

  /** 연결 요청시 추가로 지정해 받은 데이터 */
  ByteArray connection_request;

 public:
  /** Empty constructor. */
  LocalEvent() {}

  /** Construct with local-event type. */
  LocalEvent(LocalEventType type) : type(type) {}
};

}  // namespace net
}  // namespace fun
