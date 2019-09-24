#include "fun/base/math/color_list.h"

namespace fun {

DEFINE_LOG_CATEGORY_STATIC(LogColorList, Info, All);

/** Global instance of color list helper class. */
ColorList g_color_list;

const Color& ColorList::GetCColorByName(const char* color_name) const {
  const Color* color = colors_map_.FindRef(color_name);
  if (color) {
    return *color;
  }

  return White;
}

const Color& ColorList::GetCColorByIndex(int32 color_index) const {
  if (colors_lookup_.IsValidIndex(color_index) == true) {
    return *colors_lookup_[color_index];
  }

  return White;
}

const LinearColor ColorList::GetLinearColorByName(const char* color_name) const {
  if (const Color* color = colors_map_.FindRef(color_name)) {
    return LinearColor(*color);
  }

  return LinearColor::White;
}

bool ColorList::IsValidColorName(const char* color_name) const {
  return colors_map_.Contains(color_name);
}

int32 ColorList::GetColorIndex(const char* color_name) const {
  const Color& color = GetCColorByName(color_name);
  int32 color_index = 0;
  colors_lookup_.Find(&color, color_index);
  return color_index;
}

const String& ColorList::GetColorNameByIndex(int32 color_index) const {
  static const String BAD_INDEX("BadIndex");

  if (colors_lookup_.IsValidIndex(color_index) == true) {
    const Color& color = *colors_lookup_[color_index];
    const String& color_name = *colors_map_.FindKey(&color);
    return color_name;
  }

  return BAD_INDEX;
}

void ColorList::CreateColorMap() {
  int32 index = 0;

  //Blackcolormustbefirst.
  InitializeColor("black", &ColorList::Black, index);
  InitializeColor("aquamarine", &ColorList::Aquamarine, index);
  InitializeColor("bakerchocolate", &ColorList::BakerChocolate, index);
  InitializeColor("blue", &ColorList::Blue, index);
  InitializeColor("blueviolet", &ColorList::BlueViolet, index);
  InitializeColor("brass", &ColorList::Brass, index);
  InitializeColor("brightgold", &ColorList::BrightGold, index);
  InitializeColor("brown", &ColorList::Brown, index);
  InitializeColor("bronze", &ColorList::Bronze, index);
  InitializeColor("bronzeii", &ColorList::BronzeII, index);
  InitializeColor("cadetblue", &ColorList::CadetBlue, index);
  InitializeColor("coolcopper", &ColorList::CoolCopper, index);
  InitializeColor("copper", &ColorList::Copper, index);
  InitializeColor("coral", &ColorList::Coral, index);
  InitializeColor("cornflowerblue", &ColorList::CornFlowerBlue, index);
  InitializeColor("cyan", &ColorList::Cyan, index);
  InitializeColor("darkbrown", &ColorList::DarkBrown, index);
  InitializeColor("darkgreen", &ColorList::DarkGreen, index);
  InitializeColor("darkgreencopper", &ColorList::DarkGreenCopper, index);
  InitializeColor("darkolivegreen", &ColorList::DarkOliveGreen, index);
  InitializeColor("darkorchid", &ColorList::DarkOrchid, index);
  InitializeColor("darkpurple", &ColorList::DarkPurple, index);
  InitializeColor("darkslateblue", &ColorList::DarkSlateBlue, index);
  InitializeColor("darkslategrey", &ColorList::DarkSlateGrey, index);
  InitializeColor("darktan", &ColorList::DarkTan, index);
  InitializeColor("darkturquoise", &ColorList::DarkTurquoise, index);
  InitializeColor("darkwood", &ColorList::DarkWood, index);
  InitializeColor("dimgrey", &ColorList::DimGrey, index);
  InitializeColor("dustyrose", &ColorList::DustyRose, index);
  InitializeColor("feldspar", &ColorList::Feldspar, index);
  InitializeColor("firebrick", &ColorList::Firebrick, index);
  InitializeColor("forestgreen", &ColorList::ForestGreen, index);
  InitializeColor("gold", &ColorList::Gold, index);
  InitializeColor("goldenrod", &ColorList::Goldenrod, index);
  InitializeColor("green", &ColorList::Green, index);
  InitializeColor("greencopper", &ColorList::GreenCopper, index);
  InitializeColor("greenyellow", &ColorList::GreenYellow, index);
  InitializeColor("grey", &ColorList::Grey, index);
  InitializeColor("huntergreen", &ColorList::HunterGreen, index);
  InitializeColor("indianred", &ColorList::IndianRed, index);
  InitializeColor("khaki", &ColorList::Khaki, index);
  InitializeColor("lightblue", &ColorList::LightBlue, index);
  InitializeColor("lightgrey", &ColorList::LightGrey, index);
  InitializeColor("lightsteelblue", &ColorList::LightSteelBlue, index);
  InitializeColor("lightwood", &ColorList::LightWood, index);
  InitializeColor("limegreen", &ColorList::LimeGreen, index);
  InitializeColor("magenta", &ColorList::Magenta, index);
  InitializeColor("mandarianorange", &ColorList::MandarianOrange, index);
  InitializeColor("maroon", &ColorList::Maroon, index);
  InitializeColor("mediumaquamarine", &ColorList::MediumAquamarine, index);
  InitializeColor("mediumblue", &ColorList::MediumBlue, index);
  InitializeColor("mediumforestgreen", &ColorList::MediumForestGreen, index);
  InitializeColor("mediumgoldenrod", &ColorList::MediumGoldenrod, index);
  InitializeColor("mediumorchid", &ColorList::MediumOrchid, index);
  InitializeColor("mediumseagreen", &ColorList::MediumSeaGreen, index);
  InitializeColor("mediumslateblue", &ColorList::MediumSlateBlue, index);
  InitializeColor("mediumspringgreen", &ColorList::MediumSpringGreen, index);
  InitializeColor("mediumturquoise", &ColorList::MediumTurquoise, index);
  InitializeColor("mediumvioletred", &ColorList::MediumVioletRed, index);
  InitializeColor("mediumwood", &ColorList::MediumWood, index);
  InitializeColor("midnightblue", &ColorList::MidnightBlue, index);
  InitializeColor("navyblue", &ColorList::NavyBlue, index);
  InitializeColor("neonblue", &ColorList::NeonBlue, index);
  InitializeColor("neonpink", &ColorList::NeonPink, index);
  InitializeColor("newmidnightblue", &ColorList::NewMidnightBlue, index);
  InitializeColor("newtan", &ColorList::NewTan, index);
  InitializeColor("oldgold", &ColorList::OldGold, index);
  InitializeColor("orange", &ColorList::Orange, index);
  InitializeColor("orangered", &ColorList::OrangeRed, index);
  InitializeColor("orchid", &ColorList::Orchid, index);
  InitializeColor("palegreen", &ColorList::PaleGreen, index);
  InitializeColor("pink", &ColorList::Pink, index);
  InitializeColor("plum", &ColorList::Plum, index);
  InitializeColor("quartz", &ColorList::Quartz, index);
  InitializeColor("red", &ColorList::Red, index);
  InitializeColor("richblue", &ColorList::RichBlue, index);
  InitializeColor("salmon", &ColorList::Salmon, index);
  InitializeColor("scarlet", &ColorList::Scarlet, index);
  InitializeColor("seagreen", &ColorList::SeaGreen, index);
  InitializeColor("semisweetchocolate", &ColorList::SemiSweetChocolate, index);
  InitializeColor("sienna", &ColorList::Sienna, index);
  InitializeColor("silver", &ColorList::Silver, index);
  InitializeColor("skyblue", &ColorList::SkyBlue, index);
  InitializeColor("slateblue", &ColorList::SlateBlue, index);
  InitializeColor("spicypink", &ColorList::SpicyPink, index);
  InitializeColor("springgreen", &ColorList::SpringGreen, index);
  InitializeColor("steelblue", &ColorList::SteelBlue, index);
  InitializeColor("summersky", &ColorList::SummerSky, index);
  InitializeColor("tan", &ColorList::Tan, index);
  InitializeColor("thistle", &ColorList::Thistle, index);
  InitializeColor("turquoise", &ColorList::Turquoise, index);
  InitializeColor("verydarkbrown", &ColorList::VeryDarkBrown, index);
  InitializeColor("verylightgrey", &ColorList::VeryLightGrey, index);
  InitializeColor("violet", &ColorList::Violet, index);
  InitializeColor("violetred", &ColorList::VioletRed, index);
  InitializeColor("wheat", &ColorList::Wheat, index);
  InitializeColor("white", &ColorList::White, index);
  InitializeColor("yellow", &ColorList::Yellow, index);
  InitializeColor("yellowgreen", &ColorList::YellowGreen, index);

  colors_lookup_.Shrink();
}

void ColorList::InitializeColor(const char* color_name, const Color* color_ptr, int32& current_index) {
  colors_map_.Add(color_name, color_ptr);
  colors_lookup_.Add(color_ptr);
  current_index ++;
}

void ColorList::LogColors() {
  for (ColorsMap::Iterator it(colors_map_); it; ++it) {
    const Color* color_ptr = it.Value();
    const String& color_name = it.Key();

    int32 color_index = 0;
    colors_lookup_.Find(color_ptr, color_index);

    fun_log(LogColorList, Info,  "%3i - %32s -> %s", color_index, *color_name, *color_ptr->ToString());
  }
}


//
// Common colors declarations.
//

const Color ColorList::White(255, 255, 255, 255);
const Color ColorList::Red(255, 0, 0, 255);
const Color ColorList::Green(0, 255, 0, 255);
const Color ColorList::Blue(0, 0, 255, 255);
const Color ColorList::Magenta(255, 0, 255, 255);
const Color ColorList::Cyan(0, 255, 255, 255);
const Color ColorList::Yellow(255, 255, 0, 255);
const Color ColorList::Black(0, 0, 0, 255);
const Color ColorList::Aquamarine(112, 219, 147, 255);
const Color ColorList::BakerChocolate(92, 51, 23, 255);
const Color ColorList::BlueViolet(159, 95, 159, 255);
const Color ColorList::Brass(181, 166, 66, 255);
const Color ColorList::BrightGold(217, 217, 25, 255);
const Color ColorList::Brown(166, 42, 42, 255);
const Color ColorList::Bronze(140, 120, 83, 255);
const Color ColorList::BronzeII(166, 125, 61, 255);
const Color ColorList::CadetBlue(95, 159, 159, 255);
const Color ColorList::CoolCopper(217, 135, 25, 255);
const Color ColorList::Copper(184, 115, 51, 255);
const Color ColorList::Coral(255, 127, 0, 255);
const Color ColorList::CornFlowerBlue(66, 66, 111, 255);
const Color ColorList::DarkBrown(92, 64, 51, 255);
const Color ColorList::DarkGreen(47, 79, 47, 255);
const Color ColorList::DarkGreenCopper(74, 118, 110, 255);
const Color ColorList::DarkOliveGreen(79, 79, 47, 255);
const Color ColorList::DarkOrchid(153, 50, 205, 255);
const Color ColorList::DarkPurple(135, 31, 120, 255);
const Color ColorList::DarkSlateBlue(107, 35, 142, 255);
const Color ColorList::DarkSlateGrey(47, 79, 79, 255);
const Color ColorList::DarkTan(151, 105, 79, 255);
const Color ColorList::DarkTurquoise(112, 147, 219, 255);
const Color ColorList::DarkWood(133, 94, 66, 255);
const Color ColorList::DimGrey(84, 84, 84, 255);
const Color ColorList::DustyRose(133, 99, 99, 255);
const Color ColorList::Feldspar(209, 146, 117, 255);
const Color ColorList::Firebrick(142, 35, 35, 255);
const Color ColorList::ForestGreen(35, 142, 35, 255);
const Color ColorList::Gold(205, 127, 50, 255);
const Color ColorList::Goldenrod(219, 219, 112, 255);
const Color ColorList::Grey(192, 192, 192, 255);
const Color ColorList::GreenCopper(82, 127, 118, 255);
const Color ColorList::GreenYellow(147, 219, 112, 255);
const Color ColorList::HunterGreen(33, 94, 33, 255);
const Color ColorList::IndianRed(78, 47, 47, 255);
const Color ColorList::Khaki(159, 159, 95, 255);
const Color ColorList::LightBlue(192, 217, 217, 255);
const Color ColorList::LightGrey(168, 168, 168, 255);
const Color ColorList::LightSteelBlue(143, 143, 189, 255);
const Color ColorList::LightWood(233, 194, 166, 255);
const Color ColorList::LimeGreen(50, 205, 50, 255);
const Color ColorList::MandarianOrange(228, 120, 51, 255);
const Color ColorList::Maroon(142, 35, 107, 255);
const Color ColorList::MediumAquamarine(50, 205, 153, 255);
const Color ColorList::MediumBlue(50, 50, 205, 255);
const Color ColorList::MediumForestGreen(107, 142, 35, 255);
const Color ColorList::MediumGoldenrod(234, 234, 174, 255);
const Color ColorList::MediumOrchid(147, 112, 219, 255);
const Color ColorList::MediumSeaGreen(66, 111, 66, 255);
const Color ColorList::MediumSlateBlue(127, 0, 255, 255);
const Color ColorList::MediumSpringGreen(127, 255, 0, 255);
const Color ColorList::MediumTurquoise(112, 219, 219, 255);
const Color ColorList::MediumVioletRed(219, 112, 147, 255);
const Color ColorList::MediumWood(166, 128, 100, 255);
const Color ColorList::MidnightBlue(47, 47, 79, 255);
const Color ColorList::NavyBlue(35, 35, 142, 255);
const Color ColorList::NeonBlue(77, 77, 255, 255);
const Color ColorList::NeonPink(255, 110, 199, 255);
const Color ColorList::NewMidnightBlue(0, 0, 156, 255);
const Color ColorList::NewTan(235, 199, 158, 255);
const Color ColorList::OldGold(207, 181, 59, 255);
const Color ColorList::Orange(255, 127, 0, 255);
const Color ColorList::OrangeRed(255, 36, 0, 255);
const Color ColorList::Orchid(219, 112, 219, 255);
const Color ColorList::PaleGreen(143, 188, 143, 255);
const Color ColorList::Pink(188, 143, 143, 255);
const Color ColorList::Plum(234, 173, 234, 255);
const Color ColorList::Quartz(217, 217, 243, 255);
const Color ColorList::RichBlue(89, 89, 171, 255);
const Color ColorList::Salmon(111, 66, 66, 255);
const Color ColorList::Scarlet(140, 23, 23, 255);
const Color ColorList::SeaGreen(35, 142, 104, 255);
const Color ColorList::SemiSweetChocolate(107, 66, 38, 255);
const Color ColorList::Sienna(142, 107, 35, 255);
const Color ColorList::Silver(230, 232, 250, 255);
const Color ColorList::SkyBlue(50, 153, 204, 255);
const Color ColorList::SlateBlue(0, 127, 255, 255);
const Color ColorList::SpicyPink(255, 28, 174, 255);
const Color ColorList::SpringGreen(0, 255, 127, 255);
const Color ColorList::SteelBlue(35, 107, 142, 255);
const Color ColorList::SummerSky(56, 176, 222, 255);
const Color ColorList::Tan(219, 147, 112, 255);
const Color ColorList::Thistle(216, 191, 216, 255);
const Color ColorList::Turquoise(173, 234, 234, 255);
const Color ColorList::VeryDarkBrown(92, 64, 51, 255);
const Color ColorList::VeryLightGrey(205, 205, 205, 255);
const Color ColorList::Violet(79, 47, 79, 255);
const Color ColorList::VioletRed(204, 50, 153, 255);
const Color ColorList::Wheat(216, 216, 191, 255);
const Color ColorList::YellowGreen(153, 204, 50, 255);

} // namespace fun
