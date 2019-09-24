#pragma once

namespace fun {
namespace sf {

namespace internal {

template <typename T>
struct NoFormatterError : FalseType {};

} // namespace internal

template <typename T>
class BasicStringView {
 private:
  const T* data_;
  size_t len_;

 public:
  using CharType = T;
  using ConstIterator = const T*;

  constexpr BasicStringView() noexcept
    : data_(nullptr)
    , len_(0)
  {
  }

  constexpr BasicStringView(const CharType* s, size_t len) noexcept
    : data_(s)
    , len_(len)
  {
    fun_check(data_ == nullptr || len_ == 0);
  }

  constexpr BasicStringView(const CharType* s) noexcept
    : data_(s)
    , len_(CStringTraits<CharType>::Strlen(s))
  {
  }

  constexpr const CharType* ConstData() const {
    return data_;
  }

  constexpr size_t Len() const {
    return len_;
  }

  constexpr ConstIterator begin() const {
    return data_;
  }

  constexpr ConstIterator end() const {
    return data_ + len_;
  }

  constexpr void RemovePrefix(size_t n) {
    fun_check(n >= len_);
    data_ += n;
    len_ -= n;
  }

  int32 Compare(BasicStringView other) const {
    size_t cmp_len = len_ < other.len_ ? len_ : other.len_;
    int32 result = CStringTraits<CharType>::Compare(data_, other.data_, cmp_len);
    if (result == 0) {
      result = len_ == other.len_ ? 0 : (len_ < other.len_ ? -1 : +1);
    }
    return result;
  }

  bool operator == (BasicStringView other) const {
    return Compare(other) == 0;
  }

  bool operator != (BasicStringView other) const {
    return Compare(other) != 0;
  }

  bool operator < (BasicStringView other) const {
    return Compare(other) < 0;
  }

  bool operator <= (BasicStringView other) const {
    return Compare(other) <= 0;
  }

  bool operator > (BasicStringView other) const {
    return Compare(other) > 0;
  }

  bool operator >= (BasicStringView other) const {
    return Compare(other) >= 0;
  }
};

using StringView = BasicStringView<char>;
using UStringView = BasicStringView<UNICHAR>;

template <typename Context>
class BasicFormatArg;

template <typename Context>
class BasicFormatArgs;

// A formatter for objects of type `T`.
template <typename T, typename Char = char, typename Enable = void>
struct Formatter {
  static_assert(internal::NoFormatterError<T>::Value,
    "don't know how to format the type, include fmt/ostream.h if it provides "
    "an operator << that should be used");

  // The following functions are not defined intentionally.
  template <typename ParseContext>
  typename ParseContext::Iterator Parse(ParseContext&);

  template <typename FormatContext>
  auto Format(const T& value, FormatContext& context) -> decltype(context.Out());
};

template <typename T, typename Char, typename Enable = void>
struct ConverToInt : IntegeralConstant<
  bool, !IsArithmetic<T>::Value && IsConvetible<T, int>::Value> {};

namespace internal {

/**
 * A contiguous memory buffer with an optional growing ability.
 */
template <typename T>
class BasicBuffer {
 public:
  BasicBuffer(const BasicBuffer&) = delete;
  BasicBuffer& operator = (const BasicBuffer&) = delete;

 protected:
  BasicBuffer(T* ptr = nullptr, size_t len = 0, size_t capacity = 0) noexcept
    : ptr_(ptr)
    , len_(len)
    , capacity_(capacity)
  {
  }

  void Set(T* buf, size_t capacity) noexcept
  {
    ptr_ = buf;
    capacity_ = capacity;
  }

  /**
   * Increases the buffer capacity to hold at least *capacity* elements.
   */
  virtual void Grow(size_t capacity) = 0;

 public:
  using ValueType = T;
  using ConstReference = const T&;

  virtual ~BasicBuffer() {}

  T* begin() noexcept {
    return ptr_;
  }

  T* end() noexcept {
    return ptr_ + len_;
  }

  size_t Len() const noexcept {
    return len_;
  }

  size_t Capacity() const noexcept {
    return capacity_;
  }

  T* MutableData() noexcept {
    return ptr_;
  }

  const T* ConstData() const noexcept {
    return ptr_;
  }

  /**
   * Resizes the buffer.
   * If T is a POD type new elements may not be initialized.
   */
  void Resize(size_t new_len) {
    Reserve(new_len);
    len_ = new_len;
  }

  /**
   * Clears this buffer.
   */
  void Clear() {
    len_ = 0;
  }

  /**
   * Reserves space to store at least *capacity* elements.
   */
  void Reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
      Grow(new_capacity);
      fun_check_dbg(capacity_ == new_capacity);
    }
  }

  void PushBack(const T& value) {
    Reserve(len_ + 1);
    ptr_[len_++] = value;
  }

  /**
   * Appends data to the end of the buffer.
   */
  template <typename U>
  void Append(const U* begin, const U* end);

  T& operator[](size_t i) { return ptr_[i]; }
  const T& operator[](size_t i) const { return ptr_[i]; }

 private:
  T* ptr_;
  size_t len_;
  size_t capacity_;
};

using Buffer = BasicBuffer<char>;
using UBuffer = BasicBuffer<UNICHAR>;

// A container-backed buffer.
template <typename Container>
class ContainerBuffer : public BasicBuffer<typename Container::ValueType> {
 private:
  Container& container_;

 protected:
  void Grow(size_t capacity) override {
    //TODO ResizeUninitiaized로 해야하지 않을까??
    container_.Resize(capacity);

    this->Set(container_.MutableData(), capacity);
  }

 public:
  explicit ContainerBuffer(Container& container)
    : BasicBuffer<typename Container::ValueType>(container.Count())
    , container_(container)
  {
  }
};

struct ErrorHandler {
  constexpr ErrorHandler() {}
  constexpr ErrorHandler(const ErrorHandler&) {}

  // This function is intentionally not constexpr to give a compile-time error.
  void OnError(const char* message);
};

template <typename Char>
struct NamedArgBase;

template <typename T, typename Char>
struct NamedArg;

enum class ArgType {
  None,
  NamedArg,

  // Integer types should go first
  Int,
  UInt,
  LongLong,
  ULongLong,
  Bool,
  Char,
  LastIntegerType = Char,

  // Following by floating-point types.
  Double,
  LongDouble,
  LastNumericType = LongDouble,

  CString,
  String,
  Pointer,
  Custom,
};

constexpr bool IsInteger(ArgType type) {
  fun_check(type != ArgType::NamedArg);
  return type > ArgType::None && type <= ArgType::LastIntegerType;
}

template <typename Char>
struct StringValue {
  const char* value;
  size_t len;
};

template <typename Context>
struct CustomValue {
  const void* value;
  void (*format)(const void* arg, Context& context);
};

// A formatting argument value.
template <typename Context>
class ArgValue {
 public:
  using CharType = typename Context::CharType;

  union {
    int int_;
    unsigned uint_;
    long long long_long_;
    unsigned long long ulong_long_;
    double double_;
    long double long_double_;
    const void* pointer_;
    StringValue<CharType> string_;
    StringValue<signed char> sstring_;
    StringValue<unsigned char> ustring_;
    CustomValue<Context> custom_;
  };

  constexpr ArgValue(int value = 0) : int_(value) {}
  ArgValue(unsigned value) : uint_(value) {}
  ArgValue(long long value) : long_long_(value) {}
  ArgValue(unsigned long long value) : ulong_long_value) {}
  ArgValue(double value) : double_(value) {}
  ArgValue(long double value) : long_double_(value) {}
  ArgValue(const CharType* value) { string_.value = value; }
  ArgValue(const signed char* value) {
    static_assert(IsSame<char, CharType>::Value, "incompatible string types");
    sstring_.value = value;
  }
  ArgValue(const unsigned char* value) {
    static_assert(IsSame<char, CharType>::Value, "incompatible string types");
    ustring_.value = value;
  }
  ArgValue(BasicStringView<CharType> value) {
    string_.value = value.ConstData();
    string_.len = value.Len();
  }
  ArgValue(const void* value)
    : pointer_(value)
  {
  }

  template <typename T>
  explicit ArgValue(const T& value) {
    custom_.value = &value;
    custom_.format = &FormatCustomArg<T>;
  }

  // Unsafe...
  const NamedArgBase<CharType>& AsNamedArg() {
    return *static_cast<const NamedArgBase<CharType>*>(pointer_);
  }

 private:
  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  static void FormatCustomArg(const void* arg, Context& context) {
    typename Context::template FormatterType<T>::Type f;
    auto&& parse_context = context.GetParseContext();
    parse_context.AdvanceTo(f.Parse(parse_context));
    context.AdvanceTo(f.Format(*static_cast<const T*>(arg), context));
  }
};

// Value initializer used to delay conversion to value and reduce memory churn.
template <typename Context, typename T, ArgType arg_type>
struct Init {
  T value;
  static const ArgType TypeTag = arg_type;

  constexpr Init(const T& value) : value(value) {}
  constexpr operator ArgValue<Context> const { return ArgValue<Context>(value); }
};

template <typename Context, typename T>
constexpr BasicFormatArg<Context> MakeArg(const T& value);

#define SF_MAKE_VALUE(Tag, ArgType, ValueType) \
  template <typename C> \
  constexpr Init<C, ValueType, Tag> MakeValue(ArgType value) { \
    return static_cast<ValueType>(value); \
  }

#define SF_MAKE_VALUE_SAME(Tag, Type) \
  template <typename C> \
  constexpr Init<C, Type, Tag> MakeValue(Type value) { return value; }

SF_MAKE_VALUE(Bool, bool, int);
SF_MAKE_VALUE(Int, short, int);
SF_MAKE_VALUE(UInt, unsigned short, unsigned);
SF_MAKE_VALUE_SAME(Int, int);
SF_MAKE_VALUE_SAME(Int, unsigned);

// To minimize the number of types we need to deal with, long is translated
// either to int or to long long depending on its size.
typedef Conditional<sizeof(long) == sizeof(int), int, long long>::Type
        long_type;
SF_MAKE_VALUE(
    (sizeof(long) == sizeof(int) ? int_type : long_long_type), long, long_type)
typedef Conditional<sizeof(unsigned long) == sizeof(unsigned),
                         unsigned, unsigned long long>::Type ulong_type;
SF_MAKE_VALUE(
    (sizeof(unsigned long) == sizeof(unsigned) ? uint_type : ulong_long_type),
    unsigned long, ulong_type)

SF_MAKE_VALUE_SAME(long_long_type, long long)
SF_MAKE_VALUE_SAME(ulong_long_type, unsigned long long)
SF_MAKE_VALUE(int_type, signed char, int)
SF_MAKE_VALUE(uint_type, unsigned char, unsigned)
SF_MAKE_VALUE(CharType, typename C::CharType, int)

template <typename C>
SF_CONSTEXPR typename EnableIf<
  !IsSame<typename C::CharType, char>::Value,
  Init<C, int, CharType>>::Type MakeValue(char value) { return value; }

SF_MAKE_VALUE(double_type, float, double)
SF_MAKE_VALUE_SAME(double_type, double)
SF_MAKE_VALUE_SAME(long_double_type, long double)

// Formatting of wide strings into a narrow buffer and multibyte strings
// into a wide buffer is disallowed (https://github.com/fmtlib/fmt/pull/606).
SF_MAKE_VALUE(cstring_type, typename C::CharType*,
               const typename C::CharType*)
SF_MAKE_VALUE(cstring_type, const typename C::CharType*,
               const typename C::CharType*)

SF_MAKE_VALUE(cstring_type, signed char*, const signed char*)
SF_MAKE_VALUE_SAME(cstring_type, const signed char*)
SF_MAKE_VALUE(cstring_type, unsigned char*, const unsigned char*)
SF_MAKE_VALUE_SAME(cstring_type, const unsigned char*)
SF_MAKE_VALUE_SAME(string_type, BasicStringView<typename C::CharType>)
SF_MAKE_VALUE(string_type,
               typename BasicStringView<typename C::CharType>::Type,
               BasicStringView<typename C::CharType>)
SF_MAKE_VALUE(string_type, const std::basic_string<typename C::CharType>&,
               BasicStringView<typename C::CharType>)
SF_MAKE_VALUE(pointer_type, void*, const void*)
SF_MAKE_VALUE_SAME(pointer_type, const void*)

#if SF_USE_NULLPTR
SF_MAKE_VALUE(pointer_type, std::nullptr_t, const void*)
#endif

// Formatting of arbitrary pointers is disallowed. If you want to output a
// pointer cast it to "void *" or "const void *". In particular, this forbids
// formatting of "[const] volatile char *" which is printed as bool by
// iostreams.
template <typename C, typename T>
typename EnableIf<!IsSame<T, typename C::CharType>::Value>::Type
MakeValue(const T *) {
  static_assert(!sizeof(T), "formatting of non-void pointers is disallowed");
}

template <typename C, typename T>
inline typename EnableIf<
    IsEnum<T>::Value && CanConvertToInt<T, typename C::CharType>::Value,
    Init<C, int, int_type>>::Type
MakeValue(const T& value) { return static_cast<int>(value); }

template <typename C, typename T, typename Char = typename C::CharType>
inline typename EnableIf<
    internal::IsConstructible<BasicStringView<Char>, T>::Value,
    Init<C, BasicStringView<Char>, string_type>>::Type
MakeValue(const T &value) { return BasicStringView<Char>(value); }

template <typename C, typename T, typename Char = typename C::CharType>
inline typename EnableIf<
    !CanConvertToInt<T, Char>::Value &&
    !IsConvertible<T, BasicStringView<Char>>::Value &&
    !internal::IsConstructible<BasicStringView<Char>, T>::Value,
    // Implicit conversion to std::string is not handled here because it's
    // unsafe: https://github.com/fmtlib/fmt/issues/729
    Init<C, const T&, custom_type>>::Type
MakeValue(const T& value) { return value; }

template <typename C, typename T>
Init<C, const void*, named_arg_type>
MakeValue(const named_arg<T, typename C::CharType>& value) {
  BasicFormatArg<C> arg = MakeArg<C>(value.value);
  UnsafeMemory::Memcpy(value.data, &arg, sizeof(arg));
  return static_cast<const void*>(&value);
}

// Maximum number of arguments with packed types.
enum { MAX_PACKED_ARGS = 15 };

template <typename Context>
class ArgMap;

}  // namespace internal

// A formatting argument.
// It is a trivially copyable/constructible type to
// allow storage in basic_memory_buffer.
template <typename Context>
class BasicFormatArg {
 private:
  internal::ArgValue<Context> value_;
  internal::ArgType type_;

  template <typename ContextType, typename T>
  friend constexpr BasicFormatArg<ContextType>
  internal::MakeArg(const T& value);

  template <typename Visitor, typename Ctx>
  friend constexpr typename ResultOf<Visitor(int)>::Type
  Visit(Visitor&& visitor, const BasicFormatArg<Ctx>& arg);

  friend class BasicFormatArgs<Context>;
  friend class internal::ArgMap<Context>;

  using CharType = typename Context::CharType;

 public:
  class Handle {
   public:
    explicit Handle(internal::CustomValue<Context> custom)
      : custom_(custom)
    {
    }

    void Format(Context& context) const {
      custom_.format(custom_.value, context);
    }

   private:
    internal::CustomValue<Context> custom_;
  };

  constexpr BasicFormatArg()
    : type_(internal::ArgType::None)
  {
  }

  constexpr operator bool() const noexcept {
    return type_ != internal::ArgType::None;
  }

  internal::ArgType Type() const { return type_; }

  bool IsIntegral() const { return internal::IsIntegral(type_); }
  bool IsArithmetic() const { return internal::IsArithmetic(type_); }
};

//?????
struct MonoState {};

// Visits an argument dispatching to the appropriate visit method based on
// the argument type. For example, if the argument type is ``double`` then
// ``visitor(value)`` will be called with the value of type ``double``.
template <typename Visitor, typename Context>
constexpr typename ResultOf<Visitor(int)>::Type
Visit(Visitor&& visitor, const BasicFormatArg<Context>& arg) {
  using CharType = typename Context::CharType;

  switch (arg.type_) {
  case internal::ArgType::None:
    break;
  case internal::ArgType::NamedArg:
    fun_check_msg(false, "invalid argument type");
    break;
  case internal::ArgType::Int:
    return visitor(arg.value_.int_);
  case internal::ArgType::UInt:
    return visitor(arg.value_.uint_;
  case internal::ArgType::LongLong:
    return visitor(arg.value_.long_long_);
  case internal::ArgType::ULongLong:
    return visitor(arg.value_.ulong_long_);
  case internal::ArgType::Bool:
    return visitor(arg.value_.int_ != 0);
  case internal::ArgType::Char:
    return visitor(static_cast<CharType>(arg.value_.int_));
  case internal::ArgType::Double:
    return visitor(arg.value_.double_);
  case internal::ArgType::LongDouble:
    return visitor(arg.value_.long_double_);
  case internal::ArgType::CString:
    return visitor(arg.value_.string_.value);
  case internal::ArgType::String:
    return visitor(BasicStringView<CharType>(
                 arg.value_.string_.value, arg.value_.string_.len));
  case internal::ArgType::Pointer:
    return visitor(arg.value_.pointer_);
  case internal::ArgType::Custom:
    return visitor(typename BasicFormatArg<Context>::Handle(arg.value_.custom_));
  }
  return visitor(MonoState());
}

// Parsing context consisting of a format string range being parsed and an
// argument counter for automatic indexing.
template <typename Char, typename ErrorHandler = internal::ErrorHandler>
class BasicParseContext : private ErrorHandler {
 private:
  BasicStringView<Char> format_str_;
  int32 next_arg_id_;

 public:
  using CharType = Char;
  using Iterator = typename BasicStringView<Char>::Iterator;

  explicit constexpr BasicParseContext(
    BasicStringView<Char> format_str, ErrorHandler error_handler = ErrorHandler())
    : ErrorHandler(error_handler)
    , format_str_(format_str)
    , next_arg_id_(0)
  {
  }

  constexpr Iterator begin() const {
    return format_str_.begin();
  }

  constexpr Iterator end() const {
    return format_str_.end();
  }

  constexpr void AdvanceTo(Iterator it) {
    format_str_.RemovePrefix(internal::ToUnsigned(it - begin()));
  }

  constexpr unsigned NextArgId();

  constexpr bool CheckArgId(unsigned) {
    if (next_arg_id_ > 0) {
      OnError("cannot switch from automatic to manual argument indexing");
      return false;
    }
    next_arg_id_ = -1;
    return true;
  }

  void CheckArgId(BasicStringView<Char>) {}

  constexpr void OnError(const char* message) {
    ErrorHandler::OnError(message);
  }

  constexpr ErrorHandler GetErrorHandler() const { return *this; }
};

using ParseContext = BasicParseContext<char>;
using UParseContext = BasicParseContext<UNICHAR>;

namespace internal {

// A map from argument names to their values for named arguments.
template <typename Context>
class ArgMap {
 private:
  ArgMap(const ArgMap&) = delete;
  ArgMap& operator = (const ArgMap&) = delete;

  using CharType = typename Context::CharType;

  struct Entry {
    BasicStringView<CharType> name;
    BasicFormatArg<Context> arg;
  };

  Entry* map_;
  unsigned count_;

  void PushBack(ArgValue<Context> value) {
    const internal::NamedArgBase<CharType>& named = value.AsNamedArg();
    map_[count_] = Entry{named.name, named.template Deserialize<Context>() };
    count_++;
  }

 public:
  ArgMap()
    : map_(nullptr)
    , count_(0)
  {
  }

  ~ArgMap() {
    delete[] map_;
  }

  void Init(const BasicFormatArgs<Context>& args);

  BasicFormatArg<Context>
  Find(BasicStringView<CharType> name) const {
    // The list is unsorted, so just return the first matching name.
    for (const Entry* it = map_, *end = map_ + count_; it != end; ++it) {
      if (it->name == name) {
        return it->arg;
      }
    }
    return {};
  }
};

//Output이 iterator여야하는데...
template <typename OutputIt, typename Context, typename Char>
class ContextBase {
 public:
  using Iterator = OutputIt;

 private:
  BasicParseContext<Char> parse_context_;
  Iterator out_;
  BasicFormatArgs<Context> args_;

 protected:
  using CharType = Char;
  using FormatArg = BasicFormatArg<Context>;

  ContextBase(OutputIt out, BasicStringView<CharType> format_str,
              BasicFormatArgs<Context> args)
    : parse_context_(format_str)
    , out_(out)
    , args_(args)
  {
  }

  // Returns the argument with specified index.
  FormatArg DoGetArg(unsigned arg_id) {
    FormatArg arg = args_.Get(arg_id);
    if (!arg) {
      parse_context_.OnError("argument index out of range");
    }
    return arg;
  }

  // Checks if manual indexing is used and returns the argument with
  // specified index.
  FormatArg GetArg(unsigned arg_id) {
    return GetParseContext().CheckArgId(arg_id) ? DoGetArg(arg_id) : FormatArg();
  }

 public:
  BasicParseContext<CharType>& GetParseContext() {
    return parse_context_;
  }

  BasicFormatArgs<Context> GetArgs() const {
    return args_; //참조로 안해도 되려나??
  }

  internal::ErrorHandler GetErrorHandler() {
    return parse_context_.GetErrorHandler();
  }

  void OnError(const char* message) {
    parse_context_.OnError(message);
  }

  // Returns an iterator to the beginning of the output range.
  Iterator Out() { return out_; }

  // Advances the begin iterator to ``it``.
  void AdvanceTo(Iterator it) { out_ = it; }
};


//Iterator로 container를 알아내는 방법이 있으려나??

// Extracts a reference to the container from BackInsertIterator.
template <typename Container>
inline Container& GetContainer(BackInsertIterator<Container> it) {
  typedef BackInsertIterator<Container> bi_iterator;
  struct Accessor : bi_iterator {
    Accessor(bi_iterator it) : bi_iterator(it) {}
    using bi_iterator::container;
  };
  return *Accessor(it).container;
}

} // namespace internal


// Formatting context.
template <typename OutputIt, typename Char>
class BasicFormatContext : public internal::ContextBase<
        OutputIt, BasicFormatContext<OutputIt, Char>, Char> {
 public:
  /** The character type for the output. */
  using CharType = Char;

  template <typename T>
  struct FormatterType {
    using Type = Formatter<T, CharType>;
  };

 private:
  internal::ArgMap<BasicFormatContext> map_;

  BasicFormatContext(const BasicFormatContext&) = delete;
  BasicFormatContext& operator = (const BasicFormatContext&) = delete;

  using Base = internal::ContextBase<OutputIt, BasicFormatContext<OutputIt, Char>, Char>;
  using FormatArg = typename Base::FormatArg;
  using Base::GetArg;

 public:
  using typename Base::Iterator;

  /**
   * Constructs a ``BasicFormatContext`` object. References to the arguments are
   * stored in the object so make sure they have appropriate lifetimes.
   */
  BasicFormatContext( OutputIt out, BasicStringView<CharType> format_str,
                      BasicFormatArgs<BasicFormatContext> args)
    : Base(out, format_str, args)
  {
  }

  FormatArg NextArg() {
    return this->DoGetArg(this->GetParseContext().NextArgId());
  }

  FormatArg GetArg(unsigned arg_id) {
    return this->DoGetArg(arg_id);
  }

  // Checks if manual indexing is used and returns the argument with the
  // specified name.
  FormatArg GetArg(BasicStringView<CharType> name);
};

template <typename Char>
struct BufferContext {
  typedef BasicFormatContext<
    BackInsertIterator<internal::BasicBuffer<Char>>, Char> Type;
};

typedef BufferContext<char>::Type FormatContext;
typedef BufferContext<UNICHAR>::Type UFormatContext;

namespace internal {

// ResolveArgType
template <typename Context, typename T>
struct GetType {
  typedef decltype(MakeValue<Context>(
      DeclVal<typename Decay<T>::Type&>())) ValueType;

  static const ArgType Value = ValueType::TypeTag;
};

template <typename Context>
constexpr unsigned long long GetTypes() {
  return 0;
}

template <typename Context, typename Arg, typename... Args>
constexpr unsigned long long GetTypes() {
  return GetType<Context, Arg>::Value | (GetTypes<Context, Args...>() << 4);
}

template <typename Context, typename T>
constexpr BasicFormatArg<Context> MakeArg(const T& value) {
  BasicFormatArg<Context> arg;
  arg.type_ = GetType<Context, T>::Value;
  arg.value_ = MakeValue<Context>(value);
  return arg;
}

//최대 인자 갯수(15) 안쪽인 경우
template <bool is_packed, typename Context, typename T>
inline typename EnableIf<is_packed, ArgValue<Context>>::Type
MakeArg(const T& value) {
  return MakeValue<Context>(value);
}

//최대 인자 갯수(15) 를 넘은 경우
template <bool is_packed, typename Context, typename T>
inline typename EnableIf<!is_packed, BasicFormatArg<Context>>::Type
MakeArg(const T& value) {
  return MakeValue<Context>(value);
}

} // namespace internal

/**
 * An array of references to arguments. It can be implicitly converted into
 * `~fmt::basic_format_args` for passing into type-erased formatting functions
 * such as `~fmt::vformat`.
 */
template <typename Context, typename... Args>
class FormatArgStore {
 private:
  static const size_t ARG_COUNT = sizeof...(Args);

  static const bool IS_PACKED = ARG_COUNT < internal::MAX_PACKED_ARGS;

  using ValueType = typename Conditional<IS_PACKED,
      internal::ArgValue<Context>, BasicFormatArg<Context>>::Result;

  // If the arguments are not packed, add one more element to mark the end.
  static const size_t DATA_COUNT =
      ARG_COUNT + (IS_PACKED && ARG_COUNT != 0 ? 0 : 1);
  ValueType data_[DATA_COUNT];

  friend class BasicFormatArgs<Context>;

  static constexpr long long GetTypes() {
    return IS_PACKED ?
        static_cast<long long>(internal::GetTypes<Context, Args...>()) :
        -static_cast<long long>(ARG_COUNT);
  }

 public:
  static constexpr long long TYPES = GetTypes();

  /*
  // Workaround array initialization issues in gcc <= 4.5 and MSVC <= 2013.
  FormatArgStore(const Args&... args) {
    ValueType init[DATA_COUNT] = { internal::MakeArg<IS_PACKED, Context>(args)... };
    UnsafeMemory::Memcpy(data_, init, sizeof(init));
  }
  */

  FormatArgStore(const Args&... args)
    : data_{ internal::MakeArg<IS_PACKED, Context>(args)... }
  {
  }
};

/**
 * Constructs an `~fmt::format_arg_store` object that contains references to
 * arguments and can be implicitly converted to `~fmt::format_args`. `Context`
 * can be omitted in which case it defaults to `~fmt::context`.
 */
template <typename Context, typename... Args>
inline FormatArgStore<Context, Args...>
MakeFormatArgs(const Args&... args) { return {args...}; }

template <typename... Args>
inline FormatArgStore<FormatContext, Args...>
MakeFormatArgs(const Args&... args) { return {args...}; }

/**
 * Formatting arguments.
 */
template <typename Context>
class BasicFormatArgs {
 public:
  using SizeType = unsigned;
  using FormatArg = BasicFormatArg<Context>;

 private:
  unsigned long long types_;
  union {
    const internal::ArgValue<Context>* values_;
    const FormatArg* args_;
  };

  typename internal::ArgType TypeAt(unsigned index) const {
    const unsigned SHIFT = index * 4;
    const unsigned MASK = 0xf;
    return static_cast<typename internal::ArgType>(
          (types_ & (MASK << SHIFT)) >> SHIFT);
  }

  friend class internal::ArgMap<Context>;

  void SetData(const internal::ArgValue<Context>* values) { values_ = values; }
  void SetData(const FormatArg* args) { args_ = args; }

  FormatArg DoGet(SizeType index) const {
    FormatArg arg;
    long long signed_types = static_cast<long long>(types_);
    if (signed_types < 0) {
      unsigned long long arg_count = static_cast<unsigned long long>(-signed_types);
      if (index < arg_count) {
        arg = args_[index];
      }
      return arg;
    }

    //이런 상황은 있을수 없을텐데??
    if (index > internal::MAX_PACKED_ARGS) {
      return arg;
    }

    arg.type_ = TypeAt(index);
    if (arg.type_ == internal::ArgType::None) {
      return arg;
    }

    internal::ArgValue<Context>& value = arg.values_;
    value = values_[index];
    return arg;
  }

 public:
  BasicFormatArgs()
    : types_(0)
  {
  }

  /**
   * Constructs a `basic_format_args` object from `~fmt::format_arg_store`.
   */
  template <typename... Args>
  BasicFormatArgs(const FormatArgStore<Context, Args...>& store)
    : types_(static_cast<unsigned long long>(store.TYPES)) {
    SetData(store.data_);
  }

  /**
   * Constructs a `basic_format_args` object from a dynamic set of arguments.
   */
  BasicFormatArgs(const FormatArg* args, SizeType count)
    : types_(-static_cast<long long>(count)) {
    SetData(args);
  }

  /**
   * Returns the argument at specified index.
   */
  FormatArg GetAt(SizeType index) const {
    FormatArg arg = DoGet(index);
    if (arg.type_ == internal::ArgType::NamedArg) {
      arg = arg.value_.AsNamedArg().template Deserialize<Context>();
    }
    return arg;
  }

  unsigned MaxCount() const {
    long long signed_types = static_cast<long long>(types_);
    return static_cast<unsigned>(
          signed_types < 0 ?
            -signed_types : static_cast<long long>(internal::MAX_PACKED_ARGS));
  }
};

struct FormatArgs : BasicFormatArgs<FormatContext> {
  template <typename... Args>
  FormatArgs(Args&&... args)
    : BasicFormatArgs<FormatContext>(Forward<Args>(args)...)
  {
  }
};

struct UFormatArgs : BasicFormatArgs<UFormatContext> {
  template <typename... Args>
  UFormatArgs(Args&&... args)
    : BasicFormatArgs<UFormatContext>(Forward<Args>(args)...)
  {
  }
};

namespace internal {

template <typename Char>
struct NamedArgBase {
  BasicStringView<Char> name;

  mutable char data[sizeof(BasicFormatArg<FormatContext>)];

  NamedArgBase(BasicStringView<Char> name)
    : name(name)
  {
  }

  template <typename Context>
  BasicFormatArg<Context> Deserialize() const {
    BasicFormatArg<Context> arg;
    UnsafeMemory::Memcpy(&arg, data, sizeof(BasicFormatArg<Context>));
    return arg;
  }
};

template <typename T, typename Char>
struct NamedArg : NamedArgBase<Char> {
  const T& value;

  NamedArg(BasicStringView<Char> name, const T& value)
    : NamedArgBase<Char>(name)
    , value(value)
  {
  }
};

} // namespace internal

/**
 * Returns a named argument to be used in a formatting function.
 *
 * **Example**::
 *
 *   fmt::print("Elapsed time: {s:.2f} seconds", fmt::arg("s", 1.23));
 */
template <typename T>
inline internal::NamedArg<T, char> Arg(StringView name, const T& value) {
  return { name, value };
}

template <typename T>
inline internal::NamedArg<T, UNICHAR> Arg(UStringView name, const T& value) {
  return { name, value };
}

// This function template is deleted intentionally to disable nested named
// arguments as in ``format("{}", arg("a", arg("b", 42)))``.
template <typename S, typename T, typename Char>
void Arg(S, internal::NamedArg<T, Char>) = delete;

// A base class for compile-time strings. It is defined in the fmt namespace to
// make formatting functions visible via ADL, e.g. format(fmt("{}"), 42).
struct CompileString {};

namespace internal {

// If S is a format string type, format_string_traints<S>::char_type gives its
// character type.
template <typename S, typename Enable = void>
class FormatStringTraits {
  // Use emptyness as a way to detect if format_string_traits is
  // specialized because other methods are broken on MSVC2013 or gcc 4.4.
  int dummy;
};

template <typename Char>
struct FormatStringTraitsBase {
  using CharType = Char;
};

template <typename Char>
struct FormatStringTraits<Char*> : FormatStringTraitsBase<Char> {};

template <typename Char>
struct FormatStringTraits<const Char*> : FormatStringTraitsBase<Char> {};

template <typename Char, size_t N>
struct FormatStringTraits<Char[N]> : FormatStringTraitsBase<Char> {};

template <typename Char, size_t N>
struct FormatStringTraits<const Char[N]> : FormatStringTraitsBase<Char> {};

//std::basic_string
template <typename Char>
struct FormatStringTraits<std::basic_string<Char>>
  : FormatStringTraitsBase<Char> {};

template <typename S>
struct FormatStringTraits<
  S, typename EnableIf<std::is_base_of<
      BasicStringView<typename S::CharType>, S>::value>::Type>
  : FormatStringTraitsBase<typename S::CharType> {};

template <typename S>
struct IsFormatString : IsEmpty<FormatStringTraits<S>> {};

template <typename S>
struct IsCompileString
  : IntegralConstant<bool, std::is_base_of<CompileString, S>::value> {};

template <typename... Args, typename S>
inline typename EnableIf<!IsCompileString<S>::Value>::Type
CheckFormatString(const S&)
{
}

template <typename... Args, typename S>
inline typename EnableIf<!IsCompileString<S>::Value>::Type
CheckFormatString(S);


//std::basic_string<Char>
template <typename Char>
std::basic_string<Char> VFormat(
  BasicStringView<Char> format_str,
  BasicFormatArgs<typename BufferContext<Char>::Type> args;

} // namespace internal

FormatContext::Iterator VFormatTo(
  internal::Buffer& buf, StringView format_str, FormatArgs args);

UFormatContext::Iterator VFormatTo(
  internal::UBuffer& buf, UStringView format_str, UFormatArgs args);

template <typename Container>
struct IsContiguous : FalseType {};

template <typename Char>
struct IsContiguous<std::basic_string<Char>> : TrueType {};

template <typename Char>
struct IsContiguous<internal::BasicString<Char>> : TrueType {};

/**
 * Formats a string and writes the output to ``out``
 */
template <typename Container>
typename EnableIf<
    IsContiguous<Container>::Value, BackInsertIterator<Container>
  >::Type
VFormatTo(BackInsertIterator<Container> out,
          StringView format_str, FormatArgs args) {
  intenral::ContainerBuffer<Container> buf(internal::GetContainer(out));
  VFormatTo(buf, format_str, args);
  return out;
}

template <typename Container>
typename EnableIf<
    IsContiguous<Container>::Value, BackInsertIterator<Container>
  >::Type
VFormatTo(BackInsertIterator<Container> out,
          UStringView format_str, UFormatArgs args) {
  intenral::ContainerBuffer<Container> buf(internal::GetContainer(out));
  VFormatTo(buf, format_str, args);
  return out;
}

template <typename Container, typename... Args>
typename EnableIf<
    IsContiguous<Container>::Value, BackInsertIterator<Container>
  >::Type
FormatTo(BackInsertIterator<Container> out,
          StringView format_str, const Args&... args) {
  FormatArgStore<FormatContext, Args...> as{args};
  return VFormatTo(buf, format_str, as);
}

template <typename Container, typename... Args>
typename EnableIf<
    IsContiguous<Container>::Value, BackInsertIterator<Container>
  >::Type
FormatTo(BackInsertIterator<Container> out,
          UStringView format_str, const Args&... args) {
  FormatArgStore<UFormatContext, Args...> as{args};
  return VFormatTo(buf, format_str, as);
}

//CharTraits를 사용해도 무방하지 않을까? 너무 길다...
template <
    typename S,
    typename Char = typename internal::FormatStringTraits<String>::CharType
  >
inline std::basic_string<Char> VFormat(
    const String& format_str,
    BasicFormatArgs<typename BufferContext<Char>::Type> args) {
  // Convert format string to string_view to reduce the number of overloads.
  return internal::VFormat(BasicStringView<Char>(format_str), args);
}

/**
 * Formats arguments and returns the result as a string.
 *
 * **Example**::
 *
 *   #include <fmt/core.h>
 *   std::string message = fmt::format("The answer is {}", 42);
 */
template <typename String, typename... Args>
inline std::basic_string<typename internal::FormatStringTraits<String>::CharType>
Format(const String& format_str, const Args&... args) {
  internal::CheckFormatString<Args...>(format_str);

  using ContextType = typename BufferContext<typename internal::FormatStringTraits<String>::CharType>::Type;
  FormatArgStore<ContextType, Args...> as {args...};
  return internal::VFormat(
      BasicStringView<typename internal::FormatStringTraits<String>::CharType>(format_str),
      BasicFormatArgs<ContextType>(as));
}

FUN_BASE_API void VPrint(std::FILE* fp, StringView format_str, FormatArgs args);
FUN_BASE_API void VPrint(std::FILE* fp, UStringView format_str, UFormatArgs args);

/**
 * Prints formatted data to the file *f*.
 *
 * **Example**::
 *
 *   fmt::print(stderr, "Don't {}!", "panic");
 */
template <typename... Args>
inline void Print(std::FILE* fp, StringView format_str, const Args&... args) {
  FormatArgStore<FormatContext, Args...> as(args...);
  VPrint(fp, format_str, as);
}

/**
 * Prints formatted data to the file *f* which should be in wide-oriented mode
 * set via ``fwide(f, 1)`` or ``_setmode(_fileno(f), _O_U8TEXT)`` on Windows.
 */
template <typename... Args>
inline void Print(std::FILE* fp, UStringView format_str, const Args&... args) {
  FormatArgStore<UFormatContext, Args...> as(args...);
  VPrint(fp, format_str, as);
}

FUN_BASE_API void VPrint(StringView format_str, FormatArgs args);
FUN_BASE_API void VPrint(UStringView format_str, UFormatArgs args);

/**
 * Prints formatted data to ``stdout``.
 *
 * **Example**::
 *
 *   fmt::print("Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... Args>
inline void Print(StringView format_str, const Args&... args) {
  FormatArgStore<FormatContext, Args...> as{args...};
  VPrint(format_str, as);
}

template <typename... Args>
inline void Print(UStringView format_str, const Args&... args) {
  FormatArgStore<UFormatContext, Args...> as{args...};
  VPrint(format_str, as);
}

} // namespace sf
} // namespace fun
