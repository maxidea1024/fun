#pragma once

#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API ColorList : public Exec {
 public:
  typedef Map<String, const Color*> ColorsMap;
  typedef Array<const Color*> ColorsLookup;

  // Common colors.
  static const Color White;
  static const Color Red;
  static const Color Green;
  static const Color Blue;
  static const Color Magenta;
  static const Color Cyan;
  static const Color Yellow;
  static const Color Black;
  static const Color Aquamarine;
  static const Color BakerChocolate;
  static const Color BlueViolet;
  static const Color Brass;
  static const Color BrightGold;
  static const Color Brown;
  static const Color Bronze;
  static const Color BronzeII;
  static const Color CadetBlue;
  static const Color CoolCopper;
  static const Color Copper;
  static const Color Coral;
  static const Color CornFlowerBlue;
  static const Color DarkBrown;
  static const Color DarkGreen;
  static const Color DarkGreenCopper;
  static const Color DarkOliveGreen;
  static const Color DarkOrchid;
  static const Color DarkPurple;
  static const Color DarkSlateBlue;
  static const Color DarkSlateGrey;
  static const Color DarkTan;
  static const Color DarkTurquoise;
  static const Color DarkWood;
  static const Color DimGrey;
  static const Color DustyRose;
  static const Color Feldspar;
  static const Color Firebrick;
  static const Color ForestGreen;
  static const Color Gold;
  static const Color Goldenrod;
  static const Color Grey;
  static const Color GreenCopper;
  static const Color GreenYellow;
  static const Color HunterGreen;
  static const Color IndianRed;
  static const Color Khaki;
  static const Color LightBlue;
  static const Color LightGrey;
  static const Color LightSteelBlue;
  static const Color LightWood;
  static const Color LimeGreen;
  static const Color MandarianOrange;
  static const Color Maroon;
  static const Color MediumAquamarine;
  static const Color MediumBlue;
  static const Color MediumForestGreen;
  static const Color MediumGoldenrod;
  static const Color MediumOrchid;
  static const Color MediumSeaGreen;
  static const Color MediumSlateBlue;
  static const Color MediumSpringGreen;
  static const Color MediumTurquoise;
  static const Color MediumVioletRed;
  static const Color MediumWood;
  static const Color MidnightBlue;
  static const Color NavyBlue;
  static const Color NeonBlue;
  static const Color NeonPink;
  static const Color NewMidnightBlue;
  static const Color NewTan;
  static const Color OldGold;
  static const Color Orange;
  static const Color OrangeRed;
  static const Color Orchid;
  static const Color PaleGreen;
  static const Color Pink;
  static const Color Plum;
  static const Color Quartz;
  static const Color RichBlue;
  static const Color Salmon;
  static const Color Scarlet;
  static const Color SeaGreen;
  static const Color SemiSweetChocolate;
  static const Color Sienna;
  static const Color Silver;
  static const Color SkyBlue;
  static const Color SlateBlue;
  static const Color SpicyPink;
  static const Color SpringGreen;
  static const Color SteelBlue;
  static const Color SummerSky;
  static const Color Tan;
  static const Color Thistle;
  static const Color Turquoise;
  static const Color VeryDarkBrown;
  static const Color VeryLightGrey;
  static const Color Violet;
  static const Color VioletRed;
  static const Color Wheat;
  static const Color YellowGreen;

  /** Initializes list of common colors. */
  void CreateColorMap();

  //TODO 이름을 변경하자.
  /** Returns a color based on color_name if not found returs White. */
  const Color& GetCColorByName(const char* color_name) const;

  /** Returns a linear color based on color_name if not found returs White. */
  const LinearColor GetLinearColorByName(const char* color_name) const;

  /** Returns true if color is valid common colors, returns false otherwise. */
  bool IsValidColorName(const char* color_name) const;

  /** Returns index of color. */
  int32 GetColorIndex(const char* color_name) const;

  //TODO 이름을 변경하자.
  /** Returns a color based on index. If index is invalid, returns White. */
  const Color& GetCColorByIndex(int32 color_index) const;

  /** Resturn color's name based on index. If index is invalid, returns BadIndex. */
  const String& GetColorNameByIndex(int32 color_index) const;

  /** Returns the number of colors. */
  int32 GetColorsCount() const {
    return colors_map_.Count();
  }

  /**
   * Prints to log all colors information.
   */
  void LogColors();

  //TODO?
  bool Exec(class RuntimeEnv* Env, const char* Cmd, Printer& Prn) override
  {
    return false;
  }

 protected:
  void InitializeColor(const char* color_name, const Color* color_ptr, int32& current_index);

  /**
   * List of common colors.
   */
  ColorsMap colors_map_;

  /**
   * Array of colors for fast lookup when using index.
   */
  ColorsLookup colors_lookup_;
};

//TODO class 안으로 넣어주는게 좋지 아니한가??
extern FUN_BASE_API ColorList g_color_list;

} // namespace fun
