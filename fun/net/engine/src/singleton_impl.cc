#include "fun/net/net.h"

namespace fun {

// CLR app에서는 class내의 static member마저도 static function 내의 static local
// var와 같은 시점에서 파괴되므로 이건 전역 변수로 둬야 한다. 그래야 모든
// singleton들이 파괴된 후에도 이 객체가 존재한다.
// CCriticalSection2 GSingletonDepCS;
// const uint32 SingletonTryWaitMilisec = 5;

}  // namespace fun
