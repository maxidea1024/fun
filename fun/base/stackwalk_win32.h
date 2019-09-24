#pragma once

namespace fun {

class FUN_BASE_API StackwalkWin : public Stackwalk {
 public:
  static bool InitStackwalking();

  static void ProgramCounterToSymbolInfo(
      uint64 ProgramCounter, CProgramCounterSymbolInfo& OutSymbolInfo);
  static void CaptureStackBackTrace(uint64* BackTrace, uint32 MaxDepth,
                                    void* Context = nullptr);
  static void StackwalkAndDump(ANSICHAR* HumanReadableString,
                               size_t HumanReadableStringSize,
                               int32 IgnoreCount, void* Context = nullptr);
  static void ThreadStackwalkAndDump(ANSICHAR* HumanReadableString,
                                     size_t HumanReadableStringSize,
                                     int32 IgnoreCount, uint32 thread_id);

  static int32 GetProcessModuleCount();
  static int32 GetProcessModuleSignatures(
      CStackWalkModuleInfo* ModuleSignatures, const int32 ModuleSignaturesSize);

  static void RegisterOnModulesChanged();

  /**
   * Upload localy built symbols to network symbol storage.
   *
   * Use case:
   *   Game designers use game from source (without prebuild game .dll-files).
   *   In this case all game .dll-files are compiled locally.
   *   For post-mortem debug programmers need .dll and .pdb files from
   * designers.
   */
  static bool UploadLocalSymbols();

  /**
   * Get downstream storage with downloaded from remote symbol storage files.
   */
  static String GetDownstreamStorage();
};

}  // namespace fun
