#pragma once

#include "fun/base/base.h"
#include "fun/base/math/int_point.h"

namespace fun {

/**
 * Structure for integer rectangles in 2-d space.
 *
 * @todo Docs: The operators need better documentation, i.e. what does it mean to divide a rectangle?
 */
class IntRect {
 public:
  IntPoint min;
  IntPoint max;

  IntRect();
  IntRect(int32 x0, int32 y0, int32 x1, int32 y1);
  IntRect(const IntPoint& min, const IntPoint& max);

  const IntPoint& operator[](int32 index) const;
  IntPoint& operator[](int32 index);

  bool operator == (const IntRect& other) const;
  bool operator != (const IntRect& other) const;

  IntRect& operator *= (int32 scale);
  IntRect& operator += (const IntPoint& point);
  IntRect& operator -= (const IntPoint& point);
  IntRect operator * (int32 scale) const;
  IntRect operator / (int32 div) const;
  IntRect operator + (const IntPoint& point) const;
  IntRect operator / (const IntPoint& point) const;
  IntRect operator - (const IntPoint& point) const;
  IntRect operator + (const IntRect& other) const;
  IntRect operator - (const IntRect& other) const;

  int32 Area() const;
  IntRect Bottom(int32 height) const;
  void Clip(const IntRect& other);
  void Union(const IntRect& other);
  bool Contains(const IntPoint& point) const;
  void GetCenterAndExtents(IntPoint& out_center, IntPoint& out_extent) const;
  int32 Height() const;
  void InflateRect(int32 amount);
  void Include(const IntPoint& point);
  IntRect Inner(const IntPoint& shrink) const;
  IntRect Right(int32 width) const;
  IntRect Scale(float fraction) const;
  IntPoint Size() const;
  String ToString() const;
  int32 Width() const;
  bool IsEmpty() const;

  static IntRect DivideAndRoundUp(const IntRect& lhs, int32 div);
  static IntRect DivideAndRoundUp(const IntRect& lhs, const IntPoint& div);

  static int32 Count();

  friend Archive& operator & (Archive& ar, IntRect& rect) {
    return ar & rect.min.x & rect.min.y & rect.max.x & rect.max.y;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE IntRect IntRect::Scale(float fraction) const {
  Vector2 min2 = Vector2(min.x, min.y) * fraction;
  Vector2 max2 = Vector2(max.x, max.y) * fraction;
  return IntRect( Math::FloorToInt(min2.x), Math::FloorToInt(min2.y),
                  Math::CeilToInt(max2.x), Math::CeilToInt(max2.y));
}

FUN_ALWAYS_INLINE IntRect::IntRect()
  : min(ForceInit), max(ForceInit) {}

FUN_ALWAYS_INLINE IntRect::IntRect(int32 x0, int32 y0, int32 x1, int32 y1)
  : min(x0, y0), max(x1, y1) {}

FUN_ALWAYS_INLINE IntRect::IntRect(const IntPoint& min, const IntPoint& max)
  : min(min), max(max) {}

FUN_ALWAYS_INLINE const IntPoint& IntRect::operator[](int32 index) const {
  return (&min)[index];
}

FUN_ALWAYS_INLINE IntPoint& IntRect::operator[](int32 index) {
  return (&min)[index];
}

FUN_ALWAYS_INLINE bool IntRect::operator == (const IntRect& other) const {
  return min == other.min && max == other.max;
}

FUN_ALWAYS_INLINE bool IntRect::operator != (const IntRect& other) const {
  return min != other.min || max != other.max;
}

FUN_ALWAYS_INLINE IntRect& IntRect::operator *= (int32 scale) {
  min *= scale;
  max *= scale;
  return *this;
}

FUN_ALWAYS_INLINE IntRect& IntRect::operator += (const IntPoint& point) {
  min += point;
  max += point;
  return *this;
}

FUN_ALWAYS_INLINE IntRect& IntRect::operator -= (const IntPoint& point) {
  min -= point;
  max -= point;
  return *this;
}

FUN_ALWAYS_INLINE IntRect IntRect::operator * (int32 scale) const {
  return IntRect(min * scale, max * scale);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator / (int32 scale) const {
  return IntRect(min / scale, max / scale);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator + (const IntPoint& point) const {
  return IntRect(min + point, max + point);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator / (const IntPoint& point) const {
  return IntRect(min / point, max / point);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator - (const IntPoint& point) const {
  return IntRect(min - point, max - point);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator + (const IntRect& other) const {
  return IntRect(min + other.min, max + other.max);
}

FUN_ALWAYS_INLINE IntRect IntRect::operator - (const IntRect& other) const {
  return IntRect(min - other.min, max - other.max);
}

FUN_ALWAYS_INLINE int32 IntRect::Area() const {
  return (max.x - min.x) * (max.y - min.y);
}

FUN_ALWAYS_INLINE IntRect IntRect::Bottom(int32 height) const {
  return IntRect(min.x, Math::Max(min.y, max.y - height), max.x, max.y);
}

FUN_ALWAYS_INLINE void IntRect::Clip(const IntRect& r) {
  min.x = Math::Max<int32>(min.x, r.min.x);
  min.y = Math::Max<int32>(min.y, r.min.y);
  max.x = Math::Min<int32>(max.x, r.max.x);
  max.y = Math::Min<int32>(max.y, r.max.y);

  // return zero area if not overlapping
  max.x = Math::Max<int32>(min.x, max.x);
  max.y = Math::Max<int32>(min.y, max.y);
}

FUN_ALWAYS_INLINE void IntRect::Union(const IntRect& r) {
  min.x = Math::Min<int32>(min.x, r.min.x);
  min.y = Math::Min<int32>(min.y, r.min.y);
  max.x = Math::Max<int32>(max.x, r.max.x);
  max.y = Math::Max<int32>(max.y, r.max.y);
}

FUN_ALWAYS_INLINE bool IntRect::Contains(const IntPoint& point) const {
  return point.x >= min.x && point.x < max.x && point.y >= min.y && point.y < max.y;
}

FUN_ALWAYS_INLINE IntRect IntRect::DivideAndRoundUp(const IntRect& lhs, int32 div) {
  return DivideAndRoundUp(lhs, IntPoint(div, div));
}

FUN_ALWAYS_INLINE IntRect IntRect::DivideAndRoundUp(const IntRect& lhs, const IntPoint& div) {
  return IntRect(lhs.min / div, IntPoint::DivideAndRoundUp(lhs.max, div));
}

FUN_ALWAYS_INLINE void IntRect::GetCenterAndExtents(IntPoint& out_center, IntPoint& out_extent) const {
  out_extent.x = (max.x - min.x) / 2;
  out_extent.y = (max.y - min.y) / 2;

  out_center.x = min.x + out_extent.x;
  out_center.y = min.y + out_extent.y;
}

FUN_ALWAYS_INLINE int32 IntRect::Height() const {
  return max.y - min.y;
}

FUN_ALWAYS_INLINE void IntRect::InflateRect(int32 amount) {
  min.x -= amount;
  min.y -= amount;
  max.x += amount;
  max.y += amount;
}

FUN_ALWAYS_INLINE void IntRect::Include(const IntPoint& point) {
  min.x = Math::Min(min.x, point.x);
  min.y = Math::Min(min.y, point.y);
  max.x = Math::Max(max.x, point.x);
  max.y = Math::Max(max.y, point.y);
}

FUN_ALWAYS_INLINE IntRect IntRect::Inner(const IntPoint& shrink) const {
  return IntRect(min + shrink, max - shrink);
}

FUN_ALWAYS_INLINE int32 IntRect::Count() {
  return 2;
}

FUN_ALWAYS_INLINE IntRect IntRect::Right(int32 width) const {
  return IntRect(Math::Max(min.x, max.x - width), min.y, max.x, max.y);
}

FUN_ALWAYS_INLINE IntPoint IntRect::Size() const {
  return IntPoint(max.x - min.x, max.y - min.y);
}

FUN_ALWAYS_INLINE String IntRect::ToString() const {
  return String::Format("min=(%s) max=(%s)", min.ToString(), max.ToString());
}

FUN_ALWAYS_INLINE int32 IntRect::Width() const {
  return max.x - min.x;
}

FUN_ALWAYS_INLINE bool IntRect::IsEmpty() const {
  return Width() == 0 && Height() == 0;
}

} // namespace fun
