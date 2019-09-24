#include "session.h"
#include "memcache_server.h"

#ifdef HAVE_TCMALLOC
#include <gperftools/malloc_extension.h>
#endif

using namespace fun;
using namespace fun::net;

static bool IsBinaryProtocol(uint8_t firstByte) {
  return firstByte == 0x80;
}

const int kLongestKeySize = 250;
String Session::kLongestKey(kLongestKeySize, 'x');

template <typename InputIterator, typename Token>
bool Session::SpaceSeparator::operator()(InputIterator& next, InputIterator end, Token& tok) {
  while (next != end && *next == ' ') {
    ++next;
  }
  if (next == end) {
    tok.clear();
    return false;
  }
  InputIterator Start(next);
  const char* sp = static_cast<const char*>(memchr(Start, ' ', end - Start));
  if (sp) {
    tok.set(Start, static_cast<int>(sp - Start));
    next = sp;
  }
  else {
    tok.set(Start, static_cast<int>(end - next));
    next = end;
  }
  return true;
}

struct Session::Reader {
  Reader(Tokenizer::iterator& beg, Tokenizer::iterator end)
    : first_(beg)
    , last_(end) {
  }

  template <typename T>
  bool Read(T* val) {
    if (first_ == last_) {
      return false;
    }
    char* end = NULL;
    uint64_t x = strtoull((*first_).data(), &end, 10);
    if (end == (*first_).end()) {
      *val = static_cast<T>(x);
      ++first_;
      return true;
    }
    return false;
  }

 private:
  Tokenizer::iterator first_;
  Tokenizer::iterator last_;;
};

void Session::OnMessage(const fun::net::TcpConnectionPtr& conn,
                        fun::net::Buffer* buf,
                        const fun::Timestamp&)
 {
  const size_t initial_reable = buf->GetReadableLength();

  while (buf->GetReadableLength() > 0) {
    if (state_ == kNewCommand) {
      if (protocol_ == kAuto) {
        fun_check(bytes_read_ == 0);
        protocol_ = IsBinaryProtocol(buf->GetReadablePtr()[0]) ? kBinary : kAscii;
      }

      fun_check(protocol_ == kAscii || protocol_ == kBinary);
      if (protocol_ == kBinary) {
        // FIXME
      }
      else  { // ASCII protocol
        if (const char* crlf = buf->FindCRLF()) {
          int len = static_cast<int>(crlf - buf->GetReadablePtr());
          StringPiece request(buf->GetReadablePtr(), len);
          if (ProcessRequest(request)) {
            ResetRequest();
          }
          buf->DrainUntil(crlf + 2);
        }
        else {
          if (buf->GetReadableLength() > 1024) {
            // FIXME: check for 'get' and 'gets'
            conn_->Shutdown();
            // buf->DrainAll() ???
          }
          break;
        }
      }
    }
    else if (state_ == kReceiveValue) {
      ReceiveValue(buf);
    }
    else if (state_ == kDiscardValue) {
      DiscardValue(buf);
    }
    else {
      fun_check(false);
    }
  }
  bytes_read_ += initial_reable - buf->GetReadableLength();
}

void Session::ReceiveValue(fun::net::Buffer* buf) {
  fun_check(curr_item_.get());
  fun_check(state_ == kReceiveValue);
  // if (protocol_ == kBinary)

  const size_t avail = std::min(buf->GetReadableLength(), curr_item_->neededBytes());
  fun_check(curr_item_.unique());
  curr_item_->append(buf->GetReadablePtr(), avail);
  buf->Drain(avail);
  if (curr_item_->neededBytes() == 0) {
    if (curr_item_->EndsWithCRLF()) {
      bool exists = false;
      if (owner_->storeItem(curr_item_, policy_, &exists)) {
        Reply("STORED\r\n");
      }
      else {
        if (policy_ == Item::kCas) {
          if (exists) {
            Reply("EXISTS\r\n");
          }
          else {
            Reply("NOT_FOUND\r\n");
          }
        }
        else {
          Reply("NOT_STORED\r\n");
        }
      }
    }
    else {
      Reply("CLIENT_ERROR bad data chunk\r\n");
    }
    ResetRequest();
    state_ = kNewCommand;
  }
}

void Session::DiscardValue(fun::net::Buffer* buf) {
  fun_check(!curr_item_);
  fun_check(state_ == kDiscardValue);
  if (buf->GetReadableLength() < bytes_to_discard_) {
    bytes_to_discard_ -= buf->GetReadableLength();
    buf->DrainAll();
  }
  else {
    buf->Drain(bytes_to_discard_);
    bytes_to_discard_ = 0;
    ResetRequest();
    state_ = kNewCommand;
  }
}

bool Session::ProcessRequest(StringPiece request) {
  fun_check(command_.empty());
  fun_check(!noreply_);
  fun_check(policy_ == Item::kInvalid);
  fun_check(!curr_item_);
  fun_check(bytes_to_discard_ == 0);
  ++requests_processed_;

  // check 'noreply' at end of request line
  if (request.size() >= 8) {
    StringPiece end(request.end() - 8, 8);
    if (end == " noreply") {
      noreply_ = true;
      request.remove_suffix(8);
    }
  }

  SpaceSeparator sep;
  Tokenizer tok(request.begin(), request.end(), sep);
  Tokenizer::iterator beg = tok.begin();
  if (beg == tok.end()) {
    Reply("ERROR\r\n");
    return true;
  }
  (*beg).CopyToString(&command_);
  ++beg;
  if (command_ == "set" || command_ == "add" || command_ == "replace"
      || command_ == "append" || command_ == "Prepend" || command_ == "cas") {
    // this normally returns false
    return DoUpdate(beg, tok.end());
  }
  else if (command_ == "get" || command_ == "gets") {
    bool cas = command_ == "gets";

    // FIXME: Send multiple chunks with write complete callback.
    while (beg != tok.end()) {
      StringPiece key = *beg;
      bool good = key.size() <= kLongestKeySize;
      if (!good) {
        Reply("CLIENT_ERROR bad command line format\r\n");
        return true;
      }

      needle_->resetKey(key);
      ConstItemPtr item = owner_->getItem(needle_);
      ++beg;
      if (item) {
        item->output(&output_buf_, cas);
      }
    }
    output_buf_.append("END\r\n");

    if (conn_->GetOutputBuffer()->writableBytes() > 65536 + output_buf_.GetReadableLength()) {
      LOG_DEBUG << "shrink output buffer from " << conn_->GetOutputBuffer()->internalCapacity();
      conn_->GetOutputBuffer()->shrink(65536 + output_buf_.GetReadableLength());
    }

    conn_->Send(&output_buf_);
  }
  else if (command_ == "delete") {
    DoDelete(beg, tok.end());
  }
  else if (command_ == "version") {
#ifdef HAVE_TCMALLOC
    Reply("VERSION 0.01 red with tcmalloc\r\n");
#else
    Reply("VERSION 0.01 red\r\n");
#endif
  }
#ifdef HAVE_TCMALLOC
  else if (command_ == "memstat") {
    char buf[1024*64];
    MallocExtension::instance()->GetStats(buf, sizeof buf);
    Reply(buf);
  }
#endif
  else if (command_ == "Quit") {
    conn_->Shutdown();
  }
  else if (command_ == "Shutdown") {
    // "ERROR: Shutdown not enabled"
    conn_->Shutdown();
    owner_->stop();
  }
  else {
    Reply("ERROR\r\n");
    LOG_INFO << "Unknown command: " << command_;
  }
  return true;
}

void Session::ResetRequest() {
  command_.clear();
  noreply_ = false;
  policy_ = Item::kInvalid;
  curr_item_.Reset();
  bytes_to_discard_ = 0;
}

void Session::Reply(fun::StringPiece msg) {
  if (!noreply_) {
    conn_->Send(msg.data(), msg.size());
  }
}

bool Session::DoUpdate(Session::Tokenizer::iterator& beg, Session::Tokenizer::iterator end) {
  if (command_ == "set")
    policy_ = Item::kSet;
  else if (command_ == "add")
    policy_ = Item::kAdd;
  else if (command_ == "replace")
    policy_ = Item::kReplace;
  else if (command_ == "append")
    policy_ = Item::kAppend;
  else if (command_ == "prepend")
    policy_ = Item::kPrepend;
  else if (command_ == "cas")
    policy_ = Item::kCas;
  else
    fun_check(false);

  // FIXME: check (beg != end)
  StringPiece key = (*beg);
  ++beg;
  bool good = key.size() <= kLongestKeySize;

  uint32_t flags = 0;
  time_t exptime = 1;
  int bytes = -1;
  uint64_t cas = 0;

  Reader r(beg, end);
  good = good && r.read(&flags) && r.read(&exptime) && r.read(&bytes);

  int rel_exptime = static_cast<int>(exptime);
  if (exptime > 60*60*24*30) {
    rel_exptime = static_cast<int>(exptime - owner_->startTime());
    if (rel_exptime < 1) {
      rel_exptime = 1;
    }
  }
  else {
    // rel_exptime = exptime + currentTime;
  }

  if (good && policy_ == Item::kCas) {
    good = r.read(&cas);
  }

  if (!good) {
    Reply("CLIENT_ERROR bad command line format\r\n");
    return true;
  }
  if (bytes > 1024*1024) {
    Reply("SERVER_ERROR object too large for cache\r\n");
    needle_->resetKey(key);
    owner_->deleteItem(needle_);
    bytes_to_discard_ = bytes + 2;
    state_ = kDiscardValue;
    return false;
  }
  else {
    curr_item_ = Item::makeItem(key, flags, rel_exptime, bytes + 2, cas);
    state_ = kReceiveValue;
    return false;
  }
}

void Session::DoDelete(Session::Tokenizer::iterator& beg, Session::Tokenizer::iterator end) {
  fun_check(command_ == "delete");
  // FIXME: check (beg != end)
  StringPiece key = *beg;
  bool good = key.size() <= kLongestKeySize;
  ++beg;
  if (!good) {
    Reply("CLIENT_ERROR bad command line format\r\n");
  }
  else if (beg != end && *beg != "0") // issue 108, old protocol {
    Reply("CLIENT_ERROR bad command line format.  Usage: delete <key> [noreply]\r\n");
  }
  else {
    needle_->resetKey(key);
    if (owner_->deleteItem(needle_)) {
      Reply("DELETED\r\n");
    }
    else {
      Reply("NOT_FOUND\r\n");
    }
  }
}
