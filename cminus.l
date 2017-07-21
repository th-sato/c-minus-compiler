%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
static int yylex(void);
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n
whitespace  [ \t\r]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"int"		        {return INT;}
"return"	      {return RETURN;}
"void"		      {return VOID;}
"while"		      {return WHILE;}
"+"		          {return PLUS;}
"-"		          {return MINUS;}
"/"		          {return OVER;}
"*"		          {return TIMES;}
"<"             {return LT;}
"<="		        {return LTE;}
">"		          {return GT;}
">="		        {return GTE;}
"=="            {return EQ;}
"!="		        {return NEQ;}
"="             {return ASSIGN;}
";"             {return SEMI;}
","		          {return VG;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"["		          {return LBRACKET;}
"]"		          {return RBRACKET;}
"{"		          {return LKEY;}
"}"		          {return RKEY;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"            { char c; int end = 0;
                  do
                  { c = input();
                    if (c == EOF) break;
                    if (c == '\n') lineno++;
                    if (c == '*') {
			                 c = input();
			                 if (c == '/') end = 1;
			              }
                  } while (end==0);
                }
.               {return ERROR;}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}
