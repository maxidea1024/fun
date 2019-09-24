#include "fun/framework/help_formatter.h"
#include "fun/framework/option.h"
#include "fun/framework/option_set.h"

namespace fun {
namespace framework {

const int32 HelpFormatter::TAB_WIDTH = 4;
const int32 HelpFormatter::LINE_WIDTH = 78;

HelpFormatter::HelpFormatter(const OptionSet& options)
    : options_(options), width_(LINE_WIDTH), indent_(0), unix_style_(true) {
#if !FUN_PLATFORM_UNIX_FAMILY
  unix_style_ = false;
#endif
  indent_ = CalcIndent();
}

HelpFormatter::~HelpFormatter() {}

void HelpFormatter::SetCommand(const String& command) { command_ = command; }

void HelpFormatter::SetUsage(const String& usage) { usage_ = usage; }

void HelpFormatter::SetHeader(const String& header) { header_ = header; }

void HelpFormatter::SetFooter(const String& footer) { footer_ = footer; }

void HelpFormatter::Format(std::ostream& ostr) const {
  ostr << "usage: " << command_.c_str();
  if (!usage_.IsEmpty()) {
    ostr << ' ';
    FormatText(ostr, usage_, command_.Len() + 1);
  }

  ostr << '\n';
  if (!header_.IsEmpty()) {
    FormatText(ostr, header_, 0);
    ostr << "\n\n";
  }

  FormatOptions(ostr);

  if (!footer_.IsEmpty()) {
    ostr << '\n';
    FormatText(ostr, footer_, 0);
    ostr << '\n';
  }
}

void HelpFormatter::SetWidth(int32 width) {
  fun_check(width > 0);

  width_ = width;
}

void HelpFormatter::SetIndent(int32 indent) {
  fun_check(indent >= 0 && indent < width_);

  indent_ = indent;
}

void HelpFormatter::SetAutoIndent() { indent_ = CalcIndent(); }

void HelpFormatter::SetUnixStyle(bool flag) { unix_style_ = flag; }

int32 HelpFormatter::CalcIndent() const {
  int32 indent = 0;
  for (OptionSet::Iterator it = options_.begin(); it != options_.end(); ++it) {
    int32 short_len = it->GetShortName().Len();
    int32 full_len = it->GetFullName().Len();
    int32 n = 0;

    if (unix_style_ && short_len > 0) {
      n += short_len + GetShortPrefix().Len();
      if (it->TakesArgument()) {
        n += it->GetArgumentName().Len() + (it->IsArgumentRequired() ? 0 : 2);
      }
      if (full_len > 0) {
        n += 2;
      }
    }

    if (full_len > 0) {
      n += full_len + GetLongPrefix().Len();
      if (it->TakesArgument()) {
        n += 1 + it->GetArgumentName().Len() +
             (it->IsArgumentRequired() ? 0 : 2);
      }
    }
    n += 2;
    if (n > indent) {
      indent = n;
    }
  }

  return indent;
}

void HelpFormatter::FormatOptions(std::ostream& ostr) const {
  int32 option_width = CalcIndent();
  for (OptionSet::Iterator it = options_.begin(); it != options_.end(); ++it) {
    FormatOption(ostr, *it, option_width);

    if (indent_ < option_width) {
      ostr << '\n' << String(indent_, ' ').c_str();
      FormatText(ostr, it->GetDescription(), indent_, indent_);
    } else {
      FormatText(ostr, it->GetDescription(), indent_, option_width);
    }

    ostr << '\n';
  }
}

void HelpFormatter::FormatOption(std::ostream& ostr, const Option& option,
                                 int32 width) const {
  int32 short_len = option.GetShortName().Len();
  int32 full_len = option.GetFullName().Len();

  int32 n = 0;
  if (unix_style_ && short_len > 0) {
    ostr << GetShortPrefix().c_str() << option.GetShortName().c_str();
    n += GetShortPrefix().Len() + option.GetShortName().Len();
    if (option.TakesArgument()) {
      if (!option.IsArgumentRequired()) {
        ostr << '[';
        ++n;
      }
      ostr << option.GetArgumentName().c_str();
      n += option.GetArgumentName().Len();
      if (!option.IsArgumentRequired()) {
        ostr << ']';
        ++n;
      }
    }

    if (full_len > 0) {
      ostr << ", ";
      n += 2;
    }
  }

  if (full_len > 0) {
    ostr << GetLongPrefix().c_str() << option.GetFullName().c_str();
    n += GetLongPrefix().Len() + option.GetFullName().Len();
    if (option.TakesArgument()) {
      if (!option.IsArgumentRequired()) {
        ostr << '[';
        ++n;
      }
      ostr << '=';
      ++n;
      ostr << option.GetArgumentName().c_str();
      n += option.GetArgumentName().Len();
      if (!option.IsArgumentRequired()) {
        ostr << ']';
        ++n;
      }
    }
  }

  while (n < width) {
    ostr << ' ';
    ++n;
  }
}

void HelpFormatter::FormatText(std::ostream& ostr, const String& text,
                               int32 indent) const {
  FormatText(ostr, text, indent, indent);
}

void HelpFormatter::FormatText(std::ostream& ostr, const String& text,
                               int32 indent, int32 first_indent) const {
  int32 pos = first_indent;
  int32 max_word_len = width_ - indent;
  String word;
  for (String::const_iterator it = text.begin(); it != text.end(); ++it) {
    if (*it == '\n') {
      ClearWord(ostr, pos, word, indent);

      ostr << '\n';
      pos = 0;
      while (pos < indent) {
        ostr << ' ';
        ++pos;
      }
    } else if (*it == '\t') {
      ClearWord(ostr, pos, word, indent);

      if (pos < width_) {
        ++pos;
      }

      while (pos < width_ && pos % TAB_WIDTH != 0) {
        ostr << ' ';
        ++pos;
      }
    } else if (*it == ' ') {
      ClearWord(ostr, pos, word, indent);

      if (pos < width_) {
        ostr << ' ';
        ++pos;
      }
    } else {
      if (word.Len() == max_word_len) {
        ClearWord(ostr, pos, word, indent);
      } else {
        word += *it;
      }
    }
  }

  ClearWord(ostr, pos, word, indent);
}

void HelpFormatter::FormatWord(std::ostream& ostr, int32& pos,
                               const String& word, int32 indent) const {
  if (pos + word.Len() > width_) {
    ostr << '\n';
    pos = 0;
    while (pos < indent) {
      ostr << ' ';
      ++pos;
    }
  }
  ostr << word.c_str();
  pos += word.Len();
}

void HelpFormatter::ClearWord(std::ostream& ostr, int32& pos, String& word,
                              int32 indent) const {
  FormatWord(ostr, pos, word, indent);

  word.Clear();
}

String HelpFormatter::GetShortPrefix() const {
#if FUN_PLATFORM_UNIX_FAMILY
  return "-";
#else
  return unix_style_ ? "-" : "/";
#endif
}

String HelpFormatter::GetLongPrefix() const {
#if FUN_PLATFORM_UNIX_FAMILY
  return "--";
#else
  return unix_style_ ? "--" : "/";
#endif
}

}  // namespace framework
}  // namespace fun
