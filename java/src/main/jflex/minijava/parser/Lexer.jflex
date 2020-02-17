package minijava.parser;

%%
/*
 *  JFlex description file of the MiniJava lexer
 */

/* JFLex options: */

%public
%class Lexer
%unicode

%cupsym ParserSymbols
%cup
/* this corresponds to:
%implements java_cup.runtime.Scanner
%function next_token             // name of the scanning function
%returnType java_cup.runtime.Symbol    // return returnType of the scanning function
%eofval{                         // what to do if eof is reached
  return new java_cup.runtime.Symbol(<CUPSYM>.EOF);
%eofval}
%eofclose                        // close input stream after eof is reached
*/

/* turn line and column counting on, 
   so that yyline and yycolumn fields can be used */
%line
%column

/* create a main function that can be called with name of input file
   (for testing purposes)*/
%cupdebug

/* verbatim code included in lexer class:*/
%{
  public java_cup.runtime.Symbol symbol(int returnType) {
    return new java_cup.runtime.Symbol(returnType, yyline, yycolumn);
  }
  public java_cup.runtime.Symbol symbol(int returnType, Object value) {
    return new java_cup.runtime.Symbol(returnType, yyline, yycolumn, value);
  }
%}


/* white spaces */
LineTerminator = \r|\n|\r\n
InputCharacter = [^\r\n]
WhiteSpace     = {LineTerminator} | [ \t\f]

/* identifiers */
Identifier = [a-zA-Z] [a-zA-Z0-9_]*
DecIntegerLiteral = 0 | [1-9][0-9]*


/* comments */
Comment = {TraditionalComment} | {EndOfLineComment}
TraditionalComment   = "/*" [^*] ~"*/" | "/*" "*"+ "/"
EndOfLineComment     = "//" {InputCharacter}* {LineTerminator}

%%
/* rules for lexer state: YYINITIAL */
<YYINITIAL> {
  /* operators */
  "{" { return symbol(ParserSymbols.LBRACE); }
  "}" { return symbol(ParserSymbols.RBRACE); }
  "[" { return symbol(ParserSymbols.LBRACK); }
  "]" { return symbol(ParserSymbols.RBRACK); }
  "(" { return symbol(ParserSymbols.LPAR); }
  ")" { return symbol(ParserSymbols.RPAR); }
  "int" {  return symbol(ParserSymbols.INT); }
  "boolean" {  return symbol(ParserSymbols.BOOL); }
  "if" { return symbol(ParserSymbols.IF); }
  "else" { return symbol(ParserSymbols.ELSE); }
  "while" { return symbol(ParserSymbols.WHILE); }
  "System.out.println" { return symbol(ParserSymbols.PRINTLN); }
  "System.out.write" { return symbol(ParserSymbols.WRITE); }
  "System.in.read" { return symbol(ParserSymbols.READ); }
  "java.io.IOException" { return symbol(ParserSymbols.IOEXCEPTION); }
  "return" { return symbol(ParserSymbols.RETURN); }
  "&&" { return symbol(ParserSymbols.OPCAND); }
  "<" { return symbol(ParserSymbols.OPCLT); }
  "+" { return symbol(ParserSymbols.OPCPLUS); }
  "-" { return symbol(ParserSymbols.OPCMINUS); }
  "*" { return symbol(ParserSymbols.OPCTIMES); }
  "/" { return symbol(ParserSymbols.OPCDIV); }
  "static" { return symbol(ParserSymbols.STATIC); }
  "void" { return symbol(ParserSymbols.VOID); }
  "main" { return symbol(ParserSymbols.MAIN); }
  "String" { return symbol(ParserSymbols.KSTRING); }
  "length" { return symbol(ParserSymbols.LEN); }
  "throws" { return symbol(ParserSymbols.THROWS); }
  "new" { return symbol(ParserSymbols.NEW); }
  "true" { return symbol(ParserSymbols.KTRUE); }
  "false" { return symbol(ParserSymbols.KFALSE); }
  "this" { return symbol(ParserSymbols.THIS); }
  "public" { return symbol(ParserSymbols.PUBLIC); }
  "class" { return symbol(ParserSymbols.CLASS); }
  "extends" { return symbol(ParserSymbols.EXTENDS); }
  "=" { return symbol(ParserSymbols.EQUALS); }
  "." { return symbol(ParserSymbols.DOT); }
  "," { return symbol(ParserSymbols.COMMA); }
  ";" { return symbol(ParserSymbols.SEMICOL); }
  "!" { return symbol(ParserSymbols.EXCL); }

  /* comments */
  {Comment}                      { /* ignore */ }
 
  /* whitespace */
  {WhiteSpace}                   { /* ignore */ }

  /* identifiers */ 
  {Identifier}                   { return symbol(ParserSymbols.IDENTIFIER, yytext()); }
 
  /* literals */
  {DecIntegerLiteral}            { return symbol(ParserSymbols.NUMBER,
                                                 Integer.valueOf(yytext())); }
}

/* rules for all lexer states */
/* error fallback */

[^]                             { throw new Error("Illegal character <"+yytext()+">"); }
