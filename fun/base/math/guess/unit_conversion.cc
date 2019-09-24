//DEPRECATED
#include "CorePrivatePCH.h"
#include "Math/UnitConversion.h"
#include "Math/BasicMathExpressionEvaluator.h"

#define LOCTEXT_NAMESPACE  "UnitConversion"

namespace fun {

/** Structure used to match units when parsing */
struct CParseCandidate
{
  const char* String;
  EUnit Unit;
};

CParseCandidate ParseCandidates[] = {

  { TEXT("Micrometers"),          EUnit::Micrometers },           { TEXT("um"),       EUnit::Micrometers },           { TEXT("\u00B5m"),  EUnit::Micrometers },
  { TEXT("Millimeters"),          EUnit::Millimeters },           { TEXT("mm"),       EUnit::Millimeters },
  { TEXT("Centimeters"),          EUnit::Centimeters },           { TEXT("cm"),       EUnit::Centimeters },
  { TEXT("Meters"),               EUnit::Meters },                { TEXT("m"),        EUnit::Meters },
  { TEXT("Kilometers"),           EUnit::Kilometers },            { TEXT("km"),       EUnit::Kilometers },
  { TEXT("Inches"),               EUnit::Inches },                { TEXT("in"),       EUnit::Inches },
  { TEXT("Feet"),                 EUnit::Feet },                  { TEXT("ft"),       EUnit::Feet },
  { TEXT("Yards"),                EUnit::Yards },                 { TEXT("yd"),       EUnit::Yards },
  { TEXT("Miles"),                EUnit::Miles },                 { TEXT("mi"),       EUnit::Miles },
  { TEXT("Lightyears"),           EUnit::Lightyears },            { TEXT("ly"),       EUnit::Lightyears },

  { TEXT("Degrees"),              EUnit::Degrees },               { TEXT("deg"),      EUnit::Degrees },               { TEXT("\u00B0"),   EUnit::Degrees },
  { TEXT("Radians"),              EUnit::Radians },               { TEXT("rad"),      EUnit::Radians },

  { TEXT("MetersPerSecond"),      EUnit::MetersPerSecond },       { TEXT("m/s"),      EUnit::MetersPerSecond },
  { TEXT("KilometersPerHour"),    EUnit::KilometersPerHour },     { TEXT("km/h"),     EUnit::KilometersPerHour },     { TEXT("kmph"),     EUnit::KilometersPerHour },
  { TEXT("MilesPerHour"),         EUnit::MilesPerHour },          { TEXT("mi/h"),     EUnit::MilesPerHour },          { TEXT("mph"),      EUnit::MilesPerHour },

  { TEXT("Celsius"),              EUnit::Celsius },               { TEXT("C"),        EUnit::Celsius },               { TEXT("degC"),     EUnit::Celsius },           { TEXT("\u00B0C"),      EUnit::Celsius },
  { TEXT("Farenheit"),            EUnit::Farenheit },             { TEXT("F"),        EUnit::Farenheit },             { TEXT("degF"),     EUnit::Farenheit },         { TEXT("\u00B0F"),      EUnit::Farenheit },
  { TEXT("Kelvin"),               EUnit::Kelvin },                { TEXT("K"),        EUnit::Kelvin },

  { TEXT("Micrograms"),           EUnit::Micrograms },            { TEXT("ug"),       EUnit::Micrograms },            { TEXT("\u00B5g"),      EUnit::Micrograms },
  { TEXT("Milligrams"),           EUnit::Milligrams },            { TEXT("mg"),       EUnit::Milligrams },
  { TEXT("Grams"),                EUnit::Grams },                 { TEXT("g"),        EUnit::Grams },
  { TEXT("Kilograms"),            EUnit::Kilograms },             { TEXT("kg"),       EUnit::Kilograms },
  { TEXT("MetricTons"),           EUnit::MetricTons },            { TEXT("t"),        EUnit::MetricTons },
  { TEXT("Ounces"),               EUnit::Ounces },                { TEXT("oz"),       EUnit::Ounces },
  { TEXT("Pounds"),               EUnit::Pounds },                { TEXT("lb"),       EUnit::Pounds },
  { TEXT("Stones"),               EUnit::Stones },                { TEXT("st"),       EUnit::Stones },

  { TEXT("Newtons"),              EUnit::Newtons },               { TEXT("N"),        EUnit::Newtons },
  { TEXT("PoundsForce"),          EUnit::PoundsForce },           { TEXT("lbf"),      EUnit::PoundsForce },
  { TEXT("KilogramsForce"),       EUnit::KilogramsForce },        { TEXT("kgf"),      EUnit::KilogramsForce },

  { TEXT("Hertz"),                EUnit::Hertz },                 { TEXT("Hz"),       EUnit::Hertz },
  { TEXT("Kilohertz"),            EUnit::Kilohertz },             { TEXT("KHz"),      EUnit::Kilohertz },
  { TEXT("Megahertz"),            EUnit::Megahertz },             { TEXT("MHz"),      EUnit::Megahertz },
  { TEXT("Gigahertz"),            EUnit::Gigahertz },             { TEXT("GHz"),      EUnit::Gigahertz },
  { TEXT("RevolutionsPerMinute"), EUnit::RevolutionsPerMinute },  { TEXT("rpm"),      EUnit::RevolutionsPerMinute },

  { TEXT("Bytes"),                EUnit::Bytes },                 { TEXT("B"),        EUnit::Bytes },
  { TEXT("Kilobytes"),            EUnit::Kilobytes },             { TEXT("KB"),       EUnit::Kilobytes },
  { TEXT("Megabytes"),            EUnit::Megabytes },             { TEXT("MB"),       EUnit::Megabytes },
  { TEXT("Gigabytes"),            EUnit::Gigabytes },             { TEXT("GB"),       EUnit::Gigabytes },
  { TEXT("Terabytes"),            EUnit::Terabytes },             { TEXT("TB"),       EUnit::Terabytes },

  { TEXT("Lumens"),               EUnit::Lumens },                { TEXT("lm"),       EUnit::Lumens },

  { TEXT("Milliseconds"),         EUnit::Milliseconds },          { TEXT("ms"),       EUnit::Milliseconds },
  { TEXT("Seconds"),              EUnit::Seconds },               { TEXT("s"),        EUnit::Seconds },
  { TEXT("Minutes"),              EUnit::Minutes },               { TEXT("min"),      EUnit::Minutes },
  { TEXT("Hours"),                EUnit::Hours },                 { TEXT("hrs"),      EUnit::Hours },
  { TEXT("Days"),                 EUnit::Days },                  { TEXT("dy"),       EUnit::Days },
  { TEXT("Months"),               EUnit::Months },                { TEXT("mth"),      EUnit::Months },
  { TEXT("Years"),                EUnit::Years },                 { TEXT("yr"),       EUnit::Years },

  { TEXT("times"),                EUnit::Multiplier },            { TEXT("x"),    EUnit::Multiplier },            { TEXT("multiplier"),       EUnit::Multiplier },
};


/** Static array of display strings that directly map to EUnit enumerations */
const char* const DisplayStrings[] = {
  TEXT("\u00B5m"),            TEXT("mm"),                 TEXT("cm"),                 TEXT("m"),                  TEXT("km"),
  TEXT("in"),                 TEXT("ft"),                 TEXT("yd"),                 TEXT("mi"),
  TEXT("ly"),

  TEXT("\u00B0"), TEXT("rad"),

  TEXT("m/s"), TEXT("km/h"), TEXT("mi/h"),

  TEXT("\u00B0C"), TEXT("\u00B0F"), TEXT("K"),

  TEXT("\u00B5g"), TEXT("mg"), TEXT("g"), TEXT("kg"), TEXT("t"),
  TEXT("oz"), TEXT("lb"), TEXT("st"),

  TEXT("N"), TEXT("lbf"), TEXT("kgf"),

  TEXT("Hz"), TEXT("KHz"), TEXT("MHz"), TEXT("GHz"), TEXT("rpm"),

  TEXT("B"), TEXT("KB"), TEXT("MB"), TEXT("GB"), TEXT("TB"),

  TEXT("lm"),

  TEXT("ms"), TEXT("s"), TEXT("min"), TEXT("hr"), TEXT("dy"), TEXT("mth"), TEXT("yr"),

  TEXT("x"),
};


const EUnitType UnitTypes[] = {
  EUnitType::Distance,    EUnitType::Distance,    EUnitType::Distance,    EUnitType::Distance,    EUnitType::Distance,
  EUnitType::Distance,    EUnitType::Distance,    EUnitType::Distance,    EUnitType::Distance,
  EUnitType::Distance,

  EUnitType::Angle,       EUnitType::Angle,

  EUnitType::Speed,       EUnitType::Speed,       EUnitType::Speed,

  EUnitType::Temperature, EUnitType::Temperature, EUnitType::Temperature,

  EUnitType::Mass,        EUnitType::Mass,        EUnitType::Mass,        EUnitType::Mass,        EUnitType::Mass,
  EUnitType::Mass,        EUnitType::Mass,        EUnitType::Mass,

  EUnitType::Force,       EUnitType::Force,       EUnitType::Force,

  EUnitType::Frequency,   EUnitType::Frequency,   EUnitType::Frequency,   EUnitType::Frequency,   EUnitType::Frequency,

  EUnitType::DataSize,    EUnitType::DataSize,    EUnitType::DataSize,    EUnitType::DataSize,    EUnitType::DataSize,

  EUnitType::LuminousFlux,

  EUnitType::Time,        EUnitType::Time,        EUnitType::Time,        EUnitType::Time,        EUnitType::Time,        EUnitType::Time,        EUnitType::Time,

  EUnitType::Arbitrary,
};



DEFINE_EXPRESSION_NODE_TYPE(CNumericUnit<double>, 0x3C138BC9, 0x71314F0B, 0xBB469BF7, 0xED47D147)

struct CUnitExpressionParser
{
  CUnitExpressionParser(EUnit InDefaultUnit)
  {
    using namespace ExpressionParser;

    TokenDefinitions.IgnoreWhitespace();

    // Defined in order of importance
    TokenDefinitions.DefineToken(&ConsumeSymbol<CPlus>);
    TokenDefinitions.DefineToken(&ConsumeSymbol<CMinus>);
    TokenDefinitions.DefineToken(&ConsumeSymbol<CStar>);
    TokenDefinitions.DefineToken(&ConsumeSymbol<CForwardSlash>);
    TokenDefinitions.DefineToken(&ConsumeSymbol<CSubExpressionStart>);
    TokenDefinitions.DefineToken(&ConsumeSymbol<CSubExpressionEnd>);

    TokenDefinitions.DefineToken([&](CExpressionTokenConsumer& Consumer){ return ConsumeNumberWithUnits(Consumer); });

    Grammar.DefineGrouping<CSubExpressionStart, CSubExpressionEnd>();

    Grammar.DefinePreUnaryOperator<CPlus>();
    Grammar.DefinePreUnaryOperator<CMinus>();

    Grammar.DefineBinaryOperator<CPlus>(5);
    Grammar.DefineBinaryOperator<CMinus>(5);
    Grammar.DefineBinaryOperator<CStar>(4);
    Grammar.DefineBinaryOperator<CForwardSlash>(4);

    // Unary operators for numeric units
    JumpTable.MapPreUnary<CPlus>(  [](const CNumericUnit<double>& N) { return CNumericUnit<double>(N.Value, N.Units);      });
    JumpTable.MapPreUnary<CMinus>( [](const CNumericUnit<double>& N) { return CNumericUnit<double>(-N.Value, N.Units);     });

    /** Addition for numeric units */
    JumpTable.MapBinary<CPlus>([InDefaultUnit](const CNumericUnit<double>& A, const CNumericUnit<double>& B) -> CExpressionResult {

      // Have to ensure we're adding the correct units here

      EUnit UnitsLHS = A.Units, UnitsRHS = B.Units;

      if (UnitsLHS == EUnit::Unspecified && UnitsRHS != EUnit::Unspecified)
      {
        UnitsLHS = InDefaultUnit;
      }
      else if (UnitsLHS != EUnit::Unspecified && UnitsRHS == EUnit::Unspecified)
      {
        UnitsRHS = InDefaultUnit;
      }

      if (CUnitConversion::AreUnitsCompatible(UnitsLHS, UnitsRHS))
      {
        if (UnitsLHS != EUnit::Unspecified)
        {
          return MakeValue(CNumericUnit<double>(A.Value + CUnitConversion::Convert(B.Value, UnitsRHS, UnitsLHS), UnitsLHS));
        }
        else
        {
          return MakeValue(CNumericUnit<double>(CUnitConversion::Convert(A.Value, UnitsLHS, UnitsRHS) + B.Value, UnitsRHS));
        }
      }

      CFormatOrderedArguments Args;
      Args.Add(CText::FromString(CUnitConversion::GetUnitDisplayString(B.Units)));
      Args.Add(CText::FromString(CUnitConversion::GetUnitDisplayString(A.Units)));
      return MakeError(CText::Format(LOCTEXT("CannotAddErr", "Cannot add {0} to {1}"), Args));
    });

    /** Subtraction for numeric units */
    JumpTable.MapBinary<CMinus>([InDefaultUnit](const CNumericUnit<double>& A, const CNumericUnit<double>& B) -> CExpressionResult {

      // Have to ensure we're adding the correct units here

      EUnit UnitsLHS = A.Units, UnitsRHS = B.Units;

      if (UnitsLHS == EUnit::Unspecified && UnitsRHS != EUnit::Unspecified)
      {
        UnitsLHS = InDefaultUnit;
      }
      else if (UnitsLHS != EUnit::Unspecified && UnitsRHS == EUnit::Unspecified)
      {
        UnitsRHS = InDefaultUnit;
      }

      if (CUnitConversion::AreUnitsCompatible(UnitsLHS, UnitsRHS))
      {
        if (UnitsLHS != EUnit::Unspecified)
        {
          return MakeValue(CNumericUnit<double>(A.Value - CUnitConversion::Convert(B.Value, UnitsRHS, UnitsLHS), UnitsLHS));
        }
        else
        {
          return MakeValue(CNumericUnit<double>(CUnitConversion::Convert(A.Value, UnitsLHS, UnitsRHS) - B.Value, UnitsRHS));
        }
      }

      CFormatOrderedArguments Args;
      Args.Add(CText::FromString(CUnitConversion::GetUnitDisplayString(B.Units)));
      Args.Add(CText::FromString(CUnitConversion::GetUnitDisplayString(A.Units)));
      return MakeError(CText::Format(LOCTEXT("CannotSubtractErr", "Cannot subtract {1} from {0}"), Args));
    });

    /** Multiplication */
    JumpTable.MapBinary<CStar>([](const CNumericUnit<double>& A, const CNumericUnit<double>& B) -> CExpressionResult {
      if (A.Units != EUnit::Unspecified && B.Units != EUnit::Unspecified)
      {
        return MakeError(LOCTEXT("InvalidMultiplication", "Cannot multiply by numbers with units"));
      }
      return MakeValue(CNumericUnit<double>(B.Value*A.Value, A.Units == EUnit::Unspecified ? B.Units : A.Units));
    });

    /** Division */
    JumpTable.MapBinary<CForwardSlash>([](const CNumericUnit<double>& A, const CNumericUnit<double>& B) -> CExpressionResult {
      if (B.Units != EUnit::Unspecified)
      {
        return MakeError(LOCTEXT("InvalidDivision", "Cannot divide by numbers with units"));
      }
      else if (B.Value == 0)
      {
        return MakeError(LOCTEXT("DivideByZero", "DivideByZero"));
      }
      return MakeValue(CNumericUnit<double>(A.Value/B.Value, A.Units));
    });
  }

  /** Consume a number from the stream, optionally including units */
  Optional<CExpressionError> ConsumeNumberWithUnits(CExpressionTokenConsumer& Consumer)
  {
    auto& Stream = Consumer.GetStream();

    if (!CCharTraits::IsDigit(Stream.PeekChar()))
    {
      return Optional<CExpressionError>();
    }

    Optional<CStringToken> NumberToken = ExpressionParser::ParseNumber(Stream);

    if (NumberToken.IsSet())
    {
      // Skip over whitespace
      Stream.ParseToken([](char InC){ return CCharTraits::IsWhitespace(InC) ? EParseState::Continue : EParseState::StopBefore; }, &NumberToken.GetValue());

      Optional<EUnit> Unit;
      Optional<CStringToken> UnitToken;
      for (int32 Index = 0; Index < countof(ParseCandidates); ++Index)
      {
        UnitToken = Stream.ParseTokenIgnoreCase(ParseCandidates[Index].String, &NumberToken.GetValue());
        if (UnitToken.IsSet())
        {
          Unit = ParseCandidates[Index].Unit;
          break;
        }
      }

      const double Value = CCharTraits::Atod(*NumberToken.GetValue().GetString());
      if (Unit.IsSet())
      {
        Consumer.Add(NumberToken.GetValue(), CNumericUnit<double>(Value, Unit.GetValue()));
      }
      else
      {
        Consumer.Add(NumberToken.GetValue(), CNumericUnit<double>(Value));
      }
    }

    return Optional<CExpressionError>();
  }

  TValueOrError<CNumericUnit<double>, CExpressionError> Evaluate(const char* InExpression) const
  {
    TValueOrError<CExpressionNode, CExpressionError> result = ExpressionParser::Evaluate(InExpression, TokenDefinitions, Grammar, JumpTable);
    if (!result.IsValid())
    {
      return MakeError(result.GetError());
    }

    auto& Node = result.GetValue();
    if (const auto* Numeric = Node.Cast<double>())
    {
      return MakeValue(CNumericUnit<double>(*Numeric, EUnit::Unspecified));
    }
    else if (const auto* NumericUnit = Node.Cast<CNumericUnit<double>>())
    {
      return MakeValue(*NumericUnit);
    }
    else
    {
      return MakeError(LOCTEXT("UnrecognizedResult", "Unrecognized result returned from expression"));
    }
  }

private:

  CTokenDefinitions TokenDefinitions;
  CExpressionGrammar Grammar;
  COperatorJumpTable JumpTable;
};


CUnitSettings::CUnitSettings()
  : bGlobalUnitDisplay(true)
{
  DisplayUnits[(uint8)EUnitType::Distance].Add(EUnit::Centimeters);
  DisplayUnits[(uint8)EUnitType::Angle].Add(EUnit::Degrees);
  DisplayUnits[(uint8)EUnitType::Speed].Add(EUnit::MetersPerSecond);
  DisplayUnits[(uint8)EUnitType::Temperature].Add(EUnit::Celsius);
  DisplayUnits[(uint8)EUnitType::Mass].Add(EUnit::Kilograms);
  DisplayUnits[(uint8)EUnitType::Force].Add(EUnit::Newtons);
  DisplayUnits[(uint8)EUnitType::Frequency].Add(EUnit::Hertz);
  DisplayUnits[(uint8)EUnitType::DataSize].Add(EUnit::Megabytes);
  DisplayUnits[(uint8)EUnitType::LuminousFlux].Add(EUnit::Lumens);
  DisplayUnits[(uint8)EUnitType::Time].Add(EUnit::Seconds);
}


bool CUnitSettings::ShouldDisplayUnits() const
{
  return bGlobalUnitDisplay;
}


void CUnitSettings::SetShouldDisplayUnits(bool bInGlobalUnitDisplay)
{
  bGlobalUnitDisplay = bInGlobalUnitDisplay;
  SettingChangedEvent.Broadcast();
}


const Array<EUnit>& CUnitSettings::GetDisplayUnits(EUnitType InType) const
{
  return DisplayUnits[(uint8)InType];
}


void CUnitSettings::SetDisplayUnits(EUnitType InType, EUnit Unit)
{
  if (InType != EUnitType::NumberOf)
  {
    DisplayUnits[(uint8)InType].Clear();
    if (CUnitConversion::IsUnitOfType(Unit, InType))
    {
      DisplayUnits[(uint8)InType].Add(Unit);
    }

    SettingChangedEvent.Broadcast();
  }
}


void CUnitSettings::SetDisplayUnits(EUnitType InType, const Array<EUnit>& Units)
{
  if (InType != EUnitType::NumberOf)
  {
    DisplayUnits[(uint8)InType].Reset();
    for (EUnit Unit : Units)
    {
      if (CUnitConversion::IsUnitOfType(Unit, InType))
      {
        DisplayUnits[(uint8)InType].Add(Unit);
      }
    }

    SettingChangedEvent.Broadcast();
  }
}


/** Get the global settings for unit conversion/display */
CUnitSettings& CUnitConversion::Settings()
{
  static UniquePtr<CUnitSettings> Settings(new CUnitSettings);
  return *Settings;
}


/** Check whether it is possible to convert a number between the two specified units */
bool CUnitConversion::AreUnitsCompatible(EUnit from, EUnit to)
{
  return from == EUnit::Unspecified || to == EUnit::Unspecified || GetUnitType(from) == GetUnitType(to);
}


/** Check whether a unit is of the specified type */
bool CUnitConversion::IsUnitOfType(EUnit Unit, EUnitType Type)
{
  return Unit != EUnit::Unspecified && GetUnitType(Unit) == Type;
}


/** Get the type of the specified unit */
EUnitType CUnitConversion::GetUnitType(EUnit InUnit)
{
  if (ENSURE(InUnit != EUnit::Unspecified))
  {
    return UnitTypes[(uint8)InUnit];
  }
  return EUnitType::NumberOf;
}


/** Get the unit abbreviation the specified unit type */
const char* CUnitConversion::GetUnitDisplayString(EUnit Unit)
{
  static_assert(countof(UnitTypes) == (uint8)EUnit::Unspecified, "Type array does not match size of unit enum");
  static_assert(countof(DisplayStrings) == (uint8)EUnit::Unspecified, "Display String array does not match size of unit enum");

  if (Unit != EUnit::Unspecified)
  {
    return DisplayStrings[(uint8)Unit];
  }
  return nullptr;
}


/** Helper function to find a unit from a string (name or abbreviation) */
Optional<EUnit> CUnitConversion::UnitFromString(const char* UnitString)
{
  if (!UnitString || *UnitString == '\0')
  {
    return Optional<EUnit>();
  }

  for (int32 Index = 0; Index < countof(ParseCandidates); ++Index)
  {
    if (CCharTraits::Stricmp(UnitString, ParseCandidates[Index].String) == 0)
    {
      return ParseCandidates[Index].Unit;
    }
  }

  return Optional<EUnit>();
}


namespace UnitConversion
{

  double DistanceUnificationFactor(EUnit from)
  {
    // Convert to meters
    double Factor = 1;
    switch (from) {
    case EUnit::Micrometers: return 0.000001;
    case EUnit::Millimeters: return 0.001;
    case EUnit::Centimeters: return 0.01;
    case EUnit::Kilometers: return 1000;

    case EUnit::Lightyears: return 9.4605284e15;

    case EUnit::Miles: Factor *= 1760; // fallthrough
    case EUnit::Yards: Factor *= 3; // fallthrough
    case EUnit::Feet: Factor *= 12; // fallthrough
    case EUnit::Inches: Factor /= 39.3700787; // fallthrough
    default: return Factor; // return
    }
  }

  double AngleUnificationFactor(EUnit from)
  {
    // Convert to degrees
    switch (from) {
    case EUnit::Radians: return (180 / PI);
    default: return 1;
    }
  }

  double SpeedUnificationFactor(EUnit from)
  {
    // Convert to km/h
    switch (from) {
    case EUnit::MetersPerSecond: return 3.6;
    case EUnit::MilesPerHour: return DistanceUnificationFactor(EUnit::Miles) / 1000;
    default: return 1;
    }
  }

  double MassUnificationFactor(EUnit from)
  {
    double Factor = 1;
    // Convert to grams
    switch (from) {
    case EUnit::Micrograms: return 0.000001;
    case EUnit::Milligrams: return 0.001;
    case EUnit::Kilograms: return 1000;
    case EUnit::MetricTons: return 1000000;

    case EUnit::Stones: Factor *= 14; // fallthrough
    case EUnit::Pounds: Factor *= 16; // fallthrough
    case EUnit::Ounces: Factor *= 28.3495; // fallthrough
    default:
      return Factor; // return
    }
  }

  double ForceUnificationFactor(EUnit from)
  {
    // Convert to Newtons
    switch (from) {
    case EUnit::PoundsForce: return 4.44822162;
    case EUnit::KilogramsForce: return 9.80665;
    default: return 1;
    }
  }

  double FrequencyUnificationFactor(EUnit from)
  {
    // Convert to KHz
    switch (from) {
    case EUnit::Hertz: return 0.001;
    case EUnit::Megahertz: return 1000;
    case EUnit::Gigahertz: return 1000000;

    case EUnit::RevolutionsPerMinute: return 0.001/60;

    default: return 1;
    }
  }

  double DataSizeUnificationFactor(EUnit from)
  {
    // Convert to MB
    switch (from) {
    case EUnit::Bytes: return 1.0/(1024*1024);
    case EUnit::Kilobytes: return 1.0/1024;
    case EUnit::Gigabytes: return 1024;
    case EUnit::Terabytes: return 1024*1024;

    default: return 1;
    }
  }

  double TimeUnificationFactor(EUnit from)
  {
    // Convert to hours
    double Factor = 1;
    switch (from) {
    case EUnit::Months: return (365.242 * 24) / 12;

    case EUnit::Years: Factor *= 365.242; // fallthrough
    case EUnit::Days: Factor *= 24; // fallthrough
      return Factor;

    case EUnit::Milliseconds: Factor /= 1000; // fallthrough
    case EUnit::Seconds: Factor /= 60; // fallthrough
    case EUnit::Minutes: Factor /= 60; // fallthrough
      return Factor;

    default: return 1;
    }
  }

  TValueOrError<CNumericUnit<double>, CText> TryParseExpression(const char* InExpression, EUnit from)
  {
    const CUnitExpressionParser Parser(from);
    auto result = Parser.Evaluate(InExpression);
    if (result.IsValid())
    {
      if (result.GetValue().Units == EUnit::Unspecified)
      {
        return MakeValue(CNumericUnit<double>(result.GetValue().Value, from));
      }
      else
      {
        return MakeValue(result.GetValue());
      }
    }
    else
    {
      return MakeError(result.GetError().Text);
    }
  }

  Optional<const Array<CQuantizationInfo>*> GetQuantizationBounds(EUnit Unit)
  {
    struct CStaticBounds
    {
      Array<CQuantizationInfo> MetricDistance;
      Array<CQuantizationInfo> ImperialDistance;

      Array<CQuantizationInfo> MetricMass;
      Array<CQuantizationInfo> ImperialMass;

      Array<CQuantizationInfo> Frequency;
      Array<CQuantizationInfo> DataSize;

      Array<CQuantizationInfo> Time;

      CStaticBounds()
      {
        MetricDistance.Emplace(EUnit::Micrometers, 1000);
        MetricDistance.Emplace(EUnit::Millimeters, 10);
        MetricDistance.Emplace(EUnit::Centimeters, 100);
        MetricDistance.Emplace(EUnit::Meters, 1000);
        MetricDistance.Emplace(EUnit::Kilometers, 0);

        ImperialDistance.Emplace(EUnit::Inches, 12);
        ImperialDistance.Emplace(EUnit::Feet, 3);
        ImperialDistance.Emplace(EUnit::Yards, 1760);
        ImperialDistance.Emplace(EUnit::Miles, 0);

        MetricMass.Emplace(EUnit::Micrograms, 1000);
        MetricMass.Emplace(EUnit::Milligrams, 1000);
        MetricMass.Emplace(EUnit::Grams, 1000);
        MetricMass.Emplace(EUnit::Kilograms, 1000);
        MetricMass.Emplace(EUnit::MetricTons, 0);

        ImperialMass.Emplace(EUnit::Ounces, 16);
        ImperialMass.Emplace(EUnit::Pounds, 14);
        ImperialMass.Emplace(EUnit::Stones, 0);

        Frequency.Emplace(EUnit::Hertz, 1000);
        Frequency.Emplace(EUnit::Kilohertz, 1000);
        Frequency.Emplace(EUnit::Megahertz, 1000);
        Frequency.Emplace(EUnit::Gigahertz, 0);

        DataSize.Emplace(EUnit::Bytes, 1000);
        DataSize.Emplace(EUnit::Kilobytes, 1000);
        DataSize.Emplace(EUnit::Megabytes, 1000);
        DataSize.Emplace(EUnit::Gigabytes, 1000);
        DataSize.Emplace(EUnit::Terabytes, 0);

        Time.Emplace(EUnit::Milliseconds, 1000);
        Time.Emplace(EUnit::Seconds, 60);
        Time.Emplace(EUnit::Minutes, 60);
        Time.Emplace(EUnit::Hours, 24);
        Time.Emplace(EUnit::Days, 365.242f / 12);
        Time.Emplace(EUnit::Months, 12);
        Time.Emplace(EUnit::Years, 0);
      }
    };

    static CStaticBounds Bounds;

    switch (Unit) {
    case EUnit::Micrometers: case EUnit::Millimeters: case EUnit::Centimeters: case EUnit::Meters: case EUnit::Kilometers:
      return &Bounds.MetricDistance;

    case EUnit::Inches: case EUnit::Feet: case EUnit::Yards: case EUnit::Miles:
      return &Bounds.ImperialDistance;

    case EUnit::Micrograms: case EUnit::Milligrams: case EUnit::Grams: case EUnit::Kilograms: case EUnit::MetricTons:
      return &Bounds.MetricMass;

    case EUnit::Ounces: case EUnit::Pounds: case EUnit::Stones:
      return &Bounds.ImperialMass;

    case EUnit::Hertz: case EUnit::Kilohertz: case EUnit::Megahertz: case EUnit::Gigahertz: case EUnit::RevolutionsPerMinute:
      return &Bounds.Frequency;

    case EUnit::Bytes: case EUnit::Kilobytes: case EUnit::Megabytes: case EUnit::Gigabytes: case EUnit::Terabytes:
      return &Bounds.DataSize;

    case EUnit::Milliseconds: case EUnit::Seconds: case EUnit::Minutes: case EUnit::Hours: case EUnit::Days: case EUnit::Months: case EUnit::Years:
      return &Bounds.Time;

    default:
      return Optional<const Array<CQuantizationInfo>*>();
    }
  }

} // namespace UnitConversion

#undef LOCTEXT_NAMESPACE

} // namespace fun
