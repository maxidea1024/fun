#pragma once

#include "Math/MathUtility.h"
#include "Misc/Parse.h"
#include "Serialization/ArchiveBase.h"

namespace fun {

class Color;
class Vector;
class Float16Color;

/**
 * Enum for the different kinds of gamma spaces we expect to need to convert from/to.
 */
enum class GammaSpace {
  /** No gamma correction is applied to this space, the incoming colors are assumed to already be in linear space. */
  Linear,
  /** A simplified sRGB gamma correction is applied, pow(1/2.2). */
  Pow22,
  /** Use the standard sRGB conversion. */
  sRGB,
};


/**
 * A linear, 32-bit/component floating point RGBA color.
 */
class LinearColor {
 public:
  float r;
  float g;
  float b;
  float a;

  /** Static lookup table used for Color -> LinearColor conversion. Pow(2.2) */
  static float Pow22OneOver255Table[256];

  /** Static lookup table used for Color -> LinearColor conversion. sRGB */
  static float sRGBToLinearTable[256];

  FUN_ALWAYS_INLINE LinearColor() {}

  FUN_ALWAYS_INLINE explicit LinearColor(ForceInit_TAG)
    : r(0), g(0), b(0), a(0) {}

  FUN_ALWAYS_INLINE LinearColor(float r, float g, float b, float a = 1.f)
    : r(r), g(g), b(b), a(a) {}

  /**
   * Converts an Color which is assumed to be in sRGB space, into linear color space.
   *
   * \param color - The sRGB color that needs to be converted into linear space.
   */
  FUN_BASE_API LinearColor(const Color& color);

  FUN_BASE_API LinearColor(const Vector& vector);

  FUN_BASE_API explicit LinearColor(const Float16Color& c);

  // Serializer.

  friend Archive& operator & (Archive& ar, LinearColor& color) {
    return ar & color.r & color.g & color.b & color.a;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

  // Conversions.
  FUN_BASE_API Color ToRGBE() const;

  /**
   * Converts an Color coming from an observed sRGB output, into a linear color.
   *
   * \param color - The sRGB color that needs to be converted into linear space.
   */
  FUN_BASE_API static LinearColor FromSRGBColor(const Color& color);

  /**
   * Converts an Color coming from an observed Pow(1/2.2) output, into a linear color.
   *
   * \param color - The Pow(1/2.2) color that needs to be converted into linear space.
   */
  FUN_BASE_API static LinearColor FromPow22Color(const Color& color);

  // Operators.

  FUN_ALWAYS_INLINE float& Component(int32 index) {
    return (&r)[index];
  }

  FUN_ALWAYS_INLINE const float& Component(int32 index) const {
    return (&r)[index];
  }

  FUN_ALWAYS_INLINE LinearColor operator+(const LinearColor& color_b) const {
    return LinearColor(
      this->r + color_b.r,
      this->g + color_b.g,
      this->b + color_b.b,
      this->a + color_b.a
      );
  }

  FUN_ALWAYS_INLINE LinearColor& operator+=(const LinearColor& color_b) {
    r += color_b.r;
    g += color_b.g;
    b += color_b.b;
    a += color_b.a;
    return *this;
  }

  FUN_ALWAYS_INLINE LinearColor operator-(const LinearColor& color_b) const {
    return LinearColor(
      this->r - color_b.r,
      this->g - color_b.g,
      this->b - color_b.b,
      this->a - color_b.a
      );
  }

  FUN_ALWAYS_INLINE LinearColor& operator-=(const LinearColor& color_b) {
    r -= color_b.r;
    g -= color_b.g;
    b -= color_b.b;
    a -= color_b.a;
    return *this;
  }

  FUN_ALWAYS_INLINE LinearColor operator*(const LinearColor& color_b) const {
    return linearcolor(
      this->r * color_b.r,
      this->g * color_b.g,
      this->b * color_b.b,
      this->a * color_b.a
      );
  }
  FUN_ALWAYS_INLINE LinearColor& operator*=(const LinearColor& color_b) {
    r *= color_b.r;
    g *= color_b.g;
    b *= color_b.b;
    a *= color_b.a;
    return *this;
  }

  FUN_ALWAYS_INLINE LinearColor operator*(float scalar) const {
    return linearcolor(
      this->r * scalar,
      this->g * scalar,
      this->b * scalar,
      this->a * scalar
      );
  }

  FUN_ALWAYS_INLINE LinearColor& operator *= (float scalar) {
    r *= scalar;
    g *= scalar;
    b *= scalar;
    a *= scalar;
    return *this;
  }

  FUN_ALWAYS_INLINE LinearColor operator / (const LinearColor& color_b) const {
    return LinearColor(
      this->r / color_b.r,
      this->g / color_b.g,
      this->b / color_b.b,
      this->a / color_b.a
      );
  }
  FUN_ALWAYS_INLINE LinearColor& operator /= (const LinearColor& color_b) {
    r /= color_b.r;
    g /= color_b.g;
    b /= color_b.b;
    a /= color_b.a;
    return *this;
  }

  FUN_ALWAYS_INLINE LinearColor operator / (float scalar) const {
    const float one_over_scalar = 1.f / scalar;
    return LinearColor(
      this->r * one_over_scalar,
      this->g * one_over_scalar,
      this->b * one_over_scalar,
      this->a * one_over_scalar
      );
  }
  FUN_ALWAYS_INLINE LinearColor& operator /= (float scalar) {
    const float one_over_scalar = 1.f / scalar;
    r *= one_over_scalar;
    g *= one_over_scalar;
    b *= one_over_scalar;
    a *= one_over_scalar;
    return *this;
  }

  // clamped in 0..1 range
  FUN_ALWAYS_INLINE LinearColor GetClamped(float min = 0.f, float max = 1.f) const {
    LinearColor ret;
    ret.r = Math::Clamp(r, min, max);
    ret.g = Math::Clamp(g, min, max);
    ret.b = Math::Clamp(b, min, max);
    ret.a = Math::Clamp(a, min, max);
    return ret;
  }

  /** Comparison operators */
  FUN_ALWAYS_INLINE bool operator == (const LinearColor& other) const {
    return this->r == other.r && this->g == other.g && this->b == other.b && this->a == other.a;
  }
  FUN_ALWAYS_INLINE bool operator != (const LinearColor& other) const {
    return this->r != other.r || this->g != other.g || this->b != other.b || this->a != other.a;
  }

  // Error-tolerant comparison.
  FUN_ALWAYS_INLINE bool Equals(const LinearColor& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return  Math::Abs(this->r - other.r) < tolerance &&
            Math::Abs(this->g - other.g) < tolerance &&
            Math::Abs(this->b - other.b) < tolerance &&
            Math::Abs(this->a - other.a) < tolerance;
  }

  FUN_ALWAYS_INLINE LinearColor CopyWithNewOpacity(float opacity) const {
    LinearColor copy = *this;
    copy.a = opacity;
    return copy;
  }

  /**
   * Converts byte hue-saturation-brightness to floating point red-green-blue.
   */
  static FUN_BASE_API LinearColor FGetHSV(uint8 h, uint8 s, uint8 v);

  /**
   * Makes a random but quite nice color.
   */
  static FUN_BASE_API LinearColor MakeRandomColor();

  /**
   * Converts temperature in Kelvins of a black body radiator to RGB chromaticity.
   */
  static FUN_BASE_API LinearColor MakeFromColorTemperature(float tmp);

  /**
   * Euclidean distance between two points.
   */
  static FUN_ALWAYS_INLINE float Distance(const LinearColor& v1, const LinearColor& v2) {
    return Math::Sqrt(Math::Square(v2.r-v1.r) + Math::Square(v2.g-v1.g) + Math::Square(v2.b-v1.b) + Math::Square(v2.a-v1.a));
  }

  /**
   * Generates a list of sample points on a Bezier curve defined by 2 points.
   *
   * \param control_point_list - Array of 4 Linear Colors (vert1, controlpoint1, controlpoint2, vert2).
   * \param control_point_count - Number of samples.
   * \param out_points - Receives the output samples.
   *
   * \return Path length.
   */
  static FUN_BASE_API float EvaluateBezier(const LinearColor* control_point_list, int32 control_point_count, Array<LinearColor>& out_points);

  /** Converts a linear space RGB color to an HSV color */
  FUN_BASE_API LinearColor LinearRGBToHSV() const;

  /** Converts an HSV color to a linear space RGB color */
  FUN_BASE_API LinearColor HSVToLinearRGB() const;

  /**
   * Linearly interpolates between two colors by the specified progress amount.  The interpolation is performed in HSV color space
   * taking the shortest path to the new color's hue.  This can give better results than Math::Lerp(), but is much more expensive.
   * The incoming colors are in RGB space, and the output color will be RGB.  The alpha value will also be interpolated.
   *
   * \param from - The color and alpha to interpolate from as linear RGBA
   * \param to - The color and alpha to interpolate to as linear RGBA
   * \param progress - scalar interpolation amount (usually between 0.0 and 1.0 inclusive)
   *
   * \return The interpolated color in linear RGB space along with the interpolated alpha value
   */
  static FUN_BASE_API LinearColor LerpUsingHsv(const LinearColor& from, const LinearColor& to, const float progress);

  /** Quantizes the linear color and returns the result as a Color.  This bypasses the SRGB conversion. */
  FUN_BASE_API Color Quantize() const;

  /** Quantizes the linear color and returns the result as a Color with optional sRGB conversion and quality as goal. */
  FUN_BASE_API Color ToColor(const bool sRGB) const;

  /**
   * Returns a desaturated color, with 0 meaning no desaturation and 1 == full desaturation
   *
   * \param desaturation - desaturation factor in range [0..1]
   *
   * \return Desaturated color
   */
  FUN_BASE_API LinearColor Desaturate(float desaturation) const;

  /** Computes the perceptually weighted luminance value of a color. */
  FUN_BASE_API float ComputeLuminance() const;

  /**
   * Returns the maximum value in this color structure
   *
   * \return The maximum color channel value
   */
  FUN_ALWAYS_INLINE float GetMax() const {
    return Math::Max(Math::Max(Math::Max(r, g), b), a);
  }

  /** useful to detect if a light contribution needs to be rendered */
  bool IsAlmostBlack() const {
    return Math::Square(r) < DELTA && Math::Square(g) < DELTA && Math::Square(b) < DELTA;
  }

  /**
   * Returns the minimum value in this color structure
   *
   * \return The minimum color channel value
   */
  FUN_ALWAYS_INLINE float GetMin() const {
    return Math::Min(Math::Min(Math::Min(r, g), b), a);
  }

  FUN_ALWAYS_INLINE float GetLuminance() const {
    return r * 0.3f + g * 0.59f + b * 0.11f;
  }

  String ToString() const {
    return String::Format("(r={0}, g={1}, b={2}, a={3})", r, g, b, a);
  }

  /**
   * Initialize this color based on an string. The String is expected to contain r=, g=, b=, a=.
   * The LinearColor will be bogus when InitFromString returns false.
   *
   * \param string - string containing the color values.
   *
   * \return true if the r, g, b values were read successfully; false otherwise.
   */
  bool InitFromString(const String& string) {
    r = g = b = 0.f;
    a = 1.f;

    // The initialization is only successful if the r, g, and b values can all be parsed from the string
    const bool ok = Parse::Value(*string, "r=" , r) && Parse::Value(*string, "g=", g) && Parse::Value(*string, "b=", b);

    // alpha is optional, so don't factor in its presence (or lack thereof) in determining initialization success
    Parse::Value(*string, "a=", a);

    return ok;
  }

  // Common colors.
  static FUN_BASE_API const LinearColor White;
  static FUN_BASE_API const LinearColor Gray;
  static FUN_BASE_API const LinearColor Black;
  static FUN_BASE_API const LinearColor Transparent;
  static FUN_BASE_API const LinearColor Red;
  static FUN_BASE_API const LinearColor Green;
  static FUN_BASE_API const LinearColor Blue;
  static FUN_BASE_API const LinearColor Yellow;
};

//TODO friend로 안에다가 넣어주어도 될듯 한데...
FUN_ALWAYS_INLINE LinearColor operator * (float scalar, const LinearColor& color) {
  return color.operator*(scalar);
}


/**
 * Color
 */
class Color {
 public:
  // Variables.
#if FUN_ARCH_LITTLE_ENDIAN

  #if _MSC_VER
    // Win32 x86
    union { struct{ uint8 b, g, r, a; }; uint32 alignment_dummy; };
  #else
    // Linux x86, etc
    uint8 b GCC_ALIGN(4);
    uint8 g, r, a;
  #endif

#else // FUN_ARCH_LITTLE_ENDIAN

  union { struct{ uint8 a, r, g, b; }; uint32 alignment_dummy; };

#endif

  uint32& DWColor() { return *((uint32*)this); }
  const uint32& DWColor() const { return *((uint32*)this); }

  // Constructors.
  FUN_ALWAYS_INLINE Color() {}

  FUN_ALWAYS_INLINE explicit Color(ForceInit_TAG) {
    // put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
    r = g = b = a = 0;
  }

  FUN_ALWAYS_INLINE Color(uint8 r, uint8 g, uint8 b, uint8 a = 255) {
    // put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
  }

  FUN_ALWAYS_INLINE explicit Color(uint32 color) {
    DWColor() = color;
  }

  // Serializer.
  friend Archive& operator & (Archive& ar, Color& color) {
    return ar & color.DWColor();
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

  // Operators.
  FUN_ALWAYS_INLINE bool operator == (const Color& c) const {
    return DWColor() == c.DWColor();
  }

  FUN_ALWAYS_INLINE bool operator != (const Color& c) const {
    return DWColor() != c.DWColor();
  }

  FUN_ALWAYS_INLINE void operator += (const Color& c) {
    r = (uint8)Math::Min((int32)r + (int32)c.r, 255);
    g = (uint8)Math::Min((int32)g + (int32)c.g, 255);
    b = (uint8)Math::Min((int32)b + (int32)c.b, 255);
    a = (uint8)Math::Min((int32)a + (int32)c.a, 255);
  }

  FUN_BASE_API LinearColor FromRGBE() const;

  /**
   * Creates a color value from the given hexadecimal string.
   *
   * Supported formats are: RGB, RRGGBB, RRGGBBAA, #RGB, #RRGGBB, #RRGGBBAA
   *
   * \param hex_string - The hexadecimal string.
   *
   * \return The corresponding color value.
   *
   * @see ToHex
   */
  static FUN_BASE_API Color FromHex(const String& hex_string);

  /**
   * Makes a random but quite nice color.
   */
  static FUN_BASE_API Color MakeRandomColor();

  /**
   * Makes a color red->green with the passed in scalar (e.g. 0 is red, 1 is green)
   */
  static FUN_BASE_API Color MakeRedToGreenColorFromScalar(float scalar);

  /**
   * Converts temperature in Kelvins of a black body radiator to RGB chromaticity.
   */
  static FUN_BASE_API Color MakeFromColorTemperature(float tmp);

  /**
   * \return a new Color based of this color with the new alpha value.
   * Usage: const Color& MyColor = ColorList::Green.WithAlpha(128);
   */
  Color WithAlpha(uint8 alpha) const {
    return Color(r, g, b, alpha);
  }

  /**
   * Reinterprets the color as a linear color.
   *
   * \return The linear color representation.
   */
  FUN_ALWAYS_INLINE LinearColor ReinterpretAsLinear() const {
    return LinearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
  }

  /**
   * Converts this color value to a hexadecimal string.
   *
   * The format of the string is RRGGBBAA.
   *
   * @result Hexadecimal string.
   *
   * @see FromHex
   * @see ToString
   */
  FUN_ALWAYS_INLINE String ToHex() const {
    return String::Format("%02X%02X%02X%02X", r, g, b, a);
  }

  /**
   * Converts this color value to a string.
   *
   * @result The string representation.
   *
   * @see ToHex
   */
  FUN_ALWAYS_INLINE String ToString() const {
    return String::Format("(r=%i, g=%i, b=%i, a=%i)", r, g, b, a);
  }

  /**
   * Initialize this color based on an string. The String is expected to contain r=, g=, b=, a=.
   * The Color will be bogus when InitFromString returns false.
   *
   * \param string - string containing the color values.
   *
   * \return true if the r, g, b values were read successfully; false otherwise.
   */
  bool InitFromString(const String& string) {
    r = g = b = 0;
    a = 255;

    // The initialization is only successful if the r, g, and b values can all be parsed from the string
    const bool ok =
      Parse::Value(*string, "r=", r) &&
      Parse::Value(*string, "g=", g) &&
      Parse::Value(*string, "b=", b);

    // alpha is optional, so don't factor in its presence (or lack thereof) in determining initialization success
    Parse::Value(*string, "a=", a);

    return ok;
  }

  /**
   * Gets the color in a packed uint32 format packed in the order ARGB.
   */
  FUN_ALWAYS_INLINE uint32 ToPackedARGB() const {
    return (a << 24) | (r << 16) | (g << 8) | (b);
  }

  /**
   * Gets the color in a packed uint32 format packed in the order ABGR.
   */
  FUN_ALWAYS_INLINE uint32 ToPackedABGR() const {
    return (a << 24) | (b << 16) | (g << 8) | (r);
  }

  /**
   * Gets the color in a packed uint32 format packed in the order RGBA.
   */
  FUN_ALWAYS_INLINE uint32 ToPackedRGBA() const {
    return (r << 24) | (g << 16) | (b << 8) | (a);
  }

  /**
   * Gets the color in a packed uint32 format packed in the order BGRA.
   */
  FUN_ALWAYS_INLINE uint32 ToPackedBGRA() const {
    return (b << 24) | (g << 16) | (r << 8) | (a);
  }

  /** Some pre-inited colors, useful for debug code */
  static FUN_BASE_API const Color White;
  static FUN_BASE_API const Color Black;
  static FUN_BASE_API const Color Transparent;
  static FUN_BASE_API const Color Red;
  static FUN_BASE_API const Color Green;
  static FUN_BASE_API const Color Blue;
  static FUN_BASE_API const Color Yellow;
  static FUN_BASE_API const Color Cyan;
  static FUN_BASE_API const Color Magenta;
  static FUN_BASE_API const Color Orange;
  static FUN_BASE_API const Color Purple;
  static FUN_BASE_API const Color Turquoise;
  static FUN_BASE_API const Color Silver;
  static FUN_BASE_API const Color Emerald;

 private:
  /**
   * Please use .ToColor(true) on LinearColor if you wish to convert from LinearColor to Color,
   * with proper sRGB conversion applied.
   *
   * Note: Do not implement or make public.  We don't want people needlessly and implicitly converting between
   * LinearColor and Color.  It's not a free conversion.
   */
  explicit Color(const LinearColor& linear_color);
};


FUN_ALWAYS_INLINE uint32 HashOf(const Color& color) {
  return color.DWColor();
}

FUN_ALWAYS_INLINE uint32 HashOf(const LinearColor& linear_color) {
  // Note: this assumes there's no padding in LinearColor that could contain uncompared data.
  return Crc::Crc32(&linear_color, sizeof(linear_color));
}

/** Computes a brightness and a fixed point color from a floating point color. */
extern FUN_BASE_API void ComputeAndFixedColorAndIntensity(const LinearColor& linear_color, Color& out_color, float& out_intensity);

// These act like a POD
template <> struct IsPOD<Color> { enum { Value = true }; };
template <> struct IsPOD<LinearColor> { enum { Value = true }; };


/**
 * Helper struct for a 16 bit 565 color of a DXT1/3/5 block.
 */
class DXTColor565 {
 public:
  /** blue component, 5 bit. */
  uint16 b:5;
  /** green component, 6 bit. */
  uint16 g:6;
  /** red component, 5 bit */
  uint16 r:5;
};


/**
 * Helper struct for a 16 bit 565 color of a DXT1/3/5 block.
 */
class DXTColor16 {
 public:
  union {
    /** 565 color */
    DXTColor565 color565;
    /** 16 bit entity representation for easy access. */
    uint16 value;
  };
};


/**
 * Structure encompassing single DXT1 block.
 */
class DXT1 {
 public:
  /** color 0/1 */
  union {
    DXTColor16 color[2];
    uint32 colors;
  };
  /** Indices controlling how to blend colors. */
  uint32 indices;
};


/**
 * Structure encompassing single DXT5 block
 */
class DXT5 {
 public:
  /** alpha component of DXT5 */
  uint8 alpha[8];
  /** DXT1 color component. */
  DXT1 dxt1;
};

// Make DXT helpers act like PODs
template <> struct IsPOD<DXT1> { enum { Value = true }; };
template <> struct IsPOD<DXT5> { enum { Value = true }; };
template <> struct IsPOD<DXTColor16> { enum { Value = true }; };
template <> struct IsPOD<DXTColor565> { enum { Value = true }; };

} // namespace fun
