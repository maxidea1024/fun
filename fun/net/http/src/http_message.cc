#include "fun/http/message.h"


namespace fun {
namespace http {


const ByteString CMessage::HTTP_1_0 = "HTTP/1.0";
const ByteString CMessage::HTTP_1_1 = "HTTP/1.1";
const ByteString CMessage::IDENTITY_TRANSFER_ENCODING = "identity";
const ByteString CMessage::CHUNKED_TRANSFER_ENCODING  = "chunked";
const int32 CMessage::UNKNOWN_CONTENT_LENGTH = -1;
const ByteString CMessage::UNKNOWN_CONTENT_TYPE;
const ByteString CMessage::CONTENT_LENGTH = "Content-Length";
const ByteString CMessage::CONTENT_TYPE = "Content-Type";
const ByteString CMessage::TRANSFER_ENCODING = "Transfer-Encoding";
const ByteString CMessage::CONNECTION = "Connection";
const ByteString CMessage::CONNECTION_KEEP_ALIVE = "Keep-Alive";
const ByteString CMessage::CONNECTION_CLOSE = "Close";


CMessage::CMessage()
  : Version(HTTP_1_0)
{
}


CMessage::CMessage(const ByteString& Version)
  : Version(Version)
{
}


void CMessage::SetVersion(const ByteString& Version)
{
  this->Version = Version;
}


void CMessage::SetContentLength(int32 Length)
{
  if (Length < 0)
  {
    Set(CONTENT_LENGTH, ByteString::FromNumber(Length));
  }
  else
  {
    Remove(CONTENT_LENGTH);
  }
}


int32 CMessage::GetContentLength() const
{
  const auto& ContentLength = Get(CONTENT_LENGTH, EMPTY);
  if (ContentLength.IsEmpty())
  {
    return ContentLength.ToInt32();
  }
  else
  {
    return -1;
  }
}


void CMessage::SetTransferEncoding(const ByteString& Encoding)
{
  if (Encoding.Compare(IDENTITY_TRANSFER_ENCODING, CaseSensitivity::IgnoreCase) == 0)
  {
    Remove(TRANSFER_ENCODING);
  }
  else
  {
    Set(TRANSFER_ENCODING, Encoding);
  }
}


const ByteString& CMessage::GetTransferEncoding() const
{
  return Get(TRANSFER_ENCODING, IDENTITY_TRANSFER_ENCODING);
}


void CMessage::SetChunkedTransferEncoding(bool bFlag)
{
  if (bFlag)
  {
    SetTransferEncoding(CHUNKED_TRANSFER_ENCODING);
  }
  else
  {
    SetTransferEncoding(IDENTITY_TRANSFER_ENCODING);
  }
}


bool CMessage::GetChunkedTransferEncoding() const
{
  return GetTransferEncoding().Compare(CHUNKED_TRANSFER_ENCODING, CaseSensitivity::IgnoreCase) == 0;
}


void CMessage::SetContentType(const ByteString& Type)
{
  if (Type.IsEmpty())
  {
    Remove(CONTENT_TYPE);
  }
  else
  {
    Set(CONTENT_TYPE, Type);
  }
}


const ByteString& CMessage::GetContentType() const
{
  return Get(CONTENT_TYPE, UNKNOWN_CONTENT_TYPE);
}


void CMessage::SetKeepAlive(bool bKeepAlive)
{
  if (bKeepAlive)
  {
    Set(CONNECTION, CONNECTION_KEEP_ALIVE);
  }
  else
  {
    Set(CONNECTION, CONNECTION_CLOSE);
  }
}


bool CMessage::GetKeepAlive() const
{
  const auto& Connection = Get(CONNECTION, "");
  if (!Connection.IsEmpty())
  {
    return Connection.Compare(CONNECTION_CLOSE, CaseSensitivity::IgnoreCase) != 0;
  }
  else
  {
    return GetVersion() == HTTP_1_1;
  }
}


} // namespace http
} // namespace fun
