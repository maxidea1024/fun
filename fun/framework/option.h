#pragma once

#include "fun/framework/configuration_base.h"
#include "fun/framework/framework.h"
#include "fun/framework/option_callback.h"

namespace fun {
namespace framework {

class Application;
class OptionValidator;

/**
 * This class represents and stores the properties
 * of a command line option.
 *
 * An option has a full name, an optional short name,
 * a description (used for printing a usage statement),
 * and an optional argument name.
 * An option can be optional or required.
 * An option can be repeatable, which means that it can
 * be given more than once on the command line.
 *
 * An option can be part of an option group. At most one
 * option of each group may be specified on the command
 * line.
 *
 * An option can be bound to a configuration property.
 * In this case, a configuration property will automatically
 * receive the option's argument value.
 *
 * A callback method can be specified for options. This method
 * is called whenever an option is specified on the command line.
 *
 * Option argument values can be automatically validated using a
 * Validator.
 *
 * Option instances are value objects.
 *
 * Typically, after construction, an Option object is immediately
 * passed to an Options object.
 *
 * An Option object can be created by chaining the constructor
 * with any of the setter methods, as in the following example:
 *
 *     Option version_opt("include", "I", "specify an include directory")
 *        .Required(false)
 *        .Repeatable(true)
 *        .Argument("directory");
 */
class FUN_FRAMEWORK_API Option {
 public:
  /** Creates an empty Option. */
  Option();

  /** Creates an option from another one. */
  Option(const Option& option);

  /** Creates an option with the given properties. */
  Option(const String& full_name, const String& short_name);

  /** Creates an option with the given properties. */
  Option(const String& full_name, const String& short_name,
         const String& description, bool required = false);

  /** Creates an option with the given properties. */
  Option(const String& full_name, const String& short_name,
         const String& description, bool required, const String& arg_name,
         bool arg_required = false);

  /** Destroys the Option. */
  ~Option();

  /** Assignment operator. */
  Option& operator=(const Option& option);

  /** Swaps the option with another one. */
  void Swap(Option& option);

  /** Sets the short name of the option. */
  Option& ShortName(const String& name);

  /** Sets the full name of the option. */
  Option& FullName(const String& name);

  /** Sets the description of the option. */
  Option& Description(const String& text);

  /**
   * Sets whether the option is required (flag == true)
   * or optional (flag == false).
   */
  Option& Required(bool flag);

  /**
   * Sets whether the option can be specified more than once
   * (flag == true) or at most once (flag == false).
   */
  Option& Repeatable(bool flag);

  /**
   * Specifies that the option takes an (optional or required)
   * argument.
   */
  Option& Argument(const String& name, bool required = true);

  /** Specifies that the option does not take an argument (default). */
  Option& NoArgument();

  /** Specifies the option group the option is part of. */
  Option& Group(const String& group);

  /**
   * Binds the option to the configuration property with the given name.
   *
   * The configuration will automatically receive the option's argument.
   */
  Option& Binding(const String& property_name);

  /**
   * Binds the option to the configuration property with the given name,
   * using the given ConfigurationBase.
   *
   * The configuration will automatically receive the option's argument.
   */
  Option& Binding(const String& property_name, ConfigurationBase* config);

  /**
   * Binds the option to the given method.
   *
   * The callback method will be called when the option
   * has been specified on the command line.
   *
   * Usage:
   *     callback(OptionCallback<MyApplication>(this,
   * &MyApplication::myCallback));
   */
  Option& Callback(const OptionCallbackBase& cb);

  /**
   * Sets the validator for the given option.
   *
   * The Option takes ownership of the Validator and
   * deletes it when it's no longer needed.
   */
  Option& Validator(OptionValidator* validator);

  /**
   * Returns the short name of the option.
   */
  const String& GetShortName() const;

  /**
   * Returns the full name of the option.
   */
  const String& GetFullName() const;

  /**
   * Returns the description of the option.
   */
  const String& GetDescription() const;

  /**
   * Returns true if the option is required, false if not.
   */
  bool IsRequired() const;

  /**
   * Returns true if the option can be specified more than
   * once, or false if at most once.
   */
  bool IsRepeatable() const;

  /**
   * Returns true if the options takes an (optional) argument.
   */
  bool TakesArgument() const;

  /**
   * Returns true if the argument is required.
   */
  bool IsArgumentRequired() const;

  /**
   * Returns the argument name, if specified.
   */
  const String& GetArgumentName() const;

  /**
   * Returns the option group the option is part of,
   * or an empty string, if the option is not part of
   * a group.
   */
  const String& GetGroup() const;

  /**
   * Returns the property name the option is bound to,
   * or an empty string in case it is not bound.
   */
  const String& GetBinding() const;

  /**
   * Returns a pointer to the callback method for the option,
   * or NULL if no callback has been specified.
   */
  OptionCallbackBase* GetCallback() const;

  /**
   * Returns the option's Validator, if one has been specified,
   * or NULL otherwise.
   */
  OptionValidator* GetValidator() const;

  /**
   * Returns the configuration, if specified, or NULL otherwise.
   */
  ConfigurationBase::Ptr GetConfig() const;

  /**
   * Returns true if the given option string matches the
   * short name.
   *
   * The first characters of the option string must match
   * the short name of the option (case sensitive),
   * or the option string must partially match the full
   * name (case insensitive).
   */
  bool MatchesShort(const String& option) const;

  /**
   * Returns true if the given option string matches the
   * full name.
   *
   * The option string must match the full
   * name (case insensitive).
   */
  bool MatchesFull(const String& option) const;

  /**
   * Returns true if the given option string partially matches the
   * full name.
   *
   * The option string must partially match the full
   * name (case insensitive).
   */
  bool MatchesPartial(const String& option) const;

  /**
   * Verifies that the given option string matches the
   * requirements of the option, and extracts the option argument,
   * if present.
   *
   * If the option string is okay and carries an argument,
   * the argument is returned in arg.
   *
   * Throws a MissingArgumentException if a required argument
   * is missing. Throws an UnexpectedArgumentException if an
   * argument has been found, but none is expected.
   */
  void Process(const String& option, String& arg) const;

 private:
  String short_name_;
  String full_name_;
  String description_;
  bool required_;
  bool repeatable_;
  String arg_name_;
  bool arg_required_;
  String group_;
  String binding_;
  OptionValidator* validator_;
  OptionCallbackBase* callback_;
  ConfigurationBase::Ptr config_;
};

//
// inlines
//

FUN_ALWAYS_INLINE const String& Option::GetShortName() const {
  return short_name_;
}

FUN_ALWAYS_INLINE const String& Option::GetFullName() const {
  return full_name_;
}

FUN_ALWAYS_INLINE const String& Option::GetDescription() const {
  return description_;
}

FUN_ALWAYS_INLINE bool Option::IsRequired() const { return required_; }

FUN_ALWAYS_INLINE bool Option::IsRepeatable() const { return repeatable_; }

FUN_ALWAYS_INLINE bool Option::TakesArgument() const {
  return !arg_name_.IsEmpty();
}

FUN_ALWAYS_INLINE bool Option::IsArgumentRequired() const {
  return arg_required_;
}

FUN_ALWAYS_INLINE const String& Option::GetArgumentName() const {
  return arg_name_;
}

FUN_ALWAYS_INLINE const String& Option::GetGroup() const { return group_; }

FUN_ALWAYS_INLINE const String& Option::GetBinding() const { return binding_; }

FUN_ALWAYS_INLINE OptionCallbackBase* Option::GetCallback() const {
  return callback_;
}

FUN_ALWAYS_INLINE OptionValidator* Option::GetValidator() const {
  return validator_;
}

FUN_ALWAYS_INLINE ConfigurationBase::Ptr Option::GetConfig() const {
  return config_;
}

}  // namespace framework
}  // namespace fun
