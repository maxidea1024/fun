#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/map_configuration.h"

#include <istream>
#include <list>
#include <ostream>

namespace fun {
namespace framework {

/**
 * This implementation of a Configuration reads properties
 * from a Java-style properties file.
 *
 * The file syntax is implemented as follows.
 *   - a line starting with a hash '#' or exclamation mark '!' is treated as a
 * comment and ignored
 *   - every other line denotes a property assignment in the form
 *     <key> = <value> or
 *     <key> : <value>
 *
 * Keys and values may contain special characters represented by the following
 * escape sequences:
 *   - \t: tab (0x09)
 *   - \n: line feed (0x0a)
 *   - \r: carriage return (0x0d)
 *   - \f: form feed (0x0c)
 *
 * For every other sequence that starts with a backslash, the backslash is
 * removed. Therefore, the sequence \a would just yield an 'a'.
 *
 * A value can spread across multiple lines if the last character in a line (the
 * character immediately before the carriage return or line feed character) is a
 * single backslash.
 *
 * Property names are case sensitive. Leading and trailing whitespace is
 * removed from both keys and values. A property name can neither contain
 * a colon ':' nor an equal sign '=' character.
 */
class FUN_FRAMEWORK_API PropertyFileConfiguration : public MapConfiguration {
 public:
  /**
   * Creates an empty PropertyFileConfiguration.
   */
  PropertyFileConfiguration();

  /**
   * Creates an PropertyFileConfiguration and loads the configuration data
   * from the given stream, which must be in properties file Format.
   * Set the preserve_comment to preserve the comments in the given stream.
   */
  PropertyFileConfiguration(std::istream& istr, bool preserve_comment = false);

  /**
   * Creates an PropertyFileConfiguration and loads the configuration data
   * from the given file, which must be in properties file Format.
   * Set the preserve_comment to preserve the comments in the given stream.
   */
  PropertyFileConfiguration(const String& path, bool preserve_comment = false);

  /**
   * Loads the configuration data from the given stream, which
   * must be in properties file format.
   * Set the preserve_comment to preserve the comments in the given stream.
   */
  void Load(std::istream& istr, bool preserve_comment = false);

  /**
   * Loads the configuration data from the given file, which
   * must be in properties file Format.
   * Set the preserve_comment to preserve the comments in the given stream.
   */
  void Load(const String& path, bool preserve_comment = false);

  /**
   * Writes the configuration data to the given stream.
   *
   * The data is written as a sequence of statements in the form
   * <key>: <value>
   * separated by a newline character.
   */
  void Save(std::ostream& ostr) const;

  /**
   * Writes the configuration data to the given file.
   */
  void Save(const String& path) const;

 protected:
  void SetRaw(const String& key, const String& value);
  void RemoveRaw(const String& key);
  ~PropertyFileConfiguration();

 private:
  typedef List<String> FileContent;
  typedef Map<String, FileContent::Iterator> KeyFileContentItMap;

  void ParseLine(std::istream& istr);
  void SkipSpace(std::istream& istr) const;
  bool IsComment(int32 c) const;
  void SaveComments(std::istream& istr);
  void SkipLine(std::istream& istr) const;
  void SaveKeyValue(std::istream& istr);
  bool IsNewLine(int32 c) const;
  bool IsKeyValueSeparator(int32 c) const;
  String ComposeOneLine(const String& key, const String& value) const;

  bool preserve_comment_;
  FileContent file_content_;
  KeyFileContentItMap key_file_content_it_map_;

  static int32 ReadChar(std::istream& istr);
};

}  // namespace framework
}  // namespace fun
