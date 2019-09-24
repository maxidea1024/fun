#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"
#include <exception>

namespace fun {

/**
 * This is the base class for all exceptions defined
 * in the FUN class library.
 */
class FUN_BASE_API Exception : public std::exception {
 public:
  /**
   * Creates an exception.
   */
  Exception(const std::exception& std_exception);

  /**
   * Creates an exception.
   */
  Exception(const String& message, int32 code = 0);

  /**
   * Creates an exception and stores a clone
   * of the nested exception.
   */
  Exception(const String& message, const String& arg, int32 code = 0);

  /**
   * Creates an exception and stores a clone
   * of the nested exception.
   */
  Exception(const String& message, const Exception& nested_exception, int32 code = 0);

  /**
   * Copy constructor.
   */
  Exception(const Exception& rhs);

  /**
   * Destroys the exception and deletes the nested exception.
   */
  ~Exception();

  /**
   *
   */
  void Destroy();

  /**
   * Assignment operator.
   */
  Exception& operator = (const Exception& rhs);

  /**
   * Returns a static string describing the exception.
   */
  virtual const char* GetName() const throw();

  /**
   * Returns the name of the exception class.
   */
  virtual const char* GetClassName() const throw();

  /**
   * Returns a static string describing the exception.
   *
   * Same as GetName(), but for compatibility with std::exception.
   */
  virtual const char* what() const throw();

  /**
   * Returns a pointer to the nested exception, or
   * null if no nested exception exists.
   */
  const Exception* GetNested() const;

  /**
   * Returns the message text.
   */
  const String& GetMessage() const;

  /**
   * Returns the exception code if defined.
   */
  int32 GetCode() const;

  /**
   * Returns a string consisting of the
   * message name and the message text.
   */
  String GetDisplayText() const;

  /**
   * Creates an exact copy of the exception.
   *
   * The copy can later be thrown again by
   * invoking Rethrow() on it.
   */
  virtual Exception* Clone() const;

  /**
   * (Re)Throws the exception.
   *
   * This is useful for temporarily storing a
   * copy of an exception (see clone()), then
   * throwing it again.
   */
  virtual void Rethrow() const;

 protected:
  /**
   * Standard constructor.
   */
  Exception(int32 code = 0);

  /**
   * Sets the message for the exception.
   */
  void SetMessage(const String& message);

  /**
   * Sets the extended message for the exception.
   */
  void ExtendedMessage(const String& arg);

  /**
   * Appends backtrace (if available) to the message.
   */
  void ConditionalAddBacktrace();

 private:
  mutable String message_;
  const Exception* nested_;
  int32 code_;

  String& GetNamePrefixedMessage() const {
    if (!message_.Contains(GetName())) {
      String s(GetName());
      s.Append(": ");
      message_.Prepend(s);
    }
    return message_;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE const Exception* Exception::GetNested() const {
  return nested_;
}

FUN_ALWAYS_INLINE const String& Exception::GetMessage() const {
  return message_;
}

FUN_ALWAYS_INLINE void Exception::SetMessage(const String& message) {
  message_ = message;
}

FUN_ALWAYS_INLINE int32 Exception::GetCode() const {
  return code_;
}

//NOTE
//추가 필드가 없거나, 생성자의 인자가 추가 되지 않은 경우에는 아래의 매크로를 사용해서 손십게 적용하면 됩니다.
//그렇지 않은 경우에는 다음과 같은 형태로 직접 Exception 클래스를 상속받아서 작성해 주어야 합니다.

//
// Macros for quickly declaring and implementing exception classes.
// Unfortunately, we cannot use a template here because character
// pointers (which we need for specifying the exception name)
// are not allowed as template arguments.
//
/*
    const char* GetName() const override throw();

    와 같이 override 뒤에 throw()가 오면 문법 오류가남.
    아래와 같이 해주어야함!

    const char* GetName() const throw() override;
*/
#define FUN_DECLARE_EXCEPTION_CODE(API, Class, ExSuper, Code) \
  class API Class : public ExSuper { \
   public: \
    typedef ExSuper Super; \
    Class(const std::exception& std_exception); \
    Class(fun::int32 code = Code); \
    Class(const fun::String& message, fun::int32 code = Code); \
    Class(const fun::String& message, const fun::String& arg, fun::int32 code = Code); \
    Class(const fun::String& message, const fun::Exception& nested_exception, fun::int32 code = Code); \
    ~Class() throw(); \
    Class(const Class& rhs); \
    Class& operator = (const Class& rhs); \
    const char* GetName() const throw() override; \
    const char* GetClassName() const throw() override; \
    fun::Exception* Clone() const override; \
    void Rethrow() const override; \
  };

#define FUN_DECLARE_EXCEPTION(API, Class, ExSuper) \
  FUN_DECLARE_EXCEPTION_CODE(API, Class, ExSuper, 0)

#define FUN_IMPLEMENT_EXCEPTION(Class, FriendlyName) \
  Class::Class(const std::exception& std_exception) \
    : Super(std_exception) {} \
  Class::Class(fun::int32 code) \
    : Super(code) {} \
  Class::Class(const fun::String& message, fun::int32 code) \
    : Super(message, code) {} \
  Class::Class(const fun::String& message, const fun::String& arg, fun::int32 code) \
    : Super(message, arg, code) {} \
  Class::Class(const fun::String& message, const fun::Exception& nested_exception, fun::int32 code) \
    : Super(message, nested_exception, code) {} \
  Class::Class(const Class& rhs) \
    : Super(rhs) {} \
  Class::~Class() {} \
  Class& Class::operator = (const Class& rhs) { \
    Super::operator = (rhs); \
    return *this; \
  } \
  const char* Class::GetName() const throw() { \
    return FriendlyName; \
  } \
  const char* Class::GetClassName() const throw() { \
    return #Class; \
  } \
  fun::Exception* Class::Clone() const { \
    return new Class(*this); \
  } \
  void Class::Rethrow() const { \
    throw *this; \
  }


//
// Standard exception classes
//

FUN_DECLARE_EXCEPTION(FUN_BASE_API, LogicException, Exception)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, AssertionViolationException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NullPointerException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NullValueException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, BugcheckException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InvalidArgumentException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NotImplementedException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, RangeException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, IllegalStateException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InvalidAccessException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, SignalException, LogicException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UnhandledException, LogicException)

FUN_DECLARE_EXCEPTION(FUN_BASE_API, RuntimeException, Exception)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NotFoundException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ExistsException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, TimeoutException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, SystemException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, RegularExpressionException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, LibraryLoadException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, LibraryAlreadyLoadedException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NoThreadAvailableException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, PropertyNotSupportedException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, PoolOverflowException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NoPermissionException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, OutOfMemoryException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, DataException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InterruptedException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, IndexOutOfBoundsException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UnsupportedOperationException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, EmptyStackException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, StackOverflowException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ArithmeticException, RuntimeException)

FUN_DECLARE_EXCEPTION(FUN_BASE_API, DataFormatException, DataException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, SyntaxException, DataException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, CircularReferenceException, DataException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, PathSyntaxException, SyntaxException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, IoException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ProtocolException, IoException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FileException, IoException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FileExistsException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FileNotFoundException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, PathNotFoundException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FileReadOnlyException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FileAccessDeniedException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, CreateFileException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, OpenFileException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, WriteFileException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ReadFileException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, DirectoryNotEmptyException, FileException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UnknownUriSchemeException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, TooManyUriRedirectsException, RuntimeException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UriSyntaxException, SyntaxException)

FUN_DECLARE_EXCEPTION(FUN_BASE_API, ApplicationException, Exception)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, BadCastException, RuntimeException)

FUN_DECLARE_EXCEPTION(FUN_BASE_API, TypeIncompatibleException, RuntimeException)


//TODO net 폴더로 이동시켜주는게 좋을듯...
//TODO net 폴더로 이동시켜주는게 좋을듯...
//TODO net 폴더로 이동시켜주는게 좋을듯...

//
// Network exceptions
//

FUN_DECLARE_EXCEPTION(FUN_BASE_API, NetException, IoException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InvalidAddressException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InvalidSocketException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ServiceNotFoundException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ConnectionAbortedException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ConnectionResetException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, ConnectionRefusedException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, DnsException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, HostNotFoundException, DnsException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NoAddressFoundException, DnsException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, InterfaceNotFoundException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NoMessageException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, MessageException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, MultipartException, MessageException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, HttpException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NotAuthenticatedException, HttpException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UnsupportedRedirectException, HttpException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, FtpException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, SmtpException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, Pop3Exception, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, IcmpException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, NtpException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, HtmlFormException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, WebSocketException, NetException)
FUN_DECLARE_EXCEPTION(FUN_BASE_API, UnsupportedFamilyException, NetException)

} // namespace fun
