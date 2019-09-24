#include "fun/http/session.h"

#include <algorithm>
#include <functional>
#include <string>

//#define CURL_STATICLIB
#include "ThirdParty/curl-7.52.1/include/curl/curl.h"

//static도 가능할듯 싶은데...??

#if FUN_PLATFORM_WINDOWS_FAMILY
# ifdef _DEBUG
#  pragma comment(lib, "ThirdParty/curl-7.52.1/build/Win64/VC14/DLL Debug/libcurld.lib")
# else
#  pragma comment(lib, "ThirdParty/curl-7.52.1/build/Win64/VC14/DLL Release/libcurl.lib")
# endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4312)
#endif


namespace fun {
namespace http {


struct CurlContext
{
  Uri* curl;
  struct curl_slist* chunk;
  struct curl_httppost* form_post;
  char error[CURL_ERROR_SIZE];
};


class SessionImpl
{
public:

  CImpl();
  ~CImpl();

  void SetUri(const Uri& uri);
  void SetParameters(const Parameters& parameters);
  void SetParameters(Parameters&& parameters);
  void SetHeaders(const Headers& headers);
  void SetTimeout(const Timeout& timeout);
  void SetAuth(const Auth& auth);
  void SetDigest(const Digest& auth);
  void SetPayload(Payload&& payload);
  void SetPayload(const Payload& payload);
  void SetProxies(Proxies&& proxies);
  void SetProxies(const Proxies& proxies);
  void SetMultipart(Multipart&& multipart);
  void SetMultipart(const Multipart& multipart);
  void SetRedirect(const Redirect& redirect);
  void SetMaxRedirects(const max_redirects& max_redirects);
  void SetCookies(const Cookies& cookies);
  void SetBody(Body&& body);
  void SetBody(const Body& body);
  void SetLowSpeed(const LowSpeed& low_speed);
  void SetVerifySsl(const VerifySsl& verify_ssl);

  Response Delete();
  Response Get();
  Response Head();
  Response Options();
  Response Patch();
  Response Post();
  Response Put();

private:

  CurlContext* curl_context_;
  Uri uri_;
  Parameters parameters_;
  Proxies proxies_;

  Response MakeRequest(Uri* curl);
  static void FreeCurlContext(CurlContext* curl_context);
  static CurlContext* AllocCurlContext();
};


SessionImpl::SessionImpl()
{
  curl_context_ = AllocCurlContext();

  auto curl = curl_context_->curl;
  if (curl)
  {
    // set up some sensible defaults.

    //TODO 좀더 상세한 정보를 넣어주는게 바람직해보임.
    //Uri 버젼
    //엔진 버젼
    //플랫폼 이름
    //엔진 빌드일자?
    //세션 이름? (유효하다면...)
    auto version_info = curl_version_info(CURLVERSION_NOW);
    auto version = String("FUN with Uri/") + version_info->version;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, version.ConstData());

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_context_->error);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
#ifdef CPR_CURL_NOSIGNAL
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#endif
#if LIBCURL_VERSION_MAJOR >= 7
#if LIBCURL_VERSION_MINOR >= 25
#if LIBCURL_VERSION_PATCH >= 0
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
#endif
#endif
#endif
  }
}


SessionImpl::~SessionImpl()
{
  FreeCurlContext(curl_context_);
}


void SessionImpl::FreeCurlContext(CurlContext* curl_context)
{
  curl_easy_cleanup(curl_context_->curl);
  curl_slist_free_all(curl_context_->chunk);
  curl_formfree(curl_context_->form_post);
  delete curl_context_;
}


CurlContext* SessionImpl::AllocCurlContext()
{
  auto* curl_context_ = new CurlContext();
  curl_context_->curl = curl_easy_init();
  return curl_context_;
}


void SessionImpl::SetUri(const Uri& uri)
{
  uri_ = uri;
}


void SessionImpl::SetParameters(const Parameters& parameters)
{
  this->parameters_ = parameters;
}


void SessionImpl::SetParameters(Parameters&& parameters)
{
  parameters_ = MoveTemp(parameters);
}


void SessionImpl::SetHeaders(const Headers& headers)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    struct curl_slist* chunk = nullptr;
    for (const auto& pair : headers)
    {
      auto header_string = pair.key;
      if (pair.value.IsEmpty())
      {
        header_string << ";";
      }
      else
      {
        header_string << ": ";
        header_string << pair.value;
      }
      chunk = curl_slist_append(chunk, header_string.ConstData());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    }
    curl_context_->chunk = chunk;
  }
}


void SessionImpl::SetTimeout(const Timeout& timeout)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout.millisec);
  }
}


void SessionImpl::SetAuth(const Auth& auth)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth.GetAuthString());
  }
}


void SessionImpl::SetDigest(const Digest& auth)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth.GetAuthString());
  }
}


void SessionImpl::SetPayload(Payload&& payload)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.content.Len());
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, payload.content.ConstData());
  }
}


void SessionImpl::SetPayload(const Payload& payload)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.content.Len());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.content.ConstData());
  }
}


void SessionImpl::SetProxies(const Proxies& proxies)
{
  proxies_ = proxies;
}


void SessionImpl::SetProxies(Proxies&& proxies)
{
  proxies_ = MoveTemp(proxies);
}


void SessionImpl::SetMultipart(Multipart&& multipart)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    struct curl_httppost* form_post = nullptr;
    struct curl_httppost* last_ptr = nullptr;

    for (auto& part : multipart.parts)
    {
      Array<struct curl_forms,InlineAllocator<32>> form_data;
      form_data.Add({CURLFORM_COPYNAME, part.name.ConstData()});
      if (part.is_buffer)
      {
        form_data.Add({CURLFORM_BUFFER, part.value.ConstData()});
        form_data.Add({CURLFORM_COPYCONTENTS, reinterpret_cast<const char*>(part.data)});
        form_data.Add({CURLFORM_CONTENTSLENGTH, reinterpret_cast<const char*>(part.data_len)});
      }
      else if (part.is_file)
      {
        form_data.Add({CURLFORM_FILE, part.value.ConstData()});
      }
      else
      {
        form_data.Add({CURLFORM_COPYCONTENTS, part.value.ConstData()});
      }
      if (!part.content_type.IsEmpty())
      {
        form_data.Add({CURLFORM_CONTENTTYPE, part.content_type.ConstData()});
      }
      form_data.Add({CURLFORM_END, nullptr});
      curl_formadd(&form_post, &last_ptr, CURLFORM_ARRAY, form_data.GetData(), CURLFORM_END);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form_post);

    curl_formfree(curl_context_->form_post);
    curl_context_->form_post = form_post;
  }
}


void SessionImpl::SetMultipart(const Multipart& multipart)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    struct curl_httppost* form_post = nullptr;
    struct curl_httppost* last_ptr = nullptr;

    for (auto& part : multipart.parts)
    {
      Array<struct curl_forms,InlineAllocator<32>> form_data;
      form_data.Add({CURLFORM_PTRNAME, part.Name.ConstData()});
      if (part.is_buffer)
      {
        form_data.Add({CURLFORM_BUFFER, part.value.ConstData()});
        form_data.Add({CURLFORM_BUFFERPTR, reinterpret_cast<const char*>(part.data)});
        form_data.Add({CURLFORM_BUFFERLENGTH, reinterpret_cast<const char*>(part.data_len)});
      }
      else if (part.is_file)
      {
        form_data.Add({CURLFORM_FILE, part.value.ConstData()});
      }
      else
      {
        form_data.Add({CURLFORM_PTRCONTENTS, part.value.ConstData()});
      }
      if (!part.content_type.IsEmpty())
      {
        form_data.Add({CURLFORM_CONTENTTYPE, part.content_type.ConstData()});
      }
      form_data.Add({CURLFORM_END, nullptr});
      curl_formadd(&form_post, &last_ptr, CURLFORM_ARRAY, form_data.GetData(), CURLFORM_END);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form_post);

    curl_formfree(curl_context_->form_post);
    curl_context_->form_post = form_post;
  }
}


void SessionImpl::SetRedirect(const Redirect& redirect)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, (int32)redirect.redirect);
  }
}


void SessionImpl::SetMaxRedirects(const max_redirects& max_redirects)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, max_redirects.limit);
  }
}


void SessionImpl::SetCookies(const Cookies& cookies)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL");
    curl_easy_setopt(curl, CURLOPT_COOKIE, cookies.GetEncoded().ConstData());
  }
}


void SessionImpl::SetBody(Body&& body)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.data.Len());
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, body.data.ConstData());
  }
}


//FIXME 참조를 유지해주어야할듯한데?

void SessionImpl::SetBody(const Body& body)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.data.Len());
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, body.data.ConstData());
  }
}


void SessionImpl::SetLowSpeed(const LowSpeed& low_speed)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, low_speed.limit);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, low_speed.time);
  }
}


void SessionImpl::SetVerifySsl(const VerifySsl& verify_ssl)
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify_ssl.verify ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_ssl.verify ? 2L : 0L);
  }
}


Response SessionImpl::Delete()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  }

  return MakeRequest(curl);
}


Response SessionImpl::Get()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
  }

  return MakeRequest(curl);
}


Response SessionImpl::Head()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  }

  return MakeRequest(curl);
}


Response SessionImpl::Options()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
  }

  return MakeRequest(curl);
}


Response SessionImpl::Patch()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
  }

  return MakeRequest(curl);
}


Response SessionImpl::Post()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
  }

  return MakeRequest(curl);
}


Response SessionImpl::Put()
{
  auto curl = curl_context_->curl;
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  }

  return MakeRequest(curl);
}


namespace {


size_t WriteFunction(void* ptr, size_t size, size_t nmemb, String* data)
{
  data->Append((char*)ptr, (int32)(size * nmemb));
  return size * nmemb;
}


Headers ParseHeaders(const String& src)
{
  Headers dst;

  //TODO 이게 가능 하도록 수정하는게 좋아보임.
    //Array<String, InlineAllocator<32>> lines;
  Array<String> lines;
  src.SplitLines(lines);

  for (const auto& Line : lines)
  {
    // "HTTP/1.1 200 OK"
    if (Line.StartsWith("HTTP/")) // begin of response.
    {
      dst.Clear();
    }

    if (!Line.IsEmpty())
    {
      const auto colon_index = Line.IndexOf(":");
      if (colon_index != INVALID_INDEX)
      {
        const auto key = Line.Mid(0, colon_index).Trimmed();
        const auto value = Line.Mid(colon_index + 1).Trimmed();

        //FIXME [] 연산자로 접근시 추가가 안됨. (이게 바람직한건지..??)
                //dst[key] = value;
        dst.Add(key, value);
      }
    }
  }

  return MoveTemp(dst);
}


// Tracing...

struct CurlDebugContext
{
  bool ascii_only;
  String* output;

  CurlDebugContext()
    : ascii_only(false)
    , output(nullptr)
  {
  }
};


void DumpOperation(String& output, const char* operation, const uint8* data, int32 length, bool ascii_only)
{
  const int32 width = ascii_only ? (80 - 2) : 16; // hex 출력시에는 한줄당 16 octets 단위로 출력합니다. ascii만 출력시에는 78글자를 출력합니다.

  output << String::Format("%s, %d bytes\n", operation, length);

  for (int32 i = 0; i < length; i += width)
  {
    output << String::Format("   %4.4lX| ", (long)i);

    if (!ascii_only)
    {
      // hex not disabled, show it
      for (int32 c = 0; c < width; ++c)
      {
        if (i + c < length)
        {
          const uint8 Byte = data[i+c];
          output << (char)NibbleToChar(Byte >> 4);
          output << (char)NibbleToChar(Byte & 0xF);
          output << " ";
        }
        else
        {
          output << "   ";
        }
      }
    }

    for (int32 c = 0; (c < width) && (i + c < length); ++c)
    {
      // check for 0D0A; if found, skip past and start a new line of output.
      if (ascii_only && (i + c + 1 < length) && data[i + c] == 0x0D && data[i + c + 1] == 0x0A)
      {
        i += (c + 2 - width);
        break;
      }

      const char ascii_ch = (data[i + c] >= 0x20) && (data[i + c] < 0x80) ? data[i+c] : '.';
      output << ascii_ch;

      // check again for 0D0A, to avoid an extra \n if it's at width.
      if (ascii_only && (i + c + 2 < length) && data[i + c + 1] == 0x0D && data[i + c + 2] == 0x0A)
      {
        i += (c + 3 - width);
        break;
      }
    }

    output << "\n"; // newline
  }
}


//CURLINFO_TEXT
//CURLINFO_HEADER_IN
//CURLINFO_HEADER_OUT
//CURLINFO_DATA_IN
//CURLINFO_DATA_OUT
//CURLINFO_SSL_DATA_IN
//CURLINFO_SSL_DATA_OUT
//CURLINFO_END
static int32 TraceCurlOperation(Uri*, curl_infotype type, char* data, size_t length, void* user_context)
{
  CurlDebugContext* context = (CurlDebugContext*)user_context;

  if (context->output == nullptr)
  {
    return 0;
  }

  const char* operation = "";

  switch (type) {
  case CURLINFO_TEXT:
    *context->output << "== INFO: " << data;
  // FALLTHROUGH
  default: // in case a new one is introduced to shock us
    return 0;

  case CURLINFO_HEADER_OUT:   operation = "=> SEND HEADER"; break;
  case CURLINFO_DATA_OUT:     operation = "=> SEND DATA"; break;
  case CURLINFO_SSL_DATA_OUT: operation = "=> SEND SSL DATA"; break;
  case CURLINFO_HEADER_IN:    operation = "<= RECV HEADER"; break;
  case CURLINFO_DATA_IN:      operation = "<= RECV DATA"; break;
  case CURLINFO_SSL_DATA_IN:  operation = "<= RECV SSL DATA"; break;
  }

  *context->output << String::Format("%s, %d bytes\n", operation, (int32)length);
  String::BytesToDebuggableString(*context->output, String::FromRawData(data, length), context->ascii_only, "   ");

  return 0;
}

} // namespace


Response SessionImpl::MakeRequest(Uri* curl)
{
  String curl_debug_output;

  //TODO console 변수를 등록해서, 임의로 설정할 수 있도록 하는것도 좋을듯함..
  static bool trace_curl = true;//CommandLine::Get().
  if (trace_curl)
  {
    curl_debug_output.Reserve(8192);
    curl_debug_output = "\n";

    struct CurlDebugContext curl_debug_context;
    curl_debug_context.ascii_only = false;
    curl_debug_context.output = &curl_debug_output;

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // the DEBUGFUNCTION has no effect until we enable VERBOSE
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, TraceCurlOperation);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &curl_debug_context);
  }

  if (!parameters_.content.IsEmpty())
  {
    Uri new_uri(uri_.ToString() + '?' + parameters_.content);
    curl_easy_setopt(curl, CURLOPT_URL, new_uri.ToString().ConstData());
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_URL, uri_.ToString().ConstData());
  }

  const auto protocol = uri_.GetScheme();
  if (proxies.Has(protocol))
  {
    curl_easy_setopt(curl, CURLOPT_PROXY, proxies[protocol].ConstData());
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_PROXY, "");
  }

  curl_context_->error[0] = '\0';

  String response_string;
  String header_string;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

  const auto curr_error = curl_easy_perform(curl);

  if (!curl_debug_output.IsEmpty())
  {
    LOG(LogCore, Info, "%s", *String(curl_debug_output));
  }


  char* raw_url;
  long response_code;
  double elapsed;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
  curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &raw_url);


  Error error(curr_error, curl_context_->error);

  Cookies cookies;
  struct curl_slist* raw_cookies;
  curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &raw_cookies);
  for (struct curl_slist* it = raw_cookies; it; it = it->next)
  {
    auto tokens = String(it->data).Split('\t');

    const auto value = tokens.Last();
    tokens.RemoveLast();
    cookies[tokens.Last()] = value;
  }
  curl_slist_free_all(raw_cookies);

  Headers headers;
  if (!header_string.IsEmpty())
  {
    headers = ParseHeaders(header_string);
  }

  return Response{(int32)response_code, response_string, headers, raw_url, elapsed, cookies, error};
}


Session::Session() : impl_{ new CImpl{} } {}
Session::~Session() { delete impl_; }

void Session::SetUri(const Uri& uri) { impl_->SetUri(uri); }
void Session::SetUri(const String& uri) { impl_->SetUri(Uri(uri)); }
void Session::SetParameters(const Parameters& parameters) { impl_->SetParameters(parameters); }
void Session::SetParameters(Parameters&& parameters) { impl_->SetParameters(MoveTemp(parameters)); }
void Session::SetHeaders(const Headers& headers) { impl_->SetHeaders(headers); }
void Session::SetTimeout(const Timeout& timeout) { impl_->SetTimeout(timeout); }
void Session::SetAuth(const Auth& auth) { impl_->SetAuth(auth); }
void Session::SetDigest(const Digest& digest) { impl_->SetDigest(digest); }
void Session::SetPayload(const Payload& payload) { impl_->SetPayload(payload); }
void Session::SetPayload(Payload&& payload) { impl_->SetPayload(MoveTemp(payload)); }
void Session::SetProxies(const Proxies& proxies) { impl_->SetProxies(proxies); }
void Session::SetProxies(Proxies&& proxies) { impl_->SetProxies(MoveTemp(proxies)); }
void Session::SetMultipart(const Multipart& multipart) { impl_->SetMultipart(multipart); }
void Session::SetMultipart(Multipart&& multipart) { impl_->SetMultipart(MoveTemp(multipart)); }
void Session::SetRedirect(const Redirect& redirect) { impl_->SetRedirect(redirect); }
void Session::SetMaxRedirects(const max_redirects& max_redirects) { impl_->SetMaxRedirects(max_redirects); }
void Session::SetCookies(const Cookies& Cookies) { impl_->SetCookies(Cookies); }
void Session::SetBody(const Body& body) { impl_->SetBody(body); }
void Session::SetBody(Body&& body) { impl_->SetBody(MoveTemp(body)); }
void Session::SetBody(const String& body) { impl_->SetBody(Body(body)); }
void Session::SetBody(String&& body) { impl_->SetBody(Body(MoveTemp(body))); }
void Session::SetLowSpeed(const LowSpeed& low_speed) { impl_->SetLowSpeed(low_speed); }
void Session::SetVerifySsl(const VerifySsl& verify_ssl) { impl_->SetVerifySsl(verify_ssl); }

void Session::SetOption_INTERNAL(const Uri& uri) { impl_->SetUri(uri); }
void Session::SetOption_INTERNAL(const Parameters& parameters) { impl_->SetParameters(parameters); }
void Session::SetOption_INTERNAL(Parameters&& parameters) { impl_->SetParameters(MoveTemp(parameters)); }
void Session::SetOption_INTERNAL(const Headers& headers) { impl_->SetHeaders(headers); }
void Session::SetOption_INTERNAL(const Timeout& timeout) { impl_->SetTimeout(timeout); }
void Session::SetOption_INTERNAL(const Auth& auth) { impl_->SetAuth(auth); }
void Session::SetOption_INTERNAL(const Digest& digest) { impl_->SetDigest(digest); }
void Session::SetOption_INTERNAL(const Payload& payload) { impl_->SetPayload(payload); }
void Session::SetOption_INTERNAL(Payload&& payload) { impl_->SetPayload(MoveTemp(payload)); }
void Session::SetOption_INTERNAL(const Proxies& proxies) { impl_->SetProxies(proxies); }
void Session::SetOption_INTERNAL(Proxies&& proxies) { impl_->SetProxies(MoveTemp(proxies)); }
void Session::SetOption_INTERNAL(const Multipart& multipart) { impl_->SetMultipart(multipart); }
void Session::SetOption_INTERNAL(Multipart&& multipart) { impl_->SetMultipart(MoveTemp(multipart)); }
void Session::SetOption_INTERNAL(const Redirect& redirect) { impl_->SetRedirect(redirect); }
void Session::SetOption_INTERNAL(const max_redirects& max_redirects) { impl_->SetMaxRedirects(max_redirects); }
void Session::SetOption_INTERNAL(const Cookies& cookies) { impl_->SetCookies(cookies); }
void Session::SetOption_INTERNAL(const Body& body) { impl_->SetBody(body); }
void Session::SetOption_INTERNAL(Body&& body) { impl_->SetBody(MoveTemp(body)); }
void Session::SetOption_INTERNAL(const LowSpeed& low_speed) { impl_->SetLowSpeed(low_speed); }
void Session::SetOption_INTERNAL(const VerifySsl& verify_ssl) { impl_->SetVerifySsl(verify_ssl); }


Response Session::Request(Method method)
{
  switch (method) {
  case Method::DELETE: return Delete();
  case Method::GET: return Get();
  case Method::HEAD: return Head();
  case Method::OPTIONS: return Options();
  case Method::PATCH: return Patch();
  case Method::POST: return Post();
  case Method::PUT: return Put();
  }

  throw HttpException(StringLiteral("unsupported method"));
}


Response Session::Delete() { return impl_->Delete(); }
Response Session::Get() { return impl_->Get(); }
Response Session::Head() { return impl_->Head(); }
Response Session::Options() { return impl_->Options(); }
Response Session::Patch() { return impl_->Patch(); }
Response Session::Post() { return impl_->Post(); }
Response Session::Put() { return impl_->Put(); }


} // namespace http
} // namespace fun
