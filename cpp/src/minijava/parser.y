%skeleton "lalr1.cc"
%require "3.0.4"
%defines
%define api.namespace {mjc}
%define api.parser.class {parser}
%define api.value.type variant
%define api.token.constructor

%code requires
{
#include <iostream>
#include <memory>
#include <list>
#include <vector>
#include <string>
#include <optional>

#include "util/ordered_map.h"
#include "minijava/ast.h"

namespace mjc {
    class ParserContext;
}
}

%param { ParserContext& parser_context }
%locations

%code
{
  #include "minijava/parser_context.h"
}

%define api.token.prefix {TOK_}
%token END 0
%token LBRACE
%token RBRACE
%token LBRACK
%token RBRACK
%token LPAR
%token RPAR
%token INT
%token BOOL
%token IF
%token ELSE
%token WHILE
%token THROWS
%token IOEXCEPTION
%token PRINT
%token WRITE
%token READ
%token RETURN
%token OPCAND
%token OPCLT
%token OPCPLUS
%token OPCMINUS
%token OPCTIMES
%token OPCDIV
%token STATIC
%token VOID
%token MAIN
%token KSTRING
%token LEN
%token NEW
%token KTRUE
%token KFALSE
%token THIS
%token PUBLIC
%token CLASS
%token EXTENDS
%token EQUALS
%token DOT
%token COMMA
%token SEMICOL
%token EXCL
%token <std::string> IDENTIFIER
%token <int> NUMBER

%type <std::shared_ptr<Exp>> Expr
%type <std::shared_ptr<Stm>> Stm
%type <std::shared_ptr<Type>> Typ
%type <MethodDecl> MDecl
%type <VarDecl> Prm
%type <VarDecl> VDecl
%type <std::optional<Ident>> MaybeExtends
%type <bool> MaybeThrows
%type <ClassDecl> CDecl
%type <MainClassDecl> MClass
%type <Program> Prg

%type <std::vector<VarDecl>> VDeclList
%type <std::vector<MethodDecl>> MDeclList
%type <std::vector<ClassDecl>> CDeclList
%type <std::vector<std::shared_ptr<Exp>>> ExprList
%type <std::vector<std::shared_ptr<Exp>>> ExprList1
%type <std::vector<std::shared_ptr<Stm>>> StmList
%type <std::vector<std::shared_ptr<Stm>>> StmList1
%type <std::vector<VarDecl>> PrmList
%type <std::vector<VarDecl>> PrmList1

/* Precedences */
%left OPCAND
%left OPCLT
%left EXCL
%left OPCPLUS OPCMINUS
%left OPCTIMES OPCDIV
%left LBRACK
%left DOT

%%
/* The grammar */
%start Prg;
Prg:
  MClass CDeclList
    { parser_context.SetResult(
        Program{ .main_class = std::move($1),
                 .classes = std::move($2) }); }
  ;

MClass:
  CLASS IDENTIFIER LBRACE PUBLIC STATIC VOID MAIN LPAR KSTRING LBRACK
  RBRACK IDENTIFIER RPAR MaybeThrows
  LBRACE Stm RBRACE RBRACE
    { $$ = std::move(
             MainClassDecl{ .class_name = std::move($2),
                            .main_throws_io_exception= std::move($14),
                            .main_body = std::move($16),
                            .source_location = @$}); }
  ;

CDecl:
    CLASS IDENTIFIER MaybeExtends LBRACE VDeclList MDeclList RBRACE
      { $$ = std::move(
               ClassDecl{ .class_name = std::move($2),
                          .fields = std::move($5),
                          .methods = std::move($6),
                          .source_location = @$ }); }
  ;

CDeclList:
    /* empty */
      { $$ = {}; }
  | CDeclList CDecl
      { $$ = std::move($1);
        $$.push_back($2); }
  ;

MaybeExtends:
    EXTENDS IDENTIFIER
      { $$ = { std::move($2) }; }
  | /* empty */
      { $$ = {}; }
 ;

MaybeThrows:
    THROWS IOEXCEPTION
      { $$ = true; }
  | /* empty */
      { $$ = false; }
 ;

VDecl:
    Typ IDENTIFIER SEMICOL
      { $$ = { .var_name = std::move($2),
               .var_type = std::move($1),
               .source_location = @$ }; }
 ;

VDeclList:
    /* empty */
      { $$ = {}; }
  | VDeclList VDecl
      { $$ = std::move($1);
        $$.push_back($2); }
  ;

Prm:
    Typ IDENTIFIER
      { $$ = { .var_name = std::move($2),
               .var_type = std::move($1),
               .source_location = @$ }; }
  ;

PrmList:
    /* empty */
      { $$ = {}; }
  | PrmList1
      { $$ = std::move($1); }
  ;

PrmList1:
    Prm
      { $$ = {std::move($1)}; }
  | PrmList1 COMMA Prm
      { $$ = $1; $$.push_back($3); }
  ;

MDecl:
    PUBLIC Typ IDENTIFIER LPAR PrmList RPAR MaybeThrows
    LBRACE VDeclList StmList  RETURN Expr SEMICOL RBRACE
      { $$ = std::move(MethodDecl{
               .method_name = std::move($3),
               .return_type = std::move($2),
               .parameters = std::move($5),
               .throws_io_exception = std::move($7),
               .locals = std::move($9),
               .body = std::make_shared<StmSeq>(StmSeq{$10}),
               .return_exp = std::move($12),
               .source_location = @$ }); }
  ;

MDeclList:
    /* empty */
      { $$ = {}; }
  | MDeclList MDecl
      { $$ = std::move($1);
        $$.push_back($2); }
  ;

Typ:
    INT LBRACK RBRACK
      { $$ = std::make_shared<TypeArray>(std::make_shared<TypeInt>(TypeInt{}));
        $$->SetLocation(@$);}
  | BOOL
      { $$ = std::make_shared<TypeBool>();
        $$->SetLocation(@$);}
  | INT
      { $$ = std::make_shared<TypeInt>();
        $$->SetLocation(@$);}
  | IDENTIFIER
     { $$ = std::make_shared<TypeClass>($1);
       $$->SetLocation(@$);}
  ;

Stm:
    LBRACE StmList RBRACE
      { $$ = std::make_shared<StmSeq>($2);
        $$->SetLocation(@$); }
  | IF LPAR Expr RPAR Stm ELSE Stm
      { $$ = std::make_shared<StmIf>(std::move($3), std::move($5), std::move($7));
        $$->SetLocation(@$); }
  | WHILE LPAR Expr RPAR Stm
      { $$ = std::make_shared<StmWhile>(std::move($3), std::move($5));
        $$->SetLocation(@$);}
  | PRINT LPAR Expr RPAR SEMICOL
      { $$ = std::make_shared<StmPrint>(std::move($3));
        $$->SetLocation(@$);}
  | WRITE LPAR Expr RPAR SEMICOL
      { $$ = std::make_shared<StmWrite>(std::move($3));
        $$->SetLocation(@$);}
  | IDENTIFIER EQUALS Expr SEMICOL
      { $$ = std::make_shared<StmAssignment>(std::move($1), std::move($3));
        $$->SetLocation(@$);}
  | IDENTIFIER LBRACK Expr RBRACK EQUALS Expr SEMICOL
      { $$ = std::make_shared<StmArrayAssignment>(std::move($1), std::move($3), std::move($6));
        $$->SetLocation(@$);}
  ;
StmList:
    /* empty */
      { $$ = {}; }
  | StmList1
      { $$ = std::move($1); }
  ;

StmList1:
    Stm
      { $$ = {$1}; }
  | StmList1 Stm
      { $$ = std::move($1);
        $$.push_back($2); }
  ;

Expr:
    Expr OPCAND Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::STRICTAND, std::move($3));
        $$->SetLocation(@$); }
  | Expr OPCLT Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::LT, std::move($3));
        $$->SetLocation(@$); }
  | Expr OPCPLUS Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::PLUS, std::move($3));
        $$->SetLocation(@$); }
  | Expr OPCMINUS Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::MINUS, std::move($3));
        $$->SetLocation(@$); }
  | Expr OPCTIMES Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::MUL, std::move($3));
        $$->SetLocation(@$); }
  | Expr OPCDIV Expr
      { $$ = std::make_shared<ExpBinOp>(std::move($1), ExpBinOp::BinOp::DIV, std::move($3));
        $$->SetLocation(@$); }
  | Expr LBRACK Expr RBRACK
      { $$ = std::make_shared<ExpArrayGet>(std::move($1), std::move($3));
        $$->SetLocation(@$); }
  | Expr DOT LEN
      { $$ = std::make_shared<ExpArrayLength>(std::move($1));
        $$->SetLocation(@$); }
  | Expr DOT IDENTIFIER LPAR ExprList RPAR
      { $$ = std::make_shared<ExpInvoke>(std::move($1), $3, std::move($5));
        $$->SetLocation(@$); }
  | READ LPAR RPAR
      { $$ = std::make_shared<ExpRead>();
        $$->SetLocation(@$); }
  | NUMBER
      { $$ = std::make_shared<ExpNum>($1);
        $$->SetLocation(@$); }
  | OPCMINUS NUMBER
      { $$ = std::make_shared<ExpNum>(-$2);
        $$->SetLocation(@$); }
  | KTRUE
      { $$ = std::make_shared<ExpTrue>();
        $$->SetLocation(@$); }
  | KFALSE
      { $$ = std::make_shared<ExpFalse>();
        $$->SetLocation(@$); }
  | THIS
      { $$ = std::make_shared<ExpThis>();
        $$->SetLocation(@$); }
  | NEW INT LBRACK Expr RBRACK
      { $$ = std::make_shared<ExpNewIntArray>($4);
        $$->SetLocation(@$); }
  | NEW IDENTIFIER LPAR RPAR
      { $$ = std::make_shared<ExpNew>($2);
        $$->SetLocation(@$); }
  | IDENTIFIER
      { $$ = std::make_shared<ExpId>($1);
        $$->SetLocation(@$); }
  | EXCL Expr
      { $$ = std::make_shared<ExpNeg>(std::move($2));
        $$->SetLocation(@$); }
  | LPAR Expr RPAR
      { $$ = std::move($2);
        $$->SetLocation(@$); }
  ;

ExprList:
    /* empty */
      { $$ = {}; }
  | ExprList1
      { $$ = std::move($1); }
  ;

ExprList1:
    Expr
      { $$ = {}; $$.push_back($1); }
  | ExprList1 COMMA Expr
      { $$ = std::move($1);
        $$.push_back(std::move($3)); }
  ;

%%

void
mjc::parser::error (const location_type& l,
                    const std::string& m)
{
  parser_context.SetParseError(l, m);
}
