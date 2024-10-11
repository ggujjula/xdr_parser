%top{
#include "parser.h"

extern void *lexer_talloc_ctxt;
}

%{
#include <stdio.h> 
#include "token.h"

void *lexer_talloc_ctxt = NULL;
%}

%x COMMENT_EATER LINE_COMMENT_EATER

%%
\/\*                       { BEGIN(COMMENT_EATER); }
<COMMENT_EATER>\*\/        { BEGIN(INITIAL); }
<COMMENT_EATER>\*[^/]      { /* monch */ }
<COMMENT_EATER>([^*]|\n)+  { /* monch */ }
"%"                        { BEGIN(LINE_COMMENT_EATER); } // This this not part of the standard >:(
<LINE_COMMENT_EATER>.      { /* cronch */ }
<LINE_COMMENT_EATER>\n     { BEGIN(INITIAL); }
"("                        { *yylval = make_basic(lexer_talloc_ctxt, LPAREN, yytext); return LPAREN; }
")"                        { *yylval = make_basic(lexer_talloc_ctxt, RPAREN, yytext); return RPAREN; }
","                        { *yylval = make_basic(lexer_talloc_ctxt, COMMA, yytext); return COMMA; }
":"                        { *yylval = make_basic(lexer_talloc_ctxt, COLON, yytext); return COLON; }
";"                        { *yylval = make_basic(lexer_talloc_ctxt, SEMICOLON, yytext); return SEMICOLON; }
"="                        { *yylval = make_basic(lexer_talloc_ctxt, EQ, yytext); return EQ; }
"{"                        { *yylval = make_basic(lexer_talloc_ctxt, LBRACE, yytext); return LBRACE; }
"}"                        { *yylval = make_basic(lexer_talloc_ctxt, RBRACE, yytext); return RBRACE;}
"<"                        { *yylval = make_basic(lexer_talloc_ctxt, LT, yytext); return LT; }
">"                        { *yylval = make_basic(lexer_talloc_ctxt, GT, yytext); return GT; }
"*"                        { *yylval = make_basic(lexer_talloc_ctxt, STAR, yytext); return STAR; }
"["                        { *yylval = make_basic(lexer_talloc_ctxt, LBRACKET, yytext); return LBRACKET; }
"]"                        { *yylval = make_basic(lexer_talloc_ctxt, RBRACKET, yytext); return RBRACKET; }
bool                       { *yylval = make_basic(lexer_talloc_ctxt, BOOL, yytext); return BOOL; }
case                       { *yylval = make_basic(lexer_talloc_ctxt, CASE, yytext); return CASE; }
const                      { *yylval = make_basic(lexer_talloc_ctxt, CONST, yytext); return CONST; }
default                    { *yylval = make_basic(lexer_talloc_ctxt, DEFAULT, yytext); return DEFAULT; }
double                     { *yylval = make_basic(lexer_talloc_ctxt, DOUBLE, yytext); return DOUBLE; }
enum                       { *yylval = make_basic(lexer_talloc_ctxt, ENUM, yytext); return ENUM; }
float                      { *yylval = make_basic(lexer_talloc_ctxt, FLOAT, yytext); return FLOAT; }
hyper                      { *yylval = make_basic(lexer_talloc_ctxt, HYPER, yytext); return HYPER; }
int                        { *yylval = make_basic(lexer_talloc_ctxt, INT, yytext); return INT; }
opaque                     { *yylval = make_basic(lexer_talloc_ctxt, OPAQUE, yytext); return OPAQUE; }
quadruple                  { *yylval = make_basic(lexer_talloc_ctxt, QUADRUPLE, yytext); return QUADRUPLE; }
string                     { *yylval = make_basic(lexer_talloc_ctxt, STRING, yytext); return STRING; }
struct                     { *yylval = make_basic(lexer_talloc_ctxt, STRUCT, yytext); return STRUCT; }
switch                     { *yylval = make_basic(lexer_talloc_ctxt, SWITCH, yytext); return SWITCH; }
typedef                    { *yylval = make_basic(lexer_talloc_ctxt, TYPEDEF, yytext); return TYPEDEF; }
union                      { *yylval = make_basic(lexer_talloc_ctxt, UNION, yytext); return UNION; }
unsigned                   { *yylval = make_basic(lexer_talloc_ctxt, UNSIGNED, yytext); return UNSIGNED; }
void                       { *yylval = make_basic(lexer_talloc_ctxt, VOID, yytext); return VOID; }
program                    { *yylval = make_basic(lexer_talloc_ctxt, PROGRAM, yytext); return PROGRAM; }
version                    { *yylval = make_basic(lexer_talloc_ctxt, VERSION, yytext); return VERSION; }
[A-Za-z][A-Za-z0-9_]*      { *yylval = make_basic(lexer_talloc_ctxt, IDENT, yytext); return IDENT; }
[-]?[1-9][0-9]*            { *yylval = make_basic(lexer_talloc_ctxt, DECIMAL, yytext); return DECIMAL; }
0x[0-9a-fA-F]+             { *yylval = make_basic(lexer_talloc_ctxt, HEX, yytext); return HEX; }
0[0-9]*                    { *yylval = make_basic(lexer_talloc_ctxt, OCTAL, yytext); return OCTAL; }
[[:space:]]+               { /* om nom */ }
.|\n                       { fprintf(stderr, "Invalid character \n", yytext); return -1; }
