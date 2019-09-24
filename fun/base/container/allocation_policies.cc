#include "fun/base/container/allocation_policies.h"

#if TRACK_ARRAY_SLACKS

#include "Containers/StackTracker.h"

namespace fun {

CStackTracker* GSlackTracker = nullptr;

static void SlackTrackerUpdateFn(const CStackTracker::CCallStack& callstack, void* user_data)
{
  if (user_data) {
    //Callstack has been called more than once, aggregate the data
    SlackTrackData* new_lc_data = static_cast<SlackTrackData*>(user_data);
    SlackTrackData* cur_lc_data = static_cast<SlackTrackData*>(callstack.user_data);

    cur_lc_data->NumElements += new_lc_data->NumElements;
    cur_lc_data->NumSlackElements += new_lc_data->NumSlackElements;
    //cur_lc_data->Foo += 1;
  }
}

static void SlackTrackerReportFn(const CStackTracker::CCallStack& callstack, uint64 TotalStackCount, Printer& out)
{
  //Output to a csv file any relevant data
  SlackTrackData* const lc_data = static_cast<SlackTrackData*>(callstack.user_data);
  if (lc_data) {
    String UserOutput = FUN_LINE_TERMINATOR TEXT(",,,");

    UserOutput += String::Format(TEXT("NumElements: %f, NumSlackElements: %f, Curr: %f TotalStackCount: %d "),
      (float)lc_data->NumElements / TotalStackCount,
      (float)lc_data->NumSlackElements / TotalStackCount,
      (float)lc_data->CurrentSlackNum / TotalStackCount,
      TotalStackCount);

    out.Print(*UserOutput);
  }
}


static class CSlackTrackExec : private SelfRegisteringExec
{
 public:
  bool Exec(class RuntimeEnv* env, const TCHAR* cmd, Printer& out) override
  {
    if (Parse::Command(&cmd, TEXT("DUMPSLACKTRACES"))) {
      if (GSlackTracker) {
        GSlackTracker->DumpStackTraces(100, out);
      }
      return true;
    }
    else if (Parse::Command(&cmd, TEXT("RESETSLACKTRACKING"))) {
      if (GSlackTracker) {
        GSlackTracker->ResetTracking();
      }
      return true;
    }
    else if (Parse::Command(&cmd, TEXT("TOGGLESLACKTRACKING"))) {
      if (GSlackTracker) {
        GSlackTracker->ToggleTracking();
      }
      return true;
    }
    return false;
  }
} SlackTrackExec;

void TrackSlack(int32 NumElements, int32 NumAllocatedElements, size_t BytesPerElement, int32 Retval)
{
  if (!GSlackTracker)
  {
    GSlackTracker = new CStackTracker(SlackTrackerUpdateFn, SlackTrackerReportFn);
  }
  #define SLACK_TRACE_TO_SKIP 4
  SlackTrackData* const lc_data = static_cast<SlackTrackData*>(UnsafeMemory::Malloc(sizeof(SlackTrackData)));
  UnsafeMemory::Memzero(lc_data, sizeof(SlackTrackData));

  lc_data->NumElements = NumElements;
  lc_data->NumSlackElements = Retval;
  lc_data->CurrentSlackNum = NumAllocatedElements - NumElements;

  GSlackTracker->CaptureStackTrace(SLACK_TRACE_TO_SKIP, static_cast<void*>(lc_data));
}

} // namespace fun

#endif //TRACK_ARRAY_SLACKS

/*

namespace fun {

int32 DefaultCalculateSlack(int32 NumElements, int32 NumAllocatedElements, size_t BytesPerElement)
{
  int32 RetVal;

  if (NumElements < NumAllocatedElements) {
    // If the container has too much slack, shrink it to exactly fit the number of elements.
    const uint32 CurrentSlackElements = NumAllocatedElements-NumElements;
    const size_t CurrentSlackBytes = (NumAllocatedElements-NumElements)*BytesPerElement;
    const bool bTooManySlackBytes = CurrentSlackBytes >= 16384;
    const bool bTooManySlackElements = 3*NumElements < 2*NumAllocatedElements;
    if ((bTooManySlackBytes || bTooManySlackElements) && (CurrentSlackElements > 64 || !NumElements)) //  hard coded 64 :-( {
      RetVal = NumElements;
    }
    else {
      RetVal = NumAllocatedElements;
    }
  }
  else if (NumElements > 0) {
    const int32 FirstAllocation = 4;
    if (!NumAllocatedElements && NumElements <= FirstAllocation) {
      // 17 is too large for an initial allocation. Many arrays never have more one or two elements.
      RetVal = FirstAllocation;
    }
    else {
      // Allocate slack for the array proportional to its size.
      CHECK(NumElements < int32_MAX);
      RetVal = NumElements + 3*NumElements/8 + 16;
      // NumElements and MaxElements are stored in 32 bit signed integers so we must be careful not to overflow here.
      if (NumElements > RetVal) {
        RetVal = int32_MAX;
      }
    }
  }
  else
  {
    RetVal = 0;
  }

#if TRACK_ARRAY_SLACKS
  if (GSlackTracker == nullptr) {
    GSlackTracker = new CStackTracker(SlackTrackerUpdateFn, SlackTrackerReportFn);
  }

  #define SLACK_TRACE_TO_SKIP 4
  SlackTrackData* const lc_data = static_cast<SlackTrackData*>(UnsafeMemory::Malloc(sizeof(SlackTrackData)));
  UnsafeMemory::Memzero(lc_data, sizeof(SlackTrackData));

  lc_data->NumElements = NumElements;
  lc_data->NumSlackElements = Retval;
  lc_data->CurrentSlackNum = NumAllocatedElements - NumElements;

  GSlackTracker->CaptureStackTrace(SLACK_TRACE_TO_SKIP, static_cast<void*>(lc_data));
#endif //TRACK_ARRAY_SLACKS

  return RetVal;
}

} // namespace fun
*/
