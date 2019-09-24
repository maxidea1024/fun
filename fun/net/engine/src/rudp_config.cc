#include "RUdpConfig.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

//사용안됨.
// 무작위로 채울 stream의 초기 데이터 바이트 값
// uint8 RUdpConfig::first_stream_value = 33;

//사용안됨.
// 무작위로 채울 stream 단위의 크기
// int32 RUdpConfig::max_random_stream_length = 2000;

// 한번에 보내는 frame의 최대 크기
int32 RUdpConfig::frame_length = MathBase::Max(1300, NetConfig::MTU - 100);

// 처음 resend할 때까지 대기하는 시간
// 300ms는 인간이 크게 느끼지 못하는 텀이다. 따라서 이정도면 충분하다.
// 너무 짧게 잡으면 ack를 저쪽에서 보냈지만 랙 때문에(먼거리 등) 늦는 경우에도
// 엄한 재송신이 있을 것이므로 즐. 특히 resend 폭주는 왕 즐 어차피 첫 핑 나오면
// 줄여지므로 크게 잡아도 된다. 윈도 TCP는 3초가 기본이다.
// http://support.microsoft.com/kb/170359/en-us/
double RUdpConfig::first_resend_cooltime = 2;

// 최대 resend 전 대기 시간. 이게 없으니 10초까지 길어지는데, 10초나 기다려서
// resend하면 사실상 막장이나 다름없다.
double RUdpConfig::max_resend_cooltime = 8;

// 1회 resend후 재 resend를 할 때까지 기다리는 시간
// 전에는 이 값이 1.3이었으나, FirstResendCoolTime를 2로 상향한 후 이 값은
// 변경할 필요가 없어졌음.
// double RUdpConfig::enlarge_resend_cooltime_ratio = 1.0;

//사용안됨.
//// UDP 시뮬레이션에서 보내지는 패킷의 최소 랙
// double RUdpConfig::min_lag = 0.1;
//// UDP 시뮬레이션에서 보내지는 패킷의 최고 랙
// double RUdpConfig::max_lag = 0.4;
// UDP 시뮬레이션에서 패킷이 1회 정상적으로 보내질 확률
// double RUdpConfig::simulated_udp_reliability_ratio = .7;

//사용안됨.
// 이건 uint32가 아니라 int32이어야 한다.
// int32 RUdpConfig::too_old_frame_number_threshold = (2 << 23);

// 한프레임당 258개의 ACK를 보낼 수 있음.
int32 RUdpConfig::max_ack_count_in_one_frame =
    MathBase::Max((frame_length - 10) / 5, 10);

//사용안됨.
// double RUdpConfig::tick_interval = 0.001;

//사용안됨.
// double RUdpConfig::show_ui_interval = 0.4;

int32 RUdpConfig::received_speed_before_update = 100;
double RUdpConfig::calc_recent_receive_interval = 1;
int32 RUdpConfig::max_send_speed_in_frame_count =
    1024 * 1024 * 10 / frame_length;
double RUdpConfig::brake_max_send_speed_threshold = 0.8;

// 최대 대기 한계 시간(체감으로 느끼지 못하는 랙 30ms)을 두도록 한다.
double RUdpConfig::stream_to_sender_window_coalesce_interval = 0.3;

// VTG case: unreliable RPC가 안가지는 현상 제보와 관련일 듯 해서 false로
// 바꾸었다.(...취소!) VTG case: reliable UDP가 대량 UDP 통신 사용시 resend time
// out이 발생하는 경우를 최소화하기 위해 reliable UDP frame은 훨씬 높은
// 우선순위로 주고받게 바꾸었다.
bool RUdpConfig::high_priority_ack_frame = true;
bool RUdpConfig::high_priority_data_frame = true;

//  resend가 너무 많이 일어나면, recv측에서 처리가 안되어 계속 delay되는 현상이
//  있습니다.
// 그래서 resend양을 조절합니다.
double RUdpConfig::resend_limit_ratio = 0.1;

// 1개의 호스트가 초당 보낼 수 있는 총 frame의 갯수.
int32 RUdpConfig::min_resend_limit_count = 1000;

// 1개의 호스트가 초당 보낼 수 있는 총 frame의 갯수.
int32 RUdpConfig::max_resend_limit_count = 3000;

}  // namespace net
}  // namespace fun
