/* Those are called Include guards.
Once the header is included, it checks if a unique value (in this case _GLOBALS_H_) is defined. Then if it's not defined, it defines it and continues to the rest of the page.
When the code is included again, the first ifndef fails, resulting in a blank file.
That prevent double declaration of any identifiers such as types, enums and static variables.
http://stackoverflow.com/questions/1653958/why-are-ifndef-and-define-used-in-c-header-files */

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* Yacc/Bison generates internally its own values
 * for the tokens. Other files can access these values
 * by including the tab.h file generated using the
 * Yacc/Bison option -d ("generate header")
 *
 * The YYPARSER flag prevents inclusion of the tab.h
 * into the Yacc/Bison output itself
 */

#ifndef YYPARSER

/* the name of the following file may change */
#include "cminus.tab.h"

/* ENDFILE is implicitly defined by Yacc/Bison,
 * and not included in the tab.h file
 */
#define ENDFILE 0

#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 8

/* Yacc/Bison generates its own integer values
 * for tokens
 */
typedef int TokenType;

//O extern define variáveis que serão usadas em um arquivo apesar de terem sido declaradas em outro.
//http://mtm.ufsc.br/~azeredo/cursoC/aulas/ca20.html
extern FILE* source; /* source code text file */
extern FILE* listing; /* listing output text file */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {StmtK,ExpK,DeclK,TypeK} NodeKind;

typedef enum {IfK,WhileK,CallK,ReturnK,AssignK} StmtKind;
typedef enum {OpK,ConstK,IdK} ExpKind;
typedef enum {FunK,VarK,ParamK} DeclKind;
typedef enum {Void,Integer,Boolean} TypeKind;
typedef enum {Function, Variable} IdentifierKind;
/* TypeKind is used for type checking */

#define MAXCHILDREN 3

/* A union is a special data type available in C that allows to store different data types in the same memory location. You can define a union with many members, but only one member can contain a value at any given time. Unions provide an efficient way of using the same memory location for multiple-purpose.
https://www.tutorialspoint.com/cprogramming/c_unions.htm
http://pt.stackoverflow.com/questions/46668/oque-s%C3%A3o-unions-porque-utiliz%C3%A1-los-dentro-de-structs
*/

typedef struct treeNode
   { struct treeNode * child[MAXCHILDREN]; //Filhos
     struct treeNode * sibling; //Irmão
     int lineno; //nº linha
     NodeKind nodekind; //Tipo de nó: StmtK ou ExpK
     union { StmtKind stmt; ExpKind exp; DeclKind decl; TypeKind type; } kind;
     struct { TokenType op; int val; char *name; char *scope; bool vector; int temp;} attr;
     TypeKind type; /* for type checking of exps */
   } TreeNode;


typedef struct program
  { int usage;
    int name;
    int inicio;
    int tamMI;
    int tamMD;
    struct program * next;
  } *Program;

typedef struct FAT
  { int qtd_prog;
    int tam_prog;
    int prox_pos;
    struct program * p;
  } FAT;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;

extern int BIOS;
extern int SO;
extern int posSector;
extern int locationMD;
extern FAT fat;

#endif
