%{
#include "minijava/parser_context.h"
#include "minijava/parser.hh"

mjc::location yyloc;

// Code run each time a pattern is matched.
# define YY_USER_ACTION  yyloc.columns (yyleng);
%}

%option noyywrap

id    [a-zA-Z][a-zA-Z_0-9]*
num   [0-9]+
blank [ \t\r]

%x INCOMMENT

%%

%{
  // Code run each time yylex is called.
  yyloc.step ();
%}

{blank}+               yyloc.step ();
[\n]+                  yyloc.lines(yyleng); yyloc.step ();
"{"                    return mjc::parser::make_LBRACE(yyloc);
"}"                    return mjc::parser::make_RBRACE(yyloc);
"["                    return mjc::parser::make_LBRACK(yyloc);
"]"                    return mjc::parser::make_RBRACK(yyloc);
"("                    return mjc::parser::make_LPAR(yyloc);
")"                    return mjc::parser::make_RPAR(yyloc);
"int"                  return mjc::parser::make_INT(yyloc);
"boolean"              return mjc::parser::make_BOOL(yyloc);
"if"                   return mjc::parser::make_IF(yyloc);
"else"                 return mjc::parser::make_ELSE(yyloc);
"while"                return mjc::parser::make_WHILE(yyloc);
"throws"               return mjc::parser::make_THROWS(yyloc);
"java.io.IOException"  return mjc::parser::make_IOEXCEPTION(yyloc);
"System.out.println"   return mjc::parser::make_PRINT(yyloc);
"System.out.write"     return mjc::parser::make_WRITE(yyloc);
"System.in.read"       return mjc::parser::make_READ(yyloc);
"return"               return mjc::parser::make_RETURN(yyloc);
"&&"                   return mjc::parser::make_OPCAND(yyloc);
"<"                    return mjc::parser::make_OPCLT(yyloc);
"+"                    return mjc::parser::make_OPCPLUS(yyloc);
"-"                    return mjc::parser::make_OPCMINUS(yyloc);
"*"                    return mjc::parser::make_OPCTIMES(yyloc);
"/"                    return mjc::parser::make_OPCDIV(yyloc);
"static"               return mjc::parser::make_STATIC(yyloc);
"void"                 return mjc::parser::make_VOID(yyloc);
"main"                 return mjc::parser::make_MAIN(yyloc);
"String"               return mjc::parser::make_KSTRING(yyloc);
"length"               return mjc::parser::make_LEN(yyloc);
"new"                  return mjc::parser::make_NEW(yyloc);
"true"                 return mjc::parser::make_KTRUE(yyloc);
"false"                return mjc::parser::make_KFALSE(yyloc);
"this"                 return mjc::parser::make_THIS(yyloc);
"public"               return mjc::parser::make_PUBLIC(yyloc);
"class"                return mjc::parser::make_CLASS(yyloc);
"extends"              return mjc::parser::make_EXTENDS(yyloc);
"="                    return mjc::parser::make_EQUALS(yyloc);
"."                    return mjc::parser::make_DOT(yyloc);
","                    return mjc::parser::make_COMMA(yyloc);
";"                    return mjc::parser::make_SEMICOL(yyloc);
"!"                    return mjc::parser::make_EXCL(yyloc);
{id}                   return mjc::parser::make_IDENTIFIER(yytext, yyloc);
{num}                  return mjc::parser::make_NUMBER(std::stoi(yytext, 0, 10), yyloc);
"//".*                 yyloc.step ();
"/*"                   BEGIN(INCOMMENT);
.                      parser_context.SetParseError( yyloc, "Invalid character.");
<<EOF>>                return mjc::parser::make_END(yyloc);

<INCOMMENT>[\n]        yyloc.lines(yyleng); yyloc.step ();
<INCOMMENT>[^*\n]*     /* eat anything that's not a '*' */
<INCOMMENT>"*"+[^*/]*  /* eat up '*'s not followed by '/'s */
<INCOMMENT>"*"+"/"     BEGIN(INITIAL);

%%
