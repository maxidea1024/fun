#pragma once

#include "fun/base/base.h"
#include "fun/base/container/allocation_policies.h"
#include "fun/base/container/map.h"

namespace fun {

/**
 * Template for fluent map builders.
 *
 * \param KeyType - The type of keys stored in the map.
 * \param ValueType - The type of values stored in the map.
 * \param SetAllocator - The allocator to use for key-value pairs.
 */
template <
    typename _KeyType,
    typename _ValueType,
    typename SetAllocator = DefaultSetAllocator
  >
class MapBuilder
{
 public:
  using KeyType = _KeyType;
  using ValueType = _ValueType;
  using MapType = Map<KeyType, ValueType, SetAllocator>;

  /**
   * Default constructor.
   */
  MapBuilder() {}

  /**
   * Creates and initializes a new map builder from another map.
   *
   * \param map - The map to copy.
   */
  template <typename OtherAllocator>
  MapBuilder(const Map<KeyType, ValueType, OtherAllocator>& map)
    : map_(map)
  {}

  /**
   * Adds a key-value pair to the map.
   *
   * \param key - The key of the pair to add.
   * \param value - The value of the pair to add.
   *
   * \return This instance (for method chaining).
   */
  MapBuilder& Add(KeyType key, ValueType value)
  {
    map_.Add(key, value);
    return *this;
  }

  /**
   * Appends another map.
   *
   * \param other_map - The map to append.
   *
   * \return This instance (for method chaining).
   */
  MapBuilder& Append(const MapType& other_map)
  {
    map_.Append(other_map);
    return *this;
  }

  /**
   * Builds the map as configured.
   *
   * \return A new map.
   */
  MapType Build() const
  {
    return map_;
  }

  /**
   * Implicit conversion operator to build the map as configured.
   *
   * \return A new map.
   */
  operator MapType() const
  {
    return Build();
  }

 private:
  /**
   * Holds the map being built.
   */
  MapType map_;
};

} // namespace fun
