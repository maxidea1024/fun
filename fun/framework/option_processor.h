#pragma once

#include "fun/framework/framework.h"
#include "fun/base/container/set.h"

namespace fun {
namespace framework {

class OptionSet;

/**
 * An OptionProcessor is used to process the command line
 * arguments of an application.
 *
 * The Process() method takes an argument from the command line.
 * If that argument starts with an option prefix, the argument
 * is further processed. Otherwise, the argument is ignored and
 * false is returned. The argument must match one of the options
 * given in the OptionSet that is passed to the OptionProcessor
 * with the constructor. If an option is part of a group, at most
 * one option of the group can be passed to the OptionProcessor.
 * Otherwise an IncompatibleOptionsException is thrown.
 * If the same option is given multiple times, but the option
 * is not repeatable, a DuplicateOptionException is thrown.
 * If the option is not recognized, a UnexpectedArgumentException
 * is thrown.
 * If the option requires an argument, but none is given, an
 * MissingArgumentException is thrown.
 * If no argument is expected, but one is present, a
 * UnexpectedArgumentException is thrown.
 * If a partial option name is ambiguous, an AmbiguousOptionException
 * is thrown.
 *
 * The OptionProcessor supports two modes: Unix mode and default mode.
 * In Unix mode, the option prefix is a dash '-'. A dash must be followed
 * by a short option name, or another dash, followed by a (partial)
 * long option name.
 * In default mode, the option prefix is a slash '/', followed by
 * a (partial) long option name.
 * If the special option '--' is encountered in Unix mode, all following
 * options are ignored.
 *
 * Option arguments can be specified in three ways. If a Unix short option
 * ("-o") is given, the argument directly follows the option name, without
 * any delimiting character or space ("-ovalue"). In default option mode, or if a
 * Unix long option ("--option") is given, the option argument is
 * delimited from the option name with either an equal sign ('=') or
 * a colon (':'), as in "--option=value" or "/option:value". Finally,
 * a required option argument can be specified on the command line after the
 * option, delimited with a space, as in "--option value" or "-o value".
 * The latter only works for required option arguments, not optional ones.
 */
class FUN_FRAMEWORK_API OptionProcessor {
 public:
  /**
   * Creates the OptionProcessor, using the given OptionSet.
   */
  OptionProcessor(const OptionSet& options);

  /**
   * Destroys the OptionProcessor.
   */
  ~OptionProcessor();

  /**
   * Enables (flag == true) or disables (flag == false) Unix-style
   * option processing.
   *
   * If Unix-style processing is enabled, options are expected to
   * begin with a single or a double dash ('-' or '--', respectively).
   * A single dash must be followed by a short option name. A double
   * dash must be followed by a (partial) full option name.
   *
   * If Unix-style processing is disabled, options are expected to
   * begin with a slash ('/'), followed by a (partial) full option name.
   */
  void SetUnixStyle(bool flag);

  /**
   Returns true if Unix-style option processing is enabled.
   */
  bool IsUnixStyle() const;

  /**
   * Examines and processes the given command line argument.
   *
   * If the argument begins with an option prefix, the option is processed
   * and true is returned. The full option name is stored in option_name and the
   * option argument, if present, is stored in option_arg.
   *
   * If the option does not begin with an option prefix, false is returned.
   */
  bool Process(const String& argument, String& option_name, String& option_arg);

  /**
   * Checks if all required options have been processed.
   *
   * Does nothing if all required options have been processed.
   * Throws a MissingOptionException otherwise.
   */
  void CheckRequired() const;

 private:
  bool ProcessUnix( const String& argument,
                    String& option_name,
                    String& option_arg);

  bool ProcessDefault(const String& argument,
                      String& option_name,
                      String& option_arg);
  bool ProcessCommon( const String& option,
                      bool is_short,
                      String& option_name,
                      String& option_arg);

  const OptionSet& options_;
  bool unix_style_;
  bool ignore_;
  Set<String> groups_;
  Set<String> specified_options_;
  String deferred_options_;
};


//
// inlines
//

FUN_ALWAYS_INLINE bool OptionProcessor::IsUnixStyle() const {
  return unix_style_;
}

} // namespace framework
} // namespace fun
