#pragma once

#include "fun/json/json.h"

namespace fun {
namespace json {

/**
 * TODO
 */
class FUN_JSON_API Features {
 public:
  /**
   * Allows all features.
   */
  static Features All();

  /**
   * Strictly following the json specification.
   */
  static Features Strict();

  /**
   * Default constructor. (Same as All())
   */
  Features();

  /**
   * Create a Features with json-settings
   */
  Features(const JValue& features);

  /**
   * Sets whether comments are supported.
   * The json default specification does not allow comments.
   * (Comments are considered anomalous.)
   */
  bool allow_comments;

  /**
   * The json name world root should be an object, but it can be a little
   * uncomfortable, so whether or not root is allowed, even if it is not an
   * object.
   */
  bool strict_root;

  /**
   * If it is a null entry, set whether to remove (option to reduce capacity)
   */
  bool allow_dropped_null_placeholders;

  /**
   * Object field Whether to allow digits with the key.
   */
  bool allow_numeric_keys;

  bool allow_single_quoted_strings;
  bool allow_special_floats;
  bool reject_dup_keys;
  int32 stack_limit;
};

}  // namespace json
}  // namespace fun
