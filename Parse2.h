/* Roman Stratiienko 2016
   picoworm@gmail.com */

#ifndef T3_PARSE2_H
#define T3_PARSE2_H

#include "../../Common/RConsts.h"
#include "../../Common/Signals/RString.h"
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <functional>
#include <vector>
//#include "Interpreter.h"

using namespace std;

class Range {
public:
    Range() { }

    Range(const char *String) {
        string = String;
        length = (uint32_t) strlen(String);
    }

    void copyTo(char *Dest) {
        memcpy(Dest, string, length);
        Dest[length] = 0;
    }

    void copyTo(RString &Dest) {
        Dest.copyFromBuffer((void *) string, length);
    }

    bool equals(const char *String) {
        size_t L = strlen(String);
        if (L != length) return false;
        return memcmp(String, string, L) == 0;
    }

    const char *string;
    uint32_t length;
    uint32_t pos = 0;
    int ErrorCode = 0;
    const char *ErrorMessage = 0;

//    enum StringChars {
//        SCQuote, SCOpenningBracket, SCClosingBracket, SCComma,
//        SCEquals, SCSQOpenningBracket, SCSQClosingBracket
//    };
//    char StringCh[7] = {'\"', '(', ')', ',', '=', '[', ']'};

    int blankSymbolsQnt() {
        int count = pos;
        while ((count < length) && ((string[count] == ' ') || (string[count] == '\t'))) count++;
        return count - pos;
    }

    bool NextToken(const char *s) {
        skip(blankSymbolsQnt());
        uint32_t L = (int32_t) strlen(s);
        if (((pos + L) <= length) && (memcmp(string + pos, s, L) == 0)) {
            pos += L;
            return true;
        }
        return false;
    }

    void NextTokenMustBe(const char *s) {
        if (!NextToken(s)) {
            char buff[256];
            snprintf(buff, 256, "Token not found: %s", s);
            Throw(buff);
        }
    }

    bool isNextCharIdentElement() {
        if (atEnd()) return false;
        char c = string[pos];
        if (c >= 'a' && c <= 'z') return true;
        if (c >= 'A' && c <= 'Z') return true;
        if (c >= '0' && c <= '9') return true;
        if (c == '_') return true;
        return false;
    }

    bool NextIdent(const char *s) {
        skip(blankSymbolsQnt());
        uint32_t StartPos = pos;
        if (!NextToken(s)) return false;
        if (!isNextCharIdentElement()) return true;
        pos = StartPos;
        return false;
    }

    void NextChar(char ch) {
        skip(blankSymbolsQnt());
        if ((length <= pos) || (string[pos] != ch)) {
            char buff[256];
            snprintf(buff, 256, "Ожидаемый символ не обнаружен: '%c'", ch);
            Throw(buff);
        }
        pos++;
    }

    bool isNextChar(char ch) {
        skip(blankSymbolsQnt());
        if (pos >= length) return false;
        return (string[pos] == ch);
    }

    bool NextCharAction(char ch) {
        bool b = isNextChar(ch);
        if (b) pos++;
        return b;
    }

    void skip(int HowMany) {
        pos += HowMany;
        if (pos > length) Throw("Skip out of buffer");
    }

    void SkipExpression() {
        while (pos < length) {
            char ch = string[pos];
            if (NextCharAction('(')) {
                SkipExpression();
                NextChar(')');
            } else if (NextCharAction('[')) {
                SkipExpression();
                NextChar(']');
            } else if (ch == ',' || ch == ')' || ch == ']' || ch == 0 || ch == '\n') return;
            else pos++;
        }
    }

    bool atEnd() {
        return pos >= length;
    }

    double NextDouble() {
        char *endptr;
        double value = strtod(string + pos, &endptr);
        if (endptr == (string + pos)) Throw("Ожидается вещественное число");
        pos = (uint32_t) (endptr - string);
        return value;
    }

    int32_t NextInt() {
        char *endptr;
        int32_t value = (int32_t) strtol(string + pos, &endptr, 10);
        if (endptr == (string + pos)) Throw("Ожидается целое число");

        pos = (uint32_t) (endptr - string);
        return value;
    }

    void Throw(const char *_ErrorMessage) {
        ErrorMessage = _ErrorMessage;
        throw *this;
    }

    void Throw(int32_t _Code) {
        ErrorCode = _Code;
        throw *this;
    }
};


class Interpreter_c;

class m_function {
public:
    m_function(const char *Name, const function<double(double)> &F) : F(F), Name(Name) { }

    const char *Name;
    function<double(double arg)> F;
};

class m_function_2arg {
public:
    m_function_2arg(const char *Name, const function<double(double, double)> &F) : F(F), Name(Name) { }

    const char *Name;
    function<double(double,double)> F;
};

class Parse2 {
public:
    static bool init();

    static void findStringInQuotes(Range &Original, Range &Var);

    static void findString(Range &Original, Range &Var, char Ending);

    static void findAxisName(Range &Original, Range &Var);

    static bool isNextIAC(Range &Original, Range &AxisName);

    //static void findVarName(Range &Original, Range &Var);

    static void findExpression(Range &Original, Range &Expr);

    static double Evaluate(Range &Outer, Interpreter_c &Int);

    static double EvalActionElement(Range &Expr, Interpreter_c &Int, uint8_t MinActionPriority);

    static vector<m_function> FuncList;
    static vector<m_function_2arg> Func2ArgsList;
};

void testExpression();


#endif //T3_PARSE2_H
