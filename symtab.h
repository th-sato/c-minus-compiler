
#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"
#include <stdbool.h>

/* the list of line numbers of the source
 * code in which a variable is referenced
 */
typedef struct LineListRec
  { int lineno;
    struct LineListRec * next;
  } * LineList;

typedef struct ListRec
    { char * name;
      int location;
      struct ListRec * next;
    } * List;

typedef struct ParamListRec
   { int number;
     struct ListRec * list;
   } * ParamList;

/* The record in the bucket lists for
* each variable, including name,
* assigned memory location, and
* the list of line numbers in which
* it appears in the source code
*/
typedef struct BucketListRec
  { char * name; //Nome do identificador (ID)
    bool vector; //É vetor ou não?
    char * scope; //Escopo da variável
    IdentifierKind typeID; //Tipo ID: variável ou função
    TypeKind typeData; //Tipo de dado: inteiro ou void
    ParamList param; //Lista de parâmetros
    LineList lines; //Numero da linha
    int memloc; /* memory location for variable */
    struct BucketListRec * next;
  } * BucketList;

void resetBucketList();

void st_insert_line(BucketList l, int lineno);

void st_insert_param(char *scope, int lineno, char *param, int location);

void st_insert(char * name, bool vector, char * scope, IdentifierKind typeID, TypeKind typeData, int lineno, int loc);

BucketList st_lookup ( char * name, char * scope );

void printSymTab(FILE * listing);

#endif
