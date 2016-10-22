/* Roman Stratiienko 2016
   picoworm@gmail.com */

#include "Parse2.h"
#include "Interpreter.h"

class EvalAction {
public:
    const char *Name;
    uint8_t Priority;
    function<double(double &, double &)> Evaluate;
};

static vector<EvalAction> ActionList;

double Parse2::EvalActionElement(Range &Expr, Interpreter_c &Int, uint8_t MinActionPriority) {
    double value = 0;
    Expr.skip(Expr.blankSymbolsQnt());

    if (Expr.atEnd()) Expr.Throw("Ожидается выражение");

    char c = Expr.string[Expr.pos];

    if (Expr.NextCharAction('$')) {
        //Range ValueRng;
        //findVarName(Expr, ValueRng);
        Int.AccessValue(Expr, value, R::Read);
    } else if (c >= '0' && c <= '9') {
        value = Expr.NextDouble();
    } else if (Expr.NextCharAction('(')) {
        value = EvalActionElement(Expr, Int, 0);
        Expr.NextChar(')');
    } else if (c == '-' || c == '+') {
        value = 0; // skip value, works as 0- or 0+
    } else {
        bool FuncFound = false;
        for (m_function &MF : FuncList) {
            if (Expr.NextIdent(MF.Name)) {
                Expr.NextChar('(');
                value = MF.F(EvalActionElement(Expr, Int, 0));
                Expr.NextChar(')');
                FuncFound = true;
                break;
            }
        }
        if (!FuncFound)
            for (m_function_2arg &MF : Func2ArgsList) {
                if (Expr.NextIdent(MF.Name)) {
                    Expr.NextChar('(');
                    double arg1 = EvalActionElement(Expr, Int, 0);
                    Expr.NextChar(',');
                    double arg2 = EvalActionElement(Expr, Int, 0);
                    Expr.NextChar(')');
                    value = MF.F(arg1, arg2);
                    FuncFound = true;
                    break;
                }
            }

        if (!FuncFound) Expr.Throw("Неизвестный элемент выражения");
    }

    while (1) {
        uint32_t PrevPos = Expr.pos;
        EvalAction *EA = nullptr;
        for (EvalAction &_EA : ActionList) {
            if (Expr.NextToken(_EA.Name)) {
                EA = &_EA;
                break;
            }
        }

        if (EA == nullptr) {
            // Unknown Action interpret as end of expression
            return value;
        }

        if (EA->Priority <= MinActionPriority) {
            Expr.pos = PrevPos;
            return value;
        } else {
            double NextValue = EvalActionElement(Expr, Int, EA->Priority);
            value = EA->Evaluate(value, NextValue);
        }
    }
}

double Parse2::Evaluate(Range &Outer, Interpreter_c &Int) {
    return EvalActionElement(Outer, Int, 0);
}

void Parse2::findExpression(Range &Original, Range &Expr) {
    Original.skip(Original.blankSymbolsQnt());
    Expr.pos = 0;
    int StartPos = Original.pos;
    Original.SkipExpression();
    Expr.string = StartPos + Original.string;
    Expr.length = Original.pos - StartPos;
    int End = Original.pos;
}

//void Parse2::findVarName(Range &Original, Range &Var) {
//    Original.skip(Original.blankSymbolsQnt());
//    Var.pos = 0;
//    int StartPos = Original.pos;
//
//    for (; Original.pos < Original.length; Original.pos++) {
//        char c = Original.string[Original.pos];
//        bool ok = false;
//        if (c >= '0' && c <= '9') ok = true; else if (c >= 'a' && c <= 'z') ok = true; else if (c >= 'A' &&
//                                                                                                c <= 'Z')
//            ok = true;
//        else if (c == '_' || c == '[' || c == ']' || c == '.') ok = true;
//
//        if (!ok) break;
//    }
//
//    Var.string = StartPos + Original.string;
//    Var.length = Original.pos - StartPos;
//}

void Parse2::findAxisName(Range &Original, Range &Var) {
    Original.skip(Original.blankSymbolsQnt());
    Var.pos = 0;
    int StartPos = Original.pos;

    for (; Original.pos < Original.length; Original.pos++) {
        char c = Original.string[Original.pos];
        bool ok = false;
        if (c >= '0' && c <= '9') ok = true; else if (c >= 'a' && c <= 'z') ok = true; else if (c >= 'A' &&
                                                                                                c <= 'Z')
            ok = true;
        else if (c == '_') ok = true;

        if (!ok) break;
    }

    Var.string = StartPos + Original.string;
    Var.length = Original.pos - StartPos;
}

void Parse2::findString(Range &Original, Range &Var, char Ending) {
    int StartPos = Original.pos;
    for (; Original.pos < Original.length; Original.pos++) {
        if (Original.isNextChar(Ending)) {
            Var.string = StartPos + Original.string;
            Var.length = Original.pos - StartPos;
            return;
        }
    }
    Original.Throw("Закрывающий символ не обнаружен");
}

void Parse2::findStringInQuotes(Range &Original, Range &Var) {
    Original.skip(Original.blankSymbolsQnt());
    Original.NextChar('\"');
    uint32_t StartPos = Original.pos;
    for (; Original.pos < Original.length; Original.pos++) {
        if (Original.NextCharAction('\"')) {
            Var.string = StartPos + Original.string;
            Var.length = Original.pos - StartPos - 1;
            return;
        }
    }
    Original.Throw("Закрывающая \" не обнаружена");
}

bool Parse2::isNextIAC(Range &Original, Range &AxisName) {
    AxisName.string = &Original.string[Original.pos];
    AxisName.pos = 0;

    bool FirstCharFound = false;
    for (uint32_t i = Original.pos; i < Original.length; i++) {
        char c = Original.string[i];
        if (c == ':') {
            if (!FirstCharFound) return false;
            AxisName.length = i - Original.pos;
            Original.pos = i + 1;
            return true;
        } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) FirstCharFound = true;
        else return false;
    }
    return false;
}

vector<m_function> Parse2::FuncList;
vector<m_function_2arg> Parse2::Func2ArgsList;

bool Parse2::init() {
    FuncList.push_back({"SIN", [](double arg) -> double { return sin(arg); }});
    FuncList.push_back({"COS", [](double arg) -> double { return cos(arg); }});
    FuncList.push_back({"ASIN", [](double arg) -> double { return asin(arg); }});
    FuncList.push_back({"ACOS", [](double arg) -> double { return acos(arg); }});
    FuncList.push_back({"TAN", [](double arg) -> double { return tan(arg); }});
    FuncList.push_back({"ATAN", [](double arg) -> double { return atan(arg); }});
    FuncList.push_back({"SQRT", [](double arg) -> double { return sqrt(arg); }});
    FuncList.push_back({"ABS", [](double arg) -> double { return fabs(arg); }});
    FuncList.push_back({"ANG", [](double arg) -> double {
        double a = arg - floor(arg);
        if (a > 0.5) a -= 1.0;
        return a;
    }});
    FuncList.push_back({"ROUND", [](double arg) -> double { return round(arg); }});
    FuncList.push_back({"FLOOR", [](double arg) -> double { return floor(arg); }});

    Func2ArgsList.push_back({"ATAN2", [](double arg1, double arg2) -> double { return atan2(arg1, arg2); }});

    ActionList.push_back({"*", 40, [](double &arg1, double &arg2) -> double { return arg1 * arg2; }});
    ActionList.push_back({"/", 40, [](double &arg1, double &arg2) -> double { return arg1 / arg2; }});
    ActionList.push_back({"+", 30, [](double &arg1, double &arg2) -> double { return arg1 + arg2; }});
    ActionList.push_back({"-", 30, [](double &arg1, double &arg2) -> double { return arg1 - arg2; }});
    ActionList.push_back({">=", 20, [](double &arg1, double &arg2) -> double { return arg1 >= arg2 ? 1 : 0; }});
    ActionList.push_back({"<=", 20, [](double &arg1, double &arg2) -> double { return arg1 <= arg2 ? 1 : 0; }});
    ActionList.push_back({">", 20, [](double &arg1, double &arg2) -> double { return arg1 > arg2 ? 1 : 0; }});
    ActionList.push_back({"<", 20, [](double &arg1, double &arg2) -> double { return arg1 < arg2 ? 1 : 0; }});
    ActionList.push_back({"==", 20, [](double &arg1, double &arg2) -> double { return arg1 == arg2 ? 1 : 0; }});
    ActionList.push_back({"!=", 20, [](double &arg1, double &arg2) -> double { return arg1 != arg2 ? 1 : 0; }});
    ActionList.push_back({"OR", 10, [](double &arg1, double &arg2) -> double { return arg1 || arg2 ? 1 : 0; }});
    ActionList.push_back({"AND", 10, [](double &arg1, double &arg2) -> double { return arg1 && arg2 ? 1 : 0; }});

    return true;
}

bool callInit = Parse2::init();

//#define TEST_EXPRESSION
#ifdef TEST_EXPRESSION

bool test() {
    try {
        Interpreter_c Int;
        assert(Parse2::Evaluate(*new Range{"1+2+3+4+5+6+7+8+9+10"}, Int) == 55.0);
        assert(Parse2::Evaluate(*new Range{"2+2*2"}, Int) == 6.0);
        assert(Parse2::Evaluate(*new Range{"(2+2)*2"}, Int) == 8.0);
        assert(Parse2::Evaluate(*new Range{"5*5/5"}, Int) == 5.0);
        assert(Parse2::Evaluate(*new Range{"ABS(-125)"}, Int) == 125.0);
        assert(Parse2::Evaluate(*new Range{"SQRT(16)"}, Int) == 4.0);
        assert(Parse2::Evaluate(*new Range{"SIN(3.14159265358979/2)"}, Int) == 1.0);
        assert(Parse2::Evaluate(*new Range{"5+4==8"}, Int) == 0.0);
        assert(Parse2::Evaluate(*new Range{"1 OR 2"}, Int) == 1.0);
        assert(Parse2::Evaluate(*new Range{"1 AND 0"}, Int) == 0.0);
        assert(Parse2::Evaluate(*new Range{"5-5-5"}, Int) == -5.0);
        assert(Parse2::Evaluate(*new Range{"5-(5-5)"}, Int) == 5.0);
        assert(Parse2::Evaluate(*new Range{"6*12*12*(5-4/(55-4/44+5))"}, Int) ==
               (6.0 * 12 * 12 * (5.0 - 4.0 / (55.0 - 4.0 / 44.0 + 5.0))));

        assert(Parse2::Evaluate(*new Range{"5<=5"}, Int) == 1.0);
        assert(Parse2::Evaluate(*new Range{"5<5"}, Int) == 0.0);
        assert(Parse2::Evaluate(*new Range{"3.14159265358979>3.14"}, Int) == 1.0);
        assert(Parse2::Evaluate(*new Range{"6!=4"}, Int) == 1.0);


        assert(Parse2::Evaluate(*new Range{"ATAN2(10,10)"}, Int) == atan2(10,10));

        cout << "Parse test done" << endl;
        exit(0);
    } catch (Range &R) {
        cout << R.string
        << endl;
        cout << R.ErrorMessage << endl;
        exit(-1);
    }
};

bool B = test();

#endif



