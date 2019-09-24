// TODO 클래스명을 좀더 의미 있게 변경해주는게 좋을듯...

#pragma once

#include "fun/base/container/array.h"
#include "fun/framework/framework.h"
#include "fun/framework/option.h"

namespace fun {
namespace framework {

/**
 * A collection of Option objects.
 */
class FUN_FRAMEWORK_API OptionSet {
 public:
  typedef Array<Option> OptionList;
  typedef OptionList::ConstIterator Iterator;

  /** Creates the OptionSet. */
  OptionSet();

  /** Creates an option set from another one. */
  OptionSet(const OptionSet& options);

  /** Destroys the OptionSet. */
  ~OptionSet();

  /** Assignment operator. */
  OptionSet& operator=(const OptionSet& options);

  /** Adds an option to the collection. */
  void AddOption(const Option& option);

  /**
   * Returns a true if an option with the given name exists.
   *
   * The given name can either be a fully specified short name,
   * or a partially specified full name. If a partial name
   * matches more than one full name, false is returned.
   * The name must either match the short or full name of an
   * option. Comparison case sensitive for the short name and
   * not case sensitive for the full name.
   */
  bool HasOption(const String& name, bool match_short = false) const;

  /**
   * Returns a reference to the option with the given name.
   *
   * The given name can either be a fully specified short name,
   * or a partially specified full name.
   * The name must either match the short or full name of an
   * option. Comparison case sensitive for the short name and
   * not case sensitive for the full name.
   * Throws a NotFoundException if no matching option has been found.
   * Throws an UnknownOptionException if a partial full name matches
   * more than one option.
   */
  const Option& GetOption(const String& name, bool match_short = false) const;

  /** Supports iterating over all options. */
  Iterator begin() const;

  /** Supports iterating over all options. */
  Iterator end() const;

 private:
  OptionList options_;
};

}  // namespace framework
}  // namespace fun
