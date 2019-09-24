#include "fun/base/locale/locale_private.h"
#include "fun/base/locale/locale_tool.h"
#include "fun/base/container/slicing_helper.h"
#include "fun/base/string/uni_string_matcher.h"
#include "fun/base/string/string_algo.h"
#include "fun/base/string/utf.h"
#include "fun/base/string/string_conversion.h"

//TODO
//#include "fun/base/string/regex.h"

#include "fun/base/string/uni_string_view.h"

#include "fun/base/string/string_internal.h"
#include "fun/base/string/string_split.h"
#include "fun/base/string/string_join.h"

#include <cassert>

//TODO truncation 경고가 너무 많아서 임시로 꺼줌. 코드 정리 후 명시적으로 처리하는 코드를 준다던지 하는게 좋을듯...
#ifdef _MSC_VER
#pragma warning(disable : 4244) // truncation warning (size_t -> fun::int32)
#pragma warning(disable : 4267) // truncation warning (size_t -> fun::int32)
#endif

namespace fun {

/*! \class AsciiString
  \brief 문자열 변환시 사용함.

  UString에서 const char* 타입의 C 문자열을 넘기게 되면, UTF8로 자동 변환되는데,
  이때 변환없이 ASCII 문자열을 그대로 넘기기 위해서 사용함.

  궁극적으로는 이건 제거하는게 바람직해보임...
*/

//TODO 범용으로 빼주어도 좋을듯... (StringCast**)

template <size_t DEFAULT_INLINE_BUFFER_LENGTH = 500>
class ASCII_TO_UNICHAR
  : private InlineAllocator<DEFAULT_INLINE_BUFFER_LENGTH>::template ForElementType<UNICHAR> {
 public:
  static constexpr UNICHAR BOGUS_CHAR = '?';

  using AllocatorType = typename InlineAllocator<DEFAULT_INLINE_BUFFER_LENGTH>::template ForElementType<UNICHAR>;

  ASCII_TO_UNICHAR(const char* ascii, int32 ascii_len) {
    if (ascii_len < 0) {
      ascii_len = CStringTraitsA::Strlen(ascii_len);
    }

    Init(ascii, ascii_len);
  }

  ASCII_TO_UNICHAR(AsciiString ascii) {
    Init(ascii.ConstData(), ascii.Len());
  }

  const UNICHAR* ConstData() const {
    return converted_;
  }

  int32 Len() const {
    return length_;
  }

  UStringView ToUStringView() const {
    return UStringView(converted_, length_);
  }

 private:
  UNICHAR* converted_;
  int32 length_;

  void Init(const char* ascii, int32 ascii_len) {
    length_ = ascii_len;

    AllocatorType::ResizeAllocation(0, length_ + 1, sizeof(UNICHAR));
    converted_ = (UNICHAR*)AllocatorType::GetAllocation();
    const uint8* p = (const uint8*)ascii;
    const uint8* e = (const uint8*)ascii + ascii_len;
    UNICHAR* d = converted_;
    for (; p != e; ++p) {
      *d++ = *p < 128 ? *p : BOGUS_CHAR;
    }
    converted_[length_] = '\0';
  }
};

bool AsciiString::StartsWith(UNICHAR ch, CaseSensitivity casesense) const {
  return StartsWith(UStringView(&ch, 1), casesense);
}

bool AsciiString::StartsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::StartsWith(ConstData(), sub.Len(), sub.ConstData(), sub.Len(), casesense);
}

bool AsciiString::EndsWith(UNICHAR ch, CaseSensitivity casesense) const {
  return EndsWith(UStringView(&ch, 1), casesense);
}

bool AsciiString::EndsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::EndsWith(ConstData(), sub.Len(), sub.ConstData(), sub.Len(), casesense);
}

uint32 HashOf(const AsciiString& str) {
  return Crc::StringCrc32(str.ConstData(), str.Len());
}


/*! \class UString
  \brief 유니코드 기반 문자열 클래스입니다.

  \tableofconents

  \section string_initialization 초기화

  초기화하는 방법은 다음과 같은것들이 있습니다.

  기본 생성자인 UString()으로 초기화 할 경우에는 빈 문자열 객체가 됩니다.
  다음과 같은 특성을 지닙니다.
  \code
  UString str;
  fun_check(str.IsEmpty());
  fun_check(str.Len() == 0);
  \endcode

  유니코드 문자열로 초기화 ()유니코드 문자열은 '\0'로 끝나거나, 길이를 지정해야합니다.)
  \code
  UString str1(u"Hello world");
  UString str2(u"Hello world", 11);
  \endcode


  \section string_manipulation 데이터 다루기

  데이터 변경을 위해서 기본적으로 Append(), Prepend(), Insert(), Replace(), FindAndRemove()
  그리고 Remove() 함수들을 지원합니다.

  예:
  \code
  UString str = "and";
  str.Prepend("rock ");   // str == "rock and"
  str.Append(" roll");    // str == "rock and roll"
  str.Replace(5, 3, "&"); // str == "rock & roll"
  \endcode

  만약, UString에 추가해야할 글자 갯수를 이미 알고 있다면, Reserve() 함수를 호출해서 미리 메모리를 확보하십시오.
  메모리 할당 횟수를 감소시켜 성능 향상을 꾀할수 있습니다.
  할당된 메모리양은 Capacity() 함수를 통해서 알아볼 수 있습니다.

  \code
  UString str;
  str.Reserve(5);      // 5글자를 담을 공간 미리 확보
  str.Append("12345"); // 추가적인 메모리 할당 없이 바로 추가
  \endcode

  위의 예에서처럼 하나만 추가할 경우에는 차이가 없지만, 많은 양을 여러번에 걸쳐서 지속적으로 추가할
  경우에는 메모리를 미리 할당해서, 재할당 횟수를 줄이는게 효율적입니다. 메모리 재할당은 비용이 매우 큽니다.


  또한 아래와 같이 초기화와 동시에 메모리를 확보할 수도 있습니다.
  \code
  UString str(5, ReservationInit);
  str.Append("12345");
  \endcode

  Replace() 와 Remove() 함수는 위치와 길이를 지정해서 해당하는 부분을 다른 내용으로 대체하거나, 제거할 수 있습니다.
  Replace()는 첫번째 인자로 찾아서 변경할 문자열을 지정할 수도 있습니다.


  문자열내의 공백을 제거하는 기능 또한 많이 사용될 수 있습니다. 이때에는 Trimmed() 류의 함수를 사용하면 됩니다.
  문자열 파싱등을 쉽게 하기 위해서, 문자열 좌우 그리고 중간중간에 반복되어 나오는 공백문자를
  하나의 공백문자(' ')로 단순화하는 Simplify(), Simplified() 함수도 있습니다.


  UString의 특정 문자 또는 서브 문자열을 찾고자 할 경우에는 IndexOf() 또는 LastIndexOf()를 사용하십시오.
  IndexOf()의 경우에는 문자 또는 서브 문자열을 앞에서부터 찾고, LastIndexOf()는 뒤에서 부터 찾습니다.
  해당 내용을 찾았을 경우에는 \c 0 이상의 인덱스를 반환하고, 찾지 못하였을 경우에는 \c INVALID_INDEX(-1)을 돌려줍니다.


  UString은 숫자를 문자열로 혹은 그반대로 변환하는 다수의 함수를 제공합니다.
  SetNumber(), FromNumber(), ToInt32(), ToDouble()등의 함수를 통해서 문자열과 숫자간의
  상호 변화를 손쉽게 가능합니다.


  대소문자 변환은 ToUpper() 또는 ToLower() 함수를 사용하면 됩니다.
  \code
  UString str("ABCabc");
  fun_check(str.ToUpper() == "ABCABC");
  fun_check(str.ToLower() == "abcabc");
  \endcode
  ToUpper(), ToLower() 함수처럼 사본을 반환하는 형태가 아닌, 인스턴스의 내용 자체를 수정하려면,
  MakeUpper(), MakeLower() 함수를 사용하십시오.


  주어진 구분자(Delimiter) 문자 또는 문자열로 여러개의 단어로 나눌수 있는 기능을 제공합니다.
  Split() 류의 함수를 통해서 이루어지며, 복사(Deep-copy)를 없앤 SplitRef()류의 함수도 있습니다.

  반대로, 여러개의 단어를 구분자를 중간에 넣어서 결합하고자 할 경우(즉, Split의 반대)에는
  Join() 함수를 사용하면 됩니다.

  \section string_querying 데이터 조회하기

  \section string_conversion 인코딩 변환

  \table 100%
  \header \li 인코딩 \li 설명
  \row \li \c ASCII \li ASCII 인코딩
  \row \li \c UTF8 \li UTF8 인코딩
  \row \li \c UTF16 \li UTF16 인코딩
  \row \li \c UTF32 \li UTF32 인코딩
  \endtable

  <table width=100%>
  <tr><td>인코딩</td><td>설명</td></tr>
  <tr><td>ASCII</td><td>7비트 ASCII 인코딩</td></tr>
  <tr><td>UTF8</td><td>UTF8 인코딩</td></tr>
  <tr><td>UTF16</td><td>UTF16 인코딩</td></tr>
  <tr><td>UTF32</td><td>UTF32 인코딩</td></tr>
  </table>

  \section string_distingush_null_and_empty Null 그리고 빈 문자열 구분하기

  \section string_formatting 포맷팅


  \section string_efficiency_initialization 좀더 효율적인 문자열 생성

  이미, 소스코드 내에 직접 보이는 문자열들이 많이 있습니다.

  \code
  void DoSomething(const UString& A, const UString& B) {
    // ...
  }

  DoSomething("Hello", "World");
  \endcode

  이러한 문자열들은 컴파일러에 의해서 메모리 공간이 할당되어 있고,
  할당된 메모리 상에 문자열이 위치하게 됩니다.
  이러한 문자열을 다시 담기 위해서 메모리를 할당하는
  것은 그리 좋은 동작은 아닙니다.

  이때 UStringLiteral로 감싸서 선언해주면, 메모리 복사를 피할 수 있습니다.

  UStringLiteral로 감싼 문자열은 복사(Deep-Copy)가 아닌 공유 형태로 유지되므로,
  효율을 높일 수 있습니다.
  즉, UString("ABC") 보다는 UStringLiteral("ABC")가 훨씬 효율적입니다.

  UString("ABC")의 경우에는 내부적으로 메모리 블럭 할당이 발생합니다.
  반면, UStringLiteral("ABC")는 메모리 할당이 필요하지 않습니다.

  또한, \c CASCIIString으로 감싸주면, 불필요한 암묵적 인코딩 변환을 피할 수 있습니다.
  예를들어 UString("ABC") 이렇게 선언해주면, 편의를 위해서 자동으로
  UTF8 인코딩으로 변환을 시도합니다.

  하지만, 문자열 "ABC"는 알파벳으로만 구성되어 있기에, UTF8으로의 변환 동작은 무의미합니다.
  단지, CPU와 메모리 사용을 야기할 뿐입니다.
  이러한 경우에 AsciiString("ABC")와 같은 형태로 감싸주면, 시스템은 단순 ASCII 스트링으로 인식하여
  UTF8으로의 변환을 시도하지 않습니다.


  \section string_capitalization 대소 문자 구분하기

  \section string_nultem Nul Termination



  \todo 숫자 변환할때, nul-term이 아니어도 처리가 가능하도록 변경하자. (nul-term을 전재로한 C 함수 때문에 제한이 있는 상태임.)
  \todo 암묵적/명시적으로 문자열 인코딩 상호 변환 가능하도록..
  \todo SmartFormatter 적용.
*/

namespace {

const UNICHAR EXTRA_FILL_CHARACTER = 0x20; // 원래 문자열 뒷편에 붙일 경우, 초과하는 부분에는 ' ' 문자로 채워줌.

FUN_ALWAYS_INLINE UNICHAR FoldCase(UNICHAR ch) {
  return CharTraitsU::IsLower(ch) ? CharTraitsU::ToUpper(ch) : ch;
}

FUN_ALWAYS_INLINE void NulTerm(UStringData* data) {
  data->MutableData()[data->length] = '\0';
}

FUN_ALWAYS_INLINE void StringSet(UNICHAR* dst, UNICHAR ch, size_t len) {
  while (len--) {
    *dst++ = ch;
  }
}

FUN_ALWAYS_INLINE void StringCopy(UNICHAR* dst, const UNICHAR* src, size_t len) {
  UnsafeMemory::Memcpy(dst, src, len * sizeof(UNICHAR));
}

FUN_ALWAYS_INLINE void StringCopyFromASCII(UNICHAR* dst, const char* src, size_t len) {
  const uint8* u = (const uint8*)src;
  while (len--) {
    *dst = *u < 128 ? *u : '?';
    ++dst; ++u;
  }
}

FUN_ALWAYS_INLINE void StringToAscii(uint8* dst, const uint16* src, int32 len) {
  while (len--) {
    *dst++ = (*src > 127) ? '?' : *src;
    ++src;
  }
}

FUN_ALWAYS_INLINE void StringMove(UNICHAR* dst, const UNICHAR* src, size_t len) {
  UnsafeMemory::Memmove(dst, src, len * sizeof(UNICHAR));
}

} // namespace


/*! \enum UString::Base64Option

  Base64 encoding 관련 옵션 플래그들입니다.

  \value Base64Encoding 기본 옵션입니다.

  \value Base64UrlEncoding TODO documentation

  \value KeepTrailingEquals TODO documentation

  \value OmitTrailingEquals TODO documentation
*/

UString::UString(UStringDataPtr data_ptr)
  : data_(data_ptr.ptr) {}

UString::UString()
  : data_(UStringData::SharedEmpty()) {}

UString::UString(const UNICHAR* str) {
  const int32 len = CStringTraitsU::Strlen(str);
  if (len == 0) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(len + 1);
    StringCopy(data_->MutableData(), str, len);
    data_->length = len;
    NulTerm(data_);
  }
}

UString::UString(const UStringView& str) {
  if (str.IsEmpty()) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(str.Len() + 1);
    StringCopy(data_->MutableData(), str.ConstData(), str.Len());
    data_->length = str.Len();
    NulTerm(data_);
  }
}

UString::UString(const UNICHAR* str, int32 len) {
  if (len < 0) {
    len = CStringTraitsU::Strlen(str);
  }

  if (len == 0) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(len + 1);
    StringCopy(data_->MutableData(), str, len);
    data_->length = len;
    NulTerm(data_);
  }
}

UString::UString(const UNICHAR* begin, const UNICHAR* end) {
  fun_check(end >= begin);
  
  int32 len = int32(end - begin);
  
  if (len == 0) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(len + 1);
    StringCopy(data_->MutableData(), begin, len);
    data_->length = len;
    NulTerm(data_);
  }
}

UString::UString(UNICHAR ch) {
  fun_check(ch);

  data_ = UStringData::Allocate(1 + 1);

  UNICHAR* dst = data_->MutableData();
  dst[0] = ch;
  dst[1] = '\0'; // null-term

  data_->length = 1;
}

UString::UString(int32 count, UNICHAR ch) {
  fun_check(count >= 0);

  if (count <= 0) {
    data_ = UStringData::SharedEmpty();
  } else {
    fun_check(ch);
    data_ = UStringData::Allocate(count + 1);
    StringSet(data_->MutableData(), ch, count);
    data_->length = count;
    NulTerm(data_);
  }
}

UString::UString(int32 len, NoInit_TAG) {
  fun_check(len >= 0);

  if (len <= 0) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(len + 1);
    data_->length = len;
    NulTerm(data_);
  }
}

UString::UString(int32 len, ReservationInit_TAG)
  : data_(UStringData::SharedEmpty()) {
  fun_check(len >= 0);

  Reserve(len);
}

//Ambiguity problem
//UString::UString(char ch)
//  : data_(FromASCII_helper(&ch, 1))
//{}

//Ambiguity problem
//UString& UString::operator = (char ch)
//{
//  return *this = UNICHAR(ch);
//}

UString::UString(const AsciiString& str)
  : data_(FromASCII_helper(str.ConstData(), str.Len())) {}

UString& UString::operator = (const AsciiString& str) {
  *this = FromAscii(str.ConstData(), str.Len());
  return *this;
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION

UString::UString(const char* str, int32 len)
  : data_(FromAsciiOrUtf8_helper(str, len)) {}

UString::UString(const char* str)
  : data_(FromAsciiOrUtf8_helper(str)) {}

UString::UString(ByteStringView str)
  : data_(FromAsciiOrUtf8_helper(str.ConstData(), str.Len())) {}

UString& UString::operator = (const char* str) {
  *this = FromUtf8(str);
  return *this;
}

UString& UString::operator = (ByteStringView str) {
  *this = FromUtf8(str.ConstData(), str.Len());
  return *this;
}

#endif // FUN_USE_IMPLICIT_STRING_CONVERSION

UString::UString(const UStringRef& ref) {
  if (ref.IsEmpty()) {
    data_ = UStringData::SharedEmpty();
  } else {
    data_ = UStringData::Allocate(ref.Len() + 1);
    StringCopy(data_->MutableData(), ref.ConstData(), ref.Len());
    data_->length = ref.Len();
    NulTerm(data_);
  }
}

UString::UString(const UString& rhs) : data_(rhs.data_) {
  data_->ref.AddRef();
}

UString::~UString() {
  if (0 == data_->ref.DecRef()) {
    UStringData::Free(data_);
  }
}

UString& UString::operator = (const UString& rhs) {
  if (FUN_LIKELY(rhs.data_ != data_)) {
    rhs.data_->ref.AddRef();

    if (0 == data_->ref.DecRef()) {
      UStringData::Free(data_);
    }

    data_ = rhs.data_;
  }

  return *this;
}

UString& UString::operator = (const UNICHAR* str) {
  *this = UStringView(str);
  return *this;
}

UString& UString::operator = (UStringView str) {
  UStringData* new_data;
  if (str.IsEmpty()) {
    new_data = UStringData::SharedEmpty();
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

    // 필요한가?
    NulTerm(new_data);
  }

  new_data->ref.AddRef();

  if (0 == data_->ref.DecRef()) {
    UStringData::Free(data_);
  }

  data_ = new_data;

  return *this;
}

UString& UString::operator = (UNICHAR ch) {
  if (IsDetached() && Capacity() >= 1) {
    fun_check(ch);

    UNICHAR* dst = data_->MutableData();
    dst[0] = ch;
    dst[1] = '\0';
    data_->length = 1;
    //shrink는 수행하지 않음.  필요하다면, 외부에서 직접 하도록 함.
  } else {
    operator = (UString(ch));
  }
  return *this;
}

UString::UString(UString&& rhs) : data_(rhs.data_) {
  rhs.data_ = UStringData::SharedEmpty();
}

UString& UString::operator = (UString&& rhs) {
  Swap(rhs);
  return *this;
}

void UString::Swap(UString& rhs) {
  fun::Swap(data_, rhs.data_);
}

int32 UString::Len() const {
  return data_->length;
}

int32 UString::NulTermLen() const {
  //RAW가 아니더라도, 중간에 NUL문자가 들어갈 수 있음.
  //if (!data_->IsRawData()) {
  //  return data_->length;
  //}

  const UNICHAR* p = cbegin();
  const UNICHAR* e = cend();
  for (; p != e; ++p);
  return p - ConstData();
}

int32 UString::Capacity() const {
  return data_->alloc ? data_->alloc - 1 : 0;
}

bool UString::IsEmpty() const {
  return data_->length == 0;
}

// 외부 버퍼에 대한 처리를 하도록 하자.
// 단, 외부 버퍼는 공유가 불가능함.
// 복사가 일어날 경우에는 예외를 던지던가해서 처리함.
void UString::ResizeUninitialized(int32 new_len) {
  // 만약 외부버퍼를 사용중이라면, 데이터블럭의 길이만 재조정하면됨.
  // 버퍼의 길이가 고정이므로, 길이를 초과하는 경우에는 예외를 던져주어야함.
  // 또한 외부 버퍼인 경우에는 공유가 불가능하다.
  // 참조 카운팅이 아닌 Move를 해주어야하겠다.

  fun_check(new_len >= 0);

  if (new_len < 0) {
    new_len = 0;
  }

  if (new_len == 0 && !data_->capacity_reserved) {
    UStringData* new_data = UStringData::SharedEmpty();
    if (0 == data_->ref.DecRef()) {
      UStringData::Free(data_);
    }

    data_ = new_data;
    
  } else if (data_->length == 0 && data_->ref.IsPersistent()) { // 할당된 내용이 전혀 없었기 때문에, realloc이 아닌 alloc임. (카피 X)
    UStringData* new_data = UStringData::Allocate(new_len + 1);
    new_data->length = new_len;
    NulTerm(new_data);

    data_ = new_data;

  } else {
    const int32 required_alloc = new_len + 1;

    if (data_->ref.IsShared() || // 공유중이었을 경우는, 새블럭을 할당 받아야함. (COW)
      required_alloc > (int32)data_->alloc || // 메모리가 부족하다면, 새블럭을 할당 받아야함. (Grow)
      (!data_->capacity_reserved && new_len < data_->length && required_alloc < ((int32)data_->alloc / 2))) { // 담을 내용이 비대하다면, Shrink
      ReallocateData(required_alloc, data_->DetachFlags() | UStringData::Grow);
    }

    if (data_->alloc > 0) {
      data_->length = new_len;
      NulTerm(data_);
    }
  }
}

void UString::ResizeZeroed(int32 new_len) {
  fun_check(new_len >= 0);

  if (new_len < 0) {
    new_len = 0;
  }

  const int32 before_len = Len();
  ResizeUninitialized(new_len);
  const int32 difference = Len() - before_len;
  if (difference > 0) {
    UnsafeMemory::Memset(data_->MutableData() + before_len, 0x00, difference*sizeof(UNICHAR));
  }
}

void UString::Resize(int32 new_len, UNICHAR filler) {
  const int32 before_len = Len();
  ResizeUninitialized(new_len);
  const int32 difference = Len() - before_len;
  if (difference > 0) {
    StringSet(data_->MutableData() + before_len, filler, difference);
  }
}

UString& UString::Fill(UNICHAR filler, int32 len) {
  ResizeUninitialized(len < 0 ? Len() : len);

  if (Len() > 0) {
    StringSet(data_->MutableData(), filler, Len());
  }

  return *this;
}

const UNICHAR* UString::operator * () const {
  return data_->ConstData();
}

UNICHAR* UString::MutableData() {
  Detach();
  return data_->MutableData();
}

UNICHAR* UString::MutableData(int32 len) {
  Detach();
  ResizeUninitialized(len);
  return data_->MutableData();
}

const UNICHAR* UString::ConstData() const {
  return data_->ConstData();
}

const UNICHAR* UString::c_str() const {
  // NUL term을 보장해야함.
  UString* mutable_this = const_cast<UString*>(this);
  mutable_this->TrimToNulTerminator();

  return data_->ConstData();
}

bool UString::IsNulTerm() const {
  return NulTermLen() == Len();
}

UString& UString::TrimToNulTerminator() {
  //RAW가 아닐 경우, nul-term이긴 해도 중간에 NUL이 또 있을 수 있음.
  //if (!data_->IsRawData()) {
  //  // raw가 아닐 경우에는 이미 nul-terminated 상태임.
  //  return *this; // no, then we're sure we're zero terminated.
  //}

  const int32 nul_pos = IndexOf(UNICHAR('\0'));
  if (nul_pos != INVALID_INDEX) { // Len()까지만 찾기를 수행하므로, 일반적인 경우라면 NUL이 발견 안될것임.
    Truncate(nul_pos);
  }
  return *this;
}

UString UString::ToNulTerminated() const {
  return UString(*this).TrimToNulTerminator();
}

UNICHAR UString::At(int32 index) const {
  fun_check(uint32(index) < uint32(Len()));
  return ConstData()[index];
}

UNICHAR UString::operator[] (int32 index) const {
  fun_check(uint32(index) < uint32(Len()));
  return ConstData()[index];
}

UString::CharRef UString::operator[] (int32 index) {
  fun_check(index >= 0); // ExpandAt() 호출이 가능한 상황이므로, 음수만 아니면 됨.
  return CharRef(*this, index);
}

UNICHAR UString::First() const {
  return At(0);
}

UString::CharRef UString::First() {
  return operator[](0);
}

UNICHAR UString::Last() const {
  return At(Len() - 1);
}

UString::CharRef UString::Last() {
  return operator[](Len() - 1);
}

UNICHAR UString::FirstOr(const UNICHAR def) const {
  return Len() ? First() : def;
}

UNICHAR UString::LastOr(const UNICHAR def) const {
  return Len() ? Last() : def;
}

void UString::Reserve(int32 new_capacity) {
  fun_check(new_capacity >= 0);

  if (data_->ref.IsShared() || (new_capacity + 1) > (int32)data_->alloc) {
    if (new_capacity < Len()) {
      new_capacity = Len();
    }

    ReallocateData(new_capacity + 1, data_->DetachFlags() | UStringData::CapacityReserved);
  } else {
    // cannot set unconditionally, since Data could be the SharedNull or otherwise Persistent.
    data_->capacity_reserved = false;
  }
}

void UString::Shrink() {
  if (data_->ref.IsShared() || data_->length + 1 < (int32)data_->alloc) {
    ReallocateData(data_->length + 1, data_->DetachFlags() & ~UStringData::CapacityReserved); // CapacityReserved 플래그를 클리어. 즉, 여분 없이 딱 핏팅되어 있는 상태임.
    fun_check_dbg(data_->alloc == data_->length + 1);
  } else {
    // cannot set unconditionally, since Data could be the SharedNull or otherwise Persistent.
    data_->capacity_reserved = 1;
  }
}

void UString::Clear() {
  if (0 == data_->ref.DecRef()) {
    UStringData::Free(data_);
  }

  data_ = UStringData::SharedEmpty();
}

void UString::Clear(int32 initial_capacity) {
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

void UString::Detach() {
  if (data_->ref.IsShared() || data_->IsRawData()) { // 공유중이거나, raw data일 경우에는 사본을 만들어야함. (COW)
    ReallocateData(data_->length + 1, data_->DetachFlags());
  }
}

bool UString::IsDetached() const {
  return data_->ref.IsShared() == false;
}

bool UString::IsSharedWith(const UString& rhs) const {
  return data_ == rhs.data_;
}

bool UString::IsRawData() const {
  return data_->IsRawData();
}

void UString::ReallocateData(int32 new_alloc, UStringData::AllocationOptions options) {
  fun_check(new_alloc >= 0);

  if (data_->ref.IsShared() || data_->IsRawData()) {
    UStringData* new_data = UStringData::Allocate(new_alloc, options);
    new_data->length = MathBase::Min(new_alloc - 1, data_->length);
    StringCopy(new_data->MutableData(), data_->ConstData(), new_data->length);
    NulTerm(new_data);

    if (0 == data_->ref.DecRef()) { // release previously owned data block.
      UStringData::Free(data_);
    }

    data_ = new_data;
    fun_check_dbg(data_->ref.GetCounter() == RefCounter::INITIAL_OWNED_COUNTER_VALUE);
  } else {
    // 공유 / raw 아니므로, 자체적으로 공간만 재조정함.
    data_ = UStringData::ReallocateUnaligned(data_, new_alloc, options);
  }
}

void UString::ExpandAt(int32 pos) {
  if (pos >= Len()) {
    ResizeUninitialized(pos + 1);
  }
}

//TArray의존성 때문에 cpp로 옮겨옴.

int32 UString::IndexOfAny(const Array<UNICHAR>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  return IndexOfAny(UStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index, matched_len);
}

int32 UString::LastIndexOfAny(const Array<UNICHAR>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  return LastIndexOfAny(UStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index, matched_len);
}

int32 UString::IndexOf(UStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.IsEmpty()) {
    return from;
  }

  if (sub.Len() == 1) {
    return UStringMatcher::FastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }

  const int32 this_len = Len();
  if (from > this_len || (sub.Len() + from) > this_len) {
    return INVALID_INDEX;
  }

  return UStringMatcher::FastFind(ConstData(), this_len, from, sub.ConstData(), sub.Len(), casesense);
}

int32 UString::IndexOf(AsciiString sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  //TODO 변환 없이도 처리 가능하도록 구성하자.
  return IndexOf(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense, from, matched_len);
}

int32 UString::LastIndexOf(UStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.IsEmpty()) {
    //Checkme....
    return from;
  }

  if (sub.Len() == 1) {
    return UStringMatcher::FastLastFindChar(ConstData(), Len(), *sub.ConstData(), from, casesense);
  }
  return UStringMatcher::FastLastFind(ConstData(), Len(), from, sub.ConstData(), sub.Len(), casesense);
}

int32 UString::LastIndexOf(AsciiString sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  //TODO 변환 없이도 처리 가능하도록 구성하자.
  return LastIndexOf(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense, from, matched_len);
}

int32 UString::Count(UStringView sub, CaseSensitivity casesense) const {
  int32 n = 0, pos = -int32(sub.Len());
  if (Len() > 500 && sub.Len() > 5) {
    UStringMatcher matcher(sub, casesense);
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

int32 UString::Count(AsciiString sub, CaseSensitivity casesense) const {
  int32 n = 0, pos = -sub.Len();
  while ((pos = IndexOf(sub, casesense, pos + sub.Len())) != INVALID_INDEX) {
    ++n;
  }
  return n;
}

UString UString::Left(int32 len) const {
  fun_check(len >= 0);

  if (len >= Len()) {
    return *this;
  }

  if (len < 0) {
    return UString();
  }

  return UString(ConstData(), len);
}

UString UString::Right(int32 len) const {
  fun_check(len >= 0);

  if (len >= Len()) {
    return *this;
  }

  if (len < 0) {
    return UString();
  }

  return UString(cend() - len, len);
}

UString UString::Mid(int32 offset, int32 len) const {
  switch (SlicingHelper::Slice(Len(), &offset, &len)) {
    case SlicingHelper::Null:
      return UString();
    case SlicingHelper::Empty: {
      UString empty;
      empty.data_ = UStringData::SharedEmpty();
      return empty;
    }
    case SlicingHelper::Full:
      return *this;
    case SlicingHelper::Subset:
      return UString(ConstData() + offset, len);
  }
  //unreachable to here
  return UString();
}

UString UString::LeftChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Left(Len() - len);
}

UString UString::RightChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Right(Len() - len);
}

UStringRef UString::LeftRef(int32 len) const {
  return UStringRef(this).Left(len);
}

UStringRef UString::MidRef(int32 offset, int32 len) const {
  return UStringRef(this).Mid(offset, len);
}

UStringRef UString::RightRef(int32 len) const {
  return UStringRef(this).Right(len);
}

UStringRef UString::LeftChoppedRef(int32 len) const {
  return UStringRef(this).LeftChopped(len);
}

UStringRef UString::RightChoppedRef(int32 len) const {
  return UStringRef(this).RightChopped(len);
}

UString& UString::Reverse() {
  if (Len() > 0) {
    UNICHAR* b = MutableData();
    UNICHAR* e = b + Len() - 1;
    do {
      fun::Swap(*b, *e);
      ++b; --e;
    } while (b < e);
  }
  return *this;
}

UString UString::Reversed() const {
  return UString(*this).Reverse();
}

UString& UString::PrependUnitialized(int32 len) {
  return InsertUninitialized(0, len);
}

UString& UString::AppendUninitialized(int32 len) {
  fun_check(len >= 0);

  ResizeUninitialized(Len() + len);
  return *this;
}

UString& UString::InsertUninitialized(int32 pos, int32 len) {
  fun_check(pos >= 0);
  fun_check(len >= 0);

  if (pos < 0 || len <= 0) {
    return *this;
  }

  const int32 before_len = Len();
  ResizeUninitialized(MathBase::Max(pos, before_len) + len);
  UNICHAR* dst = MutableData();

  if (pos < before_len) {
    StringMove(dst + pos + len, dst + pos, before_len - pos);
  }

  return *this;
}

UString& UString::PrependZeroed(int32 len) {
  return InsertZeroed(0, len);
}

UString& UString::AppendZeroed(int32 len) {
  fun_check(len >= 0);

  if (len > 0) {
    const int32 before_len = Len();
    ResizeUninitialized(before_len + len);
    StringSet(data_->MutableData() + before_len, UNICHAR(0), len);
  }
  return *this;
}

UString& UString::InsertZeroed(int32 pos, int32 len) {
  fun_check(pos >= 0);
  fun_check(len >= 0);

  if (pos < 0 || len <= 0) {
    return *this;
  }

  const int32 before_len = Len();
  ResizeUninitialized(MathBase::Max(pos, before_len) + len);
  UNICHAR* dst = MutableData();

  if (pos > before_len) {
    StringSet(dst + before_len, UNICHAR(0), pos - before_len);
  } else if (pos < before_len) {
    StringMove(dst + pos + len, dst + pos, before_len - pos);
  }
  StringSet(dst + pos, UNICHAR(0), len);
  // already nul-terminated.
  return *this;
}

bool UString::StartsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UString::StartsWith(AsciiString sub, CaseSensitivity casesense) const {
  return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UString::EndsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UString::EndsWith(AsciiString sub, CaseSensitivity casesense) const {
  return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UString::GlobMatch(UStringView pattern, CaseSensitivity casesense) const {
  //TODO
  fun_check(0);
  return false;

  //return GlobU::Match(cbegin(),cend(), pattern.cbegin(),pattern.cend(), casesense);
}

bool UString::GlobMatch(AsciiString pattern, CaseSensitivity casesense) const {
  return GlobMatch(ASCII_TO_UNICHAR<>(pattern).ToUStringView(), casesense);
}

UString& UString::Truncate(int32 pos) {
  fun_check(pos >= 0);

  if (pos < Len()) {
    ResizeUninitialized(pos);
  }
  return *this;
}

UString& UString::LeftChop(int32 len) {
  fun_check(len >= 0);
  fun_check(len <= Len()); //strict?

  if (len > 0) {
    ResizeUninitialized(Len() - len); // Resize() 함수는 len이 음수일 경우, 0으로 취급하므로 상관 없음. 그져, 비어있는 UString()이 될뿐.
  }
  return *this;
}

UString& UString::RightChop(int32 len) {
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

UString& UString::MakeLower() {
  //TODO 변경되는 시점에서 Detach()하는 쪽으로 변경.
  UNICHAR* p = MutableData(); // detach for modyfing
  UNICHAR* e = p + Len();
  for (; p != e; ++p) {
    *p = CharTraitsU::ToLower(*p);
  }
  return *this;
}

UString& UString::MakeUpper() {
  //TODO 변경되는 시점에서 Detach()하는 쪽으로 변경.
  UNICHAR* p = MutableData(); // detach for modyfing
  UNICHAR* e = p + Len();
  for (; p != e; ++p) {
    *p = CharTraitsU::ToUpper(*p);
  }
  return *this;
}

UString UString::ToLower() const {
  return UString(*this).MakeLower();
}

UString UString::ToUpper() const {
  return UString(*this).MakeUpper();
}

UString& UString::TrimLeft() {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = LeftSpaces();
  if (space_count > 0) {
    Remove(0, space_count);
  }
  return *this;
}

UString& UString::TrimRight() {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = RightSpaces();
  if (space_count > 0) {
    //ResizeUninitialized(Len() - space_count);
    Remove(Len() - space_count, space_count);
  }
  return *this;
}

UString& UString::Trim() {
  if (Len() == 0) {
    return *this;
  }

  const int32 l_space_count = LeftSpaces();
  const int32 r_space_count = RightSpaces();

  //왼쪽(앞)부터 처리하게 되면, 오프셋의 변화가 오게 되므로, 오른쪽 부터 처리하도록 함.
  if (r_space_count > 0) {
    Remove(Len() - r_space_count, r_space_count);
  }
  if (l_space_count > 0) {
    Remove(0, l_space_count);
  }
  return *this;
}

UString UString::TrimmedLeft() const {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = LeftSpaces();
  if (space_count > 0) {
    return UString(ConstData() + space_count, Len() - space_count);
  } else {
    return *this;
  }
}

UString UString::TrimmedRight() const {
  if (Len() == 0) {
    return *this;
  }

  const int32 space_count = RightSpaces();
  if (space_count > 0) {
    return UString(ConstData(), Len() - space_count);
  } else {
    return *this;
  }
}

UString UString::Trimmed() const {
  if (Len() == 0) {
    return *this;
  }

  const UNICHAR* start = cbegin();
  const UNICHAR* end = cend();
  StringAlgo<UString>::TrimmedPositions(start, end);
  if (start == end) {
    return UString();
  }

  return UString(start, end - start);
}

int32 UString::LeftSpaces() const {
  return StringAlgo<UString>::LeftSpaces(cbegin(), cend());
}

int32 UString::RightSpaces() const {
  return StringAlgo<UString>::RightSpaces(cbegin(), cend());
}

int32 UString::SideSpaces() const {
  return StringAlgo<UString>::SideSpaces(cbegin(), cend());
}

UStringRef UString::TrimmedLeftRef() const {
  return UStringRef(this).TrimmedLeft();
}

UStringRef UString::TrimmedRightRef() const {
  return UStringRef(this).TrimmedRight();
}

UStringRef UString::TrimmedRef() const {
  return UStringRef(this).Trimmed();
}

UString& UString::Simplify() {
  return (*this = Simplified());
}

UString UString::Simplified() const {
  return StringAlgo<const UString>::Simplified(*this);
}

UString UString::LeftJustified(int32 width, UNICHAR filler, bool truncate) const {
  fun_check(width >= 0);

  UString result;

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

UString UString::RightJustified(int32 width, UNICHAR filler, bool truncate) const {
  fun_check(width >= 0);

  UString result;

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

UString& UString::Prepend(int32 count, UNICHAR ch) {
  return Insert(0, count, ch);
}

UString& UString::Prepend(UStringView str) {
  return Insert(0, str);
}

UString& UString::Prepend(AsciiString str) {
  return Insert(0, str);
}

UString& UString::Append(int32 count, UNICHAR ch) {
  fun_check(count >= 0);

  if (count > 0) {
    const int32 before_len = Len();
    ResizeUninitialized(before_len + count);

    StringSet(data_->MutableData() + before_len, ch, count);
  }
  return *this;
}

UString& UString::Append(UStringView str) {
  if (data_->length == 0 && data_->ref.IsPersistent() && !data_->IsRawData()) {
    *this = str;
  } else if (str.Len() > 0) {
    const int32 before_len = Len();
    ResizeUninitialized(before_len + str.Len());

    StringCopy(data_->MutableData() + before_len, str.ConstData(), str.Len());
  }
  return *this;
}

UString& UString::Append(AsciiString str) {
  if (data_->length == 0 && data_->ref.IsPersistent() && !data_->IsRawData()) {
    *this = str;
  } else if (str.Len() > 0) {
    const int32 before_len = Len();
    ResizeUninitialized(before_len + str.Len());

    StringCopyFromASCII(data_->MutableData() + before_len, str.ConstData(), str.Len());
  }
  return *this;
}

UString& UString::Insert(int32 pos, int32 count, UNICHAR ch) {
  fun_check(pos >= 0);
  fun_check(count >= 0);

  if (pos < 0 || count <= 0) {
    return *this;
  }

  const int32 before_len = Len();
  ResizeUninitialized(MathBase::Max(pos, before_len) + count);
  UNICHAR* dst = MutableData();

  if (pos > before_len) {
    StringSet(dst + before_len, EXTRA_FILL_CHARACTER, pos - before_len);
  } else if (pos < before_len) {
    StringMove(dst + pos + count, dst + pos, before_len - pos);
  }
  StringSet(dst + pos, ch, count);
  // already nul-terminated.
  return *this;
}

UString& UString::Insert(int32 pos, UStringView str) {
  fun_check(pos >= 0);

  if (pos < 0) {
    return *this;
  }

  if (str.IsEmpty()) {
    return *this;
  }

  const int32 before_len = Len();
  ResizeUninitialized(MathBase::Max(pos, before_len) + str.Len());

  UNICHAR* dst = MutableData();
  if (pos > before_len) {
    StringSet(dst + before_len, EXTRA_FILL_CHARACTER, pos - before_len);
  } else {
    StringMove(dst + pos + str.Len(), dst + pos, before_len - pos);
  }
  StringCopy(dst + pos, str.ConstData(), str.Len());
  return *this;
}

UString& UString::Insert(int32 pos, AsciiString str) {
  fun_check(pos >= 0);

  if (pos < 0 || str.IsEmpty()) {
    return *this;
  }

  const int32 before_len = Len();
  ResizeUninitialized(MathBase::Max(pos, before_len) + str.Len());

  UNICHAR* dst = MutableData();
  if (pos > before_len) {
    StringSet(dst + before_len, EXTRA_FILL_CHARACTER, pos - before_len);
  } else {
    StringMove(dst + pos + str.Len(), dst + pos, before_len - pos);
  }
  StringCopyFromASCII(dst + pos, str.ConstData(), str.Len());
  return *this;
}

UString& UString::Overwrite(int32 pos, int32 count, UNICHAR ch) {
  fun_check(pos >= 0);
  fun_check(count >= 0);

  Detach();
  ExpandAt(pos + count - 1);
  StringSet(data_->MutableData() + pos, ch, count);

  return *this;
}

UString& UString::Overwrite(int32 pos, UStringView str) {
  fun_check(pos >= 0);

  Detach();
  ExpandAt(pos + str.Len() - 1);
  StringCopy(data_->MutableData() + pos, str.ConstData(), str.Len());

  return *this;
}

UString& UString::Overwrite(int32 pos, AsciiString str) {
  fun_check(pos >= 0);

  Detach();
  ExpandAt(pos + str.Len() - 1);
  StringCopyFromASCII(data_->MutableData() + pos, str.ConstData(), str.Len());

  return *this;
}

UString& UString::Remove(int32 pos, int32 length_to_remove) {
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

int32 UString::TryFindAndRemove(UStringView sub, CaseSensitivity casesense) {
  int32 pos, removed_count = 0;
  while ((pos = IndexOf(sub, casesense)) != INVALID_INDEX) {
    Remove(pos, sub.Len());
    ++removed_count;
  }
  return removed_count;
}

int32 UString::TryFindAndRemove(AsciiString sub, CaseSensitivity casesense) {
  //변환없이 바로 찾아서 제거하는 형태로 수정.
  //return TryFindAndRemove(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense);
  int32 pos, removed_count = 0;
  while ((pos = IndexOf(sub, casesense)) != INVALID_INDEX) {
    Remove(pos, sub.Len());
    ++removed_count;
  }
  return removed_count;
}


//
// Replacements
//

UString& UString::Replace(int32 before_pos, int32 before_len, UStringView after) {
  // If the contents before and after the change are the same length and are in the range, overwrite
  if (before_len == after.Len() && (before_pos + before_len <= Len())) {
    Detach();
    StringCopy(MutableData() + before_pos, after.ConstData(), after.Len());
    return *this;
  } else {
    Remove(before_pos, before_len);
    return Insert(before_pos, after.ConstData(), after.Len());
  }
}

UString& UString::Replace(int32 before_pos, int32 before_len, AsciiString after) {
  // If the contents before and after the change are the same length and are in the range, overwrite
  if (before_len == after.Len() && (before_pos + before_len <= Len())) {
    Detach();
    StringCopyFromASCII(MutableData() + before_pos, after.ConstData(), after.Len());
    return *this;
  } else {
    Remove(before_pos, before_len);
    return Insert(before_pos, after);
  }
}

int32 UString::TryReplace(UStringView before, UStringView after, CaseSensitivity casesense) {
  if (before.ConstData() == after.ConstData() && before.Len() == after.Len()) {
    return 0;
  }

  // 찾아서 삭제하기로... 차후에 생각을 좀 해봐야함...
  if (after.IsEmpty()) {
    return TryFindAndRemove(before, casesense);
  }

  int32 replaced_count = 0;

  // protect against before or after being part of this
  const UNICHAR* saved_new_ptr = after.ConstData();
  const UNICHAR* saved_old_ptr = before.ConstData();

  if (after.ConstData() >= ConstData() && after.ConstData() < ConstData() + Len()) { // 자기자신에 대해서 처리할 경우 사본을 만든 후 처리 (inplace operation)
    UNICHAR* copy = (UNICHAR*)UnsafeMemory::Malloc(after.Len()*sizeof(UNICHAR)); //nul-term일 필요는 없음.
    StringCopy(copy, after.ConstData(), after.Len());
    saved_new_ptr = copy;
  }

  if (before.ConstData() >= ConstData() && before.ConstData() < ConstData() + Len()) { // 자기자신에 대해서 처리할 경우 사본을 만든 후 처리 (inplace operation)
    UNICHAR* copy = (UNICHAR*)UnsafeMemory::Malloc(before.Len()*sizeof(UNICHAR)); //nul-term일 필요는 없음.
    StringCopy(copy, before.ConstData(), before.Len());
    saved_old_ptr = copy;
  }

  UStringMatcher matcher(before, casesense);

  int32 index = 0;
  int32 this_len = Len();
  UNICHAR* d = MutableData(); // with detach for modifying

  if (before.Len() == after.Len()) {
    if (before.Len() > 0) {
      while ((index = matcher.IndexIn(*this, index)) != INVALID_INDEX) {
        StringCopy(d + index, after.ConstData(), after.Len());
        index += before.Len();
        ++replaced_count;
      }
    }
  } else if (after.Len() < before.Len()) {
    uint32 to = 0;
    uint32 move_start = 0;
    uint32 count = 0;
    while ((index = matcher.IndexIn(*this, index)) != INVALID_INDEX) {
      if (count > 0) {
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
        ++replaced_count;
      }
      index += before.Len();
      move_start = index;
      count++;
    }
    if (count > 0) {
      const int32 move_len = this_len - move_start;
      if (move_len > 0) {
        StringMove(d + to, d + move_start, move_len);
      }
      ResizeUninitialized(this_len - count * (before.Len() - after.Len()));
    }
  } else {
    // the most complex case. We don't want To lose performance by doing repeated
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
      // index has to be adjusted in case we get back int32o the loop above.
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
        StringMove(d + move_to, d + move_start, move_end - move_start);
        if (after.Len() > 0) {
          StringCopy(d + insert_start, after.ConstData(), after.Len());
          ++replaced_count;
        }
        move_end = move_start - before.Len();
      }
    }
  }

  if (saved_new_ptr != after.ConstData()) {
    UnsafeMemory::Free(const_cast<UNICHAR*>(saved_new_ptr));
  }

  if (saved_old_ptr != before.ConstData()) {
    UnsafeMemory::Free(const_cast<UNICHAR*>(saved_old_ptr));
  }

  return replaced_count;
}

Array<UString> UString::Split(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UString> UString::Split(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UString> UString::Split(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UString> UString::SplitByWhitespaces(UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; SplitByWhitespaces(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<UString> UString::SplitLines(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; SplitLines(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<UString> UString::Split(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UString> UString::Split(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UString> UString::Split(const std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UString> list; Split(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 UString::Split(Array<UString>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,UStringView>(list, *this, &UString::Mid, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,UStringView>(list, *this, &UString::Mid, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,UStringView>(list, *this, &UString::Mid, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,UStringView>(list, *this, &UString::Mid, separators, max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,UStringView>(list, *this, &UString::Mid, ASCII_TO_UNICHAR<>(separators).ToUStringView(), max_splits, split_options, casesense); }

int32 UString::Split(Array<UString>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,const UNICHAR*>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UString>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UStringRef>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UStringView>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,AsciiString>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 UString::Split(Array<UString>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,const UNICHAR*>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UString>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UStringRef>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,UStringView>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,AsciiString>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 UString::SplitByWhitespaces(Array<UString>& list, UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(u"\n") };
    return Split(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(extra_separator) };
    return Split(list, separators, max_splits, split_options, casesense);
  }
}

int32 UString::SplitLines(Array<UString>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { AsciiString("\r\n"), AsciiString("\n"), AsciiString("\r") };
  return Split(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 UString::Split(Array<UString>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UString,UString,ByteStringView>(list, *this, &UString::Mid, separators, max_splits, split_options, casesense); }

int32 UString::Split(Array<UString>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,const char*>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteString>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteStringRef>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteStringView>(list, *this, &UString::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 UString::Split(Array<UString>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,const char*>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteString>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteStringRef>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::Split(Array<UString>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UString,UString,ByteStringView>(list, *this, &UString::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
#endif

Array<UStringRef> UString::SplitRef(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separator, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitRef(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitRef(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitByWhitespacesRef(UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitByWhitespacesRef(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitLinesRef(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitLinesRef(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<UStringRef> UString::SplitRef(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitRef(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UString::SplitRef(std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UString::SplitRef(std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitRef(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 UString::SplitRef(Array<UStringRef>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, separators, max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, ASCII_TO_UNICHAR<>(separators).ToUStringView(), max_splits, split_options, casesense); }

int32 UString::SplitRef(Array<UStringRef>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,const UNICHAR*>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UString>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UStringRef>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,AsciiString>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,const UNICHAR*>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UString>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UStringRef>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,UStringView>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,AsciiString>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 UString::SplitByWhitespacesRef(Array<UStringRef>& list, UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(u"\n") };
    return SplitRef(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(extra_separator) };
    return SplitRef(list, separators, max_splits, split_options, casesense);
  }
}

int32 UString::SplitLinesRef(Array<UStringRef>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { AsciiString("\r\n"), AsciiString("\n"), AsciiString("\r") };
  return SplitRef(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 UString::SplitRef(Array<UStringRef>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UString,ByteStringView>(list, *this, &UString::MidRef, separators, max_splits, split_options, casesense); }

int32 UString::SplitRef(Array<UStringRef>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,const char*>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteString>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteStringRef>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteStringView>(list, *this, &UString::MidRef, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,const char*>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteString>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteStringRef>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UString::SplitRef(Array<UStringRef>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UString,ByteStringView>(list, *this, &UString::MidRef, separators.begin(), separators.size(), max_splits, split_options, casesense); }
#endif

bool UString::Divide(UStringView delim, UString* out_left, UString* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UString::Divide(AsciiString delim, UString* out_left, UString* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UString::Divide(UStringView delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = IndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = LeftRef(pos); out_left->Trim(); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) { *out_left = LeftRef(pos); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); }
  }
  return true;
}

bool UString::Divide(AsciiString delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  return Divide(ASCII_TO_UNICHAR<>(delim).ToUStringView(), out_left, out_right, trimming, casesense);
}

bool UString::LastDivide(UStringView delim, UString* out_left, UString* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UString::LastDivide(AsciiString delim, UString* out_left, UString* out_right, bool trimming, CaseSensitivity casesense) const {
  return LastDivide(ASCII_TO_UNICHAR<>(delim).ToUStringView(), out_left, out_right, trimming, casesense);
}

bool UString::LastDivide(UStringView delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  const int32 pos = LastIndexOf(delim, casesense);
  if (pos == INVALID_INDEX) {
    return false;
  }

  if (trimming) {
    if (out_left) { *out_left = LeftRef(pos); out_left->Trim(); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); out_right->Trim(); }
  } else {
    if (out_left) { *out_left = LeftRef(pos); }
    if (out_right) { *out_right = MidRef(pos + delim.Len()); }
  }
  return true;
}

bool UString::LastDivide(AsciiString delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
  return LastDivide(ASCII_TO_UNICHAR<>(delim).ToUStringView(), out_left, out_right, trimming, casesense);
}

UString UString::Join(const Array<const UNICHAR*>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UString>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringRef>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringView>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<AsciiString>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
UString UString::Join(const Array<const char*>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteString>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringRef>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringView>& list, UStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
#endif

UString UString::Join(std::initializer_list<const UNICHAR*> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UString> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringRef> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringView> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<AsciiString> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
UString UString::Join(std::initializer_list<const char*> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteString> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringRef> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringView> list, UStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
#endif

UString UString::Join(const Array<const UNICHAR*>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UString>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringRef>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringView>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<AsciiString>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
UString UString::Join(const Array<const char*>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteString>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringRef>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringView>& list, AsciiString separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
#endif

UString UString::Join(std::initializer_list<const UNICHAR*> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UString> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringRef> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringView> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<AsciiString> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
UString UString::Join(std::initializer_list<const char*> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteString> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringRef> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringView> list, AsciiString separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
UString UString::Join(const Array<const UNICHAR*>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UString>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringRef>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<UStringView>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<AsciiString>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<const char*>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteString>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringRef>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }
UString UString::Join(const Array<ByteStringView>& list, ByteStringView separator)
{ return StringJoin<UString>(list.ConstData(), list.Count(), separator); }

UString UString::Join(std::initializer_list<const UNICHAR*> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UString> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringRef> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<UStringView> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<AsciiString> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<const char*> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteString> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringRef> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
UString UString::Join(std::initializer_list<ByteStringView> list, ByteStringView separator)
{ return StringJoin<UString>(list.begin(), list.size(), separator); }
#endif

#if FUN_USE_REGULAR_EXPRESSION

int32 UString::IndexOf(const Regex& regex, int32 from, RegexMatch* out_match) const {
  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::IndexOf: invalid Regex object.");
    return INVALID_INDEX;
  }

  auto match = regex.Match(*this, from);
  if (match.HasMatch()) {
    const int32 pos = match.CapturedStart();
    if (out_match) {
      *out_match = match;
    }
    return pos;
  }
  return INVALID_INDEX;
}

int32 UString::LastIndexOf(const Regex& regex, int32 from, RegexMatch* out_match) const {
  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::LastIndexOf: invalid Regex object.");
    return INVALID_INDEX;
  }

  const int32 end_pos = (from < 0) ? (Len() + from + 1) : (from + 1);
  auto match_it = regex.GlobalMatch(*this);
  int32 last_index = INVALID_INDEX;
  while (match_it.HasNext()) {
    auto match = match_it.Next();
    const int32 pos = match.CapturedStart();
    if (pos < end_pos) {
      last_index = pos;
      if (out_match) {
        *out_match = match;
      }
    } else {
      break;
    }
  }
  return last_index;
}

bool UString::Contains(const Regex& regex, RegexMatch* out_match) const {
  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::Contains: invalid Regex object.");
    return false;
  }

  auto match = regex.Match(*this);
  const bool has_match = match.HasMatch();
  if (has_match && out_match) {
    *out_match = match;
  }
  return has_match;
}

int32 UString::Count(const Regex& regex) const {
  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::Count: invalid Regex object.");
    return 0;
  }

  int32 matched_count = 0;
  int32 index = -1;
  const int32 l = Len();
  while (index < l - 1) {
    auto match = regex.Match(*this, index + 1);
    if (!match.HasMatch()) {
      break;
    }
    index = match.CapturedStart();
    ++matched_count;
  }
  return matched_count;
}

int32 UString::TryFindAndRemove(const Regex& regex) {
  return TryReplace(regex, UString());
}

int32 UString::TryReplace(const Regex& regex, AsciiString after) {
  return TryReplace(regex, ASCII_TO_UNICHAR<>(after).ToUStringView());
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 UString::TryReplace(const Regex& regex, ByteStringView after) {
  return TryReplace(regex, UTF8_TO_UNICHAR_BUFFER(after).ToUStringView());
}
#endif


//TODO 헤더 쪽으로 옮겨주는것도 좋을듯 싶은데...??
struct UStringCapture {
  int32 no;
  int32 pos;
  int32 len;
};

int32 UString::TryReplace(const Regex& regex, UStringView after) {
  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::TryReplace: invalid Regex object.");
    return 0;
  }

  const UString copy(*this);
  auto match_it = regex.GlobalMatch(copy);
  if (!match_it.HasNext()) {
    return 0;
  }

  int32 replaced_count = 0;

  ReallocateData(Len() + 1, 0); // not grow

  const int32 capture_count = regex.CaptureCount();

  Array<UStringCapture> back_references;
  for (int32 i = 0; i < after.Len() - 1; ++i) {
    if (after[i] == '\\') {
      int32 no = CharTraitsU::DigitCharToInt(after[i + 1]);
      if (no >= 0 && no <= capture_count) {
        UStringCapture back_reference;
        back_reference.pos = i;
        back_reference.len = 2;

        if (i < after.Len() - 2) {
          const int32 second_digit = CharTraitsU::DigitCharToInt(after[i + 2]);
          if (second_digit != -1 && ((no * 10) + second_digit) <= capture_count) {
            no = (no * 10) + second_digit;
            back_reference.Len++;
          }
        }

        back_reference.no = no;
        back_references.Add(back_reference);
      }
    }
  }

  int32 new_len = 0;
  int32 last_end = 0;
  Array<UStringView> chunks;
  while (match_it.HasNext()) {
    auto match = match_it.Next();
    int32 len = match.CapturedStart() - last_end;
    if (len > 0) {
      chunks.Add(copy.MidRef(last_end, len));
      new_len += len;
    }

    last_end = 0;
    for (const auto& back_reference : back_references) {
      len = back_reference.pos - last_end;
      if (len > 0) {
        chunks.Add(copy.MidRef(last_end, len));
        new_len += len;
      }

      len = match.CapturedLength(back_reference.no);
      if (len > 0) {
        chunks.Add(copy.MidRef(match.CapturedStart(back_reference.no), len));
        new_len += len;
      }

      last_end = back_reference.pos + back_reference.len;
    }

    len = after.Len() - last_end;
    if (len > 0) {
      chunks.Add(after.Mid(last_end, len));
      new_len += len;
    }

    last_end = match.CapturedEnd();

    replaced_count++;
  }

  if (copy.Len() > last_end) {
    chunks.Add(copy.MidRef(last_end));
    new_len += copy.Len() - last_end;
  }

  ResizeUninitialized(new_len);
  int32 offset = 0;
  UNICHAR* dst = MutableData();
  for (const auto& chunk : chunks) {
    const int32 len = chunk.Len();
    UnsafeMemory::Memcpy(dst + offset, chunk.ConstData(), len * sizeof(UNICHAR));
    offset += len;
  }

  return replaced_count;
}

namespace {

template <typename ResultList, typename MidMethod>
ResultList SplitString(const UString& source, MidMethod mid, const Regex& regex, int32 max_splits, StringSplitOptions options) {
  //TODO max_splits적용...

  ResultList list;

  if (!regex.IsValid()) {
    fun_log(LogCore, Warning, "UString::SplitString: invalid Regex object.");
    return list;
  }

  int32 start = 0;
  int32 end = 0;
  auto match_it = regex.GlobalMatch(source);
  while (match_it.HasNext()) {
    auto match = match_it.Next();
    end = match.CapturedStart();
    if (start != end) { // TODO trimming and cull-empty
      list.Add((source.*mid)(start, end - start));
    }
    start = match.CapturedEnd();
  }
  if (start != source.Len()) { // TODO trimming and cull-empty
    list.Add((source.*mid)(start, -1));
  }
  return list;
}

} // namespace

Array<UString> UString::Split(const Regex& regex, int32 max_splits, StringSplitOptions options) const {
  return SplitString<Array<UString>>(*this, &UString::Mid, regex, max_splits, options);
}

Array<UStringRef> UString::SplitRef(const Regex& regex, int32 max_splits, StringSplitOptions options) const {
  return SplitString<Array<UStringRef>>(*this, &UString::MidRef, regex, max_splits, options);
}

#endif // FUN_USE_REGULAR_EXPRESSION

UString UString::Repeated(int32 times) const {
  // return itself if empty.
  if (Len() == 0) {
    return *this;
  }

  // 음수인 경우에는 잘못 지정한 경우일 확률이 높음. 단, 0은 허용함.
  // 오류를 찾아내는데 도우미 역활을 할뿐, 실행에 지장이 있지는 않음.
  // times <= 0 일 경우에는 그냥 빈문자열을 돌려줌.
  fun_check(times >= 0);

  if (times <= 1) {
    return times == 1 ? *this : UString();
  }

  const int32 result_len = times * Len();

  //maximum length check. 2GB?
  if (result_len != uint32(result_len)) {
    fun_check(0);
    return *this;
  }

  UString result(result_len, NoInit);
  StringCopy(result.MutableData(), this->ConstData(), this->Len());

  int32 len_so_far = Len();
  UNICHAR* end = result.MutableData() + len_so_far;

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

UString UString::operator * (int32 times) const
{
  return Repeated(times);
}

namespace {

int64 ToIntegral_helper(const UNICHAR* str, int32 len, bool* ok, int32 base, int64/*Constrain*/) {
  return LocaleData::C()->StringToInt64(str, len, base, ok, Locale::NumberOptions::RejectGroupSeparator);
}

int64 ToIntegral_helper(const UNICHAR* str, int32 len, bool* ok, int32 base, uint64/*Constrain*/) {
  return LocaleData::C()->StringToUInt64(str, len, base, ok, Locale::NumberOptions::RejectGroupSeparator);
}

template <typename T>
FUN_ALWAYS_INLINE T ToIntegral(const UNICHAR* str, int32 len, bool* ok, int32 base) {
  const bool IS_UNSIGNED = T(0) < T(-1);
  typedef typename Conditional<IS_UNSIGNED, uint64, int64>::Result INT64;
  if (base != 0 && (base < 2 || base > 36)) {
    //TODO
    //fun_log(LogCore, Warning, "UString::ToIntegral: Invalid base {0}", base);
    base = 10;
  }

  INT64 val = ToIntegral_helper(str, len, ok, base, INT64()/*Constrain*/);
  if (T(val) != val) {
    if (ok) {
      *ok = false;
    }
    val = 0;
  }
  return T(val);
}
} // namespace

int16 UString::ToInt16(bool* ok, int32 base) const {
  return ToIntegral<int16>(ConstData(), Len(), ok, base);
}

uint16 UString::ToUInt16(bool* ok, int32 base) const {
  return ToIntegral<uint16>(ConstData(), Len(), ok, base);
}

int32 UString::ToInt32(bool* ok, int32 base) const {
  return ToIntegral<int32>(ConstData(), Len(), ok, base);
}

uint32 UString::ToUInt32(bool* ok, int32 base) const {
  return ToIntegral<uint32>(ConstData(), Len(), ok, base);
}

int64 UString::ToInt64(bool* ok, int32 base) const {
  return ToIntegral<int64>(ConstData(), Len(), ok, base);
}

uint64 UString::ToUInt64(bool* ok, int32 base) const {
  return ToIntegral<uint64>(ConstData(), Len(), ok, base);
}

float UString::ToFloat(bool* ok) const {
  return LocaleData::ConvertDoubleToFloat(ToDouble(ok), ok);
}

double UString::ToDouble(bool* ok) const {
  return LocaleData::C()->StringToDouble(ConstData(), Len(), ok, Locale::RejectGroupSeparator);
}

namespace {

UNICHAR* __ulltoa(UNICHAR* p, uint64 n, int32 base) {
  if (base < 2 || base > 36) {
    //TODO
    //fun_log(LogCore, Warning, "UString::SetNumber: Invalid base {0}", base);
    base = 10;
  }

  const UNICHAR b = 'a' - 10;
  do {
    const int32 c = n % base;
    n /= base;
    *--p = c + (c < 10 ? '0' : b);
  } while (n);

  return p;
}
} // namespace

UString& UString::SetNumber(int16 value, int32 base) {
  return base == 10 ? SetNumber(int64(value), base) : SetNumber(uint64(value), base);
}

UString& UString::SetNumber(uint16 value, int32 base) {
  return SetNumber(uint64(value), base);
}

UString& UString::SetNumber(int32 value, int32 base) {
  return base == 10 ? SetNumber(int64(value), base) : SetNumber(uint64(value), base);
}

UString& UString::SetNumber(uint32 value, int32 base) {
  return SetNumber(uint64(value), base);
}

UString& UString::SetNumber(int64 value, int32 base) {
  const int32 BUFFER_SIZE = 66;
  UNICHAR buffer[BUFFER_SIZE];
  UNICHAR* p;
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

UString& UString::SetNumber(uint64 value, int32 base) {
  const int32 BUFFER_SIZE = 66;
  UNICHAR buffer[BUFFER_SIZE];
  UNICHAR* p = __ulltoa(buffer + BUFFER_SIZE, value, base);

  Clear();
  Append(p, BUFFER_SIZE - (p - buffer));
  return *this;
}

UString& UString::SetNumber(float value, UNICHAR f, int32 precision) {
  return SetNumber(double(value), f, precision);
}

UString& UString::SetNumber(double value, UNICHAR f, int32 precision) {
  LocaleData::DoubleForm form = LocaleData::DFDecimal;
  uint32 flags = 0;
  if (CharTraitsU::IsUpper(f)) {
    flags = LocaleData::CapitalEorX;
  }
  f = CharTraitsU::ToLower(f);

  switch (f) {
    case 'f':
      form = LocaleData::DFDecimal;
      break;
    case 'e':
      form = LocaleData::DFExponent;
      break;
    case 'g':
      form = LocaleData::DFSignificantDigits;
      break;
    default:
      //TODO
      //fun_log(LogCore, Warning, "UString::SetNumber: Invalid format char '%c'", f);
      break;
  }

  *this = LocaleData::C()->DoubleToString(value, precision, form, -1, flags);
  return *this;
}

namespace {

FUN_ALWAYS_INLINE bool IsPathSeparator(UNICHAR ch) {
    return ch == '/' || ch == '\\';
}

} // namespace

UString& UString::PathAppend(UStringView str) {
  if (Len() > 0 && !IsPathSeparator(Last()) &&
      (str.IsEmpty() || !IsPathSeparator(*str.ConstData()))) {
    Append('/');
  }
  Append(str);
  return *this;
}

UString& UString::PathAppend(AsciiString str) {
  return PathAppend(ASCII_TO_UNICHAR<>(str).ToUStringView());
}

bool UString::IsPathTerminated() const {
  return Len() > 0 && IsPathSeparator(Last());
}

UString& UString::MakePathTerminated() {
  if (Len() > 0 && !IsPathSeparator(Last())) {
    Append('/');
  }
  return *this;
}

UString UString::ToPathTerminated() const {
  UString result(*this);
  if (Len() > 0 && !IsPathSeparator(Last())) {
    result.Append('/');
  }
  return result;
}

UString UString::ToHtmlEscaped() const {
  UString escaped(int32(Len() * 1.1), ReservationInit); // 110%
  for (int32 i = 0; i < Len(); ++i) {
    const UNICHAR ch = ConstData()[i];

    if (ch == '<') {
      escaped.Append(AsciiString("&lt;", 4));
    } else if (ch == '>') {
      escaped.Append(AsciiString("&gt;", 4));
    } else if (ch == '&') {
      escaped.Append(AsciiString("&amp;", 5));
    } else if (ch == '"') {
      escaped.Append(AsciiString("&quot;", 6));
    } else {
      escaped.Append(ch);
    }
  }
  escaped.Shrink();

  return escaped;
}

bool UString::IsNumeric() const {
  return !IsEmpty() && CStringTraitsU::IsNumeric(cbegin(), cend());
}

bool UString::IsIdentifier() const {
  if (IsEmpty()) {
    return false;
  }

  const UNICHAR* p = ConstData();

  // leading
  if (!(CharTraitsU::IsAlpha(p[0]) || p[0] == '_')) {
    return false;
  }

  for (int32 i = 1; i < Len(); ++i) {
    if (!(CharTraitsU::IsAlnum(p[i]) || p[i] == '_')) {
      return false;
    }
  }

  return true;
}

//TODO quote문자를 지정할 수 있다면 더 좋을듯...

bool UString::IsQuoted() const {
  return (Len() >= 2 && First() == '"' && Last() == '"');
}

bool UString::IsQuoted(UStringView str) {
  return (str.Len() >= 2 && str.First() == '"' && str.Last() == '"');
}

UString UString::Unquoted(bool* out_quotes_removed) const {
  if (IsQuoted()) {
    if (out_quotes_removed) *out_quotes_removed = true;
    return Mid(1, Len() - 2);
  } else {
    if (out_quotes_removed) *out_quotes_removed = false;
    return *this;
  }
}

UString& UString::Unquotes(bool* out_quotes_removed) {
  return (*this = Unquoted(out_quotes_removed));
}

UStringRef UString::UnquotedRef(bool* out_quotes_removed) const {
  if (IsQuoted()) {
    if (out_quotes_removed) *out_quotes_removed = true;
    return MidRef(1, Len() - 2);
  } else {
    if (out_quotes_removed) *out_quotes_removed = false;
    return UStringRef();
  }
}

UString UString::ReplaceQuotesWithEscapedQuotes() const {
  if (Contains('\"')) {
    UString result;

    const UNICHAR* p = cbegin();
    const UNICHAR* e = cend();
    bool escaped = false;
    for (; p != e; ++p) {
      if (escaped) {
        escaped = false;
      } else if (*p == '\\') {
        escaped = true;
      } else if (*p == '"') {
        result.Append('\\');
      }

      result.Append(*p);
    }
    return result;
  }
  return *this;
}


static const UNICHAR* CHAR_TO_ESCAPE_SEQ_MAP[][2] = {
  // Always replace \\ first to avoid double-escaping characters
  { UTEXT("\\"), UTEXT("\\\\") },
  { UTEXT("\n"), UTEXT("\\n")  },
  { UTEXT("\r"), UTEXT("\\r")  },
  { UTEXT("\t"), UTEXT("\\t")  },
  { UTEXT("\'"), UTEXT("\\'")  },
  { UTEXT("\""), UTEXT("\\\"") }
};

static const uint32 MAX_SUPPORTED_ESCAPE_CHARS = countof(CHAR_TO_ESCAPE_SEQ_MAP);

UString UString::ReplaceCharWithEscapedChar(const Array<UNICHAR>* chars) const {
  if (Len() > 0 && (chars == nullptr || chars->Count() > 0)) {
    UString result(*this);
    for (int32 char_idx = 0; char_idx < MAX_SUPPORTED_ESCAPE_CHARS; ++char_idx) {
      if (chars == nullptr || chars->Contains(*(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]))) {
        result.Replace(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0], CHAR_TO_ESCAPE_SEQ_MAP[char_idx][1]);
      }
    }
    return result;
  }
  return *this;
}

UString UString::ReplaceEscapedCharWithChar(const Array<UNICHAR>* chars) const {
  if (Len() > 0 && (chars == nullptr || chars->Count() > 0)) {
    UString result(*this);
    for (int32 char_idx = MAX_SUPPORTED_ESCAPE_CHARS-1; char_idx >= 0; --char_idx) {
      if (chars == nullptr || chars->Contains(*(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]))) {
        result.Replace(CHAR_TO_ESCAPE_SEQ_MAP[char_idx][1], CHAR_TO_ESCAPE_SEQ_MAP[char_idx][0]);
      }
    }
    return result;
  }
  return *this;
}

UString UString::ConvertTabsToSpaces(int32 spaces_per_tab) const {
  fun_check(spaces_per_tab > 0);

  UString result(*this);
  int32 tab_pos;
  while ((tab_pos = result.IndexOf(UTEXT("\t"))) != INVALID_INDEX) {
    UString left = result.Left(tab_pos);
    UString right = result.Mid(tab_pos + 1);

    result = left;
    int32 line_begin = left.LastIndexOf(UTEXT("\n"));
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

UString UString::FromRawData(const UNICHAR* raw, int32 len) {
  if (len < 0) {
    len = 0;
  }

  UStringData* new_data;
  if (raw == nullptr || len == 0) {
    new_data = UStringData::SharedEmpty();
  } else {
    new_data = UStringData::FromRawData(raw, len);
  }

  UStringDataPtr data_ptr{ new_data };
  return UString(data_ptr); //without ref increament
}

UString& UString::SetRawData(const UNICHAR* raw, int32 len) {
  // 공유중이거나, 할당된 내용이 있다면 바로 처리안됨.
  if (data_->ref.IsShared() || data_->alloc > 0) {
    *this = FromRawData(raw, len);
  } else {
    if (raw) {
      if (len < 0) {
        len = 0;
      }

      data_->length = len;
      data_->offset = raw - reinterpret_cast<UNICHAR*>(data_);
    } else {
      data_->offset = sizeof(UntypedSharableArrayData);
      data_->length = 0;
      data_->MutableData()[0] = '\0'; // nul-terminated
    }
  }
  return *this;
}


// Explicit Conversions

// Convert from

UStringData* UString::FromASCII_helper(const char* str, int32 len) {
  UStringData* new_data;
  if (str == nullptr || len == 0 || (!*str && len < 0)) {
    new_data = UStringData::SharedEmpty();
  } else {
    if (len < 0) {
      len = CStringTraitsA::Strlen(str);
    }
    new_data = UStringData::Allocate(len + 1);
    new_data->length = len;
    NulTerm(new_data);

    StringCopyFromASCII(new_data->MutableData(), str, len);
  }
  return new_data;
}

UStringData* UString::FromUNICHARArray_helper(const UNICHAR* str, int32 len) {
  UString s(str, len);
  s.data_->ref.AddRef();
  return s.data_;
}

UStringData* UString::FromAsciiOrUtf8_helper(const char* str, int32 len) {
  UString s = FromUtf8(str, len);
  s.data_->ref.AddRef();
  return s.data_;
}

UString UString::FromUTF8_helper(const char* str, int32 len) {
  if (str == nullptr) {
    return UString();
  }

  fun_check(len != -1);

  const int32 utf16_len = Utf::LengthAsUtf16(str, len);

  UString result(utf16_len, NoInit);
  UNICHAR* dst = result.MutableData();
  Utf::Convert(str, len, dst, utf16_len);

  return result;
}

//TODO 로컬 인코딩을 고려하는 기능이 필요할수 있으므로, 반듯이 구현하도록 하자.
UString UString::FromLocal8Bit_helper(const char* str, int32 len) {
  if (str == nullptr || len == 0 || (!*str || len < 0)) {
    return UString();
  }
//TODO
//#if !NO_TEXTCODEC
//  if (len < 0) len = CStringTraitsA::Strlen(str);
//  CTextCodec* codec = CTextCodec::CodecForLocal();
//  if (codec) {
//    return codec->ToUnicode(str, len);
//  }
//#endif

  return FromAscii(str, len);
}

UString UString::FromAscii(const char* str, int32 len) {
  UStringDataPtr data_ptr = { FromASCII_helper(str, (str && len < 0) ? int32(CStringTraitsA::Strlen(str)) : len) };
  return UString(data_ptr);
}

UString UString::FromAscii(const ByteString& str) {
  return FromAscii(str.ConstData(), str.NulTermLen());
}

UString UString::FromAscii(ByteStringRef str) {
  return FromAscii(str.ConstData(), str.Len());
}

UString UString::FromLocal8Bit(const char* str, int32 len) {
  return FromLocal8Bit_helper(str, (str && len < 0) ? int32(CStringTraitsA::Strlen(str)) : len);
}

UString UString::FromLocal8Bit(const ByteString& str) {
  return FromLocal8Bit(str.ConstData(), str.NulTermLen());
}

UString UString::FromLocal8Bit(ByteStringRef str) {
  return FromLocal8Bit(str.ConstData(), str.Len());
}

UString UString::FromStdString(const std::string& str) {
  return FromUtf8(str.data(), int32(str.size()));
}

UString UString::FromStdU16String(const std::u16string& str) {
  return FromUtf16(str.data(), int32(str.size()));
}

UString UString::FromStdU32String(const std::u32string& str) {
  return FromUtf32(str.data(), int32(str.size()));
}

UString UString::FromStdWString(const std::wstring& str) {
  return FromWCharArray(str.data(), int32(str.size()));
}

UString UString::FromUtf32(const uint32* unicode, int32 len) {
  if (unicode == nullptr || len == 0) {
    return UString();
  }

  if (len < 0) {
    len = 0;
    while (unicode[len]) ++len;
  }

  const int32 utf16_len = Utf::LengthAsUtf16(unicode, len);
  UString result(utf16_len, NoInit);
  Utf::Convert(unicode, len, result.MutableData(), utf16_len);
  return result;
}

UString UString::FromUtf32(const char32_t* unicode, int32 len) {
  return FromUtf32(reinterpret_cast<const uint32*>(unicode), len);
}

UString UString::FromUtf8(const char* str, int32 len) {
  return FromUTF8_helper(str, (str && len == -1) ? int32(CStringTraitsA::Strlen(str)) : len);
}

UString UString::FromUtf8(const ByteString& str) {
  return str.IsEmpty() ? UString() : FromUtf8(str.ConstData(), str.NulTermLen());
}

UString UString::FromUtf8(ByteStringRef str) {
  return str.IsEmpty() ? UString() : FromUtf8(str.ConstData(), str.Len());
}

UString UString::FromUtf16(const uint16* unicode, int32 len) {
  static_assert(sizeof(UNICHAR) == sizeof(uint16), "sizeof(UNICHAR) must be equal to sizeof(uint16)"); // 이 가정이 현재는 문제가 없지만, wchar_t가 4바이트인 unix* 계열에서는 문제가 될수 있음.  차후 인코딩 체계 재정립해야함.
  UString result(len, NoInit);
  StringCopy(result.MutableData(), (const UNICHAR*)unicode, len);
  return result;
}

UString UString::FromUtf16(const char16_t* unicode, int32 len) {
  return FromUtf16(reinterpret_cast<const uint16*>(unicode), len);
}

UString UString::FromWCharArray(const wchar_t* str, int32 len) {
  return  sizeof(wchar_t) == sizeof(UNICHAR) ?
          FromUtf16(reinterpret_cast<const uint16*>(str), len) :
          FromUtf32(reinterpret_cast<const uint32*>(str), len);
}


namespace {
//static UString UString::FromCFString(CFStringRef str);
//static UString UString::FromNSString(const NSString* str);

ByteString ConverToAscii(UStringView str) {
  if (str.IsEmpty()) {
    return ByteString();
  }

  ByteString result(str.Len(), NoInit);
  StringToAscii(reinterpret_cast<uint8*>(const_cast<char*>(result.ConstData())),
                reinterpret_cast<const uint16*>(str.ConstData()), str.Len());
  return result;
}

ByteString ConverToUtf8(UStringView str) {
  if (str.IsEmpty()) {
    return ByteString();
  }

  const int32 utf8_len = Utf::LengthAsUtf8(str.ConstData(), str.Len());
  ByteString result(utf8_len, NoInit);
  Utf::Convert(str.ConstData(), str.Len(), result.MutableData(), result.Len());
  return result;
}

Array<uint32> ConverToUtf32(UStringView str) {
  const int32 utf32_len = Utf::LengthAsUtf32(str.ConstData(), str.Len());

  //warning FUN_ALWAYS_INLINE array를 move하면 안되겠지...??
  Array<uint32> result(utf32_len, NoInit);
  Utf::Convert(str.ConstData(), str.Len(), result.MutableData(), result.Count());
  return result;
}

ByteString ConverToLocal8Bit(UStringView str) {
  if (str.IsEmpty()) {
    return ByteString();
  }

  //TODO 반듯이 지원해야함!
  //TODO 반듯이 지원해야함!
  //TODO 반듯이 지원해야함!
  //TODO 반듯이 지원해야함!
  //TODO 반듯이 지원해야함!
  //TODO 반듯이 지원해야함!

//TODO
//#if !NO_TEXTCODEC
//  CTextCodec* codec = CTextCodec::CodecForLocale();
//  if (codec) {
//    return codec->FromUnicode(str);
//  }
//#endif

  return ConverToAscii(str);
}

} // namespace

ByteString UString::ToAscii_helper(UStringView str) {
  return ConverToAscii(str);
}

ByteString UString::ToAscii_helper_inplace(UString& str) {
  if (!str.IsDetached()) {
    return ConverToAscii(str);
  }

  const uint16* data = reinterpret_cast<const uint16*>(str.ConstData());
  const int32 len = str.Len();

  UntypedSharableArrayData* dest_data = str.data_;
  dest_data->alloc *= sizeof(uint16);

  // reset ourselves to UString()
  str.data_ = UString().data_;

  // do the in-place conversion
  uint8* dst = reinterpret_cast<uint8*>(dest_data->MutableData());
  StringToAscii(dst, data, len);
  dst[len] = '\0';

  return ByteString(ByteStringDataPtr{(ByteStringData*)dest_data});
}

ByteString UString::ToUtf8_helper(UStringView str) {
  return ConverToUtf8(str);
}

ByteString UString::ToLocal8Bit_helper(UStringView str) {
  return ConverToLocal8Bit(str);
}

//FIXME 문제가 있어보임.
//CUTF를 통해서 정상적으로 처리해주는게 바람직할것으로 보임.
int32 UString::ToUtf32_helper(UStringView str, uint32* out) {
  uint32* out_begin = out;

  //TODO UStringIterator (qstringiterator_p.h 참조)
  // surrogate 문자 때문에 군현을 해야할것 같다.
  // 아니면 여기서 생자 코드로 적용해도 될듯 하긴한데....
  const UNICHAR* p = str.cbegin();
  const UNICHAR* e = str.cend();
  for (; p != e; ) {
    *out++ = *p++;
  }
  return out - out_begin;
}

ByteString UString::ToAscii() const {
  return ConverToAscii(UStringView(*this));
}

ByteString UString::ToLocal8Bit() const {
  return IsEmpty() ? ByteString() : ToLocal8Bit_helper(UStringView(*this));
}

ByteString UString::ToUtf8() const {
  return ConverToUtf8(UStringView(*this));
}

std::string UString::ToStdString() const {
  return ToUtf8().ToStdString();
}

std::u16string UString::ToStdU16String() const {
  return std::u16string(reinterpret_cast<const char16_t*>(ToUtf16()), Len());
}

std::u32string UString::ToStdU32String() const {
  //TODO surrogate pair가 있을 경우에는 길이가 다를 수 있으므로 아래의 함수는 문제가 있을수 있음.
  //-> 길이가 줄어들기만 하므로, 문제될건 없음.
  std::u32string ut32str(Len(), char32_t(0));
  const int32 utf32_len = ToUtf32_helper(UStringView(*this), reinterpret_cast<uint32*>(&ut32str[0]));
  ut32str.resize(utf32_len);
  return ut32str;
}

std::wstring UString::ToStdWString() const
{
  if (IsEmpty()) {
    return std::wstring();
  } else {
    std::wstring wstr;
    wstr.resize(Len());
    wstr.resize(ToWCharArray(&(*wstr.begin())));
    return wstr;
  }
}

Array<uint32> UString::ToUtf32() const {
  return ConverToUtf32(*this);
}

//wchar_t가 32비트인 경우에는, 길이가 줄어들수는 있어도 늘어나지는 않으므로,
//문자열의 길이를 그대로 사용해도 되며, 반환값에 돌려진 길이가 실제길이임.
int32 UString::ToWCharArray(wchar_t* array) const {
  if (sizeof(wchar_t) == sizeof(UNICHAR)) {
    StringCopy(array, data_->ConstData(), Len());
    return Len();
  } else {
    return ToUtf32_helper(UStringView(*this), reinterpret_cast<uint32*>(array));
  }
}

const UNICHAR* UString::Unicode() const {
  return ConstData();
}

const uint16* UString::ToUtf16() const {
  //TODO TCHAR이 궁극적으로는 char16_t로 하거나, uint16으로 해야함!!
  //UNI16CHAR
  static_assert(sizeof(UNICHAR) == sizeof(uint16), "sizeof(UNICHAR) == sizeof(uint16)");
  return reinterpret_cast<const uint16*>(ConstData());
}

//CFStringRef UString::ToCFString() const;
//NSString* UString::ToNSString() const;


UString UString::FromNumber(int16 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(uint16 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(int32 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(uint32 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(int64 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(uint64 value, int32 base) {
  UString result;
  result.SetNumber(value, base);
  return result;
}

UString UString::FromNumber(float value, UNICHAR f, int32 precision) {
  UString result;
  result.SetNumber(value, f, precision);
  return result;
}

UString UString::FromNumber(double value, UNICHAR f, int32 precision) {
  UString result;
  result.SetNumber(value, f, precision);
  return result;
}

//std::string UString::ToStdString() const {
//  return std::string(ConstData(), Len()); // 길이를 가지고 가므로, nul-term 일 필요는 없음.
//}
//
//UString UString::FromStdString(const std::string& std_string) {
//  return UString(std_string.data(), int32(std_string.size())); // 길이를 가지고 가므로, 중간에 nul 문자가 있어도 문제 없음.
//}

uint32 HashOf(const UString& str) {
  return Crc::StringCrc32(str.ConstData(), str.Len());
}

Archive& operator & (Archive& ar, UString& a) {
  //TODO
  fun_check(0);

  /*
  // > 0 for ANSICHAR, < 0 for UCS2CHAR serialization

  if (ar.IsLoading()) {
    int32 save_count;
    ar & save_count;

    const bool load_ucs2_char = save_count < 0;
    if (load_ucs2_char) {
      save_count = -save_count;
    }

    // If save_count is still less than 0, they must have passed in MIN_INT. Archive is corrupted.
    //if (save_count < 0)
    //{
    //  ar.ArIsError = 1;
    //  ar.ArIsCriticalError = 1;
    //  fun_log(LogNetSerialization, Error, UTEXT("Archive is corrupted"));
    //  return ar;
    //}

    const auto max_serialize_size = ar.GetMaxSerializeSize();
    // Protect against network packets allocating too much memory
    if ((max_serialize_size > 0) && (save_count > max_serialize_size)) {
      ar.ArIsError = 1;
      ar.ArIsCriticalError = 1;
      fun_log(LogNetSerialization, Error, UTEXT("str is too large"));
      return ar;
    }

    // Resize the array only if it passes the above tests to prevent rogue packets from crashing
    //A.data_.Clear(save_count);
    //A.data_.AddUninitialized(save_count);
    a.ResizeUninitialized(save_count);

    if (save_count > 0) {
      if (load_ucs2_char) {
        // read in the unicode string and byteswap it, etc
        auto passthru = StringMemoryPassthru<UCS2CHAR>(A.MutableData(), save_count, save_count);
        ar.Serialize(passthru.Get(), save_count * sizeof(UCS2CHAR));
        // Ensure the string has a null terminator
        passthru.Get()[save_count-1] = '\0';
        passthru.Apply();

        //INTEL_ORDER_TCHARARRAY(a.MutableData())
        ByteOrder::FromLittleEndian(A.MutableData(), save_count);

        // Since Microsoft's vsnwprintf implementation raises an invalid parameter warning
        // with a ch of 0xFFFF, scan for it and terminate the string there.
        // 0xFFFF isn't an actual unicode ch anyway.
        int32 index = 0;
        if ((index = A.IndexOf(UNICHAR(0xFFFF))) != INVALID_INDEX) {
          a[index] = '\0';
          a.TrimToNulTerminator();
        }
      }
      else {
        auto passthru = StringMemoryPassthru<char>(A.MutableData(), save_count, save_count);
        ar.Serialize(passthru.Get(), save_count * sizeof(char));
        // Ensure the string has a null terminator
        passthru.Get()[save_count-1] = '\0';
        passthru.Apply();
      }

      // Throw away empty string.
      if (save_count == 1) {
        a.Clear();
      }
    }
  }
  else {
    const bool save_ucs2_char = ar.IsForcingUnicode() || !CharTraitsU::IsPureAnsi(*A); //TODO null-term이 아니라면 문제가 발생할 수 있음.
    const int32 count = a.Len() + 1; //NUL을 포함해서 저장하는데...???
    int32 save_count = save_ucs2_char ? -count : count;

    ar & save_count;

    //TODO
    //A.data_.CountBytes(ar);

    //FIXME
    //공유되고 있는 경우에는 거짓 보고를 하는 상황이 되는데,
    //이를 어쩐다...
    //ar.CountBytes(ArrayNum * sizeof(ElementType), ArrayMax * sizeof(ElementType));

    //TODO 공유중이 아닌것만 카운팅.
    ar.CountBytes(A.data_->length * sizeof(UNICHAR), a.data_->alloc * sizeof(UNICHAR));

    if (count > 0) {
      if (save_ucs2_char) {
        // TODO - This is creating a temporary in order to byte-swap.  Need to think about how to make this not necessary.
      #if !FUN_ARCH_LITTLE_ENDIAN
        //TODO 이게 뭐냐???
        UString ATemp = A;
        UString& A = ATemp;
        INTEL_ORDER_TCHARARRAY(a.MutableData());
      #endif

        ar.Serialize((void*)StringCast<UCS2CHAR>(A.ConstData(), count).Get(), sizeof(UCS2CHAR) * count);
      }
      else {
        ar.Serialize((void*)StringCast<ANSICHAR>(A.ConstData(), count).Get(), sizeof(ANSICHAR) * count);
      }
    }
  }
  */

  return ar;
}

void UString::SerializeAsASCIICharArray(Archive& ar, int32 min_characters) const {
  //TODO
  /*
  int32 len = MathBase::Max(Len(), min_characters);
  ar & len;

  for (int32 char_idx = 0; char_idx < Len(); char_idx++) {
    ANSICHAR ansi_char = CharCast<ANSICHAR>((*this)[char_idx]);
    ar & ansi_char;
  }

  // Zero pad till minimum number of characters are written.
  for (int32 i = Len(); i < len; ++i) {
    ANSICHAR null_char = 0;
    ar & null_char;
  }
  */
}

//TODO
/*
// This starting size catches 99.97% of printf calls - there are about 700k printf calls per level
#define STARTING_BUFFER_SIZE  512

//const char*도 지원하는게 좋으려나??
VARARG_BODY(UString, UString::Format, const UNICHAR*, VARARG_NONE)
{
  int32 buffer_size = STARTING_BUFFER_SIZE;
  UNICHAR starting_buffer[STARTING_BUFFER_SIZE];
  UNICHAR* buffer = starting_buffer;
  int32 result = -1;

  // First try to print to a stack allocated location
  GET_VARARGS_RESULT_WIDE(buffer, buffer_size, buffer_size - 1, fmt, fmt, result);

  // If that fails, start allocating regular memory
  if (result == -1) {
    buffer = nullptr;
    while (result == -1) {
      buffer_size *= 2;
      buffer = (UNICHAR*)UnsafeMemory::Realloc(buffer, buffer_size * sizeof(UNICHAR));
      GET_VARARGS_RESULT_WIDE(buffer, buffer_size, buffer_size - 1, fmt, fmt, result);
    };
  }

  UString result_string(buffer, result);

  if (buffer_size != STARTING_BUFFER_SIZE) {
    UnsafeMemory::Free(buffer);
  }

  return result_string;
}
*/


/*! \class UStringRef
  \brief UStringRef
*/

void UStringRef::CheckInvariants() {
#ifdef DO_CHECK
  fun_check(string_ == nullptr || position_ >= 0);
  fun_check(string_ == nullptr || position_ < string_->Len());
  fun_check(string_ == nullptr || length_ >= 0);
  fun_check(string_ == nullptr || (position_ + length_) <= string_->Len());
#endif
}

UStringRef::UStringRef()
  : string_(nullptr), position_(0), length_(0) {}

UStringRef::UStringRef(const UString* str, int32 pos, int32 len)
  : string_(str), position_(pos), length_(len) {
  CheckInvariants();
}

UStringRef::UStringRef(const UString* str)
  : string_(str), position_(0), length_(str ? str->Len() : 0) {
  CheckInvariants();
}

UStringRef::UStringRef(const UStringRef& rhs)
  : string_(rhs.string_), position_(rhs.position_), length_(rhs.length_) {
  CheckInvariants();
}

UStringRef& UStringRef::operator = (const UStringRef& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    string_ = rhs.string_;
    position_ = rhs.position_;
    length_ = rhs.length_;
    CheckInvariants();
  }
  return *this;
}

UStringRef::UStringRef(UStringRef&& rhs)
  : string_(rhs.string_), position_(rhs.position_), length_(rhs.length_) {
  rhs.string_ = nullptr;
  rhs.position_ = 0;
  rhs.length_ = 0;
  CheckInvariants();
}

UStringRef& UStringRef::operator = (UStringRef&& rhs) {
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

UStringRef& UStringRef::operator = (const UString* str) {
  string_ = str;
  position_ = 0;
  length_ = str ? str->Len() : 0;
  CheckInvariants();
  return *this;
}

void UStringRef::Swap(UStringRef& rhs) {
  fun::Swap(string_, rhs.string_);
  fun::Swap(position_, rhs.position_);
  fun::Swap(length_, rhs.length_);
}

const UString* UStringRef::Str() const {
  return string_;
}

int32 UStringRef::Position() const {
  return position_;
}

int32 UStringRef::Len() const {
  return length_;
}

bool UStringRef::IsEmpty() const {
  return length_ == 0;
}

void UStringRef::Clear() {
  string_ = nullptr;
  position_ = 0;
  length_ = 0;
}

bool UStringRef::IsNull() const {
  return string_ == nullptr;
}

const UNICHAR* UStringRef::operator * () const {
  return ConstData();
}

const UNICHAR* UStringRef::ConstData() const {
  return string_->ConstData() + position_;
}

UString UStringRef::ToString() const {
  return string_ ? UString(string_->ConstData() + position_, length_) : UString();
}

UStringRef UStringRef::AppendTo(UString* str) const {
  if (str == nullptr) {
    return UStringRef();
  }

  const int32 pos = str->Len();
  str->Insert(pos, ConstData(), Len());
  return UStringRef(str, pos, Len());
}

UNICHAR UStringRef::At(int32 index) const {
  fun_check(string_);
  fun_check(uint32(index) < uint32(Len()));
  return string_->At(position_ + index);
}

UNICHAR UStringRef::operator[] (int32 index) const {
  fun_check(string_);
  fun_check(uint32(index) < uint32(Len()));
  return string_->At(index);
}

UNICHAR UStringRef::First() const {
  return At(0);
}

UNICHAR UStringRef::Last() const {
  return At(Len() - 1);
}

UNICHAR UStringRef::FirstOr(const UNICHAR def) const {
  return Len() ? First() : def;
}

UNICHAR UStringRef::LastOr(const UNICHAR def) const {
  return Len() ? Last() : def;
}

int32 UStringRef::IndexOfAny(const Array<UNICHAR>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  return IndexOfAny(UStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index);
}

int32 UStringRef::LastIndexOfAny(const Array<UNICHAR>& chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  return LastIndexOfAny(UStringView(chars.ConstData(),chars.Count()), casesense, from, matched_index);
}

int32 UStringRef::IndexOf(UStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (matched_len) {
    *matched_len = sub.Len();
  }

  if (sub.IsEmpty()) {
    return from;
  }

  if (sub.Len() == 1) { // one letter searching.
    return IndexOf(*sub.ConstData(), casesense, from);
  }

  const int32 this_len = Len();
  if (from > this_len || (sub.Len() + from) > this_len) {
    return INVALID_INDEX;
  }

  return UStringMatcher::FastFind(ConstData(), this_len, from, sub.ConstData(), sub.Len(), casesense);
}

int32 UStringRef::IndexOf(AsciiString sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  return IndexOf(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense, from, matched_len);
}

int32 UStringRef::LastIndexOf(UStringView sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  if (sub.Len() == 1) { // one letter
    return LastIndexOf(*sub.ConstData(), casesense, from, matched_len);
  }

  if (matched_len) {
    *matched_len = sub.Len();
  }
  return UStringMatcher::FastLastFind(ConstData(), Len(), from, sub.ConstData(), sub.Len(), casesense);
}

int32 UStringRef::LastIndexOf(AsciiString sub, CaseSensitivity casesense, int32 from, int32* matched_len) const {
  return LastIndexOf(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense, from, matched_len);
}

int32 UStringRef::IndexOfAny(const Array<const UNICHAR*>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<UString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<UStringRef>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<UStringView>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<AsciiString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }

int32 UStringRef::IndexOfAny(std::initializer_list<const UNICHAR*> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<UString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<UStringRef> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<UStringView> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<AsciiString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 UStringRef::IndexOfAny(const Array<const char*>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<ByteString>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<ByteStringRef>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(const Array<ByteStringView>& strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.ConstData(), strings.Count(), casesense, from, matched_index, matched_len); }

int32 UStringRef::IndexOfAny(std::initializer_list<const char*> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<ByteString> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<ByteStringRef> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
int32 UStringRef::IndexOfAny(std::initializer_list<ByteStringView> strings, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const
{ return IndexOfAnyStrings_helper(*this, strings.begin(), strings.size(), casesense, from, matched_index, matched_len); }
#endif

int32 UStringRef::Count(UStringView sub, CaseSensitivity casesense) const {
  int32 n = 0, pos = -int32(sub.Len());
  if (Len() > 500 && sub.Len() > 5) {
    UStringMatcher matcher(sub, casesense);
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

int32 UStringRef::Count(AsciiString sub, CaseSensitivity casesense) const {
  return Count(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense);
}

UStringRef UStringRef::Left(int32 len) const {
  return uint32(len) < uint32(length_) ? UStringRef(string_, position_, len) : *this;
}

UStringRef UStringRef::Mid(int32 offset, int32 len) const {
  switch (SlicingHelper::Slice(length_, &offset, &len)) {
    case SlicingHelper::Null:
      return UStringRef();
    case SlicingHelper::Empty:
      return UStringRef(string_, 0, 0);
    case SlicingHelper::Full:
      return *this;
    case SlicingHelper::Subset:
      return UStringRef(string_, offset + position_, len);
  }
  //unreachable to here
  return UStringRef();
}

UStringRef UStringRef::Right(int32 len) const {
  return uint32(len) < uint32(length_) ? UStringRef(string_, position_ + length_ - len, len) : *this;
}

UStringRef UStringRef::LeftChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Left(Len() - len);
}

UStringRef UStringRef::RightChopped(int32 len) const {
  fun_check(uint32(len) <= uint32(Len()));
  return Right(Len() - len);
}

bool UStringRef::StartsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UStringRef::StartsWith(AsciiString sub, CaseSensitivity casesense) const {
  return StartsWith(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense);
}

bool UStringRef::EndsWith(UStringView sub, CaseSensitivity casesense) const {
  return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense);
}

bool UStringRef::EndsWith(AsciiString sub, CaseSensitivity casesense) const {
  return EndsWith(ASCII_TO_UNICHAR<>(sub).ToUStringView(), casesense);
}

bool UStringRef::GlobMatch(UStringView pattern, CaseSensitivity casesense) const {
  //TODO
  fun_check(0);
  return false;

  //return GlobU::Match(cbegin(), cend(), pattern.cbegin(), pattern.cend(), casesense);
}

bool UStringRef::GlobMatch(AsciiString pattern, CaseSensitivity casesense) const {
  return GlobMatch(ASCII_TO_UNICHAR<>(pattern).ToUStringView(), casesense);
}

UStringRef& UStringRef::Truncate(int32 pos) {
  fun_check(pos >= 0);

  if (pos < length_) {
    length_ = pos;
  }
  return *this;
}

UStringRef& UStringRef::LeftChop(int32 len) {
  fun_check(len >= 0);

  if (len > 0) {
    length_ = length_ - len;
  }
  return *this;
}

UStringRef& UStringRef::RightChop(int32 len) {
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

UStringRef& UStringRef::TrimLeft() {
  if (string_) {
    const UNICHAR* abs_begin = string_->ConstData();
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    length_ = StringAlgo<UString>::LeftTrimmedPositions(local_begin, local_end);
    position_ = local_begin - abs_begin;
  }
  return *this;
}

UStringRef& UStringRef::TrimRight() {
  if (string_) {
    const UNICHAR* abs_begin = string_->ConstData();
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    length_ = StringAlgo<UString>::RightTrimmedPositions(local_begin, local_end); // 오른쪽 trimming이므로, 길이만 변경됨.
  }
  return *this;
}

UStringRef& UStringRef::Trim() {
  if (string_) {
    const UNICHAR* abs_begin = string_->ConstData();
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    length_ = StringAlgo<UString>::TrimmedPositions(local_begin, local_end);
    position_ = local_begin - abs_begin;
  }
  return *this;
}

UStringRef UStringRef::TrimmedLeft() const {
  if (string_) {
    const UNICHAR* abs_begin = string_->ConstData();
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    StringAlgo<UString>::LeftTrimmedPositions(local_begin, local_end);
    return UStringRef(string_, local_begin - abs_begin, local_end - local_begin);
  } else {
    return *this;
  }
}

UStringRef UStringRef::TrimmedRight() const {
  if (string_) {
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    StringAlgo<UString>::RightTrimmedPositions(local_begin, local_end);
    return UStringRef(string_, position_, local_end - local_begin); // 오른쪽 trimming이므로, 길이만 변경됨.
  } else {
    return *this;
  }
}

UStringRef UStringRef::Trimmed() const {
  if (string_) {
    const UNICHAR* abs_begin = string_->ConstData();
    const UNICHAR* local_begin = ConstData();
    const UNICHAR* local_end = ConstData() + length_;
    StringAlgo<UString>::TrimmedPositions(local_begin, local_end);
    return UStringRef(string_, local_begin - abs_begin, local_end - local_begin);
  } else {
    return *this;
  }
}

int32 UStringRef::LeftSpaces() const {
  return StringAlgo<UString>::LeftSpaces(cbegin(), cend());
}

int32 UStringRef::RightSpaces() const {
  return StringAlgo<UString>::RightSpaces(cbegin(), cend());
}

int32 UStringRef::SideSpaces() const {
  return StringAlgo<UString>::SideSpaces(cbegin(), cend());
}

Array<UStringRef> UStringRef::Split(UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separator, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::Split(const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::Split(std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::SplitByWhitespaces(UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitByWhitespaces(list, extra_separator, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::SplitLines(StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; SplitLines(list, split_options, casesense); return list; }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
Array<UStringRef> UStringRef::Split(ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::Split(const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }

Array<UStringRef> UStringRef::Split(std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
Array<UStringRef> UStringRef::Split(std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ Array<UStringRef> list; Split(list, separators, max_splits, split_options, casesense); return list; }
#endif

int32 UStringRef::Split(Array<UStringRef>& list, UNICHAR separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, UStringView(&separator, 1), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<UNICHAR>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, UStringView(separators.ConstData(),separators.Count()), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<UNICHAR> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, UStringView(separators.begin(),separators.size()), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, UStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, separators, max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, AsciiString separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, ASCII_TO_UNICHAR<>(separators).ToUStringView(), max_splits, split_options, casesense); }

int32 UStringRef::Split(Array<UStringRef>& list, const Array<const UNICHAR*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,const UNICHAR*>(list, *this, &UStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<UString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UString>(list, *this, &UStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<UStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UStringRef>(list, *this, &UStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<UStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<AsciiString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,AsciiString>(list, *this, &UStringRef::Mid, separators.ConstData(), separators.Count(), max_splits, split_options, casesense); }

int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<const UNICHAR*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,const UNICHAR*>(list, *this, &UStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<UString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UString>(list, *this, &UStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<UStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UStringRef>(list, *this, &UStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<UStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,UStringView>(list, *this, &UStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<AsciiString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,AsciiString>(list, *this, &UStringRef::Mid, separators.begin(), separators.size(), max_splits, split_options, casesense); }

int32 UStringRef::SplitByWhitespaces(Array<UStringRef>& list, UString extra_separator, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const {
  if (extra_separator.IsEmpty()) {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(u"\n") };
    return Split(list, separators, max_splits, split_options, casesense);
  } else {
    auto separators = { UStringView(u" "), UStringView(u"\t"), UStringView(u"\r"), UStringView(extra_separator) };
    return Split(list, separators, max_splits, split_options, casesense);
  }
}

int32 UStringRef::SplitLines(Array<UStringRef>& out_lines, StringSplitOptions split_options, CaseSensitivity casesense) const {
  auto separators = { AsciiString("\r\n"), AsciiString("\n"), AsciiString("\r") };
  return Split(out_lines, separators, 0, split_options, casesense);
}

#if FUN_USE_IMPLICIT_STRING_CONVERSION
int32 UStringRef::Split(Array<UStringRef>& list, ByteStringView separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByCharacters<UStringRef,UStringRef,ByteStringView>(list, *this, &UStringRef::Mid, separators, max_splits, split_options, casesense); }

int32 UStringRef::Split(Array<UStringRef>& list, const Array<const char*>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,const char*>(list, *this, &UStringRef::Mid, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<ByteString>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteString>(list, *this, &UStringRef::Mid, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<ByteStringRef>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteStringRef>(list, *this, &UStringRef::Mid, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, const Array<ByteStringView>& separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteStringView>(list, *this, &UStringRef::Mid, separators.ConstData(),separators.Count(), max_splits, split_options, casesense); }

int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<const char*> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,const char*>(list, *this, &UStringRef::Mid, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<ByteString> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteString>(list, *this, &UStringRef::Mid, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<ByteStringRef> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteStringRef>(list, *this, &UStringRef::Mid, separators.begin(),separators.size(), max_splits, split_options, casesense); }
int32 UStringRef::Split(Array<UStringRef>& list, std::initializer_list<ByteStringView> separators, int32 max_splits, StringSplitOptions split_options, CaseSensitivity casesense) const
{ return SplitByStrings<UStringRef,UStringRef,ByteStringView>(list, *this, &UStringRef::Mid, separators.begin(),separators.size(), max_splits, split_options, casesense); }
#endif

bool UStringRef::Divide(UStringView delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UStringRef::Divide(AsciiString delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UStringRef::LastDivide(UStringView delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
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

bool UStringRef::LastDivide(AsciiString delim, UStringRef* out_left, UStringRef* out_right, bool trimming, CaseSensitivity casesense) const {
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

int16 UStringRef::ToInt16(bool* ok, int32 base) const {
  return ToIntegral<int16>(ConstData(), Len(), ok, base);
}

uint16 UStringRef::ToUInt16(bool* ok, int32 base) const {
  return ToIntegral<uint16>(ConstData(), Len(), ok, base);
}

int32 UStringRef::ToInt32(bool* ok, int32 base) const {
  return ToIntegral<int32>(ConstData(), Len(), ok, base);
}

uint32 UStringRef::ToUInt32(bool* ok, int32 base) const {
  return ToIntegral<uint32>(ConstData(), Len(), ok, base);
}

int64 UStringRef::ToInt64(bool* ok, int32 base) const {
  return ToIntegral<int64>(ConstData(), Len(), ok, base);
}

uint64 UStringRef::ToUInt64(bool* ok, int32 base) const {
  return ToIntegral<uint64>(ConstData(), Len(), ok, base);
}

float UStringRef::ToFloat(bool* ok) const {
  return LocaleData::ConvertDoubleToFloat(ToDouble(ok), ok);
}

double UStringRef::ToDouble(bool* ok) const {
  return LocaleData::C()->StringToDouble(ConstData(), Len(), ok, Locale::RejectGroupSeparator);
}

bool UStringRef::IsNumeric() const {
  return !IsEmpty() && CStringTraitsU::IsNumeric(cbegin(), cend());
}

bool UStringRef::IsIdentifier() const {
  if (IsEmpty()) {
    return false;
  }

  const UNICHAR* p = ConstData();

  if (!(CharTraitsU::IsAlpha(p[0]) || p[0] == '_')) { // leading alpha or '_'
    return false;
  }

  for (int32 i = 1; i < Len(); ++i) {
    if (!(CharTraitsU::IsAlnum(p[i]) || p[i] == '_')) {
      return false;
    }
  }

  return true;
}

bool UStringRef::IsQuoted() const {
  return UString::IsQuoted(*this);
}

UStringRef UStringRef::Unquoted(bool* out_quotes_removed) const {
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

UStringRef& UStringRef::Unquotes(bool* out_quotes_removed) {
  return (*this = Unquoted(out_quotes_removed));
}

uint32 HashOf(const UStringRef& str) {
  return Crc::StringCrc32(str.ConstData(), str.Len());
}

int32 CullArray(Array<UString>* array, bool trimming) {
  fun_check_ptr(array);

  if (trimming) {
    for (int32 i = 0; i < array->Count(); ++i) {
      (*array)[i].Trim();
    }
  }

  UString empty;
  array->Remove(empty);
  return array->Count();
}

int32 CullArray(Array<UStringRef>* array, bool trimming) {
  fun_check_ptr(array);

  if (trimming) {
    for (int32 i = 0; i < array->Count(); ++i) {
      (*array)[i].Trim();
    }
  }

  UStringRef empty;
  array->Remove(empty);
  return array->Count();
}

} // namespace fun
