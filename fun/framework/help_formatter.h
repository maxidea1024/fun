#pragma once

#include <ostream>
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

class OptionSet;
class Option;

/**
 * This class formats a help message from an OptionSet.
 */
class FUN_FRAMEWORK_API HelpFormatter {
 public:
  /**
   * Creates the HelpFormatter, using the given
   * options.
   *
   * The HelpFormatter just stores a reference
   * to the given OptionSet, so the OptionSet must not
   * be destroyed during the lifetime of the HelpFormatter.
   */
  HelpFormatter(const OptionSet& options);

  /** Destroys the HelpFormatter. */
  ~HelpFormatter();

  /** Sets the command name. */
  void SetCommand(const String& command);

  /** Returns the command name. */
  const String& GetCommand() const;

  /** Sets the usage string. */
  void SetUsage(const String& usage);

  /** Returns the usage string. */
  const String& GetUsage() const;

  /** Sets the header string. */
  void SetHeader(const String& header);

  /** Returns the header string. */
  const String& GetHeader() const;

  /** Sets the footer string. */
  void SetFooter(const String& footer);

  /** Returns the footer string. */
  const String& GetFooter() const;

  /** Writes the formatted help text to the given stream. */
  void Format(std::ostream& ostr) const;

  /** Sets the line width for the formatted help text. */
  void SetWidth(int32 width);

  /**
   * Returns the line width for the formatted help text.
   *
   * The default width is 72.
   */
  int32 GetWidth() const;

  /** Sets the indentation for description continuation lines. */
  void SetIndent(int32 indent);

  /** Returns the indentation for description continuation lines. */
  int32 GetIndent() const;

  /**
   * Sets the indentation for description continuation lines so that
   * the description text is left-aligned.
   */
  void SetAutoIndent();

  /**
   * Enables Unix-style options. Both short and long option names
   * are printed if Unix-style is set. Otherwise, only long option
   * names are printed.
   *
   * After calling SetUnixStyle(), SetAutoIndent() should be called
   * as well to ensure proper help text formatting.
   */
  void SetUnixStyle(bool flag);

  /** Returns if Unix-style options are set. */
  bool IsUnixStyle() const;

  /**
   * Returns the platform-specific prefix for short options.
   * "-" on Unix, "/" on Windows and OpenVMS.
   */
  String GetShortPrefix() const;

  /**
   * Returns the platform-specific prefix for long options.
   * "--" on Unix, "/" on Windows and OpenVMS.
   */
  String GetLongPrefix() const;

 protected:
  /**
   * Calculates the indentation for the option descriptions
   * from the given options.
   */
  int32 CalcIndent() const;

  /** Formats all options. */
  void FormatOptions(std::ostream& ostr) const;

  /** Formats an option, using the platform-specific prefixes. */
  void FormatOption(std::ostream& ostr, const Option& option,
                    int32 width) const;

  /** Formats the given text. */
  void FormatText(std::ostream& ostr, const String& text, int32 indent) const;

  /** Formats the given text. */
  void FormatText(std::ostream& ostr, const String& text, int32 indent,
                  int32 first_indent) const;

  /** Formats the given word. */
  void FormatWord(std::ostream& ostr, int32& pos, const String& word,
                  int32 indent) const;

  /** Formats and then clears the given word. */
  void ClearWord(std::ostream& ostr, int32& pos, String& word,
                 int32 indent) const;

 public:
  HelpFormatter(const HelpFormatter&) = delete;
  HelpFormatter& operator=(const HelpFormatter&) = delete;

 private:
  const OptionSet& options_;
  int32 width_;
  int32 indent_;
  String command_;
  String usage_;
  String header_;
  String footer_;
  bool unix_style_;

  static const int32 TAB_WIDTH;
  static const int32 LINE_WIDTH;
};

//
// inlines
//

FUN_ALWAYS_INLINE int32 HelpFormatter::GetWidth() const { return width_; }

FUN_ALWAYS_INLINE int32 HelpFormatter::GetIndent() const { return indent_; }

FUN_ALWAYS_INLINE const String& HelpFormatter::GetCommand() const {
  return command_;
}

FUN_ALWAYS_INLINE const String& HelpFormatter::GetUsage() const {
  return usage_;
}

FUN_ALWAYS_INLINE const String& HelpFormatter::GetHeader() const {
  return header_;
}

FUN_ALWAYS_INLINE const String& HelpFormatter::GetFooter() const {
  return footer_;
}

FUN_ALWAYS_INLINE bool HelpFormatter::IsUnixStyle() const {
  return unix_style_;
}

}  // namespace framework
}  // namespace fun
