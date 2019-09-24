//DEPRECATED
#include "CorePrivatePCH.h"
#include "Math/BasicMathExpressionEvaluator.h"

#define LOCTEXT_NAMESPACE  "BasicMathExpressionEvaluator"

namespace fun {

namespace ExpressionParser
{
  const char* const CSubExpressionStart::Moniker = TEXT("(");
  const char* const CSubExpressionEnd::Moniker = TEXT(")");
  const char* const CPlus::Moniker = TEXT("+");
  const char* const CMinus::Moniker = TEXT("-");
  const char* const CStar::Moniker = TEXT("*");
  const char* const CForwardSlash::Moniker = TEXT("/");
  const char* const CPercent::Moniker = TEXT("%");
  const char* const CSquareRoot::Moniker = TEXT("sqrt");
  const char* const CPower::Moniker = TEXT("^");

  Optional<CStringToken> ParseNumber(const CTokenStream& InStream, CStringToken* Accumulate)
  {
    enum class EState { LeadIn, Sign, Integer, Dot, Fractional };

    EState State = EState::LeadIn;
    return InStream.ParseToken([&](char InC){
      if (State == EState::LeadIn)
      {
        if (CCharTraits::IsDigit(InC))
        {
          State = EState::Integer;
          return EParseState::Continue;
        }
        else if (InC == '+' || InC == '-')
        {
          State = EState::Sign;
          return EParseState::Continue;
        }
        else if (InC == '.')
        {
          State = EState::Dot;
          return EParseState::Continue;
        }
        else
        {
          // Not a number
          return EParseState::Cancel;
        }
      }
      else if (State == EState::Sign)
      {
        if (CCharTraits::IsDigit(InC))
        {
          State = EState::Integer;
          return EParseState::Continue;
        }
        else if (InC == '.')
        {
          State = EState::Dot;
          return EParseState::Continue;
        }
        else
        {
          // Not a number
          return EParseState::Cancel;
        }
      }
      else if (State == EState::Integer)
      {
        if (CCharTraits::IsDigit(InC))
        {
          return EParseState::Continue;
        }
        else if (InC == '.')
        {
          State = EState::Dot;
          return EParseState::Continue;
        }
      }
      else if (State == EState::Dot)
      {
        if (CCharTraits::IsDigit(InC))
        {
          State = EState::Fractional;
          return EParseState::Continue;
        }
      }
      else if (State == EState::Fractional)
      {
        if (CCharTraits::IsDigit(InC))
        {
          return EParseState::Continue;
        }
      }

      return EParseState::StopBefore;
    }, Accumulate);
  }

  Optional<CExpressionError> ConsumeNumber(CExpressionTokenConsumer& Consumer)
  {
    auto& Stream = Consumer.GetStream();

    Optional<CStringToken> Token = ParseNumber(Stream);

    if (Token.IsSet())
    {
      double Value = CCharTraits::Atod(*Token.GetValue().GetString());
      Consumer.Add(Token.GetValue(), CExpressionNode(Value));
    }

    return Optional<CExpressionError>();
  }
}


BasicMathExpressionEvaluator::BasicMathExpressionEvaluator()
{
  using namespace ExpressionParser;

  TokenDefinitions.IgnoreWhitespace();
  TokenDefinitions.DefineToken(&ConsumeSymbol<CSubExpressionStart>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CSubExpressionEnd>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CPlus>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CMinus>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CStar>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CForwardSlash>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CPercent>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CSquareRoot>);
  TokenDefinitions.DefineToken(&ConsumeSymbol<CPower>);
  TokenDefinitions.DefineToken(&ConsumeNumber);

  Grammar.DefineGrouping<CSubExpressionStart, CSubExpressionEnd>();
  Grammar.DefinePreUnaryOperator<CPlus>();
  Grammar.DefinePreUnaryOperator<CMinus>();
  Grammar.DefinePreUnaryOperator<CSquareRoot>();
  Grammar.DefineBinaryOperator<CPlus>(5);
  Grammar.DefineBinaryOperator<CMinus>(5);
  Grammar.DefineBinaryOperator<CStar>(4);
  Grammar.DefineBinaryOperator<CForwardSlash>(4);
  Grammar.DefineBinaryOperator<CPercent>(4);
  Grammar.DefineBinaryOperator<CPower>(4);;

  JumpTable.MapPreUnary<CPlus>([](double N) { return N; });
  JumpTable.MapPreUnary<CMinus>([](double N) { return -N; });
  JumpTable.MapPreUnary<CSquareRoot>([](double A) { return double(Math::Sqrt(A)); });

  JumpTable.MapBinary<CPlus>([](double A, double B) { return A + B; });
  JumpTable.MapBinary<CMinus>([](double A, double B) { return A - B; });
  JumpTable.MapBinary<CStar>([](double A, double B) { return A * B; });
  JumpTable.MapBinary<CPower>([](double A, double B) { return double(Math::Pow(A, B)); });

  JumpTable.MapBinary<CForwardSlash>([](double A, double B) -> CExpressionResult {
    if (B == 0)
    {
      //TODO?
      //return MakeError(LOCTEXT("DivisionByZero", "Division by zero"));
    }

    return MakeValue(A / B);
  });

  JumpTable.MapBinary<CPercent>([](double A, double B) -> CExpressionResult {
    if (B == 0)
    {
      //TODO?
      //return MakeError(LOCTEXT("ModZero", "Modulo zero"));
    }

    return MakeValue(double(Math::Fmod(A, B)));
  });
}


TValueOrError<double, CExpressionError> BasicMathExpressionEvaluator::Evaluate(const char* InExpression) const
{
  TValueOrError<CExpressionNode, CExpressionError> result = ExpressionParser::Evaluate(InExpression, TokenDefinitions, Grammar, JumpTable);
  if (!result.IsValid())
  {
    return MakeError(result.GetError());
  }

  auto& Node = result.GetValue();

  if (const auto* Numeric = Node.Cast<double>())
  {
    return MakeValue(*Numeric);
  }
  else
  {
    //TODO?
    //return MakeError(LOCTEXT("UnrecognizedResult", "Unrecognized result returned from expression"));
  }
}


//TODO?
//#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
//
//bool TestExpression(CAutomationTestBase* Test, const char* Expression, double Expected)
//{
//  BasicMathExpressionEvaluator Parser;
//
//  TValueOrError<double, CExpressionError> result = Parser.Evaluate(Expression);
//  if (!result.IsValid())
//  {
//    Test->AddError(result.GetError().Text.ToString());
//    return false;
//  }
//  else if (result.GetValue() != Expected)
//  {
//    Test->AddError(string::Printf(TEXT("'%s' evaluation results: %.f != %.f"), Expression, result.GetValue(), Expected));
//    return false;
//  }
//  return true;
//}
//
//
//bool TestInvalidExpression(CAutomationTestBase* Test, const char* Expression)
//{
//  BasicMathExpressionEvaluator Parser;
//
//  TValueOrError<double, CExpressionError> result = Parser.Evaluate(Expression);
//  if (result.IsValid())
//  {
//    return false;
//  }
//
//  return true;
//}
//
//
//// Evaluates valid math expressions.
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(CBasicMathExpressionEvaluatorTest, "System.Core.Math.Evaluate.Valid Expressions", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
//bool CBasicMathExpressionEvaluatorTest::RunTest(const String& Parameters)
//{
//  TestTrue(TEXT("Valid expression, '+2', evaluated incorrectly."), TestExpression(this, TEXT("+1"), 1));
//  TestTrue(TEXT("Valid expression, '-20', evaluated incorrectly."), TestExpression(this, TEXT("-20"), -20));
//  TestTrue(TEXT("Valid expression, '-+-2', evaluated incorrectly."), TestExpression(this, TEXT("-+-2"), 2));
//  TestTrue(TEXT("Valid expression, '1 + 2', evaluated incorrectly."), TestExpression(this, TEXT("1 + 2"), 3));
//  TestTrue(TEXT("Valid expression, '1+2*3', evaluated incorrectly."), TestExpression(this, TEXT("1+2*3"), 7));
//  TestTrue(TEXT("Valid expression, '1+2*3*4+1', evaluated incorrectly."), TestExpression(this, TEXT("1+2*3*4+1"), 1 + 2 * 3 * 4 + 1));
//  TestTrue(TEXT("Valid expression, '1*2+3', evaluated incorrectly."), TestExpression(this, TEXT("1*2+3"), 1 * 2 + 3));
//  TestTrue(TEXT("Valid expression, '1+2*3*4+1', evaluated incorrectly."), TestExpression(this, TEXT("1+2*3*4+1"), 1 + 2 * 3 * 4 + 1));
//
//  TestTrue(TEXT("Valid expression, '2^2', evaluated incorrectly."), TestExpression(this, TEXT("2^2"), 4));
//  TestTrue(TEXT("Valid expression, 'sqrt(4)', evaluated incorrectly."), TestExpression(this, TEXT("sqrt(4)"), 2));
//  TestTrue(TEXT("Valid expression, '4*sqrt(4)+10', evaluated incorrectly."), TestExpression(this, TEXT("4*sqrt(4)+10"), 18));
//  TestTrue(TEXT("Valid expression, '8%6', evaluated incorrectly."), TestExpression(this, TEXT("8%6"), 2));
//
//  return true;
//}
//
//
//// Evaluates a valid math expression with leading and trailing white spaces.
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(CBasicMathExpressionEvaluatorWhitespaceExpressionsTest, "System.Core.Math.Evaluate.Valid Expressions With Whitespaces", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
//bool CBasicMathExpressionEvaluatorWhitespaceExpressionsTest::RunTest(const String& Parameters)
//{
//  TestTrue(TEXT("Expression with leading and trailing whitespaces was not evaluated correctly."), TestExpression(this, TEXT(" 1+2 "), 1 + 2));
//
//  return true;
//}
//
//
//// Evaluates valid math expressions that are grouped
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(CBasicMathExpressionEvaluatorGroupedExpressionsTest, "System.Core.Math.Evaluate.Valid Grouped Expressions", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
//bool CBasicMathExpressionEvaluatorGroupedExpressionsTest::RunTest(const String& Parameters)
//{
//  TestTrue(TEXT("Valid grouped expression, '(1+2)*3*4+1', evaluated incorrectly."), TestExpression(this, TEXT("(1+2)*3*4+1"), (1 + 2) * 3 * 4 + 1));
//  TestTrue(TEXT("Valid grouped expression, '(1+2)*3*(4+1)', evaluated incorrectly."), TestExpression(this, TEXT("(1+2)*3*(4+1)"), (1 + 2) * 3 * (4 + 1)));
//  TestTrue(TEXT("Valid grouped expression, '((1+2) / (3+1) + 2) * 3', evaluated incorrectly."), TestExpression(this, TEXT("((1+2) / (3+1) + 2) * 3"), ((1.0 + 2) / (3 + 1) + 2) * 3));
//
//  return true;
//}
//
//
//// Evaluates invalid expressions.
//// Invalid expressions will report errors and not crash.
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(CBasicMathExpressionEvaluatorInvalidExpressionTest, "System.Core.Math.Evaluate.Invalid Expressions", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
//bool CBasicMathExpressionEvaluatorInvalidExpressionTest::RunTest(const String& Parameters)
//{
//  TestTrue(TEXT("The invalid math expression, 'gobbledegook', did not report an error."), TestInvalidExpression(this, TEXT("gobbledegook")));
//  TestTrue(TEXT("The invalid math expression, '50**10', did not report an error."), TestInvalidExpression(this, TEXT("50**10")));
//  TestTrue(TEXT("The invalid math expression, '*1', did not report an error."), TestInvalidExpression(this, TEXT("*1")));
//  TestTrue(TEXT("The invalid math expression, '+', did not report an error."), TestInvalidExpression(this, TEXT("+")));
//  TestTrue(TEXT("The invalid math expression, '{+}', did not report an error."), TestInvalidExpression(this, TEXT("+")));
//
//  return true;
//}
//
//#endif //!(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)

#undef LOCTEXT_NAMESPACE

} // namespace fun
