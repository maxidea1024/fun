#pragma once

namespace fun {

/**
 * Interface that allows the CurveEditor to edit this type of object.
 */
class FUN_BASE_API CurveEdInterface
{
 public:
  /**
   * Get number of keyframes in curve.
   */
  virtual int32 GetNumKeys() const { return 0; }

  /**
   * Get number of 'sub curves' in this Curve. For example, a vector curve will have 3 sub-curves, for x, y and z.
   */
  virtual int32 GetNumSubCurves() const { return 0; }

  /**
   * Provides the color for the sub-curve button that is present on the curve tab.
   *
   * \param sub_curve_index - The index of the sub-curve. Cannot be negative nor greater or equal to the number of sub-curves.
   * \param is_sub_curve_hidden - Is the curve hidden?
   *
   * \return The color associated to the given sub-curve index.
   */
  virtual Color GetSubCurveButtonColor(int32 sub_curve_index, bool is_sub_curve_hidden) const { return  is_sub_curve_hidden ? Color(32, 0, 0) : Color(255, 0, 0); }

  /**
   * Get the input value for the Key with the specified index. key_index must be within range ie >=0 and < NumKeys.
   */
  virtual float GetKeyIn(int32 key_index) { return 0.f; }

  /**
   * Get the output value for the key with the specified index on the specified sub-curve.
   * sub_index must be within range ie >=0 and < NumSubCurves.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual float GetKeyOut(int32 sub_index, int32 key_index) { return 0.f; }

  /**
   * Provides the color for the given key at the given sub-curve.
   *
   * \param sub_index - The index of the sub-curve
   * \param key_index - The index of the key in the sub-curve
   * \param curve_color - The color of the curve
   *
   * \return The color that is associated the given key at the given sub-curve
   */
  virtual Color GetKeyColor(int32 sub_index, int32 key_index, const Color& curve_color) { return curve_color; }

  /**
   * Evaluate a subcurve at an arbitary point. Outside the keyframe range, curves are assumed to continue their end values.
   */
  virtual float EvalSub(int32 sub_index, float in_val) { return 0.f; }

  /**
   * Get the interpolation mode of the specified keyframe. This can be CIM_Constant, CIM_Linear or CIM_Curve.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual InterpCurveMode GetKeyInterpMode(int32 key_index) const { return CIM_Linear; }

  /**
   * Get the incoming and outgoing tangent for the given subcurve and key.
   * sub_index must be within range ie >=0 and < NumSubCurves.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual void GetTangents(int32 sub_index, int32 key_index, float& arrive_tangent, float& leave_tangent) const { arrive_tangent = 0.f; leave_tangent = 0.f; }

  /**
   * Get input range of keys. Outside this region curve continues constantly the start/end values.
   */
  virtual void GetInRange(float& min_in, float& max_in) const { min_in=0.f; max_in=0.f; }

  /**
   * Get overall range of output values.
   */
  virtual void GetOutRange(float& min_out, float& max_out) const { min_out=0.f; max_out=0.f; }

  /**
   * Add a new key to the curve with the specified input. Its initial value is set using EvalSub at that location.
   * Returns the index of the new key.
   */
  virtual int32 CreateNewKey(float key_in) { return INVALID_INDEX; }

  /**
   * Remove the specified key from the curve.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual void DeleteKey(int32 key_index) {}

  /**
   * Set the input value of the specified Key. This may change the index of the key, so the new index of the key is retured.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual int32 SetKeyIn(int32 key_index, float new_in_val) { return key_index; }

  /**
   * Set the output values of the specified key.
   * sub_index must be within range ie >=0 and < NumSubCurves.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual void SetKeyOut(int32 sub_index, int32 key_index, float new_out_val) {}

  /**
   * Set the method to use for interpolating between the give keyframe and the next one.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual void SetKeyInterpMode(int32 key_index, InterpCurveMode new_mode) {}

  /**
   * Set the incoming and outgoing tangent for the given subcurve and key.
   * sub_index must be within range ie >=0 and < NumSubCurves.
   * key_index must be within range ie >=0 and < NumKeys.
   */
  virtual void SetTangents(int32 sub_index, int32 key_index, float arrive_tangent, float leave_tangent) {}
};

} // namespace fun
