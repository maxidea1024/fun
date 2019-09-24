#include "fun/base/exception.h"
#include "fun/base/ndc.h"
#include "fun/base/string/string.h"

// TODO InducerTag를 구지 넣을 필요는 없어보임...

//주의:
// Exception를 상속받은 오브젝트를 throw할 경우에는 각별히 조심해야함.
// Exception으로 collapse될수가 있으므로, 명시적으로 Rethrow()함수를
// 사용해서 throw 해야함.

// TODO move연산을 추가할까?

namespace fun {

Exception::Exception(const std::exception& std_exception)
    : std::exception(std_exception), message_(), nested_(nullptr), code_(0) {
  ConditionalAddBacktrace();
}

Exception::Exception(int32 code)
    : std::exception(), message_(), nested_(nullptr), code_(code) {
  ConditionalAddBacktrace();
}

Exception::Exception(const String& message, int32 code)
    : std::exception(), message_(message), nested_(nullptr), code_(code) {
  ConditionalAddBacktrace();
}

Exception::Exception(const String& message, const String& arg, int32 code)
    : std::exception(), message_(message), nested_(nullptr), code_(code) {
  if (!arg.IsEmpty()) {
    message_ << AsciiString(": ") << arg;
  }

  ConditionalAddBacktrace();
}

Exception::Exception(const String& message, const Exception& nested_exception,
                     int32 code)
    : std::exception(),
      message_(message),
      nested_(nested_exception.Clone()),
      code_(code) {
  ConditionalAddBacktrace();
}

Exception::Exception(const Exception& rhs)
    : std::exception(rhs),
      message_(rhs.message_),
      nested_(nullptr),
      code_(rhs.code_) {
  nested_ = rhs.nested_ ? rhs.nested_->Clone() : nullptr;
}

Exception::~Exception() throw() { delete nested_; }

void Exception::Destroy() { delete this; }

Exception& Exception::operator=(const Exception& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    Exception* new_nested = rhs.nested_ ? rhs.nested_->Clone() : nullptr;
    delete nested_;
    message_ = rhs.message_;
    nested_ = new_nested;
    code_ = rhs.code_;
  }
  return *this;
}

const char* Exception::GetName() const throw() { return "Exception"; }

const char* Exception::GetClassName() const throw() {
  // TODO 재정의를 통해서 rtti를 제거할 수 있을듯!!
  return typeid(*this).name();
  // return "Exception"; //@maxidea: namespace를 포함한 fully-qualified 형태로
  // 보여주어야 할까??
}

const char* Exception::what() const throw() {
  // TODO ByteString에 c_str() 추가하자.
  // return GetMessage().c_str();
  return GetMessage().ConstData();
}

String Exception::GetDisplayText() const {
  // String text = GetName();
  //
  // if (!message_.IsEmpty()) {
  //  text << AsciiString(": ") << message_;
  //}
  //
  // if (code_ != 0) {
  //  text << AsciiString(": code=") << String::FromNumber(code_);
  //}
  //
  // return text;

  String text;
  if (!message_.IsEmpty()) {
    text.Append(GetNamePrefixedMessage());
  } else {
    text = GetName();
  }

  if (code_ != 0) {
    text << AsciiString(": code=") << String::FromNumber(code_);
  }

  return text;
}

void Exception::ExtendedMessage(const String& arg) {
  if (!arg.IsEmpty()) {
    if (!message_.IsEmpty()) {
      message_ << AsciiString(": ");
    }
    message_ << arg;
  }
}

Exception* Exception::Clone() const { return new Exception(*this); }

/*! \fn void Exception::Rethrow() const

  예외를 재전파 할 경우, throw Ex와 같은 형태로 처리하면, Ex의 클래스 정보등이
  제대로 넘어가질 않습니다.

  문제있는 코드:
  \code
  try {
    ...
  } catch (Exception& e) {
    throw e; // Exception 형태로 강제로 복사되어 rethrow 되는듯함.
  }
  \endcode

  문제없는 코드:
  \code
  try {
    ...
  }
  catch (Exception& e) {
    e.Rethrow();
  }
  \encode
*/
void Exception::Rethrow() const { throw *this; }

void Exception::ConditionalAddBacktrace() {
#if FUN_EXCEPTION_BACKTRACE
  if (Ndc::HasBacktrace()) {
    message_.Append('\n').Append(Ndc::Backtrace(1, 2));
  }
#endif
}

//
// Generic exceptions.
//

FUN_IMPLEMENT_EXCEPTION(LogicException, "logic exception")
FUN_IMPLEMENT_EXCEPTION(AssertionViolationException, "assertion violation")
FUN_IMPLEMENT_EXCEPTION(NullPointerException, "null pointer")
FUN_IMPLEMENT_EXCEPTION(NullValueException, "null value")
FUN_IMPLEMENT_EXCEPTION(BugcheckException, "bugcheck")
FUN_IMPLEMENT_EXCEPTION(InvalidArgumentException, "invalid argument")
FUN_IMPLEMENT_EXCEPTION(NotImplementedException, "not implemented")
FUN_IMPLEMENT_EXCEPTION(RangeException, "out of range")
FUN_IMPLEMENT_EXCEPTION(IllegalStateException, "illegal state")
FUN_IMPLEMENT_EXCEPTION(InvalidAccessException, "invalid access")
FUN_IMPLEMENT_EXCEPTION(SignalException, "signal received")
FUN_IMPLEMENT_EXCEPTION(UnhandledException, "unhandled exception")

FUN_IMPLEMENT_EXCEPTION(RuntimeException, "runtime exception")
FUN_IMPLEMENT_EXCEPTION(NotFoundException, "not found")
FUN_IMPLEMENT_EXCEPTION(ExistsException, "exists")
FUN_IMPLEMENT_EXCEPTION(TimeoutException, "timeout")
FUN_IMPLEMENT_EXCEPTION(SystemException, "system exception")
FUN_IMPLEMENT_EXCEPTION(RegularExpressionException,
                        "error in regular expression")
FUN_IMPLEMENT_EXCEPTION(LibraryLoadException, "cannot load library")
FUN_IMPLEMENT_EXCEPTION(LibraryAlreadyLoadedException, "library already loaded")
FUN_IMPLEMENT_EXCEPTION(NoThreadAvailableException, "no thread available")
FUN_IMPLEMENT_EXCEPTION(PropertyNotSupportedException, "property not supported")
FUN_IMPLEMENT_EXCEPTION(PoolOverflowException, "pool overflow")
FUN_IMPLEMENT_EXCEPTION(NoPermissionException, "no permission")
FUN_IMPLEMENT_EXCEPTION(OutOfMemoryException, "out of memory")
FUN_IMPLEMENT_EXCEPTION(DataException, "data error")

FUN_IMPLEMENT_EXCEPTION(InterruptedException, "interrupted")
FUN_IMPLEMENT_EXCEPTION(IndexOutOfBoundsException, "index out of bounds")
FUN_IMPLEMENT_EXCEPTION(UnsupportedOperationException, "unsupported operation")
FUN_IMPLEMENT_EXCEPTION(EmptyStackException, "empty stack")
FUN_IMPLEMENT_EXCEPTION(StackOverflowException, "stack overflow")
FUN_IMPLEMENT_EXCEPTION(ArithmeticException, "arithmetic error")

FUN_IMPLEMENT_EXCEPTION(DataFormatException, "bad data format")
FUN_IMPLEMENT_EXCEPTION(SyntaxException, "syntax error")
FUN_IMPLEMENT_EXCEPTION(CircularReferenceException, "circular reference")
FUN_IMPLEMENT_EXCEPTION(PathSyntaxException, "bad path syntax")
FUN_IMPLEMENT_EXCEPTION(IoException, "I/O error")
FUN_IMPLEMENT_EXCEPTION(ProtocolException, "protocol error")
FUN_IMPLEMENT_EXCEPTION(FileException, "file access error")
FUN_IMPLEMENT_EXCEPTION(FileExistsException, "file exists")
FUN_IMPLEMENT_EXCEPTION(FileNotFoundException, "file not found")
FUN_IMPLEMENT_EXCEPTION(PathNotFoundException, "path not found")
FUN_IMPLEMENT_EXCEPTION(FileReadOnlyException, "file is read-only")
FUN_IMPLEMENT_EXCEPTION(FileAccessDeniedException, "access to file denied")
FUN_IMPLEMENT_EXCEPTION(CreateFileException, "cannot create file")
FUN_IMPLEMENT_EXCEPTION(OpenFileException, "cannot open file")
FUN_IMPLEMENT_EXCEPTION(WriteFileException, "cannot write file")
FUN_IMPLEMENT_EXCEPTION(ReadFileException, "cannot read file")
FUN_IMPLEMENT_EXCEPTION(DirectoryNotEmptyException, "directory not empty")
FUN_IMPLEMENT_EXCEPTION(UnknownUriSchemeException, "unknown URI scheme")
FUN_IMPLEMENT_EXCEPTION(TooManyUriRedirectsException, "too many URI redirects")
FUN_IMPLEMENT_EXCEPTION(UriSyntaxException, "bad URI syntax")

FUN_IMPLEMENT_EXCEPTION(ApplicationException, "application exception")
FUN_IMPLEMENT_EXCEPTION(BadCastException, "bad cast exception")

FUN_IMPLEMENT_EXCEPTION(TypeIncompatibleException,
                        "type incompatible exception")

// TODO Network 모듈쪽으로 옮겨주도록 하자.
//
// Network exceptions
//

FUN_IMPLEMENT_EXCEPTION(NetException, "network exception")
FUN_IMPLEMENT_EXCEPTION(InvalidAddressException, "invalid address")
FUN_IMPLEMENT_EXCEPTION(InvalidSocketException, "invalid socket")
FUN_IMPLEMENT_EXCEPTION(ServiceNotFoundException, "service not found")
FUN_IMPLEMENT_EXCEPTION(ConnectionAbortedException,
                        "software caused connection abort")
FUN_IMPLEMENT_EXCEPTION(ConnectionResetException, "connection reset by peer")
FUN_IMPLEMENT_EXCEPTION(ConnectionRefusedException, "connection refused")
FUN_IMPLEMENT_EXCEPTION(DnsException, "DNS error")
FUN_IMPLEMENT_EXCEPTION(HostNotFoundException, "host not found")
FUN_IMPLEMENT_EXCEPTION(NoAddressFoundException, "no address found")
FUN_IMPLEMENT_EXCEPTION(InterfaceNotFoundException, "interface not found")
FUN_IMPLEMENT_EXCEPTION(NoMessageException, "no message received")
FUN_IMPLEMENT_EXCEPTION(MessageException, "malformed message")
FUN_IMPLEMENT_EXCEPTION(MultipartException, "malformed multipart message")
FUN_IMPLEMENT_EXCEPTION(HttpException, "HTTP Exception")
FUN_IMPLEMENT_EXCEPTION(NotAuthenticatedException,
                        "no authentication information found")
FUN_IMPLEMENT_EXCEPTION(UnsupportedRedirectException,
                        "unsupported HTTP redirect (protocol change)")
FUN_IMPLEMENT_EXCEPTION(FtpException, "FTP Exception")
FUN_IMPLEMENT_EXCEPTION(SmtpException, "SMTP Exception")
FUN_IMPLEMENT_EXCEPTION(Pop3Exception, "POP3 Exception")
FUN_IMPLEMENT_EXCEPTION(IcmpException, "ICMP Exception")
FUN_IMPLEMENT_EXCEPTION(NtpException, "NTP Exception")
FUN_IMPLEMENT_EXCEPTION(HtmlFormException, "HTML Form Exception")
FUN_IMPLEMENT_EXCEPTION(WebSocketException, "WebSocket Exception")
FUN_IMPLEMENT_EXCEPTION(UnsupportedFamilyException,
                        "unknown or unsupported socket family.")

}  // namespace fun
