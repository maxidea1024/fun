#include <red/base/Thread.h>
#include <red/base/ThreadPool.h>
#include "fun/base/logging.h"

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace fun;

#include "stat.h"

#include <stdio.h>

BOOST_AUTO_TEST_CASE(testSudokuStatSameSecond) {
  ThreadPool p;
  SudokuStat s(p);

  for (int i = 0; i < 100; ++i) {
    time_t Start = 1234567890;
    Timestamp recv = Timestamp::fromUnixTime(Start, 0);
    Timestamp Send = Timestamp::fromUnixTime(Start, i);
    s.recordResponse(Send, recv, i % 3 != 0);
  }
  printf("same second:\n%s\n", s.report().c_str());
}

BOOST_AUTO_TEST_CASE(testSudokuStatNextSecond) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  Timestamp recv = Timestamp::fromUnixTime(Start, 0);
  Timestamp Send = addTime(recv, 0.002);
  for (int i = 0; i < 10000; ++i) {
    s.recordResponse(Send, recv, true);
    recv = addTime(Send, 0.01);
    Send = addTime(recv, 0.02);
  }
  printf("next second:\n%s\n", s.report().c_str());
}

BOOST_AUTO_TEST_CASE(testSudokuStatFuzz) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  srand(static_cast<unsigned>(time(NULL)));
  for (int i = 0; i < 10000; ++i) {
    Timestamp recv = Timestamp::fromUnixTime(Start, 0);
    Timestamp Send = Timestamp::fromUnixTime(Start, 200);
    s.recordResponse(Send, recv, true);
    int jump = (rand() % 200) - 100;
    // printf("%4d ", jump);
    Start += jump;
  }
}

BOOST_AUTO_TEST_CASE(testSudokuStatJumpAhead5) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  Timestamp recv = Timestamp::fromUnixTime(Start, 0);
  Timestamp Send = Timestamp::fromUnixTime(Start, 200);
  s.recordResponse(Send, recv, true);

  recv = addTime(recv, 4);
  Send = addTime(Send, 5);
  s.recordResponse(Send, recv, true);
  printf("jump ahead 5 seconds:\n%s\n", s.report().c_str());
}

BOOST_AUTO_TEST_CASE(testSudokuStatJumpAhead59) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  Timestamp recv = Timestamp::fromUnixTime(Start, 0);
  Timestamp Send = Timestamp::fromUnixTime(Start, 200);
  s.recordResponse(Send, recv, true);

  recv = addTime(recv, 55);
  Send = addTime(Send, 59);
  s.recordResponse(Send, recv, true);
  printf("jump ahead 59 seconds:\n%s\n", s.report().c_str());
}

BOOST_AUTO_TEST_CASE(testSudokuStatJumpAhead60) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  Timestamp recv = Timestamp::fromUnixTime(Start, 0);
  Timestamp Send = Timestamp::fromUnixTime(Start, 200);
  s.recordResponse(Send, recv, true);

  recv = addTime(recv, 58);
  Send = addTime(Send, 60);
  s.recordResponse(Send, recv, true);
  printf("jump ahead 60 seconds:\n%s\n", s.report().c_str());
}

BOOST_AUTO_TEST_CASE(testSudokuStatJumpBack3) {
  ThreadPool p;
  SudokuStat s(p);

  time_t Start = 1234567890;
  Timestamp recv = Timestamp::fromUnixTime(Start, 0);
  Timestamp Send = Timestamp::fromUnixTime(Start, 200);
  s.recordResponse(Send, recv, true);

  recv = addTime(recv, 9);
  Send = addTime(Send, 10);
  s.recordResponse(Send, recv, true);

  recv = addTime(recv, -4);
  Send = addTime(Send, -3);
  s.recordResponse(Send, recv, true);

  printf("jump back 3 seconds:\n%s\n", s.report().c_str());
}
