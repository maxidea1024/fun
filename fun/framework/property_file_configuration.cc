#include "fun/framework/property_file_configuration.h"
#include "fun/base/exception.h"
#include "fun/base/path.h"
#include "fun/base/str.h"
//#include "fun/base/file_stream.h"
//#include "fun/base/line_ending_converter.h"
//#include "fun/base/ascii.h"

// using fun::Trim;
using fun::Path;

namespace fun {
namespace framework {

PropertyFileConfiguration::PropertyFileConfiguration()
    : preserve_comment_(false) {}

PropertyFileConfiguration::PropertyFileConfiguration(std::istream& istr,
                                                     bool preserve_comment)
    : preserve_comment_(preserve_comment) {
  Load(istr, preserve_comment);
}

PropertyFileConfiguration::PropertyFileConfiguration(const String& path,
                                                     bool preserve_comment)
    : preserve_comment_(preserve_comment) {
  Load(path, preserve_comment);
}

PropertyFileConfiguration::~PropertyFileConfiguration() {}

void PropertyFileConfiguration::Load(std::istream& istr,
                                     bool preserve_comment) {
  preserve_comment_ = preserve_comment;
  Clear();
  file_content_.Clear();
  key_file_content_it_map_.Clear();

  while (!istr.eof()) {
    ParseLine(istr);
  }
}

void PropertyFileConfiguration::Load(const String& path,
                                     bool preserve_comment) {
  fun::FileInputStream istr(path);
  if (istr.good()) {
    Load(istr, preserve_comment);
  } else {
    throw fun::OpenFileException(path);
  }
}

void PropertyFileConfiguration::Save(std::ostream& ostr) const {
  if (preserve_comment_) {
    for (FileContent::const_iterator it = file_content_.begin();
         it != file_content_.end(); ++it) {
      ostr << *it;
    }
  } else {
    for (MapConfiguration::iterator it = begin(); it != end(); ++it) {
      ostr << ComposeOneLine(it->first, it->second);
    }
  }
}

void PropertyFileConfiguration::Save(const String& path) const {
  fun::FileOutputStream ostr(path);
  if (ostr.good()) {
    fun::OutputLineEndingConverter lec(ostr);
    Save(lec);
    lec.flush();
    ostr.flush();
    if (!ostr.good()) {
      throw fun::WriteFileException(path);
    }
  } else {
    throw fun::CreateFileException(path);
  }
}

// If preserve_comment_ is true, not only Save key-value into map
// but also Save the entire file into file_content_.
// Otherwise, only Save key-value into map.
void PropertyFileConfiguration::ParseLine(std::istream& istr) {
  SkipSpace(istr);

  if (!istr.eof()) {
    if (IsComment(istr.peek())) {
      if (preserve_comment_) {
        SaveComment(istr);
      } else {
        SkipLine(istr);
      }
    } else {
      SaveKeyValue(istr);
    }
  }
}

void PropertyFileConfiguration::SkipSpace(std::istream& istr) const {
  while (!istr.eof() && fun::Ascii::IsSpace(istr.peek())) {
    istr.get();
  }
}

int32 PropertyFileConfiguration::ReadChar(std::istream& istr) {
  for (;;) {
    int32 c = istr.get();
    if (c == '\\') {
      c = istr.get();
      switch (c) {
        case 't':
          return '\t';
        case 'r':
          return '\r';
        case 'n':
          return '\n';
        case 'f':
          return '\f';
        case '\r':
          if (istr.peek() == '\n') {
            istr.get();
          }
          continue;
        case '\n':
          continue;
        default:
          return c;
      }
    } else if (c == '\n' || c == '\r') {
      return 0;
    } else {
      return c;
    }
  }
}

void PropertyFileConfiguration::SetRaw(const String& key, const String& value) {
  MapConfiguration::SetRaw(key, value);
  if (preserve_comment_) {
    // Insert the key-value to the end of file_content_ and update
    // key_file_content_it_map_.
    if (key_file_content_it_map_.count(key) == 0) {
      FileContent::iterator fit =
          file_content_.insert(file_content_.end(), ComposeOneLine(key, value));
      key_file_content_it_map_[key] = fit;
    }
    // Update the key-value in file_content_.
    else {
      FileContent::iterator fit = key_file_content_it_map_[key];
      *fit = ComposeOneLine(key, value);
    }
  }
}

void PropertyFileConfiguration::RemoveRaw(const String& key) {
  MapConfiguration::RemoveRaw(key);

  // remove the key from file_content_ and key_file_content_it_map_.
  if (preserve_comment_) {
    file_content_.erase(key_file_content_it_map_[key]);
    key_file_content_it_map_.erase(key);
  }
}

bool PropertyFileConfiguration::IsComment(int32 c) const {
  return c == '#' || c == '!';
}

void PropertyFileConfiguration::SaveComment(std::istream& istr) {
  String comment;

  int32 c = istr.get();
  while (!IsNewLine(c)) {
    comment += (char)c;
    c = istr.get();
  }
  comment += (char)c;

  file_content_.push_back(comment);
}

void PropertyFileConfiguration::SkipLine(std::istream& istr) const {
  int32 c = istr.get();
  while (!IsNewLine(c)) c = istr.get();
}

void PropertyFileConfiguration::SaveKeyValue(std::istream& istr) {
  int32 c = istr.get();

  String key;
  while (!IsNewLine(c) && !IsKeyValueSeparator(c)) {
    key += (char)c;
    c = istr.get();
  }

  String value;
  if (IsKeyValueSeparator(c)) {
    c = ReadChar(istr);
    while (!istr.eof() && c) {
      value += (char)c;
      c = ReadChar(istr);
    }
  }

  SetRaw(Trim(key), Trim(value));
}

bool PropertyFileConfiguration::IsNewLine(int32 c) const {
  return c == std::char_traits<char>::eof() || c == '\n' || c == '\r';
}

String PropertyFileConfiguration::ComposeOneLine(const String& key,
                                                 const String& value) const {
  String result = key + ": ";

  for (String::const_iterator its = value.begin(); its != value.end(); ++its) {
    switch (*its) {
      case '\t':
        result += "\\t";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\\':
        result += "\\\\";
        break;
      default:
        result += *its;
        break;
    }
  }

  return result += "\n";
}

bool PropertyFileConfiguration::IsKeyValueSeparator(int32 c) const {
  return c == '=' || c == ':';
}

}  // namespace framework
}  // namespace fun
