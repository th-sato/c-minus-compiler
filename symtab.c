#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* the hash table */
static BucketList hashTable[SIZE];

/* found in table, so just add line number */
void st_insert_line(BucketList l, int lineno) {
    LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
} /* st_insert_found */

void st_insert_param(char *scope, int lineno, char *param, int location) {
    BucketList l = st_lookup(scope, "Global");
    List t = l->param->list;
    List aux;
    l->param->number++;
    if(t != NULL){
      while (t->next != NULL) t = t->next;
      aux = (List) malloc(sizeof(struct ListRec));
      aux->name = param;
      aux->location = location;
      aux->next = NULL;
      t->next = aux;
    }
    else{
      l->param->list = (List) malloc(sizeof(struct ListRec));
      l->param->list->name = param;
      l->param->list->location = location;
      l->param->list->next = NULL;
    }

} /* st_insert_param */

/* variable not yet in table */
void st_insert(char * name, bool vector, char * scope, IdentifierKind typeID, TypeKind typeData, int lineno, int loc) {
  int h = hash(name);
  BucketList l =  hashTable[h];
  while (l != NULL)
    l = l->next;
  l = (BucketList) malloc(sizeof(struct BucketListRec));
  l->name = name;
  l->scope = scope;
  l->vector = vector;
  l->typeID = typeID;
  l->typeData = typeData;
  l->param = (ParamList) malloc(sizeof(struct ParamListRec));
  l->param->number = 0;
  l->param->list = NULL;
  l->lines = (LineList) malloc(sizeof(struct LineListRec));
  l->lines->lineno = lineno;
  l->memloc = loc;
  l->lines->next = NULL;
  l->next = hashTable[h]; //Inserir no início
  hashTable[h] = l;
} /* st_insert */

BucketList st_lookup ( char * name, char * scope )
{ int h = hash(name);
  BucketList l =  hashTable[h];
  while ((l != NULL) && !((strcmp(name,l->name) == 0) && ((strcmp(scope,l->scope) == 0)||(strcmp(l->scope,"Global") == 0))))
    l = l->next;
  return l;
}

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE * listing)
{ int i;
  fprintf(listing,"   Name   |Vector|  Scope  |   Type  |Location|        Params        |Numbers\n");
  fprintf(listing,"-------------------------------------------------------------------------------\n");
  for (i=0;i<SIZE;++i)
  { if (hashTable[i] != NULL)
    { BucketList l = hashTable[i];
      while (l != NULL)
      { LineList t = l->lines;
        ParamList p = l->param;
        List lp = l->param->list;
        fprintf(listing,"%-10s|",l->name);
        if(l->vector)
          fprintf(listing," Yes  |");
        else
          fprintf(listing,"  No  |");
        if(l->scope != NULL)
          fprintf(listing, "%-9s|",l->scope);
        else
          fprintf(listing,"                 ");
        if(l->typeData == Void)
          fprintf(listing,"void ");
        else if(l->typeData == Integer)
          fprintf(listing,"int  ");
        if(l->typeID == Function)
          fprintf(listing,"func|");
        else if(l->typeID == Variable)
          fprintf(listing,"var |");
        fprintf(listing,"%-8d|",l->memloc);
        fprintf(listing,"%-3d", p->number);
        while (lp != NULL){
          fprintf(listing,"%s, ", lp->name);
          lp = lp->next;
        }
        fprintf(listing,"|");
        while (t != NULL)
        { fprintf(listing,"%d, ",t->lineno);
          t = t->next;
        }
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
} /* printSymTab */
