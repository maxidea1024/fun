#pragma once

#include <map>

#include "fun/base/base.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/rw_lock.h"
#include "fun/base/str.h"

namespace fun {

class FUN_BASE_API TextEncodingRegistry;

/**
 * An abstract base class for implementing text encodings
 * like UTF-8 or ISO 8859-1.
 *
 * Subclasses must override the GetCanonicalName(), IsA(),
 * GetCharacterMap() and Convert() methods and need to be
 * thread safe and stateless.
 *
 * TextEncoding also provides static member functions
 * for managing mappings from encoding names to
 * TextEncoding objects.
 */
class FUN_BASE_API TextEncoding {
 public:
  using Ptr = SharedPtr<TextEncoding>;

  enum {
    /** The maximum character byte sequence length supported. */
    MAX_SEQUENCE_LENGTH = 4
  };

  typedef int32 CharacterMap[256];

  virtual ~TextEncoding();

  virtual const char* GetCanonicalName() const = 0;
  virtual bool IsA(const String& encoding_name) const = 0;
  virtual const CharacterMap& GetCharacterMap() const = 0;
  virtual int32 Convert(const uint8* bytes) const;
  virtual int32 QueryConvert(const uint8* bytes, int32 len) const;
  virtual int32 SequenceLength(const uint8* bytes, int32 len) const;
  virtual int32 Convert(int32 ch, uint8* bytes, int32 len) const;
  static TextEncoding& ByName(const String& encoding_name);
  static SharedPtr<TextEncoding> Find(const String& encoding_name);
  static void Add(SharedPtr<TextEncoding> encoding);
  static void Add(SharedPtr<TextEncoding> encoding, const String& name);
  static void Remove(const String& encoding_name);
  static SharedPtr<TextEncoding> Global(SharedPtr<TextEncoding> encoding);
  static TextEncoding& Global();
  static const String GLOBAL;
  static const TextEncodingRegistry& Registry();

 protected:
  static TextEncodingRegistry* Registry(int32);
};

/**
 * This class serves as the main registry for all
 * supported TextEncoding's.
 */
class FUN_BASE_API TextEncodingRegistry {
 public:
  TextEncodingRegistry();
  ~TextEncodingRegistry();

  bool Has(const String& name) const;
  void Add(SharedPtr<TextEncoding> encoding);
  void Add(SharedPtr<TextEncoding> encoding, const String& name);
  void Remove(const String& name);
  SharedPtr<TextEncoding> Find(const String& name) const;

 private:
  TextEncodingRegistry(const TextEncodingRegistry&) = delete;
  TextEncodingRegistry& operator=(const TextEncodingRegistry&) = delete;

  typedef std::map<String, SharedPtr<TextEncoding>, CILess> EncodingMap;

  EncodingMap encodings_;
  mutable RWLock lock_;
};

}  // namespace fun
