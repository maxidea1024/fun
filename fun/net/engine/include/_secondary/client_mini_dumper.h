//@deprecated
#pragma once

#include "MiniDumper.h"

namespace fun {
namespace net {

class MiniDumperSettings
{
 public:
  MINIDUMP_TYPE dump_type;

  NETENGINE_API MiniDumperSettings();
};

class ClientMiniDumper : protected Singleton<ClientMiniDumper>
{
 public:
  typedef Singleton<ClientMiniDumper> Super;

  NETENGINE_API ClientMiniDumper();
  NETENGINE_API virtual ~ClientMiniDumper();

  NETENGINE_API void Init(const MiniDumperSettings& settings = MiniDumperSettings());

  NETENGINE_API void ManualFullDump();

  NETENGINE_API void ManualMiniDump();

  NETENGINE_API static String RetrieveDumpFileName(const char* cmdline);

  inline static ClientMiniDumper& Get() { return Super::Get(); }

 private:
  struct Delegate : public IMiniDumpDelegate {
    MiniDumperSettings settings_;

    // IMiniDumpDelegate interface
    void GetDumpFilePath(char* output) override;
    void AfterWriteDumpDone() override;
    MINIDUMP_TYPE GetMiniDumpType() override;
  };

  Delegate delegate_;
};

FUN_END_NAMESPACE
