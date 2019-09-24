#pragma once

#include "fun/base/base.h"
#include "fun/base/scoped_ptr.h"
#include "fun/base/container/byte_array.h"
#include "fun/base/serialization/io_device.h"

namespace fun {

class FUN_BASE_API TextStream {
 public:
  TextStream();
  TextStream(IoDevice* device);
  TextStream(FILE* fp, IoDevice::OpenMode mode = IoDevice::ReadWrite);
  TextStream(UString* string, IoDevice::OpenMode mode = IoDevice::ReadWrite);
  TextStream(ByteArray* array, IoDevice::OpenMode mode = IoDevice::ReadWrite);
  //TODO OpenMode는 ReadOnly로 강제 해야하지 않을까??
  TextStream(const ByteArray& array, IoDevice::OpenMode mode = IoDevice::ReadOnly);
  virtual ~TextStream();

  void SetLocale(const Locale& locale);
  Locale GetLocale() const;

  void SetDevice(IoDevice* device);
  IoDevice* GetDevice() const;

  void SetEncoding(TextEncoding* encoding);
  void SetEncoding(const char* encoding_name);
  TextEncoding* GetEncoding() const;
  void SetAutoDetectUnicode(bool enabled);
  bool GetAutoDetectUnicode() const;
  void SetGenerateByteOrderMark(bool generate);
  bool GetGenerateByteOrderMark() const;

  //todo unicode 기반인 UString으로 해야처리가 쉬우려나...
  void SetString(UString* string, IoDevice::OpenMode open_mode = IoDevice::ReadWrite);
  UString* GetString() const;

  Status GetStatus() const;
  void SetStatus(Status status);
  void ResetStatus();

  bool AtEnd() const;
  void Reset();
  bool Seek(int64 position);
  int64 Tell() const;

  void Flush();

  void SkipWhitespaces();

  String ReadLine(int64 max_len = 0);
  bool ReadLineInto(String* buf, int64 max_len = 0);
  String ReadAll();
  String Read(int64 max_len);

  FieldAlignment GetFieldAlignment() const;
  void SetFieldAlignment(FieldAlignment alignment);

  UNICHAR GetPadChar() const;
  void SetPadChar(UNICHAR ch);

  int32 GetFieldWidth() const;
  void SetFieldWidth(int32 width);

  NumberFlags GetNumberFlags();
  void SetNumberFlags(NumberFlags flags);

  int32 GetIntegerBase() const;
  void SetIntegerBase(int32 base);

  ReadNumberNotation GetRealNumberNotation() const;
  void GetRealNumberNotation(ReadNumberNotation notation);

  int32 GetRealNumberPrecision() const;
  void SetRealNumberPrecision(int32 precision);


  //
  // Stream operator overloadings.
  //

  TextStream& operator >> (UNICHAR& ch);
  TextStream& operator >> (char& ch);
  TextStream& operator >> (int8& i);
  TextStream& operator >> (uint8& i);
  TextStream& operator >> (int16& i);
  TextStream& operator >> (uint16& i);
  TextStream& operator >> (int32& i);
  TextStream& operator >> (uint32& i);
  TextStream& operator >> (int64& i);
  TextStream& operator >> (uint64& i);
  TextStream& operator >> (float& f);
  TextStream& operator >> (double& d);
  TextStream& operator >> (UString& s);
  TextStream& operator >> (String& s);
  TextStream& operator >> (char* s);

  TextStream& operator << (const UNICHAR ch);
  TextStream& operator << (const char ch);
  TextStream& operator << (const int8 i);
  TextStream& operator << (const uint8 i);
  TextStream& operator << (const int16 i);
  TextStream& operator << (const uint16 i);
  TextStream& operator << (const int32 i);
  TextStream& operator << (const uint32 i);
  TextStream& operator << (const int64 i);
  TextStream& operator << (const uint64 i);
  TextStream& operator << (const float f);
  TextStream& operator << (const double d);
  TextStream& operator << (const UString& s);
  TextStream& operator << (const String& s);
  TextStream& operator << (const UStringRef& s);
  TextStream& operator << (Latin1String s);
  TextStream& operator << (const char* s);
  TextStream& operator << (const void* ptr);

 private:
  TextStream(const TextStream&) = delete;
  TextStream& operator = (const TextStream&) = delete;

  friend class DebugStateSaverImpl;
  friend class Debug;

  ScopedPtr<TextStreamImpl> impl_;
};


//
// TextStream manipulators
//

// manipulator function
typedef TextStream& (*TextStreamFunction)(TextStream&)
// manipulator w/int32 argument
typedef void (TextStream::*TSMFI)(int32);
// manipulator w/UNICHAR argument
typedef void (TextStream::*TSMFC)(UNICHAR);

class FUN_BASE_API TextStreamManipulator {
 public:
  TextStreamManipulator(TSMFI m, int32 a)
    : mi(m), arg(a), ch(0) {}

  TextStreamManipulator(TSMFC m, UNICHAR ch)
    : mc(m), arg(-1), ch(ch) {}

  void Exec(TextStream& stream) {
    if (mi) {
      (stream.*mi)(arg);
    } else {
      (stream.*mc)(ch);
    }
  }

 private:
  TSMFC mc;
  TSMFI mi;
  int32 arg;
  UNICHAR ch;
};


inline TextStream& operator >> (TextStream& stream, TextStreamFunction f) {
  return (*f)(stream);
}

inline TextStream& operator << (TextStream& stream, TextStreamFunction f) {
  return (*f)(stream);
}

inline TextStream& operator << (TextStream& stream, TextStreamManipulator m) {
  m.Exec(stream);
  return stream;
}

FUN_BASE_API TextStream& bin(TextStream& stream);
FUN_BASE_API TextStream& oct(TextStream& stream);
FUN_BASE_API TextStream& dec(TextStream& stream);
FUN_BASE_API TextStream& hex(TextStream& stream);

FUN_BASE_API TextStream& showbase(TextStream& stream);
FUN_BASE_API TextStream& forcesign(TextStream& stream);
FUN_BASE_API TextStream& forcepoint(TextStream& stream);
FUN_BASE_API TextStream& noshowbase(TextStream& stream);
FUN_BASE_API TextStream& noforcesign(TextStream& stream);
FUN_BASE_API TextStream& noforcepoint(TextStream& stream);

FUN_BASE_API TextStream& uppercasebase(TextStream& stream);
FUN_BASE_API TextStream& uppercasedigits(TextStream& stream);
FUN_BASE_API TextStream& lowercasebase(TextStream& stream);
FUN_BASE_API TextStream& lowercasedigits(TextStream& stream);

FUN_BASE_API TextStream& fixed(TextStream& stream);
FUN_BASE_API TextStream& scientific(TextStream& stream);

FUN_BASE_API TextStream& left(TextStream& stream);
FUN_BASE_API TextStream& right(TextStream& stream);
FUN_BASE_API TextStream& center(TextStream& stream);

FUN_BASE_API TextStream& endl(TextStream& stream);
FUN_BASE_API TextStream& flush(TextStream& stream);
FUN_BASE_API TextStream& reset(TextStream& stream);

FUN_BASE_API TextStream& bom(TextStream& stream);

FUN_BASE_API TextStream& ws(TextStream& stream);

//TODO 함수 이름 간소화
inline TextStreamManipulator qSetFieldWidth(int32 width) {
  TSMFI func = &TextStream::SetFieldWidth;
  return TextStreamManipulator(func, width);
}

//TODO 함수 이름 간소화
inline TextStreamManipulator qSetPadChar(UNICHAR ch) {
  TSMFC func = &TextStream::SetPadChar;
  return TextStreamManipulator(func, ch);
}

//TODO 함수 이름 간소화
inline TextStreamManipulator qSetRealNumberPrecision(int32 precision) {
  TSMFI func = &TextStream::SetRealNumberPrecision;
  return TextStreamManipulator(func, precision);
}

} // namespace fun
