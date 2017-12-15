%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void);
int yyerror(char * message);

%}

%token IF ELSE RETURN WHILE FOR
%token INT VOID NUM ID
%token PLUS MINUS OVER TIMES LT LTE GT GTE EQ NEQ ASSIGN SEMI VG LPAREN RPAREN LBRACKET RBRACKET LKEY RKEY
%token ERROR
%expect 1

%% /* Grammar for C- */

//programa -> declaracao-lista
program     : stmt_seq { savedTree = $1; }
            ;
//declaracao-lista -> declaracao-lista declaracao | declaracao
stmt_seq    : stmt_seq stmt
              { YYSTYPE t = $1;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $2;
                  $$ = $1;
                }
                else $$ = $2;
              }
            | stmt  { $$ = $1; }
            ;
//declaracao -> var-declaracao | fun-declaracao
stmt        : var_stmt { $$=$1; $$->attr.scope="Global";}
            | fun_stmt { $$=$1; $$->attr.scope="Global";}
            | error  { $$ = NULL; }
            ;
//var-declaracao -> tipo-especificador ID ; | tipo-especificador ID [ NUM ] ;
var_stmt    : type_spec identifier SEMI
				      { $$ = newDeclNode(VarK);
                $$->type = $1->kind.type;
                $$->attr.name = $2->attr.name;
                $$->lineno = $2->lineno;
				      }
	    	    | type_spec identifier LBRACKET number RBRACKET SEMI
				      { $$ = newDeclNode(VarK);
                $$->type = $1->kind.type;
                $$->attr.name = $2->attr.name;
                $$->attr.val = $4->attr.val;
                $$->attr.vector = true;
                $$->lineno = $2->lineno;
				      }
	    	    ;
//tipo-especificador -> int | void
type_spec   : INT { $$ = newTypeNode(Integer); }
	    	    | VOID { $$ = newTypeNode(Void); }
	    	    ;
//fun-declaração -> tipo-especificador ID ( params ) composto-decl
fun_stmt    : type_spec identifier LPAREN params RPAREN comp_stmt
				      { $$ = newDeclNode(FunK);
		  		      $$->type = $1->kind.type;
                $$->attr.name = $2->attr.name;
		  		      $$->child[0] = $4;
		  		      $$->child[1] = $6;
                $$->lineno = $2->lineno;
				      }
	    	    ;
//params -> param-lista | void | nada
params      : param_list { $$ = $1; }
	    	    | VOID { $$ = NULL; }
			      | { $$ = NULL; }
	    	    ;
//param-lista -> param-lista, param | param
param_list  : param_list VG param
				      { YYSTYPE t = $1;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $3;
                  $$ = $1;
                }
                else $$ = $3;
				      }
	    	    | param { $$ = $1; }
	    	    ;
//param -> tipo-especificador ID | tipo-especificador ID [ ]
param	      : type_spec identifier
				      { $$ = newDeclNode(ParamK);
				        $$->type = $1->kind.type;
                $$->attr.name = $2->attr.name;
                $$->lineno = $2->lineno;
				      }
	    	    | type_spec identifier LBRACKET RBRACKET
				      { $$ = newDeclNode(ParamK);
				        $$->type = $1->kind.type;
                $$->attr.name = $2->attr.name;
                $$->attr.vector = true;
                $$->lineno = $2->lineno;
				      }
	    	    ;
//composto-decl -> { local-declaracoes } | { statement-lista } | { local-declaracoes statement-lista } | { }
comp_stmt   : LKEY local_stmt stmt_list RKEY
				      { YYSTYPE t = $2;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $3;
                  $$ = $2;
                }
                else $$ = $3;
				      }
	    	    | LKEY stmt_list RKEY { $$ = $2; }
	    	    | LKEY local_stmt RKEY { $$ = $2; }
	    	    | LKEY RKEY { $$ = NULL; }
	    	    ;
//local-declarações -> local-declarações var-declaração | var-declaração
local_stmt  : local_stmt var_stmt
				      { YYSTYPE t = $1;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $2;
                  $$ = $1;
                }
                else $$ = $2;
				      }
	    	    | var_stmt { $$ = $1; }
	    	    ;
//statement-lista -> statement-lista statement
stmt_list   : stmt_list statement
				      { YYSTYPE t = $1;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $2;
                  $$ = $1;
                }
                else $$ = $2;
				      }
	    	    | statement { $$ = $1; }
	    	    ;
//statement -> expressao-decl | composto-decl | seleção-decl | iteração-decl | retorno-decl
statement   : expr_stmt { $$ = $1; }
	    	    | comp_stmt { $$ = $1; }
	    	    | sel_stmt { $$ = $1; }
	    	    | it_stmt { $$ = $1; }
            | for_stmt { $$ = $1; }
	    	    | ret_stmt { $$ = $1; }
	    	    ;
// expressao-decl -> expressao ; | ;
expr_stmt   : expr SEMI { $$ = $1; }
	    	    | SEMI { $$ = NULL; }
	    	    ;
//selecao-decl -> if ( expressao ) statement | if ( expressao ) statement else statement
sel_stmt    : IF LPAREN expr RPAREN statement
				      { $$ = newStmtNode(IfK);
		  		      $$->child[0] = $3;
		  		      $$->child[1] = $5;
				      }
	    	    | IF LPAREN expr RPAREN statement ELSE statement
				      { $$ = newStmtNode(IfK);
		  		      $$->child[0] = $3;
		  		      $$->child[1] = $5;
		  		      $$->child[2] = $7;
				      }
	    	    ;
//iteracao-decl -> while ( expressao ) statement
it_stmt     : WHILE LPAREN expr RPAREN statement
				      { $$ = newStmtNode(WhileK);
		  		      $$->child[0] = $3;
		  		      $$->child[1] = $5;
				      }
	    	    ;
//for-decl -> for ( expressao; expressao; expressao) statement
for_stmt    : FOR LPAREN expr SEMI expr SEMI expr RPAREN statement
            { YYSTYPE t;
              $$ = $3;
              $$->sibling = newStmtNode(WhileK);
              t = $$->sibling;
              t->child[0] = $5;
              t->child[1] = $9;
              t->child[2] = $7;
            }
            ;
//retorno-decl -> return ; | return expressão;
ret_stmt    : RETURN SEMI { $$ = newStmtNode(ReturnK); }
	    	    | RETURN expr SEMI
				      { $$ = newStmtNode(ReturnK);
			  	      $$->child[0] = $2;
				      }
	    	    ;
//expressao -> var = expressao | simples-expressao
expr	      : var ASSIGN expr
				      { $$ = newStmtNode(AssignK);
				        $$->child[0] = $1;
                $$->child[1] = $3;
				      }
	    	    | simp_expr { $$ = $1; }
	    	    ;
//var -> ID | ID [ expressao ]
var	    	  : identifier { $$ = $1; }
	    	    | identifier LBRACKET expr RBRACKET
              { $$ = $1;
                $$->attr.vector = true;
		  		      $$->child[0] = $3;
				      }
	    	    ;
//simples-expressao -> soma-expressao relacional soma-expressao | soma-expressao
simp_expr   : sum_exp relat sum_exp
				    { $$ = $2;
		  		    $$->child[0] = $1;
		  		    $$->child[1] = $3;
				    }
	    	    | sum_exp { $$ = $1; }
	    	    ;
//relacional -> <= | < | > | >= | == | !=
relat	      : LT { $$ = newExpNode(OpK); $$->attr.op = LT; }
	    	    | LTE { $$ = newExpNode(OpK); $$->attr.op = LTE; }
	    	    | GT { $$ = newExpNode(OpK); $$->attr.op = GT; }
	    	    | GTE { $$ = newExpNode(OpK); $$->attr.op = GTE; }
	    	    | EQ { $$ = newExpNode(OpK); $$->attr.op = EQ; }
	    	    | NEQ { $$ = newExpNode(OpK); $$->attr.op = NEQ; }
	    	    ;
//soma-expressao -> soma-expressao soma termo | termo
sum_exp	    : sum_exp sum term
				      { $$ = $2;
		  		      $$->child[0] = $1;
		  		      $$->child[1] = $3;
				      }
	    	    | term { $$ = $1; }
	    	    ;
//soma -> + | -
sum 	      : PLUS { $$ = newExpNode(OpK); $$->attr.op = PLUS; }
	    	    | MINUS { $$ = newExpNode(OpK); $$->attr.op = MINUS; }
	    	    ;
//termo -> termo mult fator | fator
term	      : term mult factor
				      { $$ = $2;
				        $$->child[0] = $1;
				        $$->child[1] = $3;
				      }
		        | factor { $$ = $1; }
		        ;
//mult -> * | /
mult 	      : TIMES { $$ = newExpNode(OpK); $$->attr.op = TIMES; }
		        | OVER { $$ = newExpNode(OpK); $$->attr.op = OVER; }
		        ;
//fator -> ( expressao ) | var | ativacao | NUM
factor	    : LPAREN expr RPAREN { $$ = $2; }
		        | var { $$ = $1; }
		        | activation { $$ = $1; }
		        | number { $$ = $1; }
	          | error { $$ = NULL; }
		        ;
//ativacao -> ID ( ) | ID (arg-lista)
activation  : identifier LPAREN RPAREN
              { $$ = newStmtNode(CallK);
                $$->attr.name = $1->attr.name;
                $$->lineno = $1->lineno;
              }
		        | identifier LPAREN args_seq RPAREN
				      { $$ = newStmtNode(CallK);
                $$->attr.name = $1->attr.name;
                $$->lineno = $1->lineno;
				        $$->child[0] = $3;
				      }
		        ;
//arg-lista -> arg-lista , expressao | expressao
args_seq    : args_seq VG expr
				      { YYSTYPE t = $1;
                if (t != NULL){
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $3;
                  $$ = $1;
                }
                else $$ = $3;
				      }
	    	    | expr { $$ = $1; }
	    	    ;
//Guardar número
number	    : NUM
				      { $$ = newExpNode(ConstK);
                $$->attr.val = atoi(tokenString);
                $$->type = Integer;
              }
	    	    ;
//Guardar identificador
identifier  : ID
				      { $$ = newExpNode(IdK);
		            $$->attr.name = copyString(tokenString);
				      }
			      ;
%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the C- scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}
