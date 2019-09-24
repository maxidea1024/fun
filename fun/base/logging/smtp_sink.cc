
namespace fun {

const String SmtpSink::PROP_MAIL_HOST("MailHost");
const String SmtpSink::PROP_SENDER("Sender");
const String SmtpSink::PROP_RECIPIENT("Recipient");
const String SmtpSink::PROP_LOCAL("Local");
const String SmtpSink::PROP_ATTACHMENT("Attachment");
const String SmtpSink::PROP_TYPE("Type");
const String SmtpSink::PROP_DELETE("Delete");
const String SmtpSink::PROP_THROW("Throw");

FUN_IMPLEMENT_RTCLASS(SmtpSink)

SmtpSink::SmtpSink()
    : mail_host_("localhost"),
      local_(true),
      type_("text/plain"),
      delete_(false),
      throw_(false) {}

SmtpSink::SmtpSink(const String& mail_host, const String& sender,
                   const String& recipient)
    : mail_host_(mail_host),
      sender_(sender),
      recipient_(recipient),
      local_(true),
      type_("text/plain"),
      delete_(false),
      throw_(false) {}

SmtpSink::~SmtpSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void SmtpSink::Open() {}

void SmtpSink::Close() {}

void SmtpSink::Log(const LogMessage& msg) {
  // TODO
}

void SmtpSink::SetProperty(const String& name, const String& value) {
  // TODO
}

String SmtpSink::GetProperty(const String& name) const {
  // TODO
}

// FUN_IMPLEMENT_RTCLASS(SmtpSink);

void SmtpSink::RegisterSink() {
  LoggingFactory::DefaultFactory().RegisterSinkClass(
      "SmtpSink", new Instantiator<SmtpSink, LogSink>);
}

}  // namespace fun
