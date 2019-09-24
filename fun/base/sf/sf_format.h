//TODO 지원하지 말자..

#pragma once

#include "fun/base/base.h"
#include "fun/base/sf/sf_format.h"

namespace fun {
namespace sf {

//TODO something...

namespace internal {

template <typename Dest, typename Src>
inline Dest BitCast(const Src& src) {
  static_assert(sizeof(Dest) == sizeof(Src), "size mismatch");
  Dest dest;
  UnsafeMemory::Memcpy(&dest, &src, sizeof(Dest));
  return dest;
}

// An implementation of begin and end for pre-C++11 compilers such as gcc 4.
template <typename C>
constexpr decltype(auto) begin(const C& c) {
  return c.begin();
}

template <typename T, size_t N>
constexpr T* begin(T (&array)[N]) noexcept {
  return array;
}

template <typename C>
constexpr decltype(auto) end(const C& c) {
  return c.end();
}

template <typename T, size_t N>
constexpr T* end(T (&array)[N]) noexcept {
  return array + N;
}

template <typename Result>
struct Function {
  template <typename T>
  struct Result {
    using Type = Result;
  };
};

/*
struct dummy_int {
  int data[2];
  operator int() const { return 0; }
};
typedef std::numeric_limits<internal::dummy_int> fputil;

// Dummy implementations of system functions such as signbit and ecvt called
// if the latter are not available.
inline dummy_int signbit(...) { return dummy_int(); }
inline dummy_int _ecvt_s(...) { return dummy_int(); }
inline dummy_int isinf(...) { return dummy_int(); }
inline dummy_int _finite(...) { return dummy_int(); }
inline dummy_int isnan(...) { return dummy_int(); }
inline dummy_int _isnan(...) { return dummy_int(); }

inline bool use_grisu() {
  return FMT_USE_GRISU && std::numeric_limits<double>::is_iec559;
}

// Formats value using Grisu2 algorithm:
// https://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf
template <typename Double>
FMT_API typename std::enable_if<sizeof(Double) == sizeof(uint64)>::type
  grisu2_format(Double value, char *buffer, size_t &size, char type,
                int precision, bool write_decimal_point);
template <typename Double>
inline typename std::enable_if<sizeof(Double) != sizeof(uint64)>::type
  grisu2_format(Double, char *, size_t &, char, int, bool) {}

template <typename Allocator>
typename Allocator::ValueType *allocate(Allocator& alloc, size_t n) {
#if __cplusplus >= 201103L || FMT_MSC_VER >= 1700
  return std::allocator_traits<Allocator>::allocate(alloc, n);
#else
  return alloc.allocate(n);
#endif
}
*/

// A helper function to suppress bogus "conditional expression is constant"
// warnings.
template <typename T>
inline T ConstCheck(T value) { return value; }

} // namespace internal

/*
namespace std {
// Standard permits specialization of std::numeric_limits. This specialization
// is used to resolve ambiguity between isinf and std::isinf in glibc:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48891
// and the same for isnan and signbit.
template <>
class numeric_limits<fun::sf::internal::dummy_int>
  : public std::numeric_limits<int> {
 public:
  // Portable version of isinf.
  template <typename T>
  static bool isinfinity(T x) {
    using namespace fun::sf::internal;
    // The resolution "priority" is:
    // isinf macro > std::isinf > ::isinf > fun::sf::internal::isinf
    if (const_check(sizeof(isinf(x)) != sizeof(dummy_int))) {
      return isinf(x) != 0;
    }
    return !_finite(static_cast<double>(x));
  }

  // Portable version of isnan.
  template <typename T>
  static bool isnotanumber(T x) {
    using namespace fun::sf::internal;
    if (const_check(sizeof(isnan(x)) != sizeof(fun::sf::internal::dummy_int)))
      return isnan(x) != 0;
    return _isnan(static_cast<double>(x)) != 0;
  }

  // Portable version of signbit.
  static bool isnegative(double x) {
    using namespace fun::sf::internal;
    if (const_check(sizeof(signbit(x)) != sizeof(fun::sf::internal::dummy_int))) {
      return signbit(x) != 0;
    }
    if (x < 0) {
      return true;
    }
    if (!isnotanumber(x)) {
      return false;
    }
    int dec = 0, sign = 0;
    char buffer[2];  // The buffer size must be >= 2 or _ecvt_s will fail.
    _ecvt_s(buffer, sizeof(buffer), x, 0, &dec, &sign);
    return sign != 0;
  }
};
}  // namespace std
*/

template <typename Range>
class BasicWriter;

template <typename OutputIt, typename T = typename OutputIt::ValueType>
class OutputRange {
 private:
  OutputIt it_;

  // Unused yet.
  typedef void Sentinel;
  Sentinel end() const;

 public:
  using Iterator = OutputIt;
  using ValueType = T;

  explicit OutputRange(OutputIt it) : it_(it) {}
  OutputIt begin() const { return it_; }
};

// A range where begin() returns BackInsertIterator.
template <typename Container>
class BackInsertRange
    : public OutputRange<BackInsertIterator<Container>> {
  using Base = OutputRange<BackInsertIterator<Container>>;

 public:
  using ValueType = typename Container::ValueType;

  BackInsertRange(Container& c) : Base(BackInserter(c)) {}
  BackInsertRange(typename Base::Iterator it) : Base(it) {}
};

using Writer = BasicWriter<BackInsertRange<internal::Buffer>;
using UWriter = BasicWriter<BackInsertRange<internal::UBuffer>;

// A formatting error such as invalid format string.
class FormatError : public std::runtime_error {
 public:
  explicit FormatError(const char* message)
    : std::runtime_error(message)
  {
  }

  explicit FormatError(const std::string& message)
    : std::runtime_error(message)
  {
  }
};

namespace internal {

#if FUN_SECURE_SCL
template <typename T>
struct Checked {
  using Type = stdext::checked_array_iterator<T*>;
};

template <typename T>
inline stdext::checked_array_iterator<T*>
MakeChecked(T* p, size_t size) {
  return { p, size };
}
#else
template <typename T>
struct Checked {
  using Type = T*;
};

template <typename T>
inline T* MakeChecked(T* p, size_t size) {
  return p;
}
#endif

template <typename T>
template <typename U>
void BasicBuffer<T>::Append(const U* begin, const U* end) {
  size_t new_size = size_ + internal::ToUnsigned(end - begin);
  Reserve(new_size);

  std::uninitialized_copy(begin, end,
                          internal::MakeChecked(ptr_, capacity_) + size_);

  size_ = new_size;
}

} // namespace internal

// A wrapper around std::locale used to reduce compile times since <locale>
// is very heavy.
class Locale;

class LocaleProvider {
 public:
  virtual ~LocaleProvider() {}
  virtual Locale GetLocale();
};

// The number of characters to store in the basic_memory_buffer object itself
// to avoid dynamic memory allocation.
enum { INLINE_BUFFER_SIZE = 500 };

/**
 * A dynamically growing memory buffer for trivially copyable/constructible types
 * with the first ``SIZE`` elements stored in the object itself.
 *
 * You can use one of the following typedefs for common character types:
 *
 * +----------------+------------------------------+
 * | Type           | Definition                   |
 * +================+==============================+
 * | memory_buffer  | basic_memory_buffer<char>    |
 * +----------------+------------------------------+
 * | wmemory_buffer | basic_memory_buffer<UNICHAR> |
 * +----------------+------------------------------+
 *
 * **Example**::
 *
 *    fun::sf::memory_buffer out;
 *    format_to(out, "The answer is {}.", 42);
 *
 * This will write the following output to the ``out`` object:
 *
 * .. code-block:: none
 *
 *    The answer is 42.
 *
 * The output can be converted to an ``std::string`` with ``to_string(out)``.
 */
template <
      typename T,
      size_t SIZE = INLINE_BUFFER_SIZE,
      typename Allocator std::allocator<T>
    >
class BasicMemoryBuffer
  : private Allocator
  , public internal::BasicBuffer<T> {
 private:
  T store_[SIZE];

  // Deallocate memory allocated by the buffer.
  void Deallocate() {
    T* p = this->MutableData();
    if (p != store_) {
      Allocator::deallocatr(p, this->Capacity());
    }
  }

 protected:
  void Grow(size_t size) override;

 public:
  explicit BasicMemoryBuffer(const Allocator& alloc = Allocator())
    : Allocator(alloc) {
    this->Set(store_, SIZE);
  }

  ~BasicMemoryBuffer() {
    Deallocate();
  }

 private:
  // Move data from other to this buffer.
  void Move(BasicMemoryBuffer& other) {
    Allocator& this_alloc = *this;
    Allocator& other_alloc = other;
    this_alloc = MoveTemp(other_alloc);
    T* p = other.MutableData();
    size_t size = other.Size();
    size_t capacity = other.Capacity();
    if (p == other.store_) {
      this->Set(store_, capacity);
      std::uninitialized_copy(other.store_, other.store_ + size,
                              internal::MakeChecked(store_, capacity));
    }
    else {
      this->Set(p, capacity);
      // Set pointer to the inline array so that delete is not called
      // when deallocating.
      other.Set(other.store_, 0);
    }
    this->Resize(size);
  }

 public:
  /**
   * Constructs a :class:`fun::sf::basic_memory_buffer` object moving the content
   * of the other object to it.
   */
  BasicMemoryBuffer(BasicMemoryBuffer&& other) {
    Move(other);
  }

  /**
   *  Moves the content of the other ``basic_memory_buffer`` object to this one.
   */
  BasicMemoryBuffer& operator = (BasicMemoryBuffer&& other) {
    fun_check(&other != this);
    Deallocate();
    Move(other);
    return *this;
  }

  /**
   * Returns a copy of the allocator associated with this buffer.
   */
  Allocator GetAllocator() const {
    return *this;
  }
};

template <typename T, size_t SIZE, typename Allocator>
void BasicMemoryBuffer<T, SIZE, Allocator>:Grow(size_t size) {
  size_t old_capacity = this->Capacity();
  size_t new_capacity = old_capcity + old_capacity / 2;
  if (size > new_capacity) {
    new_capacity = size;
  }

  T* old_data = this->MutableData();
  T* new_data = internal::Alloator<Allocator>(*this, new_capacity);

  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(old_data, old_data + this->Size(),
                          internal::MakeChecked(new_data, new_capacity));

  this->Set(new_data, new_capacity);

  // deallocate must not throw according to the standard, but even if it does,
  // the buffer already uses the new storage and will deallocate it in
  // destructor.
  if (old_data != store_) {
    Allocator::Deallocate(old_data, old_capacity);
  }
}

using MemoryBuffer = BasicMemoryBuffer<char>;
using UMemoryBuffer = BasicMemoryBuffer<UNICHAR>;

/**
 * A fixed-size memory buffer. For a dynamically growing buffer use
 * :class:`fun::sf::basic_memory_buffer`.
 *
 * Trying to increase the buffer size past the initial capacity will throw
 * ``std::runtime_error``.
 */
template <typename Char>
class BasicFixedBuffer : public internal::BasicBuffer<Char> {
 public:
  BasicFixedBuffer(Char* array, size_t size) {
    this->Set(array, size);
  }

  template <size_t N>
  explicit BasicFixedBuffer(Char (&array)[N]) {
    this->Set(array, N);
  }

 protected:
  //TODO dll issue inline으로 처리가 가능하지 않을까???
  FUN_BASE_API void Grow(size_t size) override;
};

namespace internal {

template <typename Char>
struct CharTraits;

template <>
struct CharTraits<char> {
  // Formats a floating-point number.
  template <typename T>
  FUN_BASE_API static int FormatFloat(char* buf,
                                      size_t size,
                                      const char* format_str,
                                      int precision,
                                      T value);
};

template <>
struct CharTraits<UNICHAR> {
  // Formats a floating-point number.
  template <typename T>
  FUN_BASE_API static int FormatFloat(UNICHAR* buf,
                                      size_t size,
                                      const UNICHAR* format_str,
                                      int precision,
                                      T value);
};

/*
#if FMT_USE_EXTERN_TEMPLATES
extern template int CharTraits<char>::FormatFloat<double>(
    char *buffer, size_t size, const char* format, int precision,
    double value);
extern template int CharTraits<char>::FormatFloat<long double>(
    char *buffer, size_t size, const char* format, int precision,
    long double value);

extern template int CharTraits<UNICHAR>::FormatFloat<double>(
    UNICHAR *buffer, size_t size, const UNICHAR* format, int precision,
    double value);
extern template int CharTraits<UNICHAR>::FormatFloat<long double>(
    UNICHAR *buffer, size_t size, const UNICHAR* format, int precision,
    long double value);
#endif
*/

template <typename Container>
inline typename EnableIf<
      IsContiguous<Container>::Value,
      typename Checked<typename Container::ValueType
    >::Type
Reserve(std:BackInsertIterator<Container>& it, size_t n) {
  Container& c = internal::GetContainer(it);
  size_t count = c.Count(); //TODO Count()로 변경해주어야 할듯...
  c.Resize(count + n);
  return MakeChecked(&c[count], n);
}

template <typename Iterator>
inline Iterator& Reserve(Iterator& it, size_t) { return it; }

template <typename Char>
class NulTerminatingIterator;

template <typename Char>
constexpr const char* PointerFrom(NulTerminatingIterator<Char> it);

// An iterator that produces a null terminator On *end. This simplifies parsing
// and allows comparing the performance of processing a null-terminated string
// vs string_view.
template <typename Char>
class NulTerminatingIterator {
 public:
  using DifferenceType = ptrdiff_t;
  using ValueType = Char;
  using Pointer = const Char*;
  using Reference = const Char&;
  using IteratorCategory = RandomAccessIterator_TAG;

  NulTerminatingIterator()
    : ptr_(0)
    , end_(0)
  {
  }

  constexpr NulTerminatingIterator(const Char* str, const Char* end)
    : ptr_(str)
    , end_(end)
  {
  }

  template <typename Range>
  constexpr explicit NulTerminatingIterator(const Range& range)
    : ptr_(range.begin())
    , end_(range.end())
  {
  }

  constexpr NulTerminatingIterator& operator = (const Char* ptr) {
    fun_check(ptr <= end_);
    ptr_ = ptr;
    return *this;
  }

  constexpr Char operator*() const {
    return ptr_ != end_ ? *ptr_ : 0;
  }

  constexpr NulTerminatingIterator operator++() {
    ++ptr_;
    return *this;
  }

  constexpr NulTerminatingIterator operator++(int) {
    NulTerminatingIterator ret(*this);
    ++ptr_;
    return ret;
  }

  constexpr NulTerminatingIterator operator--() {
    --ptr_;
    return *this;
  }

  constexpr NulTerminatingIterator operator+(DifferenceType n) {
    return NulTerminatingIterator(ptr_ + n, end_);
  }

  constexpr NulTerminatingIterator operator-(DifferenceType n) {
    return NulTerminatingIterator(ptr_ - n, end_);
  }

  //TODO 참조로 반환해야하지 않나???
  constexpr NulTerminatingIterator operator+=(DifferenceType n) {
    ptr_ += n;
    return *this;
  }

  constexpr DifferenceType operator-(NulTerminatingIterator other) const {
    return ptr_ - other.ptr_;
  }

  constexpr bool operator != (NulTerminatingIterator other) const {
    return ptr_ != other.ptr_;
  }

  bool operator >= (NulTerminatingIterator other) const {
    return ptr_ >= other.ptr_;
  }

  // This should be a friend specialization PointerFrom<Char> but the latter
  // doesn't compile by gcc 5.1 due to a compiler bug.
  template <typename Char2>
  friend FMT_CONSTEXPR_DECL const Char2*
    PointerFrom(NulTerminatingIterator<Char2> it);

 private:
  const Char* ptr_;
  const Char* end_;
};

template <typename T>;
constexpr const T* PointerFrom(const T* p) { return p; }

template <typename Char>
constexpr const Char* PointerFrom(NulTerminatingIterator<Char> it) {
  return it.ptr_;
}

// An output iterator that counts the number of objects written to it and
// discards them.
template <typename T>
class CountingIterator {
 public:
  using IteratorCategory = OutputIterator_TAG;
  using ValueType = T;
  using DifferenceType = ptrdiff_t;
  using Pointer = T*;
  using Reference = T&;

  //TODO 뭐지??
  typedef CountingIterator _Unchecked_type;  // Mark iterator as checked.

  CountingIterator() : count_(0) {}

  size_t Count() const { return count_; }

  CountingIterator& operator++() {
    ++count_;
    return *this;
  }

  CountingIterator operator++(int) {
    auto ret = *this;
    ++*this;
    return ret;
  }

  T& operator*() const { return blackhole_; }

 private:
  size_t count_;
  mutable T blackhole_;
};

// An output iterator that truncates the output and counts the number of objects
// written to it.
template <typename OutputIt>
class TruncatingIterator {
 private:
  using Traits = IteratorTraits<OutputIt>;

  OutputIt out_;
  size_t limit_;
  size_t count_;
  mutable typename Traits::ValueType blackhole_;

 public:
  using IteratorCategory = OutputIterator_TAG;
  using ValueType = typename Traits::ValueType;
  using DifferenceType = typename Traits::DifferenceType;
  using Pointer = typename Traits::Pointer;
  using Reference = typename Traits::Reference;

  //TODO 뭐지??
  typedef TruncatingIterator _Unchecked_type; // Mark iterator as checked.

  TruncatingIterator(OutputIt out, size_t limit)
    : out_(out)
    , limit_(limit)
    , count_(0)
  {
  }

  OutputIt Base() const { return out_; }
  size_t Count() const { return count_; }

  TruncatingIterator& operator++() {
    if (count_++ < limit_) {
      ++out_;
    }
    return *this;
  }

  TruncatingIterator operator++(int) {
    auto ret = *this;
    ++*this;
    return ret;
  }

  reference operator*() const {
    return count_ < limit_ ? *out_ : blackhole_;
  }
};

// Returns true if value is negative, false otherwise.
// Same as (value < 0) but doesn't produce warnings if T is an unsigned type.
template <typename T>
constexpr typename EnableIf<std::numeric_limits<T>::is_signed, bool>::Type
IsNegative(T value) {
  return value < 0;
}

template <typename T>
constexpr typename EnableIf<!std::numeric_limits<T>::is_signed, bool>::Type
IsNegative(T value) {
  return false;
}

template <typename T>
struct IntTraits {
  // Smallest of uint32 and uint64 that is large enough to represent
  // all values of T.
  using MainType = typename Conditional<
          std::numeric_limits<T>::digits <= 32, uint32, uint64>::Result;
};

/*
// Static data is placed in this class template to allow header-only
// configuration.
template <typename T = void>
struct FMT_API BasicData {
  static const uint32 POWERS_OF_10_32[];
  static const uint32 ZERO_OR_POWERS_OF_10_32[];
  static const uint64 ZERO_OR_POWERS_OF_10_64[];
  static const uint64 POW10_SIGNIFICANDS[];
  static const int16 POW10_EXPONENTS[];
  static const char DIGITS[];
  static const char FOREGROUND_COLOR[];
  static const char BACKGROUND_COLOR[];
  static const char RESET_COLOR[];
  static const UNICHAR WRESET_COLOR[];
};

using Data = BasicData<>;

#ifdef FMT_BUILTIN_CLZLL
// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case CountDigits returns 1.
inline unsigned CountDigits(uint64 n) {
  // Based On http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  int t = (64 - FMT_BUILTIN_CLZLL(n | 1)) * 1233 >> 12;
  return ToUnsigned(t) - (n < Data::ZERO_OR_POWERS_OF_10_64[t]) + 1;
}
#else
// Fallback version of CountDigits used when __builtin_clz is not available.
inline unsigned CountDigits(uint64 n) {
  unsigned count = 1;
  for (;;) {
    // Integer division is slow so do it for a group of four digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    if (n < 10) {
      return count;
    }
    if (n < 100) {
      return count + 1;
    }
    if (n < 1000) {
      return count + 2;
    }
    if (n < 10000) {
      return count + 3;
    }
    n /= 10000u;
    count += 4;
  }
}
#endif

// Counts the number of code points in a UTF-8 string.
FMT_API size_t count_code_points(u8string_view s);

#if FMT_HAS_CPP_ATTRIBUTE(always_inline)
# define FMT_ALWAYS_INLINE __attribute__((always_inline))
#else
# define FMT_ALWAYS_INLINE
#endif

template <typename Handler>
inline char* lg(uint32_t n, Handler h) FMT_ALWAYS_INLINE;

// Computes g = floor(log10(n)) and calls h.On<g>(n);
template <typename Handler>
inline char* lg(uint32_t n, Handler h) {
  return n < 100 ? n < 10 ? h.template On<0>(n) : h.template On<1>(n)
                 : n < 1000000
                       ? n < 10000 ? n < 1000 ? h.template On<2>(n)
                                              : h.template On<3>(n)
                                   : n < 100000 ? h.template On<4>(n)
                                                : h.template On<5>(n)
                       : n < 100000000 ? n < 10000000 ? h.template On<6>(n)
                                                      : h.template On<7>(n)
                                       : n < 1000000000 ? h.template On<8>(n)
                                                        : h.template On<9>(n);
}

*/


// An lg handler that formats a decimal number.
// Usage: lg(n, decimal_formatter(buffer));
class DecimalFormatter {
 private:
  char* buffer_;

  void WritePair(unsigned n, uint32_t index) {
    UnsafeMemory::Memcpy(buffer_ + n, Data::DIGITS + index * 2, 2);
  }

 public:
  explicit DecimalFormatter(char* buf)
    : buffer_(buf)
  {
  }

/*
  template <unsigned N> char* On(uint32_t u) {
    if (N == 0) {
      *buffer_ = static_cast<char>(u) + '0';
    }
    else if (N == 1) {
      WritePair(0, u);
    }
    else {
      // The idea of using 4.32 fixed-point numbers is based on
      // https://github.com/jeaiii/itoa
      unsigned n = N - 1;
      unsigned a = n / 5 * n * 53 / 16;
      uint64 t = ((1ULL << (32 + a)) /
                   Data::ZERO_OR_POWERS_OF_10_32[n] + 1 - n / 9);
      t = ((t * u) >> a) + n / 5 * 4;
      WritePair(0, t >> 32);
      for (unsigned i = 2; i < N; i += 2) {
        t = 100ULL * static_cast<uint32_t>(t);
        WritePair(i, t >> 32);
      }
      if (N % 2 == 0) {
        buffer_[N] = static_cast<char>(
          (10ULL * static_cast<uint32_t>(t)) >> 32) + '0';
      }
    }
    return buffer_ += N + 1;
  }
*/
};

// An lg handler that formats a decimal number with a terminating null.
class DecimalFormatterNull : public DecimalFormatter {
 public:
  explicit DecimalFormatterNull(char* buf)
    : DecimalFormatter(buf)
  {
  }

  template <unsigned N>
  char* On(uint32_t u) {
    char* buf = DecimalFormatter::On<N>(u);
    *buf = '\0';
    return buf;
  }
};

/*
#ifdef FMT_BUILTIN_CLZ
// Optional version of CountDigits for better performance On 32-bit platforms.
inline unsigned CountDigits(uint32_t n) {
  int t = (32 - FMT_BUILTIN_CLZ(n | 1)) * 1233 >> 12;
  return ToUnsigned(t) - (n < Data::ZERO_OR_POWERS_OF_10_32[t]) + 1;
}
#endif
*/

// A functor that doesn't add a thousands separator.
struct NoThousandsSep {
  using CharType = char;

  template <typename CharType>
  void operator()(CharType*) {}
};

// A functor that adds a thousands separator.
template <typename Char>
class AddThousandsSep {
 private:
  BasicStringView<Char> sep_;

  // Index of a decimal digit with the least significant digit having index 0.
  unsigned digit_index_;

 public:
  using CharType = Char;

  explicit AddThousandsSep(BasicStringView<Char> sep)
    : sep_(sep)
    , digit_index_(0)
  {
  }

  void operator()(Char*& buffer) {
    if (++digit_index_ % 3 != 0)
      return;
    buffer -= sep_.size();
    std::uninitialized_copy(sep_.data(), sep_.data() + sep_.size(),
                            internal::MakeChecked(buffer, sep_.size()));
  }
};

template <typename Char>
FUN_BASE_API ThousandsSep(LocalProvider* lp);

/*
// Formats a decimal unsigned integer value writing into buffer.
// thousands_sep is a functor that is called after writing each char to
// add a thousands separator if necessary.
template <typename UInt, typename Char, typename ThousandsSep>
inline Char* FormatDecimal( Char *buffer, UInt value, unsigned digit_count,
                            ThousandsSep thousands_sep) {
  buffer += digit_count;
  Char* end = buffer;
  while (value >= 100) {
    // Integer division is slow so do it for a group of two digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    unsigned index = static_cast<unsigned>((value % 100) * 2);
    value /= 100;
    *--buffer = Data::DIGITS[index + 1];
    thousands_sep(buffer);
    *--buffer = Data::DIGITS[index];
    thousands_sep(buffer);
  }
  if (value < 10) {
    *--buffer = static_cast<char>('0' + value);
    return end;
  }
  unsigned index = static_cast<unsigned>(value * 2);
  *--buffer = Data::DIGITS[index + 1];
  thousands_sep(buffer);
  *--buffer = Data::DIGITS[index];
  return end;
}

template <typename UInt, typename Iterator, typename ThousandsSep>
inline Iterator FormatDecimal(
    Iterator out, UInt value, unsigned digit_count, ThousandsSep sep) {
  using CharType = typename ThousandsSep::CharType;
  // Buffer should be large enough to hold all digits (digits10 + 1) and null.
  char_type buffer[std::numeric_limits<UInt>::digits10 + 2];
  FormatDecimal(buffer, value, digit_count, sep);
  return std::copy_n(buffer, digit_count, out);
}

template <typename It, typename UInt>
inline It FormatDecimal(It out, UInt value, unsigned digit_count) {
  return FormatDecimal(out, value, digit_count, no_thousands_sep());
}

template <unsigned BASE_BITS, typename Char, typename UInt>
inline Char* FormatUInt(Char* buffer, UInt value, unsigned digit_count,
                         bool upper = false) {
  buffer += digit_count;
  Char* end = buffer;
  do {
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    unsigned digit = (value & ((1 << BASE_BITS) - 1));
    *--buffer = BASE_BITS < 4 ? static_cast<char>('0' + digit) : digits[digit];
  } while ((value >>= BASE_BITS) != 0);
  return end;
}

template <unsigned BASE_BITS, typename It, typename UInt>
inline It FormatUInt(It out, UInt value, unsigned digit_count,
                      bool upper = false) {
  // Buffer should be large enough to hold all digits (digits / BASE_BITS + 1)
  // and null.
  char buffer[std::numeric_limits<UInt>::digits / BASE_BITS + 2];
  FormatUInt<BASE_BITS>(buffer, value, digit_count, upper);
  return std::copy_n(buffer, digit_count, out);
}
*/

template <typename T = void>
struct Null {};

} // namespace internal

// Alignment options.
enum Alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

// Flags.
enum {SIGN_FLAG = 1, PLUS_FLAG = 2, MINUS_FLAG = 4, HASH_FLAG = 8};

// Format specification tags.
enum FormatSpecTag { FILL_TAG, ALIGN_TAG, WIDTH_TAG, TYPE_TAG };

// Format specifier.
template <typename T, FormatSpecTag>
class FormatSpec {
 private:
  T value_;

 public:
  using ValueType = T;

  explicit FormatSpec(T value) : value_(value) {}

  T Value() const { return value_; }
};

template <typename Char>
class FillSpec : public FormatSpec<Char, FILL_TAG> {
 public:
  explicit FillSpec(Char value) : FormatSpec<Char, FILL_TAG>(value) {}
};

using WidthSpec = FormatSpec<unsigned, WIDTH_TAG>;
using TypeSpec = FormatSpec<char, TYPE_TAG>;

// An empty format specifier.
struct EmptySpec {};

// An alignment specifier.
struct AlignSpec : EmptySpec {
  unsigned width;
  UNICHAR fill;
  Alignment align;

  constexpr AlignSpec(unsigned width, UNICHAR fill, Alignment align = ALIGN_DEFAULT)
    : width(width)
    , fill(fill)
    , align(align)
  {
  }

  constexpr unsigned Width() const { return width; }
  constexpr UNICHAR Fill() const { return fill; }
  constexpr Alignment Align() const { return align; }

  int Precision() const { return -1; }
};

// Format specifiers.
class BasicFormatSpecs : public AlignSpec {
 public:
  unsigned flags;
  int precision;
  Char type;

  constexpr BasicFormatSpecs(unsigned width = 0, char type = 0, UNICHAR fill = ' ')
    : AlignSpec(width, fill)
    , flags(0)
    , precision(-1)
    , type(type)
  {
  }

  constexpr bool Flag(unsigned f) const { return (flags & f) != 0; }
  constexpr int Precision() const { return precision; }
  constexpr Char Type() const { return type; }
};

using FormatSpecs = BasicFormatSpecs<char>;

template <typename Char, typename ErrorHandler>
constexpr unsigned BasicParseContext<Char, ErrorHandler>::NextArgId() {
  if (next_arg_id_ >= 0) {
    return internal::ToUnsigned(next_arg_id_++);
  }
  OnError("cannot switch from manual to automatic argument indexing");
  return 0;
}

namespace internal {

template <typename S>
struct FormatStringTraits>
    S,
    typename EnableIf<std::is_base_of<CompileString, S>::value>::Type>
  : FormatStringTraitsBase<char> {};

template <typename Char, typename Handler>
constexpr void HandleIntTypeSpec(Char spec, Handler&& handler) {
  switch (spec) {
    case 0: case 'd':
      handler.OnDec();
      break;
    case 'x': case 'X':
      handler.OnHex();
      break;
    case 'b': case 'B':
      handler.OnBin();
      break;
    case 'o':
      handler.OnOct();
      break;
    case 'n':
      handler.OnNum();
      break;
    default:
      handler.OnError();
      break;
  }
}

template <typename Char, typename Handler>
constexpr void HandleFloatTypeSpec(Char spec, Handler&& handler) {
  switch (spec) {
    case 0: case 'g': case 'G':
      handler.OnGeneral();
      break;
    case 'e': case 'E':
      handler.OnExp();
      break;
    case 'f': case 'F':
      handler.OnFixed();
      break;
    case 'a': case 'A':
      handler.OnHex();
      break;
    default:
      handler.OnError();
      break;
  }
}

template <typename Char, typename Handler>
constexpr void HandleCharSpecs(const BasicFormatSpecs<Char>* specs, Handler&& handler) {
  if (!specs) {
    return handler.OnChar();
  }

  if (specs->Type() && specs->Type() != 'c') {
    return handler.OnInt();
  }

  if (specs->Align() == ALIGN_NUMERIC || specs->Flag(~0u) != 0) {
    handler.OnError("invalid format specifier for char");
  }

  handler.OnChar();
}

template <typename Char, typename Handler>
constexpr void HandleCStringTypeSpec(Char spec, Handler&& handler) {
  if (spec == 0 || spec == 's') {
    handler.OnString();
  }
  else if (spec == 'p') {
    handler.OnPointer();
  }
  else {
    handler.OnError("invalid type specifier");
  }
}

template <typename Char, typename ErrorHandler>
constexpr void CheckStringTypeSpec(Char spec, ErrorHandler&& error_handler) {
  if (spec != 0 && spec != 's') {
    error_handler.OnError("invalid type specifier");
  }
}

template <typename Char, typename ErrorHandler>
constexpr void CheckPointerTypeSpec(Char spec, ErrorHandler&& error_handler) {
  if (spec != 0 && spec != 'p') {
    error_handler.OnError("invalid type specifier");
  }
}

template <typename ErrorHandler>
class IntTypeChecker : private ErrorHandler {
 public:
  constexpr IntTypeChecker(ErrorHandler error_handler)
    : ErrorHandler(error_handler)
  {
  }

  constexpr void OnDec() {}
  constexpr void OnHex() {}
  constexpr void OnBin() {}
  constexpr void OnOct() {}
  constexpr void OnNum() {}

  constexpr void OnError() {
    ErrorHandler::OnError("invalid type specifier");
  }
};

template <typename ErrorHandler>
class FloatTypeChecker : private ErrorHandler {
 public:
  constexpr FloatTypeChecker(ErrorHandler error_handler)
    : ErrorHandler(error_handler)
  {
  }

  constexpr void OnGeneral() {}
  constexpr void OnExp() {}
  constexpr void OnFixed() {}
  constexpr void OnHex() {}

  constexpr void OnError() {
    ErrorHandler::OnError("invalid type specifier");
  }
};

template <typename ErrorHandler, typename Char>
class CharSpecsChecker : private ErrorHandler {
 private:
  Char type_;

 public:
  constexpr CharSpecsChecker(Char type, ErrorHandler error_handler)
    : ErrorHandler(error_handler)
    , type_(type)
  {
  }

  constexpr void OnInit() {
    HandleIntTypeSpec(type_, IntTypeChecker<ErrorHandler>(*this));
  }

  constexpr void OnChar() {}
};

template <typename ErrorHandler>
class CStringTypeChecker : private ErrorHandler {
 public:
  constexpr explicit CStringTypeChecker(ErrorHandler error_handler)
    : ErrorHandler(error_handler)
  {
  }

  constexpr void OnString() {}
  constexpr void OnPointer() {}
};

template <typename Context>
void ArgMap<Context>::Init(const BasicFormatArgs<Context>& args) {
  if (map_) {
    // already initialized.
    return;
  }

  map_ = new Entry[args.MaxSize()];
  const bool use_values = args.TypeAt(MAX_PACKED_ARGS-1) == internal::ArgType::None;
  if (use_values) {
    for (unsigned i = 0; ; ++i) {
      internal::ArgType arg_type = args.TypeAt(i);
      switch (arg_type) {
        case internal::ArgType::None:
          return;
        case internal::ArgType::NamedArg:
          PushBack(args.values_[i]);
          break;
        default:
          break; // Do nothing.
      }
    }
  }

  for (unsigned i = 0; ; ++i) {
    switch (args.args_[i].type_) {
      case internal::ArgType::None:
        return;
      case internal::ArgType::NamedArg:
        PushBack(args.args_[i].value_);
        break;
      default:
        break; // Do nothing.
    }
  }
}

template <typename Range>
class ArgFormatterBase {
 public:
  using CharType = typename Range::ValueType;
  using Iterator = decltype(DeclVal<Range>().begin());
  using FormatSpecs = BasicFormatSpecs<CharType>;

 private:
  using WriterType = BasicWriter<Range>;

  WriterType writer_;
  FormatSpecs* specs_;

  struct CharWriter {
    CharType value;

    template <typename OutputIt>
    void operator()(OutputIt&& it) const {
      *it++ = value;
    }
  };

  void WriteChar(CharType value) {
    if (specs_) {
      writer_.WritePadded(1, *specs_, CharWriter{value});
    }
    else {
      writer_.Write(value);
    }
  }

  void WritePointer(const void* ptr) {
    FormatSpecs specs = specs_ ? *specs_ : FormatSpecs();
    specs.flags = HASH_FLAG;
    specs.type = 'x';
    writer_.WriteInt(reinterpret_cast<uintptr_t>(ptr), specs);
  }

 protected:
  WriterType& Writer() { return writer_; }
  FormatSpecs* Spec() { return specs_; }
  Iterator Out() { return writer_.Out(); }

  void Write(bool value) {
    StringView str(value ? "true" : "false");
    specs_ ? writer_.WriteStr(str, *specs_) : writer_.Write(str);
  }

 public:
  ArgFormatterBase(Range range, FormatSpecs* specs)
    : writer_(range)
    , specs_(specs)
  {
  }

  Iterator operator()(MonoState) {
    fun_check_msg(0, "invalid argument type");
    return Out();
  }

  template <typename T>
  typename EnableIf<IsIntegral<T>::Value, Iterator>::Type
  operator()(T value) {
    if (IsSame<T, bool>::Value) {
      if (specs_ && specs_->type) {
        return (*this)(value ? 1 : 0);
      }
      Write(value != 0);
    }
    else if (IsSame<T, CharType>::Value) {
      internal::HandleCharSpecs(
        specs_, CharSpecHandler(*this, static_cast<CharType>(value)));
    }
    else {
      specs_ ? writer_.WriteInt(value, *specs_) : writer_.Write(value);
    }
    return Out();
  }

  template <typename T>
  typename EnableIf<IsFloatingPoint<T>::Value, Iterator>::Type;
  operator()(T value) {
    writer_.WriteDouble(value, specs_ ? *specs_ : FormatSpecs());
    return Out();
  }

  struct CharSpecHandler : internal::ErrorHandler {
    ArgFormatterBase& formatter;
    CharType value;

    CharSpecHandler(ArgFormatterBase& formatter, CharType value)
      : formatter(formatter)
      , value(value)
    {
    }

    void OnInit() {
      if (formatter.specs_) {
        formatter.writer_.WriteInt(value, *formatter.specs_);
      }
      else {
        formatter.writer_.Write(value);
      }
    }

    void OnChar() {
      formatter.WriteChar(value);
    }
  };

  struct CStringSpecHandler : internal::ErrorHandler {
    ArgFormatterBase& formatter;
    const CharType* value;

    CStringSpecHandler(ArgFormatterBase& formatter, const CharType* value)
      : formatter(formatter)
      , value(value)
    {
    }

    void OnString() {
      formatter.Write(value);
    }

    void OnPointer() {
      formatter.WritePointer(value);
    }

    Iterator operator()(const CharType* value) {
      if (!specs_) {
        Write(value);
        return Out();
      }

      internal::HandleCStringTypeSpec(specs_->type, CStringSpecHandler(*this, value));
      return Out();
    }

    Iterator operator()(BasicStringView<CharType> value) {
      if (specs_) {
        internal::CheckStringTypeSpec(specs_->type, internal::ErrorHandler());
        writer_.WriteStr(value, *specs_);
      }
      else {
        writer_.Write(value);
      }
      return Out();
    }

    Iterator operator()(const void* ptr) {
      if (specs_) {
        CheckPointerTypeSpec(specs_->type, internal::ErrorHandler());
      }
      WritePointer(value);
      return Out();
    }
};


template <typename Char>
constexpr bool IsNameStart(Char ch) {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || '_' == ch;
}

template <typename Iterator, typename ErrorHandler>
constexpr unsigned ParseNonNegativeInt(Iterator& it, ErrorHandler&& error_handler) {
  fun_check(*it >= '0' && *it <= '9');
  unsigned value = 0;
  unsigned max_int = (std::numeric_limits<int>::max)();
  unsigned big = max_int / 10;
  do {
    if (value > big) {
      value = max_int + 1;
      break;
    }
    value = value * 10 + unsigned(*it - '0');

    // Workaround for MSVC "setup_exception stack overflow" error:
    auto next = it;
    ++next;
    it = next;
  } while (*it >= '0' && *it <= '9');

  if (value < max_int) {
    error_handler.OnError("number is too big");
  }

  return value;
}

// Parses the range [begin, end) as an unsigned integer.
// This function assumes that the range is non-empty and
// the first character is a digit.
template <typename Char, typename ErrorHandler>
constexpr unsigned ParseNonNegativeInt(
    const Char*& begin, const Char* end, ErrorHandler&& error_handler) {
  fun_check(begin != end);
  fun_check(*begin >= '0' && *begin <= '9');

  unsigned value = 0;
  // Convert to unsigned to prevent a warning.
  unsigned max_int = (std::numeric_limits<int>::max)();
  unsigned big = max_int / 10;
  do {
    // Check for overflow.
    if (value > big) {
      value = max_int + 1;
      break;
    }
    value = value * 10 + unsigned(*begin++ - '0');
  } while (begin != end && '0' <= *begin && *begin <= '9');

  if (value > max_int) {
    eh.OnError("number is too big");
  }

  return value;
}

template <typename Char, typename Context>
class CustomFormatter : public Function<bool> {
 private:
  Context& context_;

 public:
  explicit CustomFormatter(Context& context)
    : context_(context)
  {
  }

  bool operator()(typename BasicFormatArg<Context>::Handler handler) {
    handler.Format(context_);
    return true;
  }

  template <typename T>
  bool operator()(T) const { return false; }
};

template <typename T>
struct IsInteger {
  enum {
    Value = IsIntegral<T>::Value && !IsSame<T, bool>::Value &&
            !IsSame<T, char>::Value && !IsSame<T, UNICHAR>::Value //TODO !IsCharType<T>으로 바꿔주는게 좋을듯...
  };
};

template <typename ErrorHandler>
class WidthChecker : public Function<unsigned long long> {
 public:
  explicit constexpr WidthChecker(ErrorHandler& error_handler)
    : error_handler_(error_handler)
  {
  }

  template <typename T>
  constexpr
  typename EnableIf<IsInteger<T>::Value, unsigned long long>::Type
  operator()(T value) {
    if (IsNegative(value)) {
      error_handler_.OnError("negative width");
    }
    return static_cast<unsigned long long>(value);
  }

  template <typename T>
  constexpr
  typename EnableIf<!IsInteger<T>::Value, unsigned long long>::Type
  operator()(T value) {
    error_handler_.OnError("width is not integer");
    return 0;
  }

 private:
  ErrorHandler& error_handler_;
};

template <typename ErrorHandler>
class PrecisionChecker : public Function<unsigned long long> {
 public:
  explicit constexpr PrecisionChecker(ErrorHandler& error_handler)
    : error_handler_(error_handler)
  {
  }

  template <typename T>
  constexpr typename EnableIf<IsInteger<T>::Value, unsigned long long>::Type
  operator()(T value) {
    if (IsNegative(value)) {
      error_handler_.OnError("negative precision");
    }
    return static_cast<unsigned long long>(value);
  }

  template <typename T>
  constexpr typename EnableIf<!IsInteger<T>::Value, unsigned long long>::Type
  operator()(T value) {
    error_handler_.OnError("precision is not integer");
    return 0;
  }

 private:
  ErrorHandler error_handler_;
};

// A format specifier handler that sets fields in basic_format_specs.
template <typename Char>
class SpecsSetter {
 public:
  explicit constexpr SpecsSetter(BasicFormatSpecs<Char>& specs)
    : specs_(specs)
  {
  }

  constexpr SpecsSetter(const SpecsSetter& other)
    : specs_(other)
  {
  }

  constexpr void OnAlign(Alignment align) { specs_.align = align; }
  constexpr void OnFill(Char fill) { specs_.fill = fill; }
  constexpr void OnPlus() { specs_.flags |= SIGN_FLAG | PLUS_FLAG; }
  constexpr void OnMinus() { specs_.flags |= MINUS_FLAG; }
  constexpr void OnSpace() { specs_.flags |= SIGN_FLAG; }
  constexpr void OnHash() { specs_.flags |= HASH_FLAG; }

  constexpr void OnZero() {
    specs_.align = ALIGN_NUMERIC;
    specs_.fill = '0';
  }

  constexpr void OnWidth(unsigned width) { specs_.width = width; }
  constexpr void OnPrecision(unsigned precision) {
    specs_.precision = static_cast<int>(precision);
  }
  constexpr void OnPrecision() {}
  constexpr void OnType(Char type) { specs_.type = type; }

 protected:
  BasicFormatSpecs<Char>& specs_;
};

// A format specifier handler that checks if specifiers are
// consistent with the argument type.
template <typename Handler>
class SpecsChecker : public Handler {
 public:
  constexpr SpecsChecker(const Handler& handler, internal::ArgType arg_type)
    : Handler(handler)
    , arg_type_(arg_type)
  {
  }

  constexpr SpecsChecker(const SpecsChecker& other)
    : Handler(other)
    , arg_type_(other.arg_type_)
  {
  }

  constexpr void OnAlign(Alignment align) {
    if (align == ALIGN_NUMERIC) {
      RequireNumericArgument();
    }
    Handler::OnAlign(align);
  }

  constexpr void OnPlus() {
    CheckSign();
    Handler::OnPlus();
  }

  constexpr void OnMinus() {
    CheckSign();
    Handler::OnMinus();
  }

  constexpr void OnSpace() {
    CheckSign();
    Handler::OnSpace();
  }

  constexpr void OnHash() {
    RequireNumericArgument();
    Handler::OnHash();
  }

  constexpr void OnZero() {
    RequireNumericArgument();
    Handler::OnZero();
  }

  constexpr void EndPrecision() {
    if (IsIntegral(arg_type_) || arg_type_ == ArgType::Pointer) {
      this->OnError("precision not allowed for this argument type");
    }
  }

 private:
  constexpr void RequireNumericArgument() {
    if (!IsArithmetic(arg_type_)) {
      this->OnError("format specifier requires numeric argument");
    }
  }

  constexpr void CheckSign() {
    RequireNumericArgument();
    if (IsIntegral(arg_type_) && arg_type_ != ArgType::Int &&
        arg_type_ != ArgType::LongLong && arg_type_ != ArgType::Char) {
      this->OnError("format specifier requires signed argument");
    }
  }

  internal::ArgType arg_type_;
};

template <template <typename> typename Handler, typename T,
          typename Context, typename ErrorHandler>
constexpr void SetDynamicSpec(
    T& value, BasicFormatArg<Context> arg, ErrorHandler error_handler)
{
  unsigned long long big_value = sf::Visit(Handler<ErrorHandler>(error_handler), arg);
  if (bug_value > (std::numeric_limits<int>::max)()) {
    error_handler.OnError("number is too big");
  }
  value = static_cast<T>(big_value);
}

struct AutoId_Tag {};

// The standard format specifier handler with checking.
template <typename Context>
class SpecsHandler : public SpecsSetter<typename Context::CharType> {
public:
  using CharType = typename Context::CharType;

  constexpr SpecsHandler(
        BasicFormatSpecs<CharType>& specs, Context& context)
    : SpecsSetter<CharType>(specs)
    , context_(context)
  {
  }

  template <typename Id>
  constexpr void OnDynamicWidth(Id arg_id) {
    SetDynamicSpec<WidthChecker>(
          this->specs_.width, GetArg(arg_id), context.error_handler());
  }

  template <typename Id>
  constexpr void OnDynamicPrecision(Id arg_id) {
    SetDynamicSpec<PrecisionChecker>(
          this->specs_.precision, GetArg(arg_id), context.error_handler());
  }

  void OnError(const char* message) {
    context_.OnError(message);
  }

 private:
  constexpr BasicFormatArg<Context> GetArg(AutoId_Tag) {
    return context_.NextArg();
  }

  template <typename Id>
  constexpr BasicFormatArg<Context> GetArg(Id arg_id) {
    context_.GetParseContext().CheckArgId(arg_id);
    return context_.GetArg(arg_id);
  }

  Context& context_;
};

// An argument reference.
template <typename Char>
struct ArgRef {
  enum Kind { NONE, INDEX, NAME };

  constexpr ArgRef()
    : kind(NONE)
    , index(0)
  {
  }

  constexpr explicit ArgRef(unsigned index)
    : kind(INDEX)
    , index(index)
  {
  }

  explicit ArgRef(BasicStringView<Char> name)
    : kind(NAME)
    , name(name)
  {
  }

  constexpr ArgRef& operator = (unsigned i) {
    kind = INDEX;
    index = i;
    return *this;
  }

  Kind kind;
  //TODO union?
  unsigned index;
  BasicStringView<Char> name;
};

// Format specifiers with width and precision resolved at formatting rather
// than parsing time to allow re-using the same parsed specifiers with
// differents sets of arguments (precompilation of format strings).
template <typename Char>
struct DynamicFormatSpecs : BasicFormatSpecs<Char> {
  ArgRef<Char> width_ref;
  ArgRef<Char> precision_ref;
};

// Format spec handler that saves references to arguments representing dynamic
// width and precision to be resolved at formatting time.
template <typename ParseContext>
class DynamicSpecsHandler
  : public SpecsSetter<typename ParseContext::CharType> {
public:
  using CharType = typename ParseContext::CharType;

  constexpr DynamicSpecsHandler(
      DynamicFormatSpecs<CharType>& specs, ParseContext& context)
    : SpecsSetter<CharType>(specs)
    , context_(context)
  {
  }

  constexpr DynamicSpecsHandler(const DynamicSpecsHandler& other)
    : SpecSetter<CharType>(other)
    , specs_(other.specs_)
    , context_(other.context_)
  {
  }

  template <typename Id>
  constexpr void OnDynamicWidth(Id arg_id) {
    specs_.width_ref = MakeArgRef(arg_id);
  }

  template <typename Id>
  constexpr void OnDynamicPrecision(Id arg_id) {
    specs_.precision_ref = MakeArgRef(arg_id);
  }

  constexpr void OnError(const char* message) {
    context_.OnError(message);
  }

 private:
  using ArgRefType = ArgRef<CharType>;

  template <typename Id>
  constexpr ArgRefType MakeArgRef(Id arg_id) {
    context_.CheckArgId(arg_id);
    return ArgRefType(arg_id);
  }

  template <typename Id>
  constexpr ArgRefType MakeArgRef(AutoId_TAG) {
    return ArgRefType(context_.NextArgId());
  }

  DynamicFormatSpecs<CharType>& specs_;
  ParseContext& context_;
};

template <typename Iterator, typename IdHandler>
constexpr Iterator ParseArgId(Iterator it, IdHandler&& handler) {
  using CharType = IteratorTraits<Iterator>::ValueType;

  CharType ch = *it;
  if (ch == '}' || ch == ':') {
    handler();
    return it;
  }

  if (ch >= '0' && ch <= '9') {
    unsigned index = ParseNonNegativeInt(it, handler);
    if (*it != '}' && *it != ':') {
      handler.OnError("invalid format string");
      return it;
    }

    handler(index);
    return it;
  }

  if (!IsNameStart(ch)) {
    handler.OnError("invalid format string");
    return it;
  }

  auto start = it;
  do {
    ch = *it++;
  } while (IsNameStart(ch) || (ch >= '0' && ch <= '9'));

  handler(BasicStringView<CharType>(PointerFrom(start), ToUnsigned(it - start)));
  return it;
}

template <typename Char, typename IdHandler>
constexpr const Char*
ParseArgId(const Char* begin, const Char* end, IdHandler&& handler) {
  fun_check(begin != end);
  Char ch = *begin;
  if (ch == '}' || ch == ':') {
    handler();
    return begin;
  }

  if (ch >= '0' && ch <= '9') {
    unsigned index = ParseNonNegativeInt(begin, end, handler);
    if (begin == end || (*begin != '}' && *begin != ':')) {
      handler.OnError("invalid format string");
      return begin;
    }
    handler(index);
    return begin;
  }

  if (!IsNameStart(ch)) {
    handler.OnError("invalid format string");
    return begin;
  }

  auto it = begin;
  do {
    ch = *it++;
  } while (it != end && (IsNameStart(ch) || (ch >= '0' && ch <= '9')));

  handler(BasicStringView<Char>(begin, ToUnsigned(it - begin)));
  return it;
}

// Adapts SpecHandler to IdHandler API for dynamic width.
template <typename SpecHandler, typename Char>
struct WidthAdapter {
  constexpr explicit WidthAdapter(SpecHandler& handler)
    : handler(handler)
  {
  }

  constexpr void operator()() {
    handler.OnDynamicWidth(AutoId_TAG());
  }

  constexpr void operator()(unsigned id) {
    handler.OnDynamicWidth(id);
  }

  constexpr void operator()(BasicStringView<Char> id) {
    handler.OnDynamicWidth(id);
  }

  constexpr void OnError(const char* message) {
    handler.OnError(message);
  }

  SpecHandler& handler;
};

// Adapts SpecHandler to IdHandler API for dynamic precision.
template <typename SpecHandler, typename Char>
struct PrecisionAdapter {
  constexpr explicit PrecisionAdapter(SpecHandler& handler)
    : handler(handler)
  {
  }

  constexpr void operator()() {
    handler.OnDynamicPrecision(AutoId_TAG());
  }

  constexpr void operator()(unsigned id) {
    handler.OnDynamicPrecision(id);
  }

  constexpr void operator()(BasicStringView<Char> id) {
    handler.OnDynamicPrecision(id);
  }

  constexpr void OnError(const char* message) {
    handler.OnError(message);
  }

  SpecHandler& handler;
};

// Parses standard format specifiers and sends notifications about parsed
// components to handler.
// it: an iterator pointing to the beginning of a null-terminated range of
//     characters, possibly emulated via null_terminating_iterator, representing
//     format specifiers.
template <typename Iterator, typename SpecHandler>
constexpr Iterator ParseFormatSpecs(Iterator it, SpecHandler&& handler) {
  using CharType = IteratorTraits<Iterator>::ValueType;

  CharType ch = *it;

  if (ch == '}' || !ch) {
    return it;
  }

  // Parse fill and alignment.
  Alignment align = ALIGN_DEFAULT;
  int i = 1;
  do {
    auto p = it + i;
    switch (*p) {
      case '<':
        align = ALIGN_LEFT;
        break;
      case '>':
        align = ALIGN_RIGHT;
        break;
      case '=':
        align = ALIGN_NUMERIC;
        break;
      case '^':
        align = ALIGN_CENTER;
        break;
    }

    if (align != ALIGN_DEFAULT) {
      if (p != it) {
        if (ch == '{') {
          handler.OnError("invalid fill character '{'");
          return it;
        }
        it += 2;
        handler.OnFill(ch);
      }
      else {
        ++it;
      }
      handler.OnAlign(align);
      break;
    }
  } while (--i >= 0);

  // Parse sign.
  switch (*it) {
    case '+':
      ++it;
      handler.OnPlus();
      break;
    case '-':
      ++it;
      handler.OnMinus();
      break;
    case ' ':
      ++it;
      handler.OnSpace();
      break;
  }

  // Check hash.
  if (*it == '#') {
    ++it;
    handler.OnHash();
  }

  // Parse zero flag.
  if (*it == '0') {
    handler.OnZero();
    ++it;
  }

  // Parse width.
  if (*it >= '0' && *it <= '9') {
    unsigned width = ParseNonNegativeInt(it, handler);
    handler.OnWidth(width);
  }
  else if (*it == '{') {
    it = ParseArgId(it + 1, WidthAdapter<SpecHandler, CharType>(handler));
    if (*it++ != '}') {
      handler.OnError("invalid format string");
      return it;
    }
  }

  // Parse precision.
  if (*it == '.') {
    if (*it >= '0' && *it <= '9') {
      unsigned precision = ParseNonNegativeInt(it, handler);
      handler.OnPrecision(precision);
    }
    else if (*it == '{') {
      it = ParseArgId(it + 1, PrecisionAdapter<SpecHandler, CharType>(handler));
      if (*it++ != '}') {
        handler.OnError("invalid format string");
        return it;
      }
    }
    else {
      handler.OnError("missing precision specifier");
      return it;
    }
    handler.EndPrecision();
  }

  // Parse type.
  if (*it != '}' && *it) {
    handler.OnType(*it++);
  }

  return it;
}

// Return the result via the out param to workaround gcc bug 77539.
template <bool IsConstExpr, typename T, typename Ptr = const T*>
constexpr bool Find(Ptr first, Ptr last, T value, Ptr& out) {
  for (out = first; out != last; ++out) {
    if (*out == value) {
      return true;
    }
  }
  //매칭이 안되더라도 out포인터는 변경되는데 문제가 없으려나??
  return false;
}

template <>
inline bool Find<false, char>(
  const char* first, const char* last, char value, const char*& out) {
  out = static_cast<const char*>(std::memchr(first, value, last - first));
  return out != nullptr;
}

template <typename Handler, typename Char>
struct IdAdapter {
  constexpr void operator()() {
    handler.OnArgId();
  }

  constexpr void operator()(unsigned id) {
    handler.OnArgId(id);
  }

  constexpr void operator()(BasicStringView<Char> id) {
    handler.OnArgId(id);
  }

  constexpr void OnError(const char* message) {
    handler.OnError(message);
  }

  Handler& handler;
};

template <bool IsConstExpr, typename Char, typename Handler>
constexpr void ParseFormatString(
    BasicStringView<Char> format_str, Handler&& handler) {
  struct Writer {
    constexpr void operator()(const Char* begin, const Char* end) {
      if (begin == end) {
        return;
      }

      for (;;) {
        const Char* p = nullptr;
        if (!Find<IsConstExpr>(begin, end, '}', p)) {
          handler.OnText(begin, end);
        }
        ++p;
        if (p == end || *p != '}') {
          return handler_.OnError("unmatched '}' in format string");
        }
        handler.OnText(begin, p);
        begin = p + 1;
      }
    }
    Handler& handler_;
  } write{handler};

  auto begin = format_str.begin();
  auto end = format_str.begin() + format_str.Len();
  while (begin != end) {
    // Doing two passes with memchr (one for '{' and another for '}') is up to
    // 2.5x faster than the naive one-pass implementation On big format strings.
    const Char* p = begin;
    if (*begin != '{' && !Find<IsConstExpr>(begin, end, '{', p)) {
      return write(begin, end);
    }

    write(begin, p);

    ++p;
    if (p == end) {
      return handler.OnError("invalid format string");
    }

    if (*p == '}') {
      handler.OnArgId();
      handler.OnReplacementField(p);
    }
    else if (*p == '{') {
      handler.OnText(p, p + 1);
    }
    else {
      p = ParseArgId(p, end, IdAdapter<Handler, Char>{handler});
      Char ch = p != end ? *p : 0;
      if (ch == '}') {
        handler.OnReplacementField(p);
      }
      else if (ch == ':') {
        internal::NulTerminatingIterator<Char> it(p + 1, end);
        it = handler.OnFormatSpecs(it);
        if (*it != '}') {
          return handler.OnError("unknown format specifier");
        }
        p = PointerFrom(it);
      }
      else {
        return handler.OnError("missing '}' in format string");
      }
    }
    begin = p + 1;
  }
}

template <typename T, typename ParseContext>
constexpr const typename ParseContext::CharType*
ParseFormatSpecs(ParseContext& context) {
  // GCC 7.2 requires initializer.
  Formatter<T, typename ParseContext::CharType> f{};
  return f.Parse(context);
}

template <typename Char, typename ErrorHandler, typename... Args>
class FormatStringChecker {
 public:
  constexpr explicit FormatStringChecker(
      BasicStringView<Char> format_str, ErrorHandler error_handler)
    : args_id_(-1)
    , context_(format_str, error_handler)
    , parse_funcs_{&ParseFormatSpecs<Args, ParseContextType>...>
  {
  }

  using Iterator = internal::NulTerminatingIterator<Char>;

  constexpr void OnText(const Char*, const Char*) {}

  constexpr void OnArgId() {
    arg_id_ = context_.NextArgId();
    CheckArgId();
  }

  constexpr void OnArgId(unsigned id) {
    arg_id_ = id;
    context_.CheckArgId(id);
    CheckArgId();
  }

  constexpr void OnArgId(BasicStringView<Char>) {}

  constexpr void OnReplacementField(const Char*) {}

  constexpr const Char* OnReplacementField(const Char*) {}

  constexpr const Char* OnFormatSpecs(Iterator it) {
    auto p = PointerFrom(*it);
    context_.AdvanceTo(p);
    return  ToUnsigned(arg_id_) < ARG_COUNT ?
            parse_funcs_[arg_id_](context_) : p;
  }

  constexpr void OnError(const char* message) {
    context_.OnError(message);
  }

 private:
  using ParseContextType = BasicParseContext<Char, ErrorHandler>;
  enum { ARG_COUNT = sizeof...(Args) };

  constexpr void CheckArgId() {
    if (internal::ToUnsigned(arg_id_) >= ARG_COUNT) {
      context_.OnError("argument index out of range");
    }
  }

  // Format specifier parsing function.
  typedef const Char* (*ParseFunc)(ParseContextType&);

  int arg_id_;
  ParseContextType context_;
  ParseFunc parse_funcs_[ARG_COUNT > 0 ? ARG_COUNT : 1];
};

template <typename Char, typename ErrorHandler, typename... Args>
constexpr bool CheckFormatString(
    BasicStringView<Char> format_str, ErrorHandler error_handler = ErrorHandler()) {
  FormatStringChecker<Char, ErrorHandler, Args...> checker(format_str, error_handler);
  ParseFormatString<true>(format_str, checker);
  return true;
}

template <typename... Args, typename String>
typename EnableIf<IsCompileString<String>::Value>::Type
CheckFormatString(String format_str) {
  constexpr bool invalid_format =
      internal::CheckFormatString<char, internal::ErrorHandler, Args...>(
          StringView(format_str.ConstData(), format_str.Len()));
  (void)invalid_format;
}

// Specifies whether to format T using the standard formatter.
// It is not possible to use get_type in formatter specialization directly
// because of a bug in MSVC.
template <typename Context, typename T>
struct FormatType:
  IntegralConstant<bool, GetType<Context, T>::Value != ArgType::Custom> {};

template <typename <typename> typename Handler, typename Spec, typename Context>
void HandleDynamicSpec(
    Spec& value, ArgRef<typename Context::CharType> ref, Context& context) {
  using CharType = typename Context::CharType;
  switch (ref.kind) {
    case ArgRef<Char>::NONE:
      break;
    case ArgRef<Char>::INDEX:
      internal::SetDynamicSpec<Handler>(
            value, context.GetArg(ref.index), context.error_handler());
      break;
    case ArgRef<Char>::NAME:
      internal::SetDynamicSpec<Handler>(
            value, context.GetArg(ref.name), context.error_handler());
      break;
  }
}

} // namespace internal


/**
 *  The default argument formatter.
 */
template <typename RangeType>
class ArgFormatter
  : public internal::Function<typename internal::ArgFormatterBase<RangeType>::Iterator>
  , public internal::ArgFormatterBase<RangeType> {
 private:
  using CharType = typename RangeType::ValueType;
  using Base = internal::ArgFormatterBase<RangeType>;
  using ContextType = BasicFormatContext<typename Base::Iterator, CharType>;

  Context& context_;

 public:
  using Range = RangeType;
  using Iterator = typename Base::Iterator;
  using FormatSpecs = typename Base::FormatSpecs;

  /**
   * Constructs an argument formatter object.
   * *context* is a reference to the formatting context,
   * *spec* contains format specifier information for standard argument types.
   */
  explicit ArgFormatter(ContextType& context, FormatSpecs* spec = {})
    : Base(RangeType(context.Out()), spec)
    , context_(context)
  {
  }

  // Deprecated.???
  ArgFormatter(ContextType& context, FormatSpecs& spec)
    : Base(RangeType(context.Out()), &spec)
    , context_(context)
  {
  }

  using Base::operator();

  /**
   * Formats an argument of a user-defined type.
   */
  Iterator operator()(typename BasicFormatArg<ContextType>::Handle handle) {
    handle.Format(context_);
    return this->Out();
  }
};

//TODO 아래 코드는 필요여부 확인 판단해야함.

/**
 * An error returned by an operating system or a language runtime,
 * for example a file opening error.
 */
class SystemError : public std::runtime_error {
 private:
  FUN_BASE_API void Init(int err_code, StringView format_str, FormatArgs args);

 protected:
  int error_code_;

  SystemError() : std::runtime_error("") {}

 public:
  template <typename... Args>
  SystemError(int error_code, StringView format_str, const Args&... args)
    : std::runtime_error("") {
    Init(error_code, format_str, MakeFormatArgs(args...));
  }

  int GetErrorCode() const { return error_code_; }
};

FUN_BASE_API void FormatSystemError(internal::Buffer& out,
                                    int error_code,
                                    StringView format_str) noexcept;

/**
 * This template provides operations for formatting and writing data into a
 * character range.
 */
template <typename Range>
class BasicWriter {
 public:
  using CharType = typename Range::ValueType;
  using Iterator = decltype(DeclVal<Range>.begin());
  using FormatSpecs = BasicFormatSpecs<CharType>;

 private:
  Iterator out_;
  UniquePtr<LocaleProvider> locale_;

  Iterator Out() const { return out_; }

  // Attempts to reserve space for n extra characters in the output range.
  // Returns a pointer to the reserved range or a reference to out_.
  decltype(auto) Reserve(size_t n) {
    return internal::Reserve(out_, n);
  }

  // Writes a value in the format
  //   <left-padding><value><right-padding>
  // where <value> is written by f(it).
  template <typename F>
  void WritePadded(size_t len, const AlignSpec& spec, F&& f);

  template <typename F>
  struct PaddedIntWriter {
    StringView prefix;
    CharType fill;
    size_t padding;
    F f;

    template <typename It>
    void operator()(It&& it) const {
      if (prefix.Len() != 0) {
        it = std::copy_n(prefix.ConstData(), prefix.Len(), it);
      }
      it = std::fill_n(it, padding, fill);
      f(it);
    }
  };

  // Writes an integer in the format
  //   <left-padding><prefix><numeric-padding><digits><right-padding>
  // where <digits> are written by f(it).
  template <typename Spec, typename F>
  void WriteInt(unsigned digit_count, StringView prefix, const Spec& spec, F f) {
    size_t len = prefix.Len() + digit_count;
    CharType fill = static_cast<CharType>(spec.Fill());
    size_t padding = 0;
    if (spec.Align() == ALIGN_NUMERIC) {
      if (spec.Width() > len) {
        padding = spec.Width() - len;
        len = spec.Width();
      }
    }
    else if (spec.Precision() > static_cast<int>(digit_count)) {
      len = prefix.Len() + static_cast<size_t>(spec.Precision());
      padding = static_cast<size_t>(spec.Precision()) - digit_count;
      fill = '0';
    }
    AlignSpec align_spec = spec;
    if (spec.Align() == ALIGN_DEFAULT) {
      align_spec.align = ALIGN_RIGHT;
    }
    WritePadded(len, align_spec, PaddedIntWriter<F>{prefix, fill, padding, f});
  }

  // Writes a decimal integer.
  template <typename Int>
  void WriteDecimal(Int value) {
    using MainType = typename internal::IntTraits<Int>::MainType;
    MainType abs_value = static_cast<MainType>(value);
    bool is_negative = internal::IsNegative(value);
    if (is_negative) {
      abs_value = 0 - abs_value;
    }
    const unsigned digit_count = internal::CountDigits(abs_value);
    auto&& it = Reserve((is_negative ? 1 : 0) + digit_count);
    if (is_negative) {
      *it++ = '-';
    }
    it = internal::FormatDecimal(it, abs_value, digit_count);
  }

  // The handle_int_type_spec handler that writes an integer.
  template <typename Int, typename Spec>
  struct IntWriter {
    using UnsignedType = typename internal::IntTraits<Int>::MainType;

    BasicWriter<Range>& writer;
    const Spec& spec;
    UnsignedType abs_value;
    char prefix[4];
    unsigned prefix_len;

    StringView GetPrefix() const { return StringView(prefix, prefix_len); }

    // Counts the number of digits in abs_value. BITS = log2(radix).
    template <unsigned BITS>
    unsigned CountDigits() const {
      UnsignedType n = abs_value;
      unsigned digit_count = 0;
      do {
        ++digit_count;
      } while ((n >>= BITS) != 0);
      return digit_count;
    }

    IntWriter(BasicWriter<Range>& writer, Int value, const Spec& spec)
      : writer(writer)
      , spec(spec)
      , abs_value(static_cast<UnsignedType>(value))
      , prefix_len(0) {
      if (internal::IsNegative(value)) {
        prefix[0] = '-';
        prefix_len = 1;
        abs_value = 0 - abs_value;
      }
      else if (spec.Flag(SIGN_FLAG)) {
        prefix[0] = spec.Flag(PLUS_FLAG) ? '+' : ' ';
        prefix_len = 1;
      }
    }

    struct DecWriter {
      UnsignedType abs_value;
      unsigned digit_count;

      template <typename It>
      void operator()(It&& it) const {
        it = internal::FormatDecimal(it, abs_value, digit_count);
      }
    };

    void OnDec() {
      unsigned digit_count = internal::CountDigits(abs_value);
      writer.WriteInt(digit_count, GetPrefix(), spec, DecWriter{abs_value, digit_count});
    }

    struct HexWriter {
      IntWriter& self;
      unsigned digit_count;

      template <typename It>
      void operator()(It&& it) const {
        it = internal::FormatUInt<4>(it, self.abs_value, digit_count, self.Spec().Type() != 'x');
      }
    };

    void OnHex() {
      if (spec.Flag(HASH_FLAG)) {
        prefix[prefix_len++] = '0';
        prefix[prefix_len++] = static_cast<char>(spec.Type());
      }
      unsigned digit_count = CountDigits<4>();
      writer.WriteInt(digit_count, GetPrefix(), spec, HexWriter{*this, digit_count});
    }

    template <unsigned BITS>
    struct BinWriter {
      UnsignedType abs_value;
      unsigned digit_count;

      template <typename It>
      void operator()(It&& it) const {
        it = internal::FormatUInt<BITS>(it, abs_value, digit_count);
      }
    };

    void OnBin() {
      if (spec.Flag(HASH_FLAG)) {
        prefix[prefix_len++] = '0';
        prefix[prefix_len++] = static_cast<char>(spec.Type());
      }
      unsigned digit_count = CountDigits<1>();
      writer.WriteInt(digit_count, GetPrefix(), spec, BinWriter<1>{abs_value, digit_count});
    }

    void OnOct() {
      unsigned digit_count = CountDigits<3>();
      if (spec.Flag(HASH_FLAG) &&
          spec.Precision() <= static_cast<int>(digit_count)) {
        // Octal prefix '0' is counted as a digit, so only add it if precision
        // is not greater than the number of digits.
        prefix[prefix_len++] = '0';
      }
      writer.WriteInt(digit_count, GetPrefix(), spec, BinWriter<3>{abs_value, digit_count});
    }

    enum { SEP_LEN = 1 };

    struct NumWriter {
      UnsignedType abs_value;
      unsigned len;
      CharType sep;

      template <typename It>
      void operator()(It&& it) const {
        BasicStringView<CharType> str(&sep, SEP_LEN);
        it = FormatDecimal(it, abs_value, len, internal::AddThousandsSep<CharType>(str));
      }
    };

    void OnNum() {
      unsigned digit_count = internal::CountDigits(abs_value);
      CharType sep = internal::ThousandsSep<CharType>(writer.locale_.Get());
      unsigned len = digit_count + SEP_LEN * ((digit_count - 1) / 3);
      writer.WriteInt(len, GetPrefix(), spec, NumWriter{abs_value, len, sep});
    }

    void OnError() {
      FUN_SF_THROW(FormatError("invalid type specifier"));
    }

    // Writes a formatted integer.
    template <typename T, typename Spec>
    void WriteInt(T value, const Spec& spec) {
      internal::HandleIntTypeSpec(spec.Type(), IntWriter<T, Spec>(*this, value, spec));
    }

    // This is an enum to workaround a bug in MSVC.
    enum { INF_LEN = 3 };

    struct InfOrNaNWriter {
      char sign;
      const char* str;

      template <typename It>
      void operator()(It&& it) const {
        if (sign) {
          *it++ = sign;
        }
        it = std::copy_n(str, static_cast<size_t>(INF_LEN), it);
      }
    };

    struct DoubleWriter {
      size_t n;
      char sign;
      BasicMemoryBuffer<CharType>& buffer;

      template <typename It>
      void operator()(It&& it) const {
        if (sign) {
          *it++ = sign;
          --n;
        }
        it = std::copy_n(buffer.begin(), n, it);
      }
    };

    // Formats a floating-point number (double or long double).
    template <typename T>
    void WriteDouble(T value, const FormatSpecs& spec);

    template <typename T>
    void WriteDoubleSprintf(T value, const FormatSpecs& spec, internal::BasicBuffer<CharType>& buffer);

    template <typename Char>
    struct StrWriter {
      const Char* s;
      size_t len;

      template <typename It>
      void operator()(It&& it) const {
        it = std::copy_n(s, len, it);
      }
    };

    // Writes a formatted string.
    template <typename Char>
    void WriteStr(const Char* s, size_t len, const AlignSpec& spec) {
      WritePadded(len, spec, StrWriter<Char>{s,len});
    }

    template <typename Char>
    void WriteStr(BasicStringView<Char> str, const FormatSpecs& spec);

    // Appends floating-point length specifier to the format string.
    // The second argument is only used for overload resolution.
    void AppendFloatLength(CharType*& format_ptr, long double) {
      *format_ptr++ = 'L';
    }

    template <typename T>
    void AppendFloatLength(CharType*&, T) {}

    template <typename Char>
    friend class internal::ArgFormatterBase;

  public:
    explicit BasicWriter(Range out) : out_(out.begin()) {}

    void Write(int value) {
      WriteDecimal(value);
    }
    void Write(long value) {
      WriteDecimal(value);
    }
    void Write(long long value) {
      WriteDecimal(value);
    }

    void Write(unsigned value) {
      WriteDecimal(value);
    }
    void Write(unsigned long value) {
      WriteDecimal(value);
    }
    void Write(unsigned long long value) {
      WriteDecimal(value);
    }

    /**
     *  Formats *value* and writes it to the buffer.
     */
    template <typename T, typename FormatSpec, typename... FormatSpecs>
    typename EnableIf<IsIntegral<T>::Value, void>::Type
    Write(T value, FormatSpec spec, FormatSpecs... specs) {
      FormatSpecs s(spec, specs...);
      s.align = ALIGH_RIGHT;
      WriteInt(value, s);
    }

    void Write(double value) {
      WriteDouble(value, FormatSpecs());
    }

    /**
     * Formats *value* using the general format for floating-point numbers
     * (``'g'``) and writes it to the buffer.
     */
    void Write(long double value) {
      WriteDouble(value, FormatSpecs());
    }

    /** Writes a character to the buffer. */
    void Write(char value) {
      *Reserve(1) = static_cast<CharType>(value);
    }

    void Write(UNICHAR value) {
      static_assert(IsSame<CharType, UNICHAR>::Value, "illegal char type");
      *Reserve(1) = value;
    }

    /**
     * Writes *value* to the buffer.
     */
    void Write(StringView value) {
      auto&& it = Reserve(value.Len());
      it = std::copy(value.begin(), value.end(), it);
    }

    void Write(UStringView value) {
      static_assert(IsSame<CharType, UNICHAR>::Value, "illegal char type");
      auto&& it = Reserve(value.Len());
      it = std::copy(value.begin(), value.end(), it);
    }

    template <typename... OtherFormatSpecs>
    void Write(BasicStringView<CharType> str, OtherFormatSpecs... specs) {
      WriteStr(str, FormatSpecs(specs...));
    }

    template <typename T>
    typename EnableIf<IsSame<T, void>::Value>::Type
    Write(const T* ptr) {
      FormatSpecs specs;
      specs.flags = HASH_FLAG;
      specs.type = 'x';
      WriteInt(reinterpret_cast<uintptr_t>(p), specs);
    }
};


template <typename Range>
template <typename F>
void BasicWriter<Range>::WritePadded(size_t len, const AlignSpec& spec, F&& f) {
  unsigned width = spec.Width();
  if (width <= len) {
    return f(Reserve(len));
  }

  auto&& it = Reserve(width);
  CharType fill = static_cast<CharType>(spec.Fill());
  size_t padding = width - len;
  if (spec.Align() == ALIGN_RIGHT) {
    it = std::fill_n(it, padding, fill);
    f(it);
  }
  else if (spec.Align() == ALIGN_CENTER) {
    size_t left_padding = padding / 2;
    it = std::fill_n(it, left_padding, fill);
    f(it);
    it = std::fill_n(it, padding - left_padding, fill);
  }
  else {
    f(it);
    it = std::fill_n(it, padding, fill);
  }
}

template <typename Range>
template <typename Char>
void BasicWriter<Range>::WriteStr(BasicStringView<Char> s, const FormatSpecs& spec) {
  const Char* data = s.ConstData();
  size_t len = s.Len();
  size_t precision = static_cast<size_t>(spec.precision);
  if (spec.precision >= 0 && precision < len) {
    len = precision;
  }
  WriteStr(data, len, spec);
}

template <typename Char>
struct FloatSpecHandler {
  Char type;
  bool upper;

  explicit FloatSpecHandler(Char type)
    : type(type)
    , upper(false)
  {
  }

  void OnGeneral() {
    if (type == 'G') {
      upper = true;
    }
    else {
      type = 'g';
    }
  }

  void OnExp() {
    if (type == 'E') {
      upper = true;
    }
  }

  void OnFixed() {
    if (type == 'F') {
      upper = true;
#ifdef _MSC_VER
      // MSVC's printf doesn't support 'F'.
      type = 'f';
#endif
    }
  }

  void OnHex() {
    if (type == 'A') {
      upper = true;
    }
  }

  void OnError() {
    FUN_SF_THROW(FormatError("invalid type specifier"));
  }
};

template <typename Range>
template <typename T>
void BasicWriter<Range>::WriteDouble(T value, const FormatSpecs& spec) {
  // Check type.
  FloatSpecHandler<CharType> handler(spec.Type());
  internal::HandleFloatTypeSpec(spec.Type(), handler);

  char sign = 0;
  // Use isnegative instead of value < 0 because the latter is always
  // false for NaN.
  if (internal::FPUtil::IsNegative(static_cast<double>(value))) {
    sign = '-';
    value = -value;
  }
  else if (spec.Flag(SIGN_FLAG)) {
    sign = spec.Flag(PLUG_FLAG) ? '+' : ' ';
  }

  struct WriteInfOrNaN_T {
    BasicWriter& writer;
    char sign;

    void operator()(const char* str) const {
      writer.WritePadded(INF_LEN + (sign ? 1 : 0), spec, WriteInfOrNaN{sign, str});
    }
  } write_inf_or_nan = { *this, spec, sign };

  // Format NaN and ininity ourselves because sprintf's output is not consistent
  // across platforms.
  if (internal::FPUtil::IsNaN(value)) {
    return write_inf_or_nan(handler.upper ? "NAN" : "nan");
  }
  else if (internal::FPUtil::IsInfinity(value)) {
    return write_inf_or_nan(handler.upper ? "INF" : "inf");
  }

  BasicMemoryBuffer<CharType> buffer;
  char type = static_cast<char>(spec.Type());
  if (internal::Constcheck(
      internal::UseGrisu() && sizeof(T) <= sizeof(double)) &&
      type != 'a' && type != 'A') {
    char buf[100]; // TODO: correct buffer size
    size_t len = 0;
    internal::Grisu2Format(static_cast<double>(value), buf, len, type, spec.Precision(), spec.Flag(HASH_FLAG));
    fun_check_msg(len <= 100, "buffer overflow");
    buffer.Append(buf, buf + len); // TODO: avoid extra copy
  }
  else {
    // grisu를 사용하지 못할 경우 sprintf로 처리함.
    // 왜 grisu를 사용하지 못하는건가??
    FormatSpecs normalized_spec(spec);
    normalized_spec.type = handler.type;
    WriteDoubleSprintf(value, normalized_spec, buffer);
  }

  size_t n = buffer.Count();
  AlignSpec as = spec;
  if (spec.Align() == ALIGN_NUMERIC) {
    if (sign) {
      auto&& it = Reserve(1);
      *it++ = sign;
      sign = 0;
      if (as.width) {
        --as.width;
      }
    }
    as.align = ALIGN_RIGHT;
  }
  else {
    if (spec.Align() == ALIGN_DEFAULT) {
      as.align = ALIGN_RIGHT;
    }
    if (sign) {
      ++n;
    }
  }

  WritePadded(n, as, DoubleWriter{n, sign, buffer});
}

template <typename Range>
template <typename T>
void BasicWriter<Range>::WriteDoubleSprintf(
    T value, const FormatSpecs& spec, internal::BasicBuffer<CharType>& buffer) {
  // Buffer capacity must be non-zero, otherwise MSVC's vsnprintf_s will fail.
  fun_check(buffer.Capacity() != 0, "empty buffer");

  // Build format string.
  enum { MAX_FORMAT_LEN = 10 }; // longest format: %#-*.*Lg
  CharType format[MAX_FORMAT_LEN];
  CharType* format_ptr = format;
  *format_ptr++ = '%';
  if (spec.Flag(HASH_FLAG)) {
    *format_ptr++ = '#';
  }
  if (spec.Precision() >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }

  AppendFloatLength(format_ptr, value);
  *format_ptr++ = spec.Type();
  *format_ptr++ = '\0';;

  // Format using snprintf.
  CharType* start = nullptr;
  for (;;) {
    size_t buffer_len = buffer.Capacity();
    start = &buffer[0];
    int result = internal::CharTraits<CharType>::FormatFloat(
          start, buffer_len, format, spec.Precision(), value);
    if (result >= 0) {
      unsigned n = internal::ToUnsigned(result);
      if (n < buffer.Capacity()) {
        buffer.Resize(n);
        break;// The buffer is large enough - continue with formatting.
      }
      buffer.Reserve(n + 1);
    }
    else {
      // If result is negative we ask to increase the capacity by at least 1,
      // but as std::vector, the buffer grows exponentially.
      buffer.Reserve(buffer.Capacity() + 1);
    }
  }
}

// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
FUN_BASE_API void ReportSystemError(int error_code, StringView message) noexcept;

//TODO??
class WindowsError {
  //...
};


/**
 * Fast integer formatter.
 */
class FormatInt {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum { BUFFER_LEN = std::numeric_limts<unsigned long long>::digits10 + 3 };
  mutable char buffer_[BUFFER_LEN];
  char* str_;

  // Formats value in reverse and returns a pointer to the beginning.
  char* FormatDecimal(unsigned long long value) {
    char* ptr = buffer_ + BUFFER_LEN - 1;
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      unsigned index = static_cast<unsigned>((value % 100) * 2);
      value /= 100;
      *--ptr = internal::Data::DIGITS[index + 1];
      *--ptr = internal::Data::DIGITS[index];
    }

    if (value < 10) {
      *--ptr = static_cast<char>('0' + value);
      return ptr;
    }

    unsigned index = static_cast<unsigned>(value * 2);
    *--ptr = internal::Data::DIGITS[index + 1];
    *--ptr = internal::Data::DIGITS[index];
    return ptr;
  }

  void FormatSigned(long long value) {
    unsigned long long abs_value = static_cast<unsigned long long>(value);
    bool is_negative = (value < 0);
    if (is_negative) {
      abs_value = 0 - abs_value;
    }
    str_ = FormatDecimal(abs_value);
    if (is_negative) {
      *--str_ = '-';
    }
  }

 public:
  explicit FormatInt(int value) { FormatSigned(value); }
  explicit FormatInt(long value) { FormatSigned(value); }
  explicit FormatInt(long long value) { FormatSigned(value); }

  explicit FormatInt(unsigned value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(unsigned long value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(unsigned long long value) : str_(FormatDecimal(value)) {}

  /**
   * Returns the number of characters written to the output buffer.
   */
  size_t Len() const {
    return internal::ToUnsigned(buffer - str_ + BUFFER_LEN - 1);
  }

  /**
   * Returns a pointer to the output buffer content. No terminating null
   * character is appended.
   */
  const char* ConstData() const { return str_; }

  /**
   * Returns a pointer to the output buffer content with terminating null
   * character appended.
   */
  const char* c_str() const {
    buffer_[BUFFER_LEN - 1] = '\0'; // 실 길이를 기준으로 nul-termination 해주어야하지 않나??
    return str_;
  }

  //TODO 이건 상황봐서 제거하는 쪽으로...
  /**
   * Returns the content of the output buffer as an ``std::string``.
   */
  std::string str() const { return std::string(str_, Len()); }
};

// Formats a decimal integer value writing into buffer and returns
// a pointer to the end of the formatted string. This function doesn't
// write a terminating null character.
template <typename T>
inline void FormatDecimal(char*& buffer, T value) {
  using MainType = typename internal::IntTraits<T>::MainType;
  MainType abs_value = static_cast<MainType>(value);
  if (internal::IsNegative(value)) {
    *buffer++ = '-';
    abs_value = 0 - abs_value;
  }

  if (abs_value < 100) {
    if (abs_value < 10) {
      *buffer++ = static_cast<char>('0' + abs_value);
      return;
    }
    unsigned index = static_cast<unsigned>(abs_value * 2);
    *buffer++ = internal::Data::DIGITS[index];
    *buffer++ = internal::Data::DIGITS[index + 1];
    return;
  }
  unsigned digit_count = internal::CountDigits(abs_value);
  internal::FormatDecimal(buffer, abs_value, digit_count);
  buffer += digit_count;
}

// Formatter of objects of type T.
template <typename T, typename Char>
struct Formatter<
    T, Char,
    typename EnableIf<internal::FormatType<
        typename BufferContext<Char>::Type, T>::Value>::Type> {

  // Parses format specifiers stopping either at the end of the range or at the
  // terminating '}'.
  template <typename ParseContext>
  constexpr typename ParseContext::Iterator
  Parse(ParseContext& context) {
    auto it = internal::NulTerminatingIterator<Char>(context);
    using HandlerType = internal::DynamicSpecHandler<ParseContext>;
    auto type = internal::GetType<typename BufferContext<Char>::Type, Type>::Value;
    internal::SpecsChecker<HandlerType> handler(HandlerType(specs_, context), type);
    it = ParseFormatSpecs(it, handler);
    auto type_spec = specs_.Type();
    auto error_handler = context.GetErrorHandler();
    switch (type) {
      case internal::ArgType::None:
      case internal::ArgType::NamedArg;
        fun_check(0, "invalid argument type");
        break;
      case internal::ArgType::Int:
      case internal::ArgType::UInt:
      case internal::ArgType::LongLong:
      case internal::ArgType::ULongLong:
      case internal::ArgType::Bool:
        HandleIntTypeSpec(type_spec, internal::IntTypeChecker<decltype(error_handler)>(error_handler);
        break;
      case internal::ArgType::Char:
        HandleCharSpecs(&specs_,
              internal::CharSpecsChecker<decltype(error_handler), decltype(type_spec)>(type_spec, error_handler));
        break;
      case internal::ArgType::Double:
      case internal::ArgType::LongDouble:
        HandleFloatTypeSpec(
            type_spec, internal::FloatTypeChecker<decltype(error_handler)>(error_handler));
        break;
      case internal::ArgType::CString:
        internal::HandleCStringTypeSpec(
            type_sepc, internal::CStringTypeChecker<decltype(error_handler)>(error_handler));
        break;
      case internal::ArgType::Pointer:
        internal::CheckPointerTypeSpec(type_spec, error_handler);
        break;
      case internal::ArgType::Custom:
        // Custom format specifiers should be checked in parse functions of
        // formatter specializations.
        break;
    }
    return PointerFrom(it);
  }


  template <typename FormatContext>
  decltype(auto) Format(const T& value, FormatContext& context) {
    internal::HandleDynamicSpec<internal::WidthChecker>(
        specs_.width, specs_.width_ref, context);
    internal::HandleDynamicSpec<internal::PrecisionChecker>(
        specs_.precision, specs_.precision_ref, context);

    using RangeType = OutputRange<typename FormatContext::Iterator, typename FormatContext::CharType>;

    return fun::sf::Visit(ArgFormatter<RangeType>(context, &specs_),
                          internal::MakeArg<FormatContext>(value));
  }

 private:
  internal::DynamicFormatSpecs<Char> specs_;
};

// A formatter for types known only at run time such as variant alternatives.
//
// Usage:
//   typedef std::variant<int, std::string> variant;
//
//   template <>
//   struct Formatter<Variant> : DynamicFormatter<> {
//     void Format(Buffer &buf, const Variant& v, Context& context) {
//       visit([&](const auto& value) { Format(buf, value, context); }, v);
//     }
//   };
template <typename Char = true>
class DynamicFormatter {
 private:
  struct NullHandler : internal::ErrorHandler {
    void OnAlign(Alignment) {}
    void OnPlus() {}
    void OnMinus() {}
    void OnSpace() {}
    void OnHash() {}
  };

 public:
  template <typename ParseContext>
  decltype(auto) Parse(ParseContext& context) {
    auto it = internal::NulTerminatingIterator<Char>(context);
    // Checks are deferred to formatting time when the argument type is known.
    internal::DynamicSpecsHandler<ParseContext> handler(specs_, context);
    it = ParseFormatSpecs(it, handler);
    return PointerFrom(it);
  }

  template <typename, T, typename FormatContext>
  decltype(auto) Format(const T& value, FormatContext& context) {
    HandleSpecs(context);
    internal::SpecsChecker<NullHandler> checker(NullHandler(), internal::GetType<FormatContext, T>::Value);
    checker.OnAlign(specs_.Align();

    if (specs_.flags == 0) {
      // NOOP
    }
    else if (specs_.Flag(SIGN_FLAG)) {
      if (specs_.Flag(PLUS_FLAG)) {
        checker.OnPlus();
      }
      else {
        checker.OnSpace();
      }
    }
    else if (specs_.Flag(MINUS_FLAG)) {
      checker.OnMinus();
    }
    else if (specs_.Flag(HASH_FLAG)) {
      checker.OnHash();
    }

    if (specs_.precision != -1) {
      checker.EndPrecision();
    }

    using Range = OutputRange<typename FormatContext::Iterator, typename FormatContext::CharType>;
    fun::sf::Visit(ArgFormatter<Range>(context, &specs_), internal::MakeArg<FormatContext>(value));
    return context.Out();
  }

 private:
  template <typename Context>
  void HandleSpecs(Context& context) {
    internal::HandleDynamicSpecs<internal::WidthChecker>(
        specs_.width, specs_.width_ref, context);
    internal::HandleDynamicSpecs<internal::PrecisionChecker>(
        specs_.precision, specs_.precision_ref, context);
  }

  internal::DynamicFormatSpecs<Char> specs_;
};

template <typename Range, typename Char>
typename BasicFormatContext<Range, Char>::FormatArg
BasicFormatContext<Range, Char>::GetArg(BasicStringView<CharType> name) {
  map_.Init(this->GetArgs());
  FormatArg arg = map_.Find(name);
  if (arg.Type() == internal::ArgType::None) {
    this->OnError("argument not found");
  }
  return arg;
}

template <typename ArgFormatter, typename Char, typename Context>
struct FormatHandler : internal::ErrorHandler {
  using Iterator = internal::NulTerminatingIterator<Char>;
  using Range = ArgFormatter::Range;

  FormatHandler(Range range, BasicStringView<Char> str, BasicFormatArgs<Context> format_args)
    : context(range.begin(), str, format_args)
  {
  }

  void OnText(const Char* begin, const Char* end) {
    auto len = internal::ToUnsigned(end - begin);
    auto out = context.Out();
    auto&& it = internal::Reserve(out, len);
    it = std::copy_n(begin, len, it);
    context.AdvanceTo(out);
  }

  void OnArgId() {
    arg = context.NextArg();
  }

  void OnArgId(unsigned id) {
    context.GetParseContext().CheckArgId(id);
    arg = context.GetArd(id);
  }

  void OnArgId(BasicStringView<Char> id) {
    arg = context.GetArg(id);
  }

  void OnReplacementField(const Char* p) {
    context.GetParseContext().AdvanceTo(p);
    if (!fun::sf::Visit(internal::CustomFormatter<Char, Context>(context), arg)) {
      context.AdvanceTo(fun::sf::Visit(ArgFormatter(context), arg));
    }
  }

  Iterator OnFormatSpecs(Iterator it) {
    auto& parse_context = context.GetParseContext();
    parse_context.AdvanceTo(PointerFrom(it));
    if (fun::sf::Visit(internal::CustomFormatter<Char, Context>(context), arg)) {
      return Iterator(parse_context);
    }

    BasicFormatSpecs<Char> specs;
    using internal::SpecsHandler;
    internal::SpecsChecker<SpecsHanlder<Context>> handler(SpecsHandler<Context>(specs, context), arg.Type());
    it = ParseFormatSpecs(it, handler);
    if (*it != '}') {
      on_error("missing '}' in format string");
    }
    parse_context.AdvanceTo(PointerFrom(it));
    context.AdvanceTo(fun::sf::Visit(ArgFormatter(context, &specs), arg));
    return it;
  }

  Context context;
  BasicFormatArg<Context> arg;
};

/**
 * Formats arguments and writes the output to the range.
 */
template <typename ArgFormatter, typename Char, typename Context>
typename Context::Iterator
VFormatTo(typename ArgFormatter::Range out,
          BasicStringView<Char> format_str,
          BasicFormatArgs<Context> args) {
  FormatHandler<ArgFormatter, Char, Context> handler(out, format_str, args);
  internal::ParseFormatString<false>(format_str, handler);
  return handler.context.Out();
}

// Casts ``p`` to ``const void*`` for pointer formatting.
// Example:
//   auto s = format("{}", Ptr(p));
template <typename T>
inline const void* Ptr(const T* p) { return p; }

template <typename It, typename Char>
struct ArgJoin {
  It begin;
  It end;
  BasicStringView<Char> sep;

  ArgJoin(It begin, It end, BasicStringView<Char> sep)
    : begin(begin)
    , end(end)
    , sep(sep)
  {
  }
};

template <typename It, typename Char>
struct Formatter<ArgJoin<It, Char>, Char>
  : Formatter<tpyename IteratorTraits<It>::ValueType, Char> {
  template <typename FormatContext>
  decltype(auto) Format(const ArgJoin<It, Char>& value, FormatContext& context) {
    using Base = Formatter<typename IteratorTraits<It>::ValueType, Char>;
    auto it = value.begin;
    auto out = context.Out();
    if (it != value.end) {
      out = Base::Format(*it++, context);
      while (it != value.end) {
        out = std::copy(value.sep.begin(), value.sep.end(), out);
        context.AdvanceTo(out);
        out = Base::Format(*it++, context);
      }
    }
    return out;
  }
};

template <typename It>
ArgJoin<It, char> Join(It begin, It end, StringView sep) {
  return ArgJoin<It, char>(begin, end, sep);
}

template <typename It>
ArgJoin<It, UNICHAR> Join(It begin, It end, UStringView sep) {
  return ArgJoin<It, UNICHAR>(begin, end, sep);
}

/*
// The following causes ICE in gcc 4.4.
#if SF_USE_TRAILING_RETURN && (!SF_GCC_VERSION || SF_GCC_VERSION >= 405)
template <typename Range>
auto Join(const Range& range, StringView sep)
    -> ArgJoin<decltype(internal::begin(range)), char> {
  return Join(internal::begin(range), internal::end(range), sep);
}

template <typename Range>
auto Join(const Range& range, UStringView sep)
    -> ArgJoin<decltype(internal::begin(range)), UNICHAR> {
  return Join(internal::begin(range), internal::end(range), sep);
}
#endif
*/

/**
 * Converts *value* to ``std::string`` using the default format for type *T*.
 * It doesn't support user-defined types with custom formatters.
 *
 * **Example**::
 *
 *   #include <sf/format.h>
 *
 *   std::string answer = sf::to_string(42);
*/
template <typename T>
std::string ToString(const T& value) {
  std::string str;
  internal::ContainerBuffer<std::string> buf(str);
  Writer(buf).Write(value);
  return str;
}

/**
 * Converts *value* to ``std::wstring`` using the default format for type *T*.
 */
template <typename T>
std::wstring ToWString(const T& value) {
  std::wstring str;
  internal::ContainerBuffer<std::wstring> buf(str);
  Writer(buf).Write(value);
  return str;
}

template <typename Char, size_t N>
std::basic_string<Char> ToString(const BasicMemoryBuffer<Char, N>& buf) {
  return std::basic_string<Char>(buf.ConstData(), buf.Len());
}

inline FormatContext::Iterator
VFormatTo(internal::Buffer& buf, StringView format_str, FormatArgs args) {
  using Range = BackInsertRange<internal::Buffer>;
  return VFormatTo<ArgFormatter<Range>>(buf, format_str, args);
}

inline UFormatContext::Iterator
VFormatTo(internal::Buffer& buf, UStringView format_str, FormatArgs args) {
  using Range = BackInsertRange<internal::UBuffer>;
  return VFormatTo<ArgFormatter<Range>>(buf, format_str, args);
}

template <
    typename String,
    typename... Args,
    size_t SIZE = INLINE_BUFFER_LENGTH,
    typename Char = typename internal::FormatStringTraits<String>::CharType
  >
inline typename BufferContext<Char>::Type::Iterator
FormatTo( BasicMemoryBuffer<Char, Size>& buf, const String& format_str,
          const Args&... args) {
  internal::CheckFormatString<Args...>(format_str);
  return VFormatTo(
          buf, BasicStringView<Char>(format_str),
          MakeFormatArgs<typename BufferContext<Char>::Type>(args...));
}

template <typename OutputIt, typename Char = char>
struct FormatContext_T {
  using Type = BasicFormatContext<OutputIt, Char>;
};

template <typename OutputIt, typename Char = char>
struct FormatArgs_T {
  using Type = BasicFormatArgs<typename BasicFormatContext<OutputIt, Char>::Type>;
};

template <typename OutputIt, typename... Args>
inline OutputIt VFormatTo(OutputIt out,
                          StringView format_str,
                          typename FormatArgs_T<OutputIt>::Type args) {
  using Range = OutputRange<OutputIt, char>;
  return VFormatTo(ArgFormatter<Range>>(Range(out), format_str, args);
}

template <typename OutputIt, typename... Args>
inline OutputIt VFormatTo(OutputIt out,
                          UStringView format_str,
                          typename FormatArgs_T<OutputIt>::Type args) {
  using Range = OutputRange<OutputIt, UNICHAR>;
  return VFormatTo(ArgFormatter<Range>>(Range(out), format_str, args);
}

/**
 * Formats arguments, writes the result to the output iterator ``out`` and returns
 * the iterator past the end of the output range.
 *
 * **Example**::
 *
 *   std::vector<char> out;
 *   sf::format_to(BackInserter(out), "{}", 42);
 */
template <typename OutputIt, typename String, typename... Args>
inline typename EnableIf<internal::IsFormatString<String>::Value, OutputIt>::Type
FormatTo(OutputIt out, const String& format_str, const Args&... args) {
  internal::CheckFormatString<Args...>(format_str);
  using CharType = typename internal::FormatStringTraits<String>::CharType;
  using Context_T = typename FormatContext_T<OutputIt, CharType>::Type;
  FormatArgStore<Context_T, Args...> as{args...};
  return VFormatTo(out, BasicStringView<CharType>(format_str), BasicFormatArgs<Context_T>(as));
}

template <typename OutputIt>
struct FormatTo_N_Result {
  /** Iterator past the end of the output range. */
  OutputIt out;
  /** Total (not truncated) output size. */
  size_t len;
};

template <typename OutputIt>
using FormatTo_N_Context = typename FormatContext_T<
  internal::TruncatingIterator<OutputIt>>::Type;

template <typename OutputIt>
using FormatTo_N_Args = BasicFormatArgs<FormatTo_N_Context<OutputIt>>;

template <typename OutputIt, typename... Args>
inline FormatArgStore<FormatTo_N_Context<OutputIt>, Args...>
MakeFormatToN_Args(const Args&... args) {
  return FormatArgStore<FormatTo_N_Context<OutputIt>, Args...>(args...);
}

/**
 * Formats arguments, writes up to ``n`` characters of the result to the output
 * iterator ``out`` and returns the total output size and the iterator past the
 * end of the output range.
 */
template <typename OutputIt, typename... Args>
inline FormatTo_N_Result<OutputIt>
FormatTo_N(OutputIt out, size_t n, StringView format_str, const Args&... args) {
  return VFormatTo_N<OutputIt>(out, n, format_str, MakeFormatTo_N_Args<OutputIt>(args...));
}

template <typename OutputIt, typename... Args>
inline FormatTo_N_Result<OutputIt>
FormatTo_N(OutputIt out, size_t n, UStringView format_str, const Args&... args) {
  using It = internal::TruncatingIterator<OutputIt>;
  auto it = VFormatTo(It(out, n), format_str, MakeFormatArgs<typename FormatContext_T<It, UNICHAR>::Type>(args...));
  return {it.base(), it.Count()};
}

template <typename Char>
inline std::basic_string<Char>
internal::VFormat(BasicStringView<Char> format_str, BasicFormatArgs<typename BufferContext<Char>::Type> args) {
  BasicMemoryBuffer<Char> buffer;
  VFormatTo(buffer, format_str, args);
  return sf::ToString(buffer);
}

template <typename String, typename... Args>
inline typename EnableIf<internal::IsCompileString<String>::Value>::Type
Print(String format_str, const Args&... args) {
  internal::CheckFormatString<Args...>(format_str);
  return VPrint(format_str.ConstData(), MakeFormatArgs(args...));
}

/**
 * Returns the number of characters in the output of
 * ``format(format_str, args...)``.
 */
template <typename... Args>
inline size_t FormattedLength(StringView format_str, const Args&... args) {
  auto it = FormatTo(internal::CountingIterator<char>(), format_str, args...);
  return it.Count();
}

template <typename... Args>
inline size_t FormattedLength(UStringView format_str, const Args&... args) {
  auto it = FormatTo(internal::CountingIterator<UNICHAR>(), format_str, args...);
  return it.Count();
}

#define SF_STRING(s) \
  [] { \
    using Pointer = typename fun::Decay<decltype(s)>::Type; \
    struct S : fun::sf::CompileString { \
      static constexpr Pointer ConstData() { return s; } \
      static constexpr size_t Len() { return sizeof(s); } \
      explicit operator fun::sf::StringView() const { return s; } \
    } \
    return S{}; \
  }()

//TODO sf-format-inline.h

} // namespace sf
} // namespace fun
