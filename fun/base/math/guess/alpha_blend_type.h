#pragma once

namespace fun {

/** Various ways to interpolate TAlphaBlend. */
enum EAlphaBlendType
{
  ABT_Linear              = 0,
  ABT_Cubic               = 1,
  ABT_Sinusoidal          = 2,
  ABT_EaseInOutExponent2  = 3,
  ABT_EaseInOutExponent3  = 4,
  ABT_EaseInOutExponent4  = 5,
  ABT_EaseInOutExponent5  = 6,
  ABT_MAX                 = 7,
};

/** Turn a linear interpolated alpha into the corresponding AlphaBlendType */
inline float AlphaToBlendType(float alpha, uint8 BlendType)
{
  switch (BlendType) {
  case ABT_Sinusoidal         : return Math::Clamp<float>((Math::Sin(alpha * PI - HALF_PI) + 1.f) / 2.f, 0.f, 1.f);
  case ABT_Cubic              : return Math::Clamp<float>(Math::CubicInterp<float>(0.f, 0.f, 1.f, 0.f, alpha), 0.f, 1.f);
  case ABT_EaseInOutExponent2 : return Math::Clamp<float>(Math::InterpEaseInOut<float>(0.f, 1.f, alpha, 2), 0.f, 1.f);
  case ABT_EaseInOutExponent3 : return Math::Clamp<float>(Math::InterpEaseInOut<float>(0.f, 1.f, alpha, 3), 0.f, 1.f);
  case ABT_EaseInOutExponent4 : return Math::Clamp<float>(Math::InterpEaseInOut<float>(0.f, 1.f, alpha, 4), 0.f, 1.f);
  case ABT_EaseInOutExponent5 : return Math::Clamp<float>(Math::InterpEaseInOut<float>(0.f, 1.f, alpha, 5), 0.f, 1.f);
  }
  return alpha;
}

/**
alpha Blend Type
*/
struct
DEPRECATED(4.9, "CTAlphaBlend is deprecated, please use CAlphaBlend instead")
CTAlphaBlend
{
  /** Internal Lerped value for alpha */
  float AlphaIn;
  /** Resulting alpha value, between 0.f and 1.f */
  float AlphaOut;
  /** target to reach */
  float AlphaTarget;
  /** Default blend time */
  float BlendTime;
  /** Time left to reach target */
  float BlendTimeToGo;
  /** Type of blending used (Linear, Cubic, etc.) */
  uint8 BlendType;

  // Constructor
  CTAlphaBlend();

  /**
  Constructor

  \param InAlphaIn Internal Lerped value for alpha
  \param InAlphaOut Resulting alpha value, between 0.f and 1.f
  \param InAlphaTarget target to reach
  \param InBlendTime Default blend time
  \param InBlendTimeToGo Time left to reach target
  \param InBlendType Type of blending used (Linear, Cubic, etc.)
  */
   CTAlphaBlend(float InAlphaIn, float InAlphaOut, float InAlphaTarget, float InBlendTime, float InBlendTimeToGo, uint8 InBlendType);

  /**
  Serializes the alpha Blend.

  \param ar Reference to the serialization archive.
  \param AlphaBlend Reference to the alpha blend being serialized.

  \return Reference to the Archive after serialization.
  */
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  friend Archive& operator & (Archive& ar, CTAlphaBlend& AlphaBlend)
  {
    return ar & AlphaBlend.AlphaIn & AlphaBlend.AlphaOut & AlphaBlend.AlphaTarget & AlphaBlend.BlendTime & AlphaBlend.BlendTimeToGo & AlphaBlend.BlendType;
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /** Update transition blend time */
  inline void SetBlendTime(float InBlendTime);

  /** Reset to zero */
  void Reset();

  /** Returns true if target is > 0.f, or false otherwise */
  inline bool GetToggleStatus();

  /** Enable (1.f) or Disable (0.f) */
  inline void Toggle(bool bEnable);

  /** SetTarget, but check that we're actually setting a different target */
  inline void ConditionalSetTarget(float InAlphaTarget);

  /** Set target for interpolation */
  void SetTarget(float InAlphaTarget);

  /** Update interpolation, has to be called once every frame */
  void Update(float InDeltaTime);
};

// We're disabling deprecation warnings here as we get a bunch because we're using
// methods and members from the deprecated struct, we're only really concerned with
// uses outside of this struct
PRAGMA_DISABLE_DEPRECATION_WARNINGS
inline CTAlphaBlend::CTAlphaBlend() {}

inline CTAlphaBlend::CTAlphaBlend(float InAlphaIn, float InAlphaOut, float InAlphaTarget, float InBlendTime, float InBlendTimeToGo, uint8 InBlendType)
  : AlphaIn(InAlphaIn)
  , AlphaOut(InAlphaOut)
  , AlphaTarget(InAlphaTarget)
  , BlendTime(InBlendTime)
  , BlendTimeToGo(InBlendTimeToGo)
  , BlendType(InBlendType)
{}

inline void CTAlphaBlend::SetBlendTime(float InBlendTime)
{
  BlendTime = Math::Max(InBlendTime, 0.f);
}

inline void CTAlphaBlend::Reset()
{
  AlphaIn = 0.f;
  AlphaOut = 0.f;
  AlphaTarget = 0.f;
  BlendTimeToGo = 0.f;
}

inline bool CTAlphaBlend::GetToggleStatus()
{
  return (AlphaTarget > 0.f);
}

inline void CTAlphaBlend::Toggle(bool bEnable)
{
  ConditionalSetTarget(bEnable ? 1.f : 0.f);
}

inline void CTAlphaBlend::ConditionalSetTarget(float InAlphaTarget)
{
  if (AlphaTarget != InAlphaTarget)
  {
    SetTarget(InAlphaTarget);
  }
}

inline void CTAlphaBlend::SetTarget(float InAlphaTarget)
{
  // Clamp parameters to valid range
  AlphaTarget = Math::Clamp<float>(InAlphaTarget, 0.f, 1.f);

  // if blend time is zero, transition now, don't wait to call update.
  if (BlendTime <= 0.f)
  {
    AlphaIn = AlphaTarget;
    AlphaOut = AlphaToBlendType(AlphaIn, BlendType);
    BlendTimeToGo = 0.f;
  }
  else
  {
    // Blend time is to go all the way, so scale that by how much we have to travel
    BlendTimeToGo = BlendTime * Math::Abs(AlphaTarget - AlphaIn);
  }
}

inline void CTAlphaBlend::Update(float InDeltaTime)
{
  // Make sure passed in delta time is positive
  fun_check(InDeltaTime >= 0.f);

  if (AlphaIn != AlphaTarget)
  {
    if (BlendTimeToGo >= InDeltaTime)
    {
      const float BlendDelta = AlphaTarget - AlphaIn;
      AlphaIn += (BlendDelta / BlendTimeToGo) * InDeltaTime;
      BlendTimeToGo -= InDeltaTime;
    }
    else
    {
      BlendTimeToGo = 0.f;
      AlphaIn = AlphaTarget;
    }

    // Convert Lerped alpha to corresponding blend type.
    AlphaOut = AlphaToBlendType(AlphaIn, BlendType);
  }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

} // namespace fun
