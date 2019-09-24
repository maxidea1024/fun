#include "fun/base/encoding/text_encoding.h"
#include "fun/base/exception.h"
#include "fun/base/encoding/ascii_encoding.h"
#include "fun/base/encoding/latin1_encoding.h"
#include "fun/base/encoding/latin2_encoding.h"
#include "fun/base/encoding/latin9_encoding.h"
#include "fun/base/encoding/utf32_encoding.h"
#include "fun/base/encoding/utf16_encoding.h"
#include "fun/base/encoding/utf8_encoding.h"
#include "fun/base/encoding/windows1250_encoding.h"
#include "fun/base/encoding/windows1251_encoding.h"
#include "fun/base/encoding/windows1252_encoding.h"
#include "fun/base/singleton.h"

namespace fun {

//
// TextEncodingRegistry
//

TextEncodingRegistry::TextEncodingRegistry() {
  // UTF8 encoding is default!
  SharedPtr<TextEncoding> utf8_encoding(new Utf8Encoding);
  Add(utf8_encoding, TextEncoding::GLOBAL);

  Add(new ASCIIEncoding);
  Add(new Latin1Encoding);
  Add(new Latin2Encoding);
  Add(new Latin9Encoding);
  Add(utf8_encoding);
  Add(new Utf16Encoding);
  Add(new Utf32Encoding);
  Add(new Windows1250Encoding);
  Add(new Windows1251Encoding);
  Add(new Windows1252Encoding);
}

TextEncodingRegistry::~TextEncodingRegistry() {}

bool TextEncodingRegistry::Has(const String& name) const
{
  if (encodings_.find(name) != encodings_.end()) {
    return true;
  }

  for (const auto& enc : encodings_) {
    if (enc.second->IsA(name)) {
      return true;
    }
  }
  return false;
}

void TextEncodingRegistry::Add(SharedPtr<TextEncoding> encoding) {
  Add(encoding, encoding->GetCanonicalName());
}

void TextEncodingRegistry::Add(SharedPtr<TextEncoding> encoding, const String& name) {
  RWLock::ScopedLock lock(lock_, true);

  encodings_[name] = encoding;
}

void TextEncodingRegistry::Remove(const String& name) {
  RWLock::ScopedLock lock(lock_, true);

  encodings_.erase(name);
}

SharedPtr<TextEncoding> TextEncodingRegistry::Find(const String& name) const {
  RWLock::ScopedLock lock(lock_);

  EncodingMap::const_iterator it = encodings_.find(name);
  if (it != encodings_.end()) {
    return it->second;
  }

  for (it = encodings_.begin(); it != encodings_.end(); ++it) {
    if (it->second->IsA(name)) {
      return it->second;
    }
  }
  return SharedPtr<TextEncoding>();
}


//
// TextEncoding
//

const String TextEncoding::GLOBAL;

TextEncoding::~TextEncoding() {}

int32 TextEncoding::Convert(const uint8* bytes) const {
  return static_cast<int32>(*bytes);
}

int32 TextEncoding::Convert(int32 /*ch*/, uint8* /*bytes*/, int32 /*len*/) const {
  return 0;
}

int32 TextEncoding::QueryConvert(const uint8* bytes, int32 /*len*/) const {
  return (int32)*bytes;
}

int32 TextEncoding::SequenceLength(const uint8* /*bytes*/, int32 /*len*/) const {
  return 1;
}

TextEncoding& TextEncoding::ByName(const String& encoding_name) {
  TextEncoding* encoding = registry(0)->Find(encoding_name);
  if (encoding) {
    return *encoding;
  } else {
    throw NotFoundException(encoding_name);
  }
}

SharedPtr<TextEncoding> TextEncoding::Find(const String& encoding_name) {
  return registry(0)->Find(encoding_name);
}

void TextEncoding::Add(SharedPtr<TextEncoding> encoding) {
  registry(0)->Add(encoding, encoding->GetCanonicalName());
}

void TextEncoding::Add(SharedPtr<TextEncoding> encoding, const String& name) {
  registry(0)->Add(encoding, name);
}

void TextEncoding::Remove(const String& encoding_name) {
  registry(0)->Remove(encoding_name);
}

SharedPtr<TextEncoding> TextEncoding::Global(SharedPtr<TextEncoding> encoding) {
  SharedPtr<TextEncoding> prev = find(GLOBAL);
  Add(encoding, GLOBAL);
  return prev;
}

TextEncoding& TextEncoding::Global() {
  return ByName(GLOBAL);
}

namespace {
TextEncodingRegistry* GetRegistry() {
  static Singleton<TextEncodingRegistry>::Holder sh;
  return sh.Get();
}
} // namespace

const TextEncodingRegistry& TextEncoding::Registry() {
  return *GetRegistry();
}

TextEncodingRegistry* TextEncoding::Registry(int32) {
  return GetRegistry();
}

} // namespace fun
