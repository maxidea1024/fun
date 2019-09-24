#include "Tracer.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

LogWriter* LogWriter::New(const char* InNewFilename,
                          int32 InNewFileForLineLimit) {
  return new LogWriter(InNewFilename, InNewFileForLineLimit);
}

LogWriter::LogWriter(const char* InNewFilename, int32 InNewFileForLineLimit) {}

LogWriter::~LogWriter() {}

void LogWriter::SetFilename(const char* NewFilename) {}

void LogWriter::WriteLine(LogCategory Category, const char* text) {
  LOG(LogNetEngine, Info, TEXT("[%s] %s"), *ToString(Category), text);
}

void LogWriter::WriteLine(const char* text) {
  LOG(LogNetEngine, Info, TEXT("%s"), text);
}

}  // namespace net
}  // namespace fun
