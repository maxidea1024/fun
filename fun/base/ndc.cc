#include "fun/base/ndc.h"
#include "fun/base/path.h"
#include "fun/base/singleton.h"
#include "fun/base/str.h"
#include "fun/base/thread_local.h"

namespace fun {

Ndc::Ndc() {}

Ndc::Ndc(const Ndc& rhs) : stack_(rhs.stack_) {}

Ndc::~Ndc() {}

Ndc& Ndc::operator=(const Ndc& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    stack_ = rhs.stack_;
  }
  return *this;
}

void Ndc::Push(const String& info) {
  Context c;
  c.info = info;
  c.line = -1;
  c.file = 0;
  stack_.PushBack(c);
}

void Ndc::Push(const String& info, int32 line, const char* file) {
  Context ctx;
  ctx.info = info;
  ctx.line = line;
  ctx.file = file;
  stack_.PushBack(ctx);
}

void Ndc::Pop() {
  if (!stack_.IsEmpty()) {
    stack_.PopBack();
  }
}

int32 Ndc::GetDepth() const { return stack_.Count(); }

String Ndc::ToString() const {
  String result;
  for (const auto& context : stack_) {
    if (!result.IsEmpty()) {
      result.Append(":");
    }
    result.Append(context.info);
  }
  return result;
}

void Ndc::Dump(std::ostream& ostr) const { Dump(ostr, "\n"); }

void Ndc::Dump(std::ostream& ostr, const String& delimiter,
               bool name_only) const {
  // TODO
  fun_check(0);

  /*
  for (Stack::const_iterator it = stack_.begin();;) {
    String file = it->file;
    if (name_only) {
      file = Path(file).GetFileName();
    }
    ostr << it->info;
    if (it->file) {
      ostr << " (in \"" << file << "\", line " << it->line << ")";
    }
    if (!it->trace.IsEmpty()) {
      ostr << "\nbacktrace:" << it->trace;
    }
    ++it;
    if (it == stack_.end()) {
      break;
    }
    ostr << delimiter;
  }
  */
}

void Ndc::Clear() { stack_.Clear(); }

// TODO windows 환경에서 backtrace 지원하기.
// thread별로 해야하지 않으려나??
//그냥 별도로 만들어주어야할까??
String Ndc::Backtrace(int32 skip_end, int32 skip_begin, int32 stack_size,
                      int32 buf_size) {
  String trace_buf;
#if FUN_HAS_BACKTRACE
#ifdef FUN_COMPILER_GCC
  const int kMaxFrames = sizeof(void*) * stack_size;
  void* addr_list[kMaxFrames + 1];
  int addr_len = ::backtrace(addr_list, kMaxFrames);

  if (addr_len != 0) {
    char** symbol_list = backtrace_symbols(addr_list, addr_len);

    size_t func_name_size = 256;
    char* func_name = (char*)malloc(func_name_size);

    int begin = skip_end;
    int end = addr_len - ((skip_begin <= addr_len) ? skip_begin : 0);
    trace_buf.Append("backtrace");
    if (begin > 1) {  // 0 is this function
      int skip = begin - 1;
      if (skip == 1) {
        trace_buf.Append(" (entry 1 skipped)");
      } else {
        trace_buf.Append(" (entries 1-" + std::to_string(begin - 1) +
                         " skipped)");
      }
    }
    trace_buf.Append(":\n");

    String prev_symbol;
    for (int i = begin; i < end; ++i) {
      char* begin_name = nullptr;
      char* begin_offset = nullptr;
      char* end_offset = nullptr;

      // find parentheses and +address offset surrounding the mangled name:
      // ./module(function+0x15c) [0x8048a6d]
      for (char* p = symbol_list[i]; *p; ++p) {
        if (*p == '(') {
          begin_name = p;
        } else {
          if (*p == '+') {
            begin_offset = p;
          } else if (*p == ')' && begin_offset) {
            end_offset = p;
            break;
          }
        }
      }

      if (begin_name && begin_offset && end_offset &&
          begin_name < begin_offset) {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        // mangled name:  [begin_name, begin_offset),
        // caller offset: [begin_offset, end_offset)
        int status;
        char* ret = abi::__cxa_demangle(begin_name, func_name, &func_name_size,
                                        &status);
        if (status == 0)  // demangled {
          func_name = ret;
        if (prev_symbol != symbol_list[i]) {  // output module once
          trace_buf.Append("\n> ");
          trace_buf.Append(symbol_list[i]);
          trace_buf.Append(":\n");
          prev_symbol = symbol_list[i];
        }
        trace_buf.Append(1, ' ').Append(std::to_string(i)).Append(1, ' ');
        trace_buf.Append(func_name).Append(1, '+').Append(begin_offset);
        trace_buf.Append(1, '\n');
      } else {  // demangling failed, output mangled
        trace_buf.Append("\n> ");
        trace_buf.Append(symbol_list[i]);
        trace_buf.Append(":\n");
        trace_buf.Append(1, ' ').Append(std::to_string(i)).Append(1, ' ');
        trace_buf.Append(begin_name).Append(1, '+').Append(begin_offset);
        trace_buf.Append(1, '\n');
      }
    }
    else {  // couldn't parse, output everything
      trace_buf.Append(1, ' ').Append(std::to_string(i)).Append(1, ' ');
      trace_buf.Append(symbol_list[i]).Append(1, '\n');
    }
  }
  if (trace_buf.size()) {
    trace_buf.erase(trace_buf.end() - 1);
  }
  free(func_name);
  free(symbol_list);

  if (end < addr_len) {
    if (end != addr_len - 1) {
      trace_buf.Append("\n " + std::to_string(end).Append(1, '-') +
                       std::to_string(addr_len - 1) + " (skipped)");
    } else {
      trace_buf.Append("\n " + std::to_string(end).Append(" (skipped)"));
    }
  }
}
#endif  // FUN_COMPILER_GCC
#else
  trace_buf = "[call trace not available]";
  (void)skip_end;
  (void)skip_begin;
  (void)stack_size;
  (void)buf_size;
#endif
return trace_buf;
}  // namespace fun

namespace {
static ThreadLocal<Ndc> ndc;
// TODO 그냥 아래처럼 해도 될듯한데...
//  static thread_local Ndc ndc;
}  // namespace

Ndc& Ndc::Current() { return ndc.Get(); }

}  // namespace fun
