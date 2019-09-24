#include "fun/base/string/byte_string.h"
#include "fun/base/locale/locale_private.h"
#include "fun/base/locale/locale_tool.h"
#include "fun/base/container/slicing_helper.h"
#include "fun/base/string/byte_string_matcher.h"
#include "fun/base/string/string_algo.h"
#include "fun/base/string/byte_string_lut.h"
#include "fun/base/string/utf.h"

#include "fun/base/string/string_internal.h"
#include "fun/base/string/string_split.h"
#include "fun/base/string/string_join.h"

//NOTE data_->bCapacityReserved는 Reserve() 함수를 통해서, capacity를 일부러 잡아놓은 경우인지 아닌지를 판단하는 용도임.

/*! \class ByteString
  \brief ANSI / UTF8 문자열 뿐만 아니라, 바이너리 RAW 데이터도 다룰수 있는 클래스.

  기본적으로 std::string 클래스와 유사한 기능들을 지원합니다.

  통신으로 오고가는 데이터처리에서는 유니코드 기반의 UString을 사용하기에는 다소 비효율적 부분이 있으며,
  처리가 애매한 부분이 있다.

  Redis, Zookeeper 등의 적지 않은 외부 솔류션들이 바이트 스트링(std::string같은)을 사용하기 때문에,
  단지 타입 때문에, 불필요한 변환과 복사를 수반하게 된다.

  이럴 경우, 가급적 바이트 스트링을 그대로 액세스 할 수 있는 수단이 필요하게 되는데,
  이때 사용되는 클래스가 \c ByteString이다.

  기본 구조는 std::string과 유사한 성격을 띄고 있다.

  \c UString과 통합되어 있으므로, 상호 변환등의 작업을 명시적으로 하지 않아도 자동으로 처리가 가능하다.


  \todo 검색관련해서, casesenseitive를 선택적으로 적용할 수 있도록 함.
  \todo 버퍼 공유, 외부 버퍼를 사용하는 등의 기능도 고려해야함. 기존의 CByteArrayPtr이 좀 많이 후짐...
  \todo 작업이 어느정도 안정화되면, UString도 유사한 형태로 정리하도록 한다.

  일반 문자열(nul-terminated) 뿐만 아니라, raw binary도 처리할 수 있어야함.

  버퍼 관리를 어떤식으로 해야할지??


  \todo Visualizer 지원.
  \todo 외부 버퍼를 attach해서 채우기 용도로 사용할 수 있으면 좋을듯..
*/

//TODO truncation 경고가 너무 많아서 임시로 꺼줌. 코드 정리 후 명시적으로 처리하는 코드를 준다던지 하는게 좋을듯...
#ifdef _MSC_VER
#pragma warning(disable : 4244) // truncation warning (size_t -> fun::int32)
#pragma warning(disable : 4267) // truncation warning (size_t -> fun::int32)
#endif

namespace fun {

/*! \enum ByteString::Base64Option
  \since FUN 1.0.2

  base64 encoding 관련 옵션 플래그들입니다.

  \value Base64Encoding   기본 옵션입니다.
  \value Base64UrlEncoding  TODO documentation
  \value KeepTrailingEquals TODO documentation
  \value OmitTrailingEquals TODO documentation
*/

namespace {

// 원래 문자열 뒷편에 붙일 경우, 초과하는 부분에는 ' ' 문자로 채워줌.
const char EXTRA_FILL_CHARACTER = 0x20;

FUN_ALWAYS_INLINE void NulTerm(ByteString::Data* data) {
  data->MutableData()[data->length] = '\0';
}

FUN_ALWAYS_INLINE void StringSet(char* dst, char ch, size_t len) {
  UnsafeMemory::Memset(dst, ch, len);
}

FUN_ALWAYS_INLINE void StringCopy(char* dst, const char* src, size_t len) {
  UnsafeMemory::Memcpy(dst, src, len * sizeof(char));
}

FUN_ALWAYS_INLINE void StringMove(char* dst, const char* src, size_t len) {
  UnsafeMemory::Memmove(dst, src, len * sizeof(char));
}

} // namespace


ByteString::ByteString(ByteStringDataPtr data_ptr)
  : data_(static_cast<Data*>(data_ptr.ptr)) {}

ByteString::ByteString()
  : data_(Data::SharedEmpty()) {}

ByteString::ByteString(const char* str) {
  const int32 len = CStringTraitsA::Strlen(str);

  if (len == 0) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(len + 1);
    StringCopy(data_->MutableData(), str, len);
    data_->length = len;
    NulTerm(data_);
  }
}

ByteString::ByteString(const char* str, int32 len) {
  fun_check(str != nullptr || len == 0);

  if (len < 0) {
    len = CStringTraitsA::Strlen(str);
  }

  if (len == 0) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(len + 1);
    StringCopy(data_->MutableData(), str, len);
    data_->length = len;
    NulTerm(data_);
  }
}

ByteString::ByteString(const char* begin, const char* end) {
  fun_check(end >= begin);
  
  int32 len = int32(end - begin);
  if (len == 0) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(len + 1);
    StringCopy(data_->MutableData(), begin, len);
    data_->length = len;
    NulTerm(data_);
  }
}

ByteString::ByteString(int32 count, char ch) {
  if (count <= 0) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(count + 1);
    StringSet(data_->MutableData(), ch, count);
    data_->length = count;
    NulTerm(data_);
  }
}

ByteString::ByteString(char ch) {
  //fun_check(ch != 0); // strict?

  data_ = Data::Allocate(1 + 1);

  char* dst = data_->MutableData();
  dst[0] = ch;
  dst[1] = '\0';

  data_->length = 1;
}

ByteString::ByteString(int32 len, NoInit_TAG) {
  if (len <= 0) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(len + 1);
    data_->length = len;
    NulTerm(data_);
  }
}

ByteString::ByteString(int32 len, ReservationInit_TAG)
  : data_(Data::SharedEmpty()) {
  Reserve(len);
}

ByteString::ByteString(const ByteString& rhs)
  : data_(rhs.data_) {
  data_->ref.AddRef();
}

ByteString::ByteString(ByteStringView str) {
  if (str.IsEmpty()) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(str.Len() + 1);
    StringCopy(data_->MutableData(), str.ConstData(), str.Len());
    data_->length = str.Len();
    NulTerm(data_);
  }
}

ByteString& ByteString::operator = (const char* str) {
  return (*this = ByteStringView(str));
}

ByteString& ByteString::operator = (ByteStringView str) {
  Data* new_data;
  if (str.IsEmpty()) {
    new_data = Data::SharedEmpty();
  } else {
    const int32 str_len = str.Len();
    const int32 final_len = str_len + 1;

    if (data_->ref.IsShared() || // If data block is sharing, you have to be new. (COW)
        final_len > (int32)data_->alloc || // Space larger than the currently allocated size is required. (Grow)
        (str_len < data_->length && final_len < ((int32)data_->alloc / 2)) // Existing space is big compared to new contents. (Shrink)
      ) {
      ReallocateData(final_len, data_->DetachFlags());
    }

    new_data = data_; // changed

    StringCopy(new_data->MutableData(), str.ConstData(), final_len); // with nul-terminator.
    new_data->length = str_len;

    //필요한가?
    NulTerm(new_data);
  }

  new_data->ref.AddRef();

  if (0 == data_->ref.DecRef()) {
    Data::Free(data_);
  }

  data_ = new_data;

  return *this;
}

ByteString::ByteString(AsciiString str) {
  if (str.IsEmpty()) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(str.Len() + 1);
    StringCopy(data_->MutableData(), str.ConstData(), str.Len());
    data_->length = str.Len();
    NulTerm(data_);
  }
}

ByteString& ByteString::operator = (AsciiString str) {
  return (*this = ByteStringView(str.ConstData(), str.Len()));
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString::ByteString(const UNICHAR* str, int32 len) {
  UNICHAR_TO_UTF8_BUFFER str_as_utf8(str,len);

  if (str_as_utf8.IsEmpty()) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(str_as_utf8.Len() + 1);
    StringCopy(data_->MutableData(), str_as_utf8.ConstData(), str_as_utf8.Len());
    data_->length = str_as_utf8.Len();
    NulTerm(data_);
  }
}

ByteString::ByteString(UNICHAR ch) {
  UNICHAR_TO_UTF8_BUFFER char_as_utf8(&ch,1);

  data_ = Data::Allocate(char_as_utf8.Len() + 1);
  StringCopy(data_->MutableData(), char_as_utf8.ConstData(), char_as_utf8.Len());
  data_->length = char_as_utf8.Len();
  NulTerm(data_);
}

ByteString::ByteString(int32 count, UNICHAR ch) {
  fun_check(count >= 0);

  if (count <= 0) {
    data_ = Data::SharedEmpty();
  } else {
    UNICHAR_TO_UTF8_BUFFER char_as_utf8(&ch,1);

    data_ = Data::Allocate(char_as_utf8.Len()*count + 1);
    char* dst = data_->MutableData();
    for (int32 i = 0; i < count; ++i) {
      StringCopy(dst, char_as_utf8.ConstData(), char_as_utf8.Len());
      dst += char_as_utf8.Len();
    }
    data_->length = char_as_utf8.Len()*count;
    NulTerm(data_);
  }
}

ByteString::ByteString(UStringView str) {
  UNICHAR_TO_UTF8_BUFFER str_as_utf8(str);

  if (str_as_utf8.IsEmpty()) {
    data_ = Data::SharedEmpty();
  } else {
    data_ = Data::Allocate(str_as_utf8.Len() + 1);
    StringCopy(data_->MutableData(), str_as_utf8.ConstData(), str_as_utf8.Len());
    data_->length = str_as_utf8.Len();
    NulTerm(data_);
  }
}

ByteString& ByteString::operator = (UNICHAR ch) {
  return *this = UStringView(&ch,1);
}

ByteString& ByteString::operator = (UStringView str) {
  return (*this = UNICHAR_TO_UTF8_BUFFER(str).ToByteStringView());
}
#endif //FUN_USE_IMPLICIT_STRING_CONVERSION


ByteString::~ByteString() {
  if (0 == data_->ref.DecRef()) {
    Data::Free(data_);
  }
}

ByteString& ByteString::operator = (const ByteString& other) {
  if (FUN_LIKELY(other.data_ != data_)) {
    other.data_->ref.AddRef();

    if (0 == data_->ref.DecRef()) {
      Data::Free(data_);
    }

    data_ = other.data_;
  }
  return *this;
}

ByteString& ByteString::operator = (char ch) {
  if (IsDetached() && Capacity() >= 1) { // capacity는 순수하게 글자 갯수임. 즉, 실제 할당은 글자수+1임 (단, 비어있는 경우에는 0)
    char* dst = data_->MutableData();
    dst[0] = ch;
    dst[1] = '\0';
    data_->length = 1;
  } else {
    operator = (ByteString(ch));
  }
  return *this;
}

ByteString::ByteString(ByteString&& rhs)
  : data_(rhs.data_) {
  rhs.data_ = Data::SharedEmpty();
}

ByteString& ByteString::operator = (ByteString&& rhs) {
  Swap(rhs);
  return *this;
}

void ByteString::Swap(ByteString& rhs) {
  fun::Swap(data_, rhs.data_);
}

int32 ByteString::Len() const {
  return data_->length;
}

int32 ByteString::NulTermLen() const {
  const char* p = cbegin();
  const char* e = cend();
  while (p != e && *p) ++p;
  return p - cbegin();
}

int32 ByteString::Capacity() const {
  return data_->alloc ? data_->alloc - 1 : 0;
}

bool ByteString::IsEmpty() const {
  return data_->length == 0;
}

//TODO 공유버퍼에 대한 감안을 해야할듯...
void ByteString::ResizeUninitialized(int32 new_len) {
  fun_check(new_len >= 0);

  if (new_len < 0) {
    new_len = 0;
  }

  if (data_->IsRawData() && !data_->ref.IsShared() && new_len < data_->length) {
    data_->length = new_len;
    return;
  }

  if (new_len == 0 && !data_->capacity_reserved) {
    Data* new_data = Data::Allocate(0);
    if (0 == data_->ref.DecRef()) {
      Data::Free(data_);
    }

    data_ = new_data;
    
  } else if (data_->length == 0 && data_->ref.IsPersistent()) { // 할당된 내용이 전혀 없었기 때문에, realloc이 아닌 alloc임. (카피 X)
    Data* new_data = Data::Allocate(new_len + 1);
    new_data->length = new_len;
    NulTerm(new_data);

    data_ = new_data;
    
  } else {
    const int32 required_alloc = new_len + 1;

    if (data_->ref.IsShared() || // 공유중이었을 경우는, 새블럭을 할당 받아야함. (COW)
        required_alloc > (int32)data_->alloc || // 메모리가 부족하다면, 새블럭을 할당 받아야함. (Grow)
        (!data_->capacity_reserved && new_len < data_->length && required_alloc < ((int32)data_->alloc / 2))) { // 담을 내용이 비대하다면, Shrink
      ReallocateData(required_alloc, data_->DetachFlags() | Data::Grow);
    }

    if (data_->alloc > 0) {
      data_->length = new_len;
      NulTerm(data_);
    }
  }
}

void ByteString::ResizeZeroed(int32 new_len) {
  Resize(new_len, char(0));
}

void ByteString::Resize(int32 new_len, char filler) {
  fun_check(new_len >= 0);

  const int32 old_len = Len();
  ResizeUninitialized(new_len);
  const int32 difference = Len() - old_len;
  if (difference > 0) {
    StringSet(data_->MutableData() + old_len, filler, difference);
  }
}

ByteString& ByteString::Fill(char filler, int32 len) {
  ResizeUninitialized(len < 0 ? Len() : len);

  if (Len() > 0) {
    StringSet(data_->MutableData(), filler, Len());
  }

  return *this;
}

const char* ByteString::operator * () const {
  return data_->ConstData();
}

char* ByteString::MutableData() {
  Detach();
  return data_->MutableData();
}

char* ByteString::MutableData(int32 len) {
  Detach();
  ResizeUninitialized(len);
  return data_->MutableData();
}

const char* ByteString::ConstData() const {
  return data_->ConstData();
}

const char* ByteString::c_str() const {
  // NUL term을 보장해야함.
  ByteString* mutable_this = const_cast<ByteString*>(this);
  mutable_this->TrimToNulTerminator();

  return data_->ConstData();
}

bool ByteString::IsNulTerm() const {
  return NulTermLen() == Len();
}

ByteString& ByteString::TrimToNulTerminator() {
  const int32 nul_pos = IndexOf('\0');
  if (nul_pos != INVALID_INDEX) { // Len()까지만 찾기를 수행하므로, 일반적인 경우라면 NUL이 발견 안될것임.
    Truncate(nul_pos);
  }
  return *this;
}

ByteString ByteString::ToNulTerminated() const {
  return ByteString(*this).TrimToNulTerminator();
}

char ByteString::At(int32 index) const {
  fun_check(uint32(index) < uint32(Len()));
  return ConstData()[index];
}

char ByteString::operator[] (int32 index) const {
  fun_check(uint32(index) < uint32(Len()));
  return ConstData()[index];
}

ByteString::CharRef ByteString::operator[] (int32 index) {
  fun_check(index >= 0);
  return CharRef(*this, index);
}

char ByteString::First() const {
  return At(0);
}

ByteString::CharRef ByteString::First() {
  return operator[](0);
}

char ByteString::Last() const {
  return At(Len() - 1);
}

ByteString::CharRef ByteString::Last() {
  return operator[](Len() - 1);
}

char ByteString::FirstOr(const char def) const {
  return Len() ? ConstData()[0] : def;
}

char ByteString::LastOr(const char def) const {
  return Len() ? ConstData()[Len() - 1] : def;
}

void ByteString::Reserve(int32 new_capacity) {
  if (data_->ref.IsShared() || (new_capacity + 1) > (int32)data_->alloc) {
    if (new_capacity < Len()) {
      new_capacity = Len();
    }

    ReallocateData(new_capacity + 1, data_->DetachFlags() | Data::CapacityReserved);
  } else {
    // cannot set unconditionally, since data could be the SharedNull or otherwise Persistent.
    data_->capacity_reserved = false;
  }
}

void ByteString::Shrink() {
  if (data_->ref.IsShared() || data_->length + 1 < (int32)data_->alloc) {
    ReallocateData(data_->length + 1, data_->DetachFlags() & ~Data::CapacityReserved); // CapacityReserved 플래그를 클리어. 즉, 여분 없이 딱 핏팅되어 있는 상태임.
    fun_check_dbg(data_->alloc == data_->length + 1);
  } else {
    // cannot set unconditionally, since data could be the SharedNull or otherwise Persistent.
    data_->capacity_reserved = 1;
  }
}

void ByteString::Clear() {
  if (0 == data_->ref.DecRef()) {
    Data::Free(data_);
  }

  data_ = Data::SharedEmpty();
}

void ByteString::Clear(int32 initial_capacity) {
  if (initial_capacity < 0) {
    initial_capacity = 0;
  }

  if (initial_capacity <= 0) {
    Clear();
  } else {
    //TODO optimize
    ResizeUninitialized(0);
    Reserve(initial_capacity);
  }
}

void ByteString::Detach() {
  if (data_->ref.IsShared() || data_->IsRawData()) { // 공유중이거나, raw data일 경우에는 사본을 만들어야함. (COW)
    ReallocateData(data_->length + 1, data_->DetachFlags());
  }
}

bool ByteString::IsDetached() const {
  return data_->ref.IsShared() == false;
}

bool ByteString::IsSharedWith(const ByteString& rhs) const {
  return data_ == rhs.data_;
}

bool ByteString::IsRawData() const {
  return data_->IsRawData();
}

void ByteString::ReallocateData(int32 new_alloc, Data::AllocationOptions options) {
  fun_check(new_alloc >= 0);

  if (data_->ref.IsShared() || data_->IsRawData()) {
    Data* new_data = Data::Allocate(new_alloc, options);
    new_data->length = MathBase::Min(new_alloc - 1, data_->length);
    StringCopy(new_data->MutableData(), data_->ConstData(), new_data->length);
    NulTerm(new_data);

    if (0 == data_->ref.DecRef()) { // release previously owned data block.
      Data::Free(data_);
    }

    data_ = new_data;
    fun_check_dbg(data_->ref.GetCounter() == RefCounter::INITIAL_OWNED_COUNTER_VALUE);
    
  } else {
    // 공유/raw도 아니므로, 자체적으로 공간만 재조정함.
    data_ = Data::ReallocateUnaligned(data_, new_alloc, options);
  }
}

void ByteString::ExpandAt(int32 pos) {
  if (pos >= Len()) {
    ResizeUninitialized(pos + 1);
  }
}


int32 ByteString::IndexOfAny(const Array<char>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAny(ByteStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index, matched_len); }

int32 ByteString::LastIndexOfAny(const Array<char>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return LastIndexOfAny(ByteStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index, matched_len); }

int32 ByteString::IndexOfAny(const Array<const char*>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(const Array<ByteString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(const Array<ByteStringRef>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(const Array<ByteStringView>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(const Array<AsciiString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }

int32 ByteString::IndexOfAny(std::initializer_list<const char*> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(std::initializer_list<ByteString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(std::initializer_list<ByteStringRef> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(std::initializer_list<ByteStringView> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteString::IndexOfAny(std::initializer_list<AsciiString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }

int32 ByteString::IndexOf(ByteStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.IsEmpty()) {
    return from;
  }

  if (sub.Len() == 1) { // 한글자 찾기로 전환.. 이게 좀더 빠름...
    return ByteStringMatcher::FastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }

  const int32 this_len = Len();
  if (from > this_len || (sub.Len() + from) > this_len) {
    return INVALID_INDEX;
  }

  return ByteStringMatcher::FastFind(ConstData(), this_len, from, sub.ConstData(), sub.Len(), casesense);
}

int32 ByteString::LastIndexOf(ByteStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.Len() == 1) {
    return ByteStringMatcher::FastLastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }

  return ByteStringMatcher::FastLastFind(ConstData(), Len(), from, sub.ConstData(), sub.Len(), casesense);
}

int32 ByteString::Count(ByteStringView sub, CaseSensitivity casesense) const {
  int32 n = 0, pos = -int32(sub.Len());
  while ((pos = IndexOf(sub, casesense, pos + sub.Len())) != INVALID_INDEX) ++n;
  return n;
}

ByteString ByteString::Left(int32 len) const {
  fun_check(len >= 0);

  if (len >= Len()) {
    return *this;
  }

  if (len < 0) {
    return ByteString();
  }

  return ByteString(ConstData(), len);
}

ByteString ByteString::Right(int32 len) const {
  fun_check(len >= 0);

  if (len >= Len()) {
    return *this;
  }

  if (len < 0) {
    return ByteString();
  }

  return ByteString(ConstData() + Len() - len, len);
}

ByteString ByteString::Mid(int32 offset, int32 len) const {
  switch (SlicingHelper::Slice(Len(), &offset, &len)) {
    case SlicingHelper::Null:
      return ByteString();
    case SlicingHelper::Empty: {
        ByteString empty;
        empty.data_ = Data::SharedEmpty();
        return empty;
      }
    case SlicingHelper::Full:
      return *this;
    case SlicingHelper::Subset:
      return ByteString(ConstData() + offset, len);
  }
  //unreachable to here
  return ByteString();
}

ByteString ByteString::LeftChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Left(Len() - len);
}

ByteString ByteString::RightChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Right(Len() - len);
}

ByteStringRef ByteString::LeftRef(int32 len) const {
  return ByteStringRef(this).Left(len);
}

ByteStringRef ByteString::MidRef(int32 offset, int32 len) const {
  return ByteStringRef(this).Mid(offset, len);
}

ByteStringRef ByteString::RightRef(int32 len) const {
  return ByteStringRef(this).Right(len);
}

ByteStringRef ByteString::LeftChoppedRef(int32 len) const {
  return ByteStringRef(this).LeftChopped(len);
}

ByteStringRef ByteString::RightChoppedRef(int32 len) const {
  return ByteStringRef(this).RightChopped(len);
}

ByteString& ByteString::Reverse() {
  if (Len() > 0) {
    char* b = MutableData();
    char* e = b + Len() - 1;
    do {
      fun::Swap(*b, *e);
      ++b; --e;
    } while (b < e);
  }
  return *this;
}

ByteString ByteString::Reversed() const {
  return ByteString(*this).Reverse();
}

ByteString& ByteString::PrependUnitialized(int32 len) {
  return InsertUninitialized(0, len);
}

ByteString& ByteString::AppendUninitialized(int32 len) {
  fun_check(len >= 0);

  ResizeUninitialized(Len() + len);
  return *this;
}

ByteString& ByteString::InsertUninitialized(int32 pos, int32 len) {
  fun_check(pos >= 0);
  fun_check(len >= 0);

  if (pos < 0 || len <= 0) {
    return *this;
  }

  const int32 old_len = Len();
  ResizeUninitialized(MathBase::Max(pos, old_len) + len);

  if (pos < old_len) {
    char* dst = MutableData();
    StringMove(dst + pos + len, dst + pos, old_len - pos);
  }

  return *this;
}

ByteString& ByteString::PrependZeroed(int32 len) {
  return InsertZeroed(0, len);
}

ByteString& ByteString::AppendZeroed(int32 len) {
  fun_check(len >= 0);

  if (len > 0) {
    const int32 old_len = Len();
    ResizeUninitialized(old_len + len);
    StringSet(data_->MutableData() + old_len, 0x00, len);
  }
  return *this;
}

ByteString& ByteString::InsertZeroed(int32 pos, int32 len) {
  fun_check(pos >= 0);
  fun_check(len >= 0);

  if (pos < 0 || len <= 0) {
    return *this;
  }

  const int32 old_len = Len();
  ResizeUninitialized(MathBase::Max(pos, old_len) + len);
  char* dst = MutableData();

  if (pos > old_len) {
    StringSet(dst + old_len, 0x00, pos - old_len);
  } else if (pos < old_len) {
    StringMove(dst + pos + len, dst + pos, old_len - pos);
  }
  StringSet(dst + pos, 0x00, len);
  return *this;
}

bool ByteString::StartsWith(ByteStringView sub, CaseSensitivity casesense) const {
  return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool ByteString::EndsWith(ByteStringView sub, CaseSensitivity casesense) const {
  return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool ByteString::GlobMatch(ByteStringView pattern, CaseSensitivity casesense) const {
  //TODO
  fun_check(0);
  return false;

  //return GlobA::Match(cbegin(), cend(), pattern.cbegin(), pattern.cend(), casesense);
}

ByteString& ByteString::Truncate(int32 pos) {
  fun_check(pos >= 0);

  if (pos < Len()) {
    ResizeUninitialized(pos);
  }
  return *this;
}

ByteString& ByteString::LeftChop(int32 len) {
  fun_check(len >= 0);
  fun_check(len <= Len()); //strict?

  if (len > 0) {
    ResizeUninitialized(Len() - len); // Resize() 함수는 Length가 음수일 경우, 0으로 취급하므로 상관 없음. 그져, 비어있는 ByteString()이 될뿐.
  }
  return *this;
}

ByteString& ByteString::RightChop(int32 len) {
  fun_check(len >= 0);
  fun_check(len <= Len()); //strict?

  if (len > 0) {
    if (len > Len()) {
      Clear();
    } else {
      Remove(0, len);
    }
  }
  return *this;
}

ByteString& ByteString::MakeLower() {
  //TODO 변경되는 시점에서 Detach()하는 쪽으로 변경.
  char* p = MutableData(); // detach for modyfing
  char* e = p + Len();
  for (; p != e; ++p) {
    *p = char(latin1_lowercased[uint8(*p)]);
  }
  return *this;
}

ByteString& ByteString::MakeUpper() {
  //TODO 변경되는 시점에서 Detach()하는 쪽으로 변경.
  char* p = MutableData(); // detach for modyfing
  char* e = p + Len();
  for (; p != e; ++p) {
    *p = char(latin1_uppercased[uint8(*p)]);
  }
  return *this;
}

ByteString ByteString::ToLower() const {
  return ByteString(*this).MakeLower();
}

ByteString ByteString::ToUpper() const {
  return ByteString(*this).MakeUpper();
}

ByteString& ByteString::TrimLeft() {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = LeftSpaces();
  if (space_count) {
    Remove(0, space_count);
  }
  return *this;
}

ByteString& ByteString::TrimRight() {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = RightSpaces();
  if (space_count) {
    //Resize(Len() - space_count);
    Remove(Len() - space_count, space_count);
  }
  return *this;
}

ByteString& ByteString::Trim() {
  if (Len() == 0) {
    return *this;
  }

  const int32 l_space_count = LeftSpaces();
  const int32 r_space_count = RightSpaces();

  //왼쪽부터 처리하게 되면, 오프셋의 변화가 오게 되므로, 오른쪽 부터 처리하도록 함.
  if (r_space_count) {
    Remove(Len() - r_space_count, r_space_count);
  }
  if (l_space_count) {
    Remove(0, l_space_count);
  }
  return *this;
}

ByteString ByteString::TrimmedLeft() const {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = LeftSpaces();
  if (space_count) {
    return ByteString(ConstData() + space_count, Len() - space_count);
  } else {
    return *this;
  }
}

ByteString ByteString::TrimmedRight() const {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = RightSpaces();
  if (space_count) {
    return ByteString(ConstData(), Len() - space_count);
  } else {
    return *this;
  }
}

ByteString ByteString::Trimmed() const {
  if (Len() == 0) {
    return *this;
  }

  const char* start = cbegin();
  const char* end = cend();
  StringAlgo<ByteString>::TrimmedPositions(start, end);
  if (start == end) {
    return ByteString();
  }

  return ByteString(start, end - start);
}

int32 ByteString::LeftSpaces() const {
  return StringAlgo<ByteString>::LeftSpaces(cbegin(), cend());
}

int32 ByteString::RightSpaces() const {
  return StringAlgo<ByteString>::RightSpaces(cbegin(), cend());
}

int32 ByteString::SideSpaces() const {
  return StringAlgo<ByteString>::SideSpaces(cbegin(), cend());
}

ByteStringRef ByteString::TrimmedLeftRef() const {
  return ByteStringRef(this).TrimmedLeft();
}

ByteStringRef ByteString::TrimmedRightRef() const {
  return ByteStringRef(this).TrimmedRight();
}

ByteStringRef ByteString::TrimmedRef() const {
  return ByteStringRef(this).Trimmed();
}

ByteString& ByteString::Simplify() {
  return (*this = Simplified());
}

ByteString ByteString::Simplified() const {
  return StringAlgo<const ByteString>::Simplified(*this);
}

ByteString ByteString::LeftJustified(int32 width, char filler, bool truncate) const {
  fun_check(width >= 0);

  ByteString result;

  const int32 this_len = Len();
  const int32 pad_len = width - this_len;

  // Since the length of the original string is shorter than the width, it adds extra padding.
  if (pad_len > 0) {
    result.ResizeUninitialized(this_len + pad_len);

    if (this_len) {
      StringCopy(result.MutableData(), ConstData(), this_len);
    }

    StringSet(result.MutableData() + this_len, filler, pad_len);
  }
  // Because the length of the string is longer than width, it returns the string as it is.
  // However, if the truncate option is true, it forces the length to be the width.
  else {
    if (truncate) {
      result = Left(width);
    } else {
      result = *this;
    }
  }
  return result;
}

ByteString ByteString::RightJustified(int32 width, char filler, bool truncate) const {
  fun_check(width >= 0);

  ByteString result;

  const int32 this_len = Len();
  const int32 pad_len = width - this_len;

  // Since the length of the original string is shorter than the width, it adds extra padding.
  if (pad_len > 0) {
    result.ResizeUninitialized(this_len + pad_len);

    if (this_len) {
      StringCopy(result.MutableData() + pad_len, ConstData(), this_len);
    }

    StringSet(result.MutableData(), filler, pad_len);
  }
  // Because the length of the string is longer than width, it returns the string as it is.
  // However, if the truncate option is true, it forces the length to be the width.
  else {
    if (truncate) {
      result = Left(width);
    } else {
      result = *this;
    }
  }

  return result;
}

ByteString& ByteString::Prepend(char ch) {
  return Insert(0, ch);
}

ByteString& ByteString::Prepend(int32 count, char ch) {
  return Insert(0, count, ch);
}

ByteString& ByteString::Prepend(ByteStringView str) {
  return Insert(0, str);
}

ByteString& ByteString::Append(int32 count, char ch) {
  fun_check(count >= 0);

  if (count > 0) {
    const int32 old_len = Len();
    ResizeUninitialized(old_len + count);

    StringSet(data_->MutableData() + old_len, ch, count);
  }
  return *this;
}

ByteString& ByteString::Append(ByteStringView str) {
  if (str.Len() > 0) {
    const int32 old_len = Len();
    ResizeUninitialized(old_len + str.Len());

    StringCopy(data_->MutableData() + old_len, str.ConstData(), str.Len());
  }
  return *this;
}

ByteString& ByteString::Insert(int32 pos, ByteStringView str) {
  fun_check(pos >= 0);

  if (str.IsEmpty()) {
    return *this;
  }

  const int32 old_len = Len();
  ResizeUninitialized(MathBase::Max(pos, old_len) + str.Len());

  char* dst = MutableData();
  if (pos > old_len) {
    StringSet(dst + old_len, EXTRA_FILL_CHARACTER, pos - old_len);
  } else {
    StringMove(dst + pos + str.Len(), dst + pos, old_len - pos);
  }
  StringCopy(dst + pos, str.ConstData(), str.Len());
  return *this;
}

ByteString& ByteString::Insert(int32 pos, int32 count, char ch) {
  fun_check(pos >= 0);
  fun_check(count >= 0);

  if (pos < 0 || count <= 0) {
    return *this;
  }

  const int32 old_len = Len();
  ResizeUninitialized(MathBase::Max(pos, old_len) + count);
  char* dst = MutableData();

  if (pos > old_len) {
    StringSet(dst + old_len, EXTRA_FILL_CHARACTER, pos - old_len);
  } else if (pos < old_len) {
    StringMove(dst + pos + count, dst + pos, old_len - pos);
  }
  StringSet(dst + pos, ch, count);
  return *this;
}

ByteString& ByteString::Overwrite(int32 pos, int32 count, char ch) {
  fun_check(pos >= 0);
  fun_check(count >= 0);

  if (count > 0) {
    Detach();
    ExpandAt(pos + count - 1);
    StringSet(data_->MutableData() + pos, ch, count);
  }

  return *this;
}

ByteString& ByteString::Overwrite(int32 pos, ByteStringView str) {
  fun_check(pos >= 0);

  if (str.Len() != 0) {
    Detach();
    ExpandAt(pos + str.Len() - 1);
    StringCopy(data_->MutableData() + pos, str.ConstData(), str.Len());
  }

  return *this;
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString& ByteString::Overwrite(int32 pos, int32 count, UNICHAR ch) {
  UNICHAR_TO_UTF8_BUFFER char_as_utf8(&ch,1);
  for (int32 i = 0; i < count; ++i) {
    Overwrite(pos, char_as_utf8.ConstData(), char_as_utf8.Len());
    pos += char_as_utf8.Len();
  }
  return *this;
}
#endif

ByteString& ByteString::Remove(int32 pos, int32 length_to_remove) {
  fun_check(pos >= 0);
  fun_check(length_to_remove >= 0);
  fun_check((pos + length_to_remove) <= Len());

  if (length_to_remove <= 0 || uint32(pos) >= uint32(Len())) {
    return *this;
  }

  if (length_to_remove >= (Len() - pos)) {
    ResizeUninitialized(pos);
  } else {
    StringMove(MutableData() + pos, MutableData() + pos + length_to_remove, Len() - pos - length_to_remove);
    ResizeUninitialized(Len() - length_to_remove);
  }

  return *this;
}

int32 ByteString::TryFindAndRemove(ByteStringView sub, CaseSensitivity casesense) {
  int32 pos, removed_count = 0;
  while ((pos = IndexOf(sub, casesense)) != INVALID_INDEX) {
    Remove(pos, sub.Len());
    ++removed_count;
  }
  return removed_count;
}

ByteString& ByteString::Replace(int32 before_pos, int32 old_len, ByteStringView after) {
  if (old_len == after.Len() && (before_pos + old_len <= Len())) { // If the contents before and after the change are the same length and are in the range, overwrite
    Detach();
    StringCopy(MutableData() + before_pos, after.ConstData(), after.Len());
    return *this;
  } else {
    Remove(before_pos, old_len);
    return Insert(before_pos, after.ConstData(), after.Len());
  }
}

int32 ByteString::TryReplace(ByteStringView before, ByteStringView after, CaseSensitivity casesense) {
  if (before.ConstData() == after.ConstData() && before.Len() == after.Len()) {
    return 0;
  }

  // 찾아서 삭제하기로... 차후에 생각을 좀 해봐야함...
  if (after.ConstData() == nullptr || after.Len() <= 0) {
    return TryFindAndRemove(before, casesense);
  }

  int32 removed_count = 0;

  // protect against before.ConstData() or after.ConstData() being part of this
  const char* saved_new_ptr = after.ConstData();
  const char* saved_old_ptr = before.ConstData();

  if (after.ConstData() >= ConstData() && after.ConstData() < ConstData() + Len()) { // 자기자신에 대해서 처리할 경우 사본을 만든 후 처리 (inplace operation)
    char* copy = (char*)UnsafeMemory::Malloc(after.Len());
    StringCopy(copy, after.ConstData(), after.Len());
    saved_new_ptr = copy;
  }

  if (before.ConstData() >= ConstData() && before.ConstData() < ConstData() + Len()) { // 자기자신에 대해서 처리할 경우 사본을 만든 후 처리 (inplace operation)
    char* copy = (char*)UnsafeMemory::Malloc(before.Len());
    StringCopy(copy, before.ConstData(), before.Len());
    saved_old_ptr = copy;
  }


  ByteStringMatcher matcher(before, casesense);

  int32 index = 0;
  int32 this_len = Len();
  char* d = MutableData(); // with detach for modifying

  if (before.Len() == after.Len()) {
    if (before.Len() > 0) {
      while ((index = matcher.IndexIn(*this, index)) != INVALID_INDEX) {
        StringCopy(d + index, after.ConstData(), after.Len());
        index += before.Len();
        ++removed_count;
      }
    }
  } else if (after.Len() < before.Len()) {
    uint32 to = 0;
    uint32 move_start = 0;
    uint32 num = 0;
    while ((index = matcher.IndexIn(*this, index)) != INVALID_INDEX) {
      if (num > 0) {
        const int32 move_len = index - move_start;
        if (move_len > 0) {
          StringMove(d + to, d + move_start, move_len);
          to += move_len;
        }
      } else {
        to = index;
      }
      if (after.Len() > 0) {
        StringCopy(d + to, after.ConstData(), after.Len());
        to += after.Len();
        ++removed_count;
      }
      index += before.Len();
      move_start = index;
      ++num;
    }
    if (num > 0) {
      const int32 move_len = this_len - move_start;
      if (move_len > 0) {
        StringMove(d + to, d + move_start, move_len);
      }
      ResizeUninitialized(this_len - num * (before.Len() - after.Len()));
    }
  } else {
    // the most complex case. We don't want to lose performance by doing repeated
    // copies and reallocs of the string.
    while (index != INVALID_INDEX) {
      uint32 indices[4096];
      uint32 pos = 0;
      while (pos < 4095) {
        index = matcher.IndexIn(*this, index);
        if (index == INVALID_INDEX) {
          break;
        }
        indices[pos++] = index;
        index += before.Len();
        // avoid infinite loop
        if (before.IsEmpty()) {
          index++;
        }
      }
      if (pos == 0) {
        break;
      }

      // we have saved_new_ptr table of replacement positions, use them for fast replacing
      const int32 adjust = pos * (after.Len() - before.Len());
      // index has To be adjusted in case we get back int32o the loop above.
      if (index != INVALID_INDEX) {
        index += adjust;
      }
      const int32 final_len = this_len + adjust;
      int32 move_end = this_len;
      if (final_len > this_len) {
        ResizeUninitialized(final_len);
        this_len = final_len;
      }
      d = this->MutableData();

      while (pos > 0) {
        --pos;
        const int32 move_start = indices[pos] + before.Len();
        const int32 insert_start = indices[pos] + pos * (after.Len() - before.Len());
        const int32 move_to = insert_start + after.Len();
        StringMove(d + move_to, d + move_start, (move_end - move_start));
        if (after.Len() > 0) {
          StringCopy(d + insert_start, after.ConstData(), after.Len());
          ++removed_count;
        }
        move_end = move_start - before.Len();
      }
    }
  }

  if (saved_new_ptr != after.ConstData()) {
    UnsafeMemory::Free(const_cast<char*>(saved_new_ptr));
  }

  if (saved_old_ptr != before.ConstData()) {
    UnsafeMemory::Free(const_cast<char*>(saved_old_ptr));
  }

  return removed_count;
}


Array<ByteString> ByteString::Split(char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::Split(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::Split(std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::SplitByWhitespaces(ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; SplitByWhitespaces(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::SplitLines(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; SplitLines(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<ByteString> ByteString::Split(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::Split(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::Split(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteString> ByteString::Split(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteString> ByteString::Split(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 ByteString::Split(Array<ByteString>& list, char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, ByteStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, ByteStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, ByteStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, separators, max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, ByteStringView(separators.ConstData(),separators.Len()), max_splits, split_options, casesense); }

int32 ByteString::Split(Array<ByteString>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,const char*>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteString>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteStringRef>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,AsciiString>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,const char*>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteString>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteStringRef>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,ByteStringView>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,AsciiString>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 ByteString::SplitByWhitespaces(Array<ByteString>& list, ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView("\n") };
    return Split(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView(extra_separator) };
    return Split(list, separators, max_splits, split_options, casesense);
  }
}

int32 ByteString::SplitLines(Array<ByteString>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { ByteStringView("\r\n"), ByteStringView("\n"), ByteStringView("\r") };
  return Split(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 ByteString::Split(Array<ByteString>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }

int32 ByteString::Split(Array<ByteString>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, separators, max_splits, split_options, casesense); }

int32 ByteString::Split(Array<ByteString>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,const UNICHAR*>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UString>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UStringRef>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,const UNICHAR*>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UString>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UStringRef>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::Split(Array<ByteString>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteString,ByteString,UStringView>(list, *this, &ByteString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
#endif

Array<ByteStringRef> ByteString::SplitRef(char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitByWhitespacesRef(ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitByWhitespacesRef(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitLinesRef(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitLinesRef(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<ByteStringRef> ByteString::SplitRef(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteString::SplitRef(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 ByteString::SplitRef(Array<ByteStringRef>& list, char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, ByteStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, ByteStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, ByteStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, separators, max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, ByteStringView(separators.ConstData(),separators.Len()), max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,const char*>(list, *this, &ByteString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteString>(list, *this, &ByteString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteStringRef>(list, *this, &ByteString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,AsciiString>(list, *this, &ByteString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,const char*>(list, *this, &ByteString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteString>(list, *this, &ByteString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteStringRef>(list, *this, &ByteString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,ByteStringView>(list, *this, &ByteString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,AsciiString>(list, *this, &ByteString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 ByteString::SplitByWhitespacesRef(Array<ByteStringRef>& list, ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView("\n") };
    return SplitRef(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView(extra_separator) };
    return SplitRef(list, separators, max_splits, split_options, casesense);
  }
}

int32 ByteString::SplitLinesRef(Array<ByteStringRef>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { AsciiString("\r\n"), AsciiString("\n"), AsciiString("\r") };
  return SplitRef(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 ByteString::SplitRef(Array<ByteStringRef>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, separators, max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,const UNICHAR*>(list, *this, &ByteString::MidRef, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UString>(list, *this, &ByteString::MidRef, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UStringRef>(list, *this, &ByteString::MidRef, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }

int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,const UNICHAR*>(list, *this, &ByteString::MidRef, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UString>(list, *this, &ByteString::MidRef, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UStringRef>(list, *this, &ByteString::MidRef, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 ByteString::SplitRef(Array<ByteStringRef>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteString,UStringView>(list, *this, &ByteString::MidRef, separators.begin(),separators.size(), max_splits, split_options, casesense); }
#endif

bool ByteString::Divide(ByteStringView delim, ByteString* out_left, ByteString* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = IndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = Left(pos); out_left->Trim(); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) *out_left = Left(pos);
    if (out_right) *out_right = Mid(pos + delim.Len());
  }
  return true;
}

bool ByteString::Divide(ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = IndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = LeftRef(pos); out_left->Trim(); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) *out_left = LeftRef(pos);
    if (out_right) *out_right = MidRef(pos + delim.Len());
  }
  return true;
}

bool ByteString::LastDivide(ByteStringView delim, ByteString* out_left, ByteString* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = LastIndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = Left(pos); out_left->Trim(); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) *out_left = Left(pos);
    if (out_right) *out_right = Mid(pos + delim.Len());
  }
  return true;
}

bool ByteString::LastDivide(ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = LastIndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = LeftRef(pos); out_left->Trim(); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) *out_left = LeftRef(pos);
    if (out_right) *out_right = MidRef(pos + delim.Len());
  }
  return true;
}

ByteString ByteString::Join(const Array<const char*>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteString>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringRef>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringView>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<AsciiString>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString ByteString::Join(const Array<const UNICHAR*>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UString>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringRef>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringView>& list, ByteStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
#endif

ByteString ByteString::Join(std::initializer_list<const char*> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteString> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringRef> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringView> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<AsciiString> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString ByteString::Join(std::initializer_list<const UNICHAR*> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UString> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringRef> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringView> list, ByteStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
#endif

ByteString ByteString::Join(const Array<const char*>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteString>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringRef>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringView>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<AsciiString>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString ByteString::Join(const Array<const UNICHAR*>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UString>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringRef>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringView>& list, AsciiString separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
#endif

ByteString ByteString::Join(std::initializer_list<const char*> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteString> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringRef> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringView> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<AsciiString> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString ByteString::Join(std::initializer_list<const UNICHAR*> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UString> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringRef> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringView> list, AsciiString separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
ByteString ByteString::Join(const Array<const char*>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteString>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringRef>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<ByteStringView>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<AsciiString>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<const UNICHAR*>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UString>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringRef>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }
ByteString ByteString::Join(const Array<UStringView>& list, UStringView separator)
{ return StringJoin<ByteString>(list.ConstData(),list.Count(),separator); }

ByteString ByteString::Join(std::initializer_list<const char*> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteString> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringRef> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<ByteStringView> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<AsciiString> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<const UNICHAR*> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UString> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringRef> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
ByteString ByteString::Join(std::initializer_list<UStringView> list, UStringView separator)
{ return StringJoin<ByteString>(list.begin(),list.size(),separator); }
#endif

ByteString ByteString::Repeated(int32 times) const {
  // return itself if empty.
  if (Len() == 0) {
    return *this;
  }

  // 음수인 경우에는 잘못 지정한 경우일 확률이 높음. 단, 0은 허용함.
  // 오류를 찾아내는데 도우미 역활을 할뿐, 실행에 지장이 있지는 않음.
  // times <= 0 일 경우에는 그냥 빈문자열을 돌려줌.
  fun_check(times >= 0);

  if (times <= 1) {
    return times == 1 ? *this : ByteString();
  }

  const int32 result_len = times * Len();

  //maximum length check. 2GB?
  if (result_len != uint32(result_len)) {
    fun_check(0);
    return *this;
  }

  ByteString result(result_len, NoInit);
  StringCopy(result.MutableData(), this->ConstData(), this->Len());

  int32 len_so_far = Len();
  char* end = result.MutableData() + len_so_far;

  const int32 half_result_len = result_len / 2;
  while (len_so_far <= half_result_len) {
    StringCopy(end, result.ConstData(), len_so_far);
    end += len_so_far;
    len_so_far *= 2;
  }
  StringCopy(end, result.ConstData(), result_len - len_so_far);
  NulTerm(result.data_);

  return result;
}

ByteString ByteString::operator * (int32 times) const {
  return Repeated(times);
}

namespace {

int64 ToIntegral_helper(const char* str, int32 len, bool* ok, int32 base, int64/*constrain*/) {
  return LocaleData::ByteStringToInt64(str, len, base, ok);
}

int64 ToIntegral_helper(const char* str, int32 len, bool* ok, int32 base, uint64/*constrain*/) {
  return LocaleData::ByteStringToUInt64(str, len, base, ok);
}

template <typename T>
FUN_ALWAYS_INLINE T ToIntegral(const char* str, int32 len, bool* ok, int32 base) {
  const bool IS_UNSIGNED = T(0) < T(-1);
  typedef typename Conditional<IS_UNSIGNED, uint64, int64>::Result Int64;
  if (base != 0 && (base < 2 || base > 36)) {
    //TODO
    //fun_log(LogCore, Warning, "ByteString::ToIntegral: Invalid base {0}", base);
    base = 10;
  }

  Int64 val = ToIntegral_helper(str, len, ok, base, Int64()/*constrain*/);
  if (T(val) != val) {
    if (ok) {
      *ok = false;
    }
    val = 0;
  }

  return T(val);
}
} // namespace

int16 ByteString::ToInt16(bool* ok, int32 base) const {
  return ToIntegral<int16>(ConstData(), Len(), ok, base);
}

uint16 ByteString::ToUInt16(bool* ok, int32 base) const {
  return ToIntegral<uint16>(ConstData(), Len(), ok, base);
}

int32 ByteString::ToInt32(bool* ok, int32 base) const {
  return ToIntegral<int32>(ConstData(), Len(), ok, base);
}

uint32 ByteString::ToUInt32(bool* ok, int32 base) const {
  return ToIntegral<uint32>(ConstData(), Len(), ok, base);
}

int64 ByteString::ToInt64(bool* ok, int32 base) const {
  return ToIntegral<int64>(ConstData(), Len(), ok, base);
}

uint64 ByteString::ToUInt64(bool* ok, int32 base) const {
  return ToIntegral<uint64>(ConstData(), Len(), ok, base);
}

float ByteString::ToFloat(bool* ok) const {
  return LocaleData::ConvertDoubleToFloat(ToDouble(ok), ok);
}

double ByteString::ToDouble(bool* ok) const {
  bool non_null_ok = false;
  int32 processed;
  const double double_value = AsciiToDouble(ConstData(), Len(), non_null_ok, processed);
  if (ok) {
    *ok = non_null_ok;
  }
  return double_value;
}

namespace {

char* __ulltoa(char* p, uint64 n, int32 base) {
  if (base < 2 || base > 36) {
    //TODO
    //fun_log(LogCore, Warning, "ByteString::SetNumber: Invalid base {0}", base);
    base = 10;
  }

  const char b = 'a' - 10;
  do {
    const int32 c = n % base;
    n /= base;
    *--p = c + (c < 10 ? '0' : b);
  } while (n);

  return p;
}

} // namespace

ByteString& ByteString::SetNumber(int16 value, int32 base) {
  return base == 10 ? SetNumber(int64(value), base) : SetNumber(uint64(value), base);
}

ByteString& ByteString::SetNumber(uint16 value, int32 base) {
  return SetNumber(uint64(value), base);
}

ByteString& ByteString::SetNumber(int32 value, int32 base) {
  return base == 10 ? SetNumber(int64(value), base) : SetNumber(uint64(value), base);
}

ByteString& ByteString::SetNumber(uint32 value, int32 base) {
  return SetNumber(uint64(value), base);
}

ByteString& ByteString::SetNumber(int64 value, int32 base) {
  const int32 BUFFER_SIZE = 66;
  char buffer[BUFFER_SIZE];
  char* p;
  if (value < 0 && base == 10) {
    p = __ulltoa(buffer + BUFFER_SIZE, uint64(-(1 + value)) + 1, base);
    *--p = '-';
  } else {
    p = __ulltoa(buffer + BUFFER_SIZE, uint64(value), base);
  }

  Clear();
  Append(p, BUFFER_SIZE - (p - buffer));
  return *this;
}

ByteString& ByteString::SetNumber(uint64 value, int32 base) {
  const int32 BUFFER_SIZE = 66;
  char buffer[BUFFER_SIZE];
  char* p = __ulltoa(buffer + BUFFER_SIZE, value, base);

  Clear();
  Append(p, BUFFER_SIZE - (p - buffer));
  return *this;
}

ByteString& ByteString::SetNumber(float value, char f, int32 precision) {
  return SetNumber(double(value), f, precision);
}

ByteString& ByteString::SetNumber(double value, char f, int32 precision) {
  LocaleData::DoubleForm form = LocaleData::DFDecimal;
  uint32 flags = LocaleData::ZeroPadExponent;

  if (CharTraitsA::IsUpper(f)) {
    flags |= LocaleData::CapitalEorX;
  }
  f = CharTraitsA::ToLower(f);

  switch (f) {
    case 'f': form = LocaleData::DFDecimal; break;
    case 'e': form = LocaleData::DFExponent; break;
    case 'g': form = LocaleData::DFSignificantDigits; break;
    default:
      //TODO
      //fun_log(LogCore, Warning, "ByteString::SetNumber: Invalid format char '%c'", f);
      break;
  }

  *this = WCHAR_TO_UTF8(*LocaleData::C()->DoubleToString(value, precision, form, -1, flags));
  return *this;
}

namespace {

FUN_ALWAYS_INLINE bool IsPathSeparator(char ch) {
    return ch == '/' || ch == '\\';
}

} // namespace

ByteString& ByteString::PathAppend(ByteStringView str) {
  if (Len() > 0 && !IsPathSeparator(Last()) &&
      (str.IsEmpty() || !IsPathSeparator(*str.ConstData()))) {
    Append("/");
  }
  Append(str);
  return *this;
}

bool ByteString::IsPathTerminated() const {
  return Len() > 0 && IsPathSeparator(Last());
}

ByteString ByteString::ToPathTerminated() const {
  ByteString result(*this);
  if (Len() > 0 && !IsPathSeparator(Last())) {
    result.Append('/');
  }
  return result;
}

ByteString ByteString::ToBase64() const {
  return ToBase64(Base64Encoding);
}

ByteString ByteString::ToBase64(Base64Options options) const {
  const char ALPHABET_BASE64[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef" "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
  const char ALPHABET_BASE64_URL[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef" "ghijklmn" "opqrstuv" "wxyz0123" "456789-_";
  const char* const ALPHABET = options & Base64UrlEncoding ? ALPHABET_BASE64_URL : ALPHABET_BASE64;
  const char pad_char = '=';
  int32 pad_len = 0;

  ByteString tmp((Len() + 2) / 3 * 4, NoInit);

  int32 i = 0;
  char* dst = tmp.MutableData();
  while (i < Len()) {
    // encode 3 bytes at a time
    int32 chunk = 0;
    chunk |= int32(uint8(ConstData()[i++])) << 16;
    if (i == Len()) {
      pad_len = 2;
    } else {
      chunk |= int32(uint8(ConstData()[i++])) << 8;
      if (i == Len()) {
        pad_len = 1;
      } else {
        chunk |= int32(uint8(ConstData()[i++]));
      }
    }

    const int32 j = (chunk & 0x00FC0000) >> 18;
    const int32 k = (chunk & 0x0003F000) >> 12;
    const int32 l = (chunk & 0x00000FC0) >> 6;
    const int32 m = (chunk & 0x0000003F);
    *dst++ = ALPHABET[j];
    *dst++ = ALPHABET[k];

    if (pad_len > 1) {
      if ((options & OmitTrailingEquals) == 0) {
        *dst++ = pad_char;
      }
    } else {
      *dst++ = ALPHABET[l];
    }
    if (pad_len > 0) {
      if ((options & OmitTrailingEquals) == 0) {
        *dst++ = pad_char;
      }
    } else {
      *dst++ = ALPHABET[m];
    }
  }
  fun_check((options & OmitTrailingEquals) || (dst == tmp.Len() + tmp.ConstData()));
  if (options & OmitTrailingEquals) {
    tmp.Truncate(dst - tmp.ConstData());
  }
  return tmp;
}

ByteString ByteString::FromBase64(const ByteString& base64, Base64Options options) {
  ByteString tmp((base64.Len() * 3) / 4, NoInit);

  uint32 buf = 0;
  int32 nbits = 0;
  int32 offset = 0;
  for (int32 i = 0; i < base64.Len(); ++i) {
    const int32 ch = base64[i];

    int32 d;

    if (ch >= 'A' && ch <= 'Z') {
      d = ch - 'A';
    } else if (ch >= 'a' && ch <= 'z') {
      d = ch - 'a' + 26;
    } else if (ch >= '0' && ch <= '9') {
      d = ch - '0' + 52;
    } else if (ch == '+' && (options & Base64UrlEncoding) == 0) {
      d = 62;
    } else if (ch == '-' && (options & Base64UrlEncoding) != 0) {
      d = 62;
    } else if (ch == '/' && (options & Base64UrlEncoding) == 0) {
      d = 63;
    } else if (ch == '_' && (options & Base64UrlEncoding) != 0) {
      d = 63;
    } else {
      d = -1;
    }

    if (d != -1) {
      buf = (buf << 6) | d;
      nbits += 6;
      if (nbits >= 8) {
        nbits -= 8;
        tmp[offset++] = buf >> nbits;
        buf &= (1 << nbits) - 1;
      }
    }
  }

  tmp.Truncate(offset);
  return tmp;
}

ByteString ByteString::FromBase64(const ByteString& base64) {
  return FromBase64(base64, Base64Encoding);
}

ByteString ByteString::ToHex() const {
  return ToHex('\0');
}

ByteString ByteString::ToHex(char separator) const {
  if (Len() == 0) {
    return ByteString();
  }

  const int32 len = separator ? (Len() * 3 - 1) : (Len() * 2);
  ByteString hex(len, NoInit);
  char* hex_data = hex.MutableData();
  const uint8* data = (const uint8*)this->ConstData();
  for (int32 i = 0, out_pos = 0; i < Len(); ++i) {
    hex_data[out_pos++] = CharTraitsA::NibbleToHexChar(data[i] >> 4);
    hex_data[out_pos++] = CharTraitsA::NibbleToHexChar(data[i] & 0xF);

    if (separator && out_pos < len) {
      hex_data[out_pos++] = separator;
    }
  }
  return hex;
}

ByteString ByteString::FromHex(const ByteString& hex_encoded) {
  ByteString ret((hex_encoded.Len() + 1) / 2, NoInit);
  uint8* result = (uint8*)ret.ConstData() + ret.Len();

  bool odd_digit = true;
  for (int32 i = hex_encoded.Len() - 1; i >= 0; --i) {
    const uint8 ch = uint8(hex_encoded[i]);
    const int32 nibble = CharTraitsA::HexCharToNibble(ch, -1);
    if (nibble == -1) {
      continue;
    }
    if (odd_digit) {
      --result;
      *result = nibble;
      odd_digit = false;
    } else {
      *result |= nibble << 4;
      odd_digit = true;
    }
  }

  ret.Remove(0, result - (const uint8*)ret.ConstData());
  return ret;
}

namespace {

void FromPercentEncoding_helper(ByteString* str, char percent) {
  if (str->IsEmpty()) {
    return;
  }

  char* data = str->MutableData();
  const char* input_ptr = data;

  int32 i = 0;
  int32 len = str->Len();
  int32 out_len = 0;
  int32 a, b;
  char ch;
  while (i < len) {
    ch = input_ptr[i];

    if (ch == percent && i + 2 < len) {
      a = CharTraitsA::HexCharToNibble(input_ptr[++i], 0);
      b = CharTraitsA::HexCharToNibble(input_ptr[++i], 0);

      *data++ = (char)((a << 4) | b);
    } else {
      *data++ = ch;
    }

    ++i;
    ++out_len;
  }

  if (out_len != len) {
    str->Truncate(out_len);
  }
}

} // namespace

ByteString ByteString::FromPercentEncoding(const ByteString& pct_encoded, char percent) {
  if (pct_encoded.IsEmpty()) {
    return ByteString();
  }

  ByteString tmp(pct_encoded);
  FromPercentEncoding_helper(&tmp, percent);
  return tmp;
}

namespace {

void ToPercentEncoding_helper(ByteString* str,
                              const char* dont_encode,
                              const char* also_encode,
                              char percent) {
  if (str->IsEmpty()) {
    return;
  }

  ByteString input = *str;
  int32 len = input.Len();
  const char* input_data = input.ConstData();
  char* dst = nullptr;
  int32 out_pos = 0;

  for (int32 i = 0; i < len; ++i) {
    const uint8 c = *input_data++;

    if (((c >= 0x61 && c <= 0x7A)     // ALPHA
         || (c >= 0x41 && c <= 0x5A)  // ALPHA
         || (c >= 0x30 && c <= 0x39)  // DIGIT
         || c == 0x2D         // -
         || c == 0x2E         // .
         || c == 0x5F         // _
         || c == 0x7E         // ~
         || CStringTraitsA::Strchr(dont_encode, c)) // exclude
        && !CStringTraitsA::Strchr(also_encode, c)) // include
    {
      if (dst) {
        dst[out_pos] = c;
      }
      ++out_pos;
    } else {
      if (dst == nullptr) {
        // detach now
        str->ResizeUninitialized(len * 3); // worst case
        dst = str->MutableData();
      }
      dst[out_pos++] = percent;
      dst[out_pos++] = CharTraitsA::HexCharToNibble((c & 0xf0) >> 4);
      dst[out_pos++] = CharTraitsA::HexCharToNibble((c & 0x0f));
    }
  }

  if (dst) {
    str->Truncate(out_pos);
  }
}

} // namespace

ByteString ByteString::ToPercentEncoding( const ByteString& exclude,
                                          const ByteString& include,
                                          char percent) const {
  if (IsEmpty()) {
    return *this;
  }

  ByteString include2 = include;
  if (percent != '%') {                           // the default
    if ((percent >= 0x61 && percent <= 0x7A)      // ALPHA
        || (percent >= 0x41 && percent <= 0x5A)   // ALPHA
        || (percent >= 0x30 && percent <= 0x39)   // DIGIT
        || percent == 0x2D                        // -
        || percent == 0x2E                        // .
        || percent == 0x5F                        // _
        || percent == 0x7E)                       // ~
    {
      include2 += percent;
    }
  }

  ByteString result = *this;
  ToPercentEncoding_helper(&result, *exclude.ToNulTerminated(), *include2.ToNulTerminated(), percent);
  return result;
}

ByteString ByteString::ToHtmlEscaped() const {
  ByteString escaped;
  escaped.Reserve(int32(Len() * 1.1)); // 110%
  for (int32 i = 0; i < Len(); ++i) {
    const char ch = ConstData()[i];

    if (ch == '<') {
      escaped.Append("&lt;", 4);
    } else if (ch == '>') {
      escaped.Append("&gt;", 4);
    } else if (ch == '&') {
      escaped.Append("&amp;", 5);
    } else if (ch == '"') {
      escaped.Append("&quot;", 6);
    } else {
      escaped.Append(ch);
    }
  }
  escaped.Shrink();

  return escaped;
}

bool ByteString::IsNumeric() const {
  return !IsEmpty() && CStringTraitsA::IsNumeric(ConstData(), ConstData() + Len());
}

bool ByteString::IsIdentifier() const {
  if (IsEmpty()) {
    return false;
  }

  const char* p = ConstData();

  // leading
  if (!(CharTraitsA::IsAlpha(p[0]) || p[0] == '_')) {
    return false;
  }

  for (int32 i = 1; i < Len(); ++i) {
    if (!(CharTraitsA::IsAlnum(p[i]) || p[i] == '_')) {
      return false;
    }
  }

  return true;
}

bool ByteString::IsQuoted() const {
  return (Len() >= 2 && First() == '"' && Last() == '"');
}

bool ByteString::IsQuoted(ByteStringView str) {
  return (str.Len() >= 2 && str.First() == '"' && str.Last() == '"');
}

ByteString ByteString::Unquoted(bool* out_quotes_removed) const {
  if (IsQuoted()) {
    if (out_quotes_removed) {
      *out_quotes_removed = true;
    }
    return Mid(1, Len() - 2);
  } else {
    if (out_quotes_removed) {
      *out_quotes_removed = false;
    }
    return *this;
  }
}

ByteString& ByteString::Unquotes(bool* out_quotes_removed) {
  return (*this = Unquoted(out_quotes_removed));
}

ByteStringRef ByteString::UnquotedRef(bool* out_quotes_removed) const {
  if (IsQuoted()) {
    if (out_quotes_removed) *out_quotes_removed = true;
    return MidRef(1, Len() - 2);
  } else {
    if (out_quotes_removed) *out_quotes_removed = false;
    return ByteStringRef();
  }
}

ByteString ByteString::ReplaceQuotesWithEscapedQuotes() const {
  if (Contains("\"")) {
    ByteString result;

    const char* p = cbegin();
    const char* e = cend();
    bool is_escaped = false;
    for (; p != e; ++p) {
      if (is_escaped) {
        is_escaped = false;
      } else if (*p == '\\') {
        is_escaped = true;
      } else if (*p == '"') {
        result.Append('\\');
      }

      result.Append(*p);
    }
    return result;
  }
  return *this;
}

static const char* CHAR_TO_ESCAPE_SEQ_MAP[][2] = {
  // Always replace \\ first to avoid double-escaping characters
  { "\\", "\\\\" },
  { "\n", "\\n"  },
  { "\r", "\\r"  },
  { "\t", "\\t"  },
  { "\'", "\\'"  },
  { "\"", "\\\"" }
};

static const uint32 MAX_SUPPORTED_ESCAPE_CHARS = countof(CHAR_TO_ESCAPE_SEQ_MAP);

ByteString ByteString::ReplaceCharWithEscapedChar(const Array<char>* chars) const {
  if (Len() > 0 && (chars == nullptr || chars->Count() > 0)) {
    ByteString result(*this);
    for (int32 char_idx = 0; char_idx < MAX_SUPPORTED_ESCAPE_CHARS; ++char_idx) {
      if (chars == nullptr || chars->Contains(*(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]))) {
        result.Replace(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0], CHAR_TO_ESCAPE_SEQ_MAP[char_idx][1]);
      }
    }
    return result;
  }
  return *this;
}

ByteString ByteString::ReplaceEscapedCharWithChar(const Array<char>* chars) const {
  if (Len() > 0 && (chars == nullptr || chars->Count() > 0)) {
    ByteString result(*this);
    for (int32 char_idx = MAX_SUPPORTED_ESCAPE_CHARS-1; char_idx >= 0; --char_idx) {
      if (chars == nullptr || chars->Contains(*(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]))) {
        result.Replace(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][1], CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]);
      }
    }
    return result;
  }
  return *this;
}

ByteString ByteString::ConvertTabsToSpaces(int32 spaces_per_tab) const {
  fun_check(spaces_per_tab > 0);

  ByteString result(*this);
  int32 tab_pos;
  while ((tab_pos = result.IndexOf("\t")) != INVALID_INDEX) {
    ByteString left = result.Left(tab_pos);
    ByteString right = result.Mid(tab_pos + 1);

    result = left;
    int32 line_begin = left.LastIndexOf("\n");
    if (line_begin == INVALID_INDEX) {
      line_begin = 0;
    }
    const int32 characters_on_line = (left.Len() - line_begin);
    const int32 num_spaces_for_tab = spaces_per_tab - (characters_on_line % spaces_per_tab);
    result.Append(num_spaces_for_tab, ' ');
    result.Append(right);
  }
  return result;
}

//TODO Raw를 단순히 읽기 전용 데이터로만 하지말고, 버퍼로 사용할수도 있지 않을까?

/*
ByteString ByteString::FromExternalWritableData(char* ExternalData, int32 len) {
  //공유는 안되어야 하고, 단지 쓰기 용도로 활용하기 위함임...???
  //기존 코드와 대조를 통해서 확인을 재차 해봐야할듯...
}
*/

ByteString ByteString::FromRawData(const char* raw, int32 len) {
  fun_check(len >= 0);

  if (len < 0) {
    len = 0;
  }

  Data* new_data;
  if (raw == nullptr || len == 0) {
    new_data = Data::SharedEmpty();
  } else {
    new_data = Data::FromRawData(raw, len);
  }

  ByteStringDataPtr data_ptr{ new_data };
  return ByteString(data_ptr); //without ref increament
}

ByteString& ByteString::SetRawData(const char* raw, int32 len) {
  // 공유중이거나, 할당된 내용이 있다면 바로 처리안됨.
  if (data_->ref.IsShared() || data_->alloc > 0) {
    *this = FromRawData(raw, len);
  } else {
    if (raw) {
      if (len < 0) {
        len = 0;
      }

      data_->length = len;
      data_->offset = raw - reinterpret_cast<char*>(data_);
    } else {
      data_->offset = sizeof(UntypedSharableArrayData);
      data_->length = 0;
      data_->MutableData()[0] = '\0'; // nul-terminated
    }
  }
  return *this;
}


//TODO inline으로 옮겨주는게 좋을듯...

ByteString ByteString::FromNumber(int16 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(uint16 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(int32 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(uint32 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(int64 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(uint64 value, int32 base) {
  return ByteString().SetNumber(value, base);
}

ByteString ByteString::FromNumber(float value, char f, int32 precision) {
  return ByteString().SetNumber(value, f, precision);
}

ByteString ByteString::FromNumber(double value, char f, int32 precision) {
  return ByteString().SetNumber(value, f, precision);
}

std::string ByteString::ToStdString() const {
  return std::string(ConstData(), Len()); // 길이를 가지고 가므로, nul-term 일 필요는 없음.
}

ByteString ByteString::FromStdString(const std::string& std_string) {
  return ByteString(std_string.data(), int32(std_string.size())); // 길이를 가지고 가므로, 중간에 nul 문자가 있어도 문제 없음.
}

uint32 HashOf(const ByteString& str) {
  return Crc::Crc32(str.ConstData(), str.Len());
}

Archive& operator & (Archive& ar, ByteString& str) {
  if (ar.IsLoading()) {
    int32 len;
    ar & len;
    str.ResizeUninitialized(len);
    ar.Serialize(str.MutableData(), len);
  } else {
    int32 len = str.Len();
    ar & len;
    //ar.Serialize(str.MutableData(), len);
    ar.Serialize(const_cast<char*>(str.ConstData()), len);
  }

  return ar;
}


//
// ByteStringRef
//

void ByteStringRef::CheckInvariants() {
#ifdef DO_CHECK
  fun_check(string_ == nullptr || position_ >= 0);
  fun_check(string_ == nullptr || position_ < string_->Len());
  fun_check(string_ == nullptr || length_ >= 0);
  fun_check(string_ == nullptr || (position_ + length_) <= string_->Len());
#endif
}

ByteStringRef::ByteStringRef()
  : string_(nullptr), position_(0), length_(0) {}

ByteStringRef::ByteStringRef(const ByteString* str, int32 pos, int32 len)
  : string_(str), position_(pos), length_(len) {
  CheckInvariants();
}

ByteStringRef::ByteStringRef(const ByteString* str)
  : string_(str), position_(0), length_(str ? str->Len() : 0) {
  CheckInvariants();
}

ByteStringRef::ByteStringRef(const ByteStringRef& rhs)
  : string_(rhs.string_), position_(rhs.position_), length_(rhs.length_) {
  CheckInvariants();
}

ByteStringRef& ByteStringRef::operator = (const ByteStringRef& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    string_ = rhs.string_;
    position_ = rhs.position_;
    length_ = rhs.length_;
    CheckInvariants();
  }
  return *this;
}

ByteStringRef::ByteStringRef(ByteStringRef&& rhs)
  : string_(rhs.string_), position_(rhs.position_), length_(rhs.length_) {
  rhs.string_ = nullptr;
  rhs.position_ = 0;
  rhs.length_ = 0;
  CheckInvariants();
}

ByteStringRef& ByteStringRef::operator = (ByteStringRef&& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    string_ = rhs.string_;
    position_ = rhs.position_;
    length_ = rhs.length_;

    rhs.string_ = nullptr;
    rhs.position_ = 0;
    rhs.length_ = 0;

    CheckInvariants();
  }
  return *this;
}

ByteStringRef& ByteStringRef::operator = (const ByteString* str) {
  string_ = str;
  position_ = 0;
  length_ = str ? str->Len() : 0;
  CheckInvariants();
  return *this;
}

void ByteStringRef::Swap(ByteStringRef& rhs) {
  fun::Swap(string_, rhs.string_);
  fun::Swap(position_, rhs.position_);
  fun::Swap(length_, rhs.length_);
}

const ByteString* ByteStringRef::Str() const {
  return string_;
}

int32 ByteStringRef::Position() const {
  return position_;
}

int32 ByteStringRef::Len() const {
  return length_;
}

bool ByteStringRef::IsEmpty() const {
  return length_ == 0;
}

void ByteStringRef::Clear() {
  string_ = nullptr;
  position_ = 0;
  length_ = 0;
}

bool ByteStringRef::IsNull() const {
  return string_ == nullptr;
}

const char* ByteStringRef::operator * () const {
  return ConstData();
}

const char* ByteStringRef::ConstData() const {
  return string_->ConstData() + position_;
}

ByteString ByteStringRef::ToString() const {
  return string_ ? ByteString(string_->ConstData() + position_, length_) : ByteString();
}

ByteStringRef ByteStringRef::AppendTo(ByteString* str) const {
  if (str == nullptr) {
    return ByteStringRef();
  }

  const int32 pos = str->Len();
  str->Insert(pos, ConstData(), Len());
  return ByteStringRef(str, pos, Len());
}

char ByteStringRef::At(int32 index) const {
  fun_check(string_);
  fun_check(uint32(index) < uint32(Len()));
  return string_->At(position_ + index);
}

char ByteStringRef::operator[] (int32 index) const {
  fun_check(string_);
  fun_check(uint32(index) < uint32(Len()));
  return string_->At(index);
}

char ByteStringRef::First() const {
  return At(0);
}

char ByteStringRef::Last() const {
  return At(Len() - 1);
}

char ByteStringRef::FirstOr(const char def) const {
  return Len() ? First() : def;
}

char ByteStringRef::LastOr(const char def) const {
  return Len() ? Last() : def;
}

int32 ByteStringRef::IndexOf( ByteStringView sub,
                              CaseSensitivity casesense,
                              int32 from,
                              int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.IsEmpty()) {
    return from;
  }

  if (sub.Len() == 1) { // 한글자 찾기로 전환.. 이게 좀더 빠름...
    return ByteStringMatcher::FastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }

  const int32 this_len = Len();
  if (from > this_len || (sub.Len() + from) > this_len) {
    return INVALID_INDEX;
  }

  return ByteStringMatcher::FastFind(ConstData(), this_len, from, sub.ConstData(), sub.Len(), casesense);
}

int32 ByteStringRef::LastIndexOf( ByteStringView sub,
                                  CaseSensitivity casesense,
                                  int32 from,
                                  int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.Len() == 1) {
    return ByteStringMatcher::FastLastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }
  return ByteStringMatcher::FastLastFind(ConstData(), Len(), from, sub.ConstData(), sub.Len(), casesense);
}

int32 ByteStringRef::IndexOfAny(const Array<const char*>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<ByteString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<ByteStringRef>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<ByteStringView>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<AsciiString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }

int32 ByteStringRef::IndexOfAny(std::initializer_list<const char*> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<ByteString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<ByteStringRef> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<ByteStringView> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<AsciiString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 ByteStringRef::IndexOfAny(const Array<const UNICHAR*>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<UString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<UStringRef>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(const Array<UStringView>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }

int32 ByteStringRef::IndexOfAny(std::initializer_list<const UNICHAR*> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<UString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<UStringRef> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 ByteStringRef::IndexOfAny(std::initializer_list<UStringView> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
#endif

int32 ByteStringRef::Count(ByteStringView sub, CaseSensitivity casesense) const {
  int32 n = 0, pos = -int32(sub.Len());
  if (Len() > 500 && sub.Len() > 5) {
    ByteStringMatcher matcher(sub, casesense);
    while ((pos = matcher.IndexIn(*this, pos + sub.Len())) != INVALID_INDEX) {
      ++n;
    }
  } else {
    while ((pos = IndexOf(sub, casesense, pos + sub.Len())) != INVALID_INDEX) {
      ++n;
    }
  }
  return n;
}

ByteStringRef ByteStringRef::Left(int32 len) const {
  fun_check(len >= 0);
  fun_check(len <= Len());
  return uint32(len) < uint32(length_) ? ByteStringRef(string_, position_, len) : *this;
}

ByteStringRef ByteStringRef::Mid(int32 offset, int32 len) const {
  switch (SlicingHelper::Slice(length_, &offset, &len)) {
    case SlicingHelper::Null:
      return ByteStringRef();
    case SlicingHelper::Empty:
      return ByteStringRef(string_, 0, 0);
    case SlicingHelper::Full:
      return *this;
    case SlicingHelper::Subset:
      return ByteStringRef(string_, offset + position_, len);
  }
  //unreachable to here
  return ByteStringRef();
}

ByteStringRef ByteStringRef::Right(int32 len) const {
  fun_check(len >= 0);
  fun_check(len <= Len());
  return uint32(len) < uint32(length_) ? ByteStringRef(string_, position_ + length_ - len, len) : *this;
}

ByteStringRef ByteStringRef::LeftChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Left(Len() - len);
}

ByteStringRef ByteStringRef::RightChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Right(Len() - len);
}

bool ByteStringRef::StartsWith(ByteStringView sub, CaseSensitivity casesense) const {
  if (sub.IsEmpty()) {
    return true;
  }

  if (sub.Len() > Len()) {
    return false;
  }

  return StringCmp::Compare(ConstData(), sub.ConstData(), sub.Len(), casesense) == 0;
}

bool ByteStringRef::EndsWith(ByteStringView sub, CaseSensitivity casesense) const {
  if (sub.IsEmpty()) {
    return true;
  }

  if (sub.Len() > Len()) {
    return false;
  }

  return StringCmp::Compare(ConstData() + Len() - sub.Len(), sub.ConstData(), sub.Len(), casesense) == 0;
}

bool ByteStringRef::GlobMatch(ByteStringView pattern, CaseSensitivity casesense) const {
  //TODO
  fun_check(0);
  return false;

  //return GlobA::Match(cbegin(), cend(), pattern.cbegin(), pattern.cend(), casesense);
}

ByteStringRef& ByteStringRef::Truncate(int32 pos) {
  fun_check(pos >= 0);

  if (pos < length_) {
    length_ = pos;
  }
  return *this;
}

ByteStringRef& ByteStringRef::LeftChop(int32 len) {
  fun_check(len >= 0);

  if (len > 0) {
    if (len > Len()) {
      Clear();
    } else {
      length_ -= len;
    }
  }
  return *this;
}

ByteStringRef& ByteStringRef::RightChop(int32 len) {
  fun_check(len >= 0);

  if (len > 0) {
    if (len > Len()) {
      Clear();
    } else {
      length_ -= len;
      position_ += len;
    }
  }
  return *this;
}

ByteStringRef& ByteStringRef::TrimLeft() {
  if (string_) {
    const char* abs_begin = string_->ConstData();
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    length_ = StringAlgo<ByteString>::LeftTrimmedPositions(local_begin, local_end);
    position_ = local_begin - abs_begin;
  }
  return *this;
}

ByteStringRef& ByteStringRef::TrimRight() {
  if (string_) {
    const char* abs_begin = string_->ConstData();
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    length_ = StringAlgo<ByteString>::RightTrimmedPositions(local_begin, local_end); // 오른쪽 trimming이므로, 길이만 변경됨.
  }
  return *this;
}

ByteStringRef& ByteStringRef::Trim() {
  if (string_) {
    const char* abs_begin = string_->ConstData();
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    length_ = StringAlgo<ByteString>::TrimmedPositions(local_begin, local_end);
    position_ = local_begin - abs_begin;
  }
  return *this;
}

ByteStringRef ByteStringRef::TrimmedLeft() const {
  if (string_) {
    const char* abs_begin = string_->ConstData();
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    StringAlgo<ByteString>::LeftTrimmedPositions(local_begin, local_end);
    return ByteStringRef(string_, local_begin - abs_begin, local_end - local_begin);
  }
  return *this;
}

ByteStringRef ByteStringRef::TrimmedRight() const {
  if (string_) {
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    StringAlgo<ByteString>::RightTrimmedPositions(local_begin, local_end);
    return ByteStringRef(string_, position_, local_end - local_begin); // 오른쪽 trimming이므로, 길이만 변경됨.
  }
  return *this;
}

ByteStringRef ByteStringRef::Trimmed() const {
  if (string_) {
    const char* abs_begin = string_->ConstData();
    const char* local_begin = ConstData();
    const char* local_end = ConstData() + length_;
    StringAlgo<ByteString>::TrimmedPositions(local_begin, local_end);
    return ByteStringRef(string_, local_begin - abs_begin, local_end - local_begin);
  }
  return *this;
}

int32 ByteStringRef::LeftSpaces() const {
  return StringAlgo<ByteStringRef>::LeftSpaces(cbegin(), cend());
}

int32 ByteStringRef::RightSpaces() const {
  return StringAlgo<ByteStringRef>::RightSpaces(cbegin(), cend());
}

int32 ByteStringRef::SideSpaces() const {
  return StringAlgo<ByteStringRef>::SideSpaces(cbegin(), cend());
}

Array<ByteStringRef> ByteStringRef::Split(char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::SplitByWhitespaces(ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitByWhitespaces(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::SplitLines(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; SplitLines(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<ByteStringRef> ByteStringRef::Split(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<ByteStringRef> ByteStringRef::Split(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<ByteStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 ByteStringRef::Split(Array<ByteStringRef>& list, char separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, ByteStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<char>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, ByteStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<char> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, ByteStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, separators, max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, ByteStringView(separators.ConstData(),separators.Len()), max_splits, split_options, casesense); }

int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,const char*>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteString>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteStringRef>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,AsciiString>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,const char*>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteString>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteStringRef>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,ByteStringView>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,AsciiString>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 ByteStringRef::SplitByWhitespaces(Array<ByteStringRef>& list, ByteString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView("\n") };
    return Split(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { ByteStringView(" "), ByteStringView("\t"), ByteStringView("\r"), ByteStringView(extra_separator) };
    return Split(list, separators, max_splits, split_options, casesense);
  }
}

int32 ByteStringRef::SplitLines(Array<ByteStringRef>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { ByteStringView("\r\n"), ByteStringView("\n"), ByteStringView("\r") };
  return Split(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 ByteStringRef::Split(Array<ByteStringRef>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }

int32 ByteStringRef::Split(Array<ByteStringRef>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, separators, max_splits, split_options, casesense); }

int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,const UNICHAR*>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UString>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UStringRef>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,const UNICHAR*>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UString>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UStringRef>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 ByteStringRef::Split(Array<ByteStringRef>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<ByteStringRef,ByteStringRef,UStringView>(list, *this, &ByteStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
#endif

bool ByteStringRef::Divide(ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = IndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = Left(pos); out_left->Trim(); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) { *out_left = Left(pos); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); }
  }
  return true;
}

bool ByteStringRef::LastDivide(ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = LastIndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = Left(pos); out_left->Trim(); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) { *out_left = Left(pos); }
    if (out_right) { *out_right = Mid(pos + delim.Len()); }
  }
  return true;
}

int16 ByteStringRef::ToInt16(bool* ok, int32 base) const {
  return ToIntegral<int16>(ConstData(), Len(), ok, base);
}

uint16 ByteStringRef::ToUInt16(bool* ok, int32 base) const {
  return ToIntegral<uint16>(ConstData(), Len(), ok, base);
}

int32 ByteStringRef::ToInt32(bool* ok, int32 base) const {
  return ToIntegral<int32>(ConstData(), Len(), ok, base);
}

uint32 ByteStringRef::ToUInt32(bool* ok, int32 base) const {
  return ToIntegral<uint32>(ConstData(), Len(), ok, base);
}

int64 ByteStringRef::ToInt64(bool* ok, int32 base) const {
  return ToIntegral<int64>(ConstData(), Len(), ok, base);
}

uint64 ByteStringRef::ToUInt64(bool* ok, int32 base) const {
  return ToIntegral<uint64>(ConstData(), Len(), ok, base);
}

float ByteStringRef::ToFloat(bool* ok) const {
  return LocaleData::ConvertDoubleToFloat(ToDouble(ok), ok);
}

double ByteStringRef::ToDouble(bool* ok) const {
  bool non_null_ok = false;
  int32 processed;
  const double dbl = AsciiToDouble(ConstData(), Len(), non_null_ok, processed);
  if (ok) {
    *ok = non_null_ok;
  }
  return dbl;
}

bool ByteStringRef::IsNumeric() const {
  return !IsEmpty() && CStringTraitsA::IsNumeric(ConstData(), ConstData() + Len());
}

bool ByteStringRef::IsIdentifier() const {
  if (IsEmpty()) {
    return false;
  }

  const char* p = ConstData();

  if (!(CharTraitsA::IsAlpha(p[0]) || p[0] == '_')) { // leading alpha or '_'
    return false;
  }

  for (int32 i = 1; i < Len(); ++i) {
    if (!(CharTraitsA::IsAlnum(p[i]) || p[i] == '_')) {
      return false;
    }
  }

  return true;
}

bool ByteStringRef::IsQuoted() const {
  return ByteString::IsQuoted(*this);
}

ByteStringRef ByteStringRef::Unquoted(bool* out_quotes_removed) const {
  if (IsQuoted()) {
    if (out_quotes_removed) {
      *out_quotes_removed = true;
    }
    return Mid(1, Len() - 2);
  } else {
    if (out_quotes_removed) {
      *out_quotes_removed = false;
    }
    return *this;
  }
}

ByteStringRef& ByteStringRef::Unquotes(bool* out_quotes_removed) {
  return (*this = Unquoted(out_quotes_removed));
}

uint32 HashOf(const ByteStringRef& str) {
  return Crc::Crc32(str.ConstData(), str.Len());
}

int32 CullArray(Array<ByteString>* array, bool trimming) {
  fun_check_ptr(array);

  if (trimming) {
    for (int32 i = 0; i < array->Count(); ++i) {
      (*array)[i].Trim();
    }
  }

  ByteString empty;
  array->Remove(empty);
  return array->Count();
}

int32 CullArray(Array<ByteStringRef>* array, bool trimming) {
  fun_check_ptr(array);

  if (trimming) {
    for (int32 i = 0; i < array->Count(); ++i) {
      (*array)[i].Trim();
    }
  }

  ByteStringRef empty;
  array->Remove(empty);
  return array->Count();
}

// Format

//TODO
/*
// This starting size catches 99.97% of printf calls - there are about 700k printf calls per level
#define STARTING_BUFFER_SIZE  512

VARARG_BODY(ByteString, ByteString::Format, const char*, VARARG_NONE)
{
  int32 buffer_size = STARTING_BUFFER_SIZE;
  char starting_buffer[STARTING_BUFFER_SIZE];
  char* buffer = starting_buffer;
  int32 result = -1;

  // First try to print to a stack allocated location
  GET_VARARGS_RESULT_ANSI(buffer, buffer_size, buffer_size - 1, fmt, fmt, result);

  // If that fails, start allocating regular memory
  if (result == -1) {
    buffer = nullptr;
    while (result == -1) {
      buffer_size *= 2;
      buffer = (char*)UnsafeMemory::Realloc(buffer, buffer_size * sizeof(char));
      GET_VARARGS_RESULT_ANSI(buffer, buffer_size, buffer_size - 1, fmt, fmt, result);
    };
  }

  ByteString result_string(buffer, result);

  if (buffer_size != STARTING_BUFFER_SIZE) {
    UnsafeMemory::Free(buffer);
  }

  return result_string;
}
*/

// Debugging helper

void ByteString::BytesToDebuggableString( ByteString& output,
                                          const ByteString& data,
                                          bool ascii_only,
                                          const ByteString& indent) {
  output.Reserve(output.Len() + data.Len()*4); //TODO optimize...

  const int32 width = ascii_only ? (80-2) : 16;

  for (int32 i = 0; i < data.Len(); i += width) {
    output << indent << ByteString::Format("%4.4lX : ", i); //TODO fix format specifier.

    if (!ascii_only) {
      for (int32 c = 0; c < width; ++c) {
        if ((i+c) < data.Len()) {
          const uint8 byte = data[i+c];
          output << CharTraitsA::HexCharToNibble(byte >> 4);
          output << CharTraitsA::HexCharToNibble(byte & 0xF);
          output << " ";
        } else {
          output << "   ";
        }

        if (c > 0 && c % 8 == 7) {
          output << " ";
        }
      }
    }

    output << " "; // spacer between hex and ascii view.

    for (int32 c = 0; c < width && (i+c) < data.Len(); ++c) {
      // check for 0D0A; if found, skip past and start a new line of output.
      if (ascii_only && (i+c+1) < data.Len() && data[i+c] == 0x0D && data[i+c+1] == 0x0A) {
        i += (c+2-width);
        break;
      }

      const char ascii_char = (data[i+c] >= 0x20) && (data[i+c] < 0x80) ? data[i+c] : '.';
      output << ascii_char;

      if (!ascii_only) {
        if (c > 0 && c % 8 == 7) {
          output << " ";
        }
      }

      // check again for 0D0A, to avoid an extra \n if it's at width.
      if (ascii_only && (i+c+2) < data.Len() && data[i+c+1] == 0x0D && data[i+c+2] == 0x0A) {
        i += (c+3-width);
        break;
      }
    }

    output << "\n"; // nextline
  }
}

} // namespace fun
