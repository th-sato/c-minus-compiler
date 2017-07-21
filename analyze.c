#include "symtab.h"
#include "analyze.h"

/*O funcionamento das variáveis declaradas como static depende se estas são globais ou locais.
Variáveis globais static funcionam como variáveis globais dentro de um módulo, ou seja, são variáveis globais que não são (e nem podem ser) conhecidas em outros modulos. Isto é util se quisermos isolar pedaços de um programa para evitar mudanças acidentais em variáveis globais.
Variáveis locais static são variáveis cujo valor é mantido de uma chamada da função para a outra.
http://mtm.ufsc.br/~azeredo/cursoC/aulas/ca20.html */

static BucketList pointer;
static char *scope;
static bool main_declared = false;
static int location = 0;

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t, void (* preProc) (TreeNode *), void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

static void declareError(TreeNode * t, char * message)
{ fprintf(listing,"Declare error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static void mainError( char * message)
{ fprintf(listing,"Error: %s\n",message);
  Error = TRUE;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode( TreeNode * t) //Preencher tabela de símbolos
{ switch (t->nodekind) {
  case StmtK:
    switch (t->kind.stmt) {
      case IfK:
        break;
      case WhileK:
        break;
      case CallK: //Chamada de função.
        t->attr.scope = "Global";
        pointer = st_lookup(t->attr.name, t->attr.scope);
        if(pointer != NULL){ //A função foi declarada
          st_insert_line(pointer, t->lineno);
          t->type = pointer->typeData;
        }
        else{ //A função não foi declarada
          if(strcmp(t->attr.name, "input")==0){
            t->type = Integer;
            st_insert("input", false, "Global", Function, Integer, -1, location++);
          }
          else if (strcmp(t->attr.name, "output")==0){
            t->type = Void;
            st_insert("output", false, "Global", Function, Void, -1, location++);
          }
          else declareError(t, "Undeclared function!");
        }
        break;
      case ReturnK:
        break;
      case AssignK:
        break;
      default:
        break;
    }
    break;
  case ExpK:
    switch (t->kind.exp) {
      case OpK:
        break;
      case ConstK:
        break;
      case IdK:
        t->attr.scope = scope;
        pointer = st_lookup(t->attr.name, t->attr.scope);
        if(pointer != NULL){
          st_insert_line(pointer, t->lineno);
          t->type = pointer->typeData;
        }
        else
          declareError(t, "Undeclared variable!");
        break;
      default:
        break;
      }
      break;
  case DeclK:
    switch (t->kind.decl) {
      case FunK: //Global
        if(strcmp(t->attr.name, "main")==0)
          main_declared = true;
        scope = t->attr.name;
        pointer = st_lookup(t->attr.name, t->attr.scope);
        if(pointer==NULL) //A função não foi declarada
          st_insert(t->attr.name, t->attr.vector, t->attr.scope, Function, t->type, t->lineno, location++);
        else //A função foi declarada
          declareError(t, "Function already declared!");
        break;
      case VarK:
        if(t->attr.scope==NULL) t->attr.scope = scope;
        pointer = st_lookup(t->attr.name, t->attr.scope);
        if(pointer == NULL){ //variável não declarada
              st_insert(t->attr.name, t->attr.vector, t->attr.scope, Variable, t->type, t->lineno, location);
              if(t->attr.vector) location += t->attr.val;
              else location++;
        } else //variável declarada
          declareError(t, "Variable already declared!");
        break;
      case ParamK:
        if(t->attr.scope==NULL) t->attr.scope = scope;
        pointer = st_lookup(t->attr.name, t->attr.scope);
        if(pointer == NULL){ //Não encontrou
          st_insert (t->attr.name, t->attr.vector, t->attr.scope, Variable, t->type, t->lineno, location);
          st_insert_param (t->attr.scope, t->lineno, t->attr.name, location);
          location++;
        }
        else
          declareError(t, "Variable already declared!");
        break;
      default:
        break;
    }
    break;
  case TypeK:
    switch (t->kind.type) {
      case Void:
        break;
      case Integer:
        break;
      default:
        break;
    }
    break;
  default:
    break;
  }
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ traverse(syntaxTree,insertNode,nullProc);
  if (TraceAnalyze)
  { if(!main_declared)
      mainError("Main not declared!");
    fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{
  switch (t->nodekind) {
    case StmtK:
      switch (t->kind.stmt) {
        case IfK:
          if(t->child[0]->type != Boolean)
            typeError(t->child[0],"IF test is not Boolean!");
          break;
        case WhileK:
          if(t->child[0]->type != Boolean)
            typeError(t->child[0],"WHILE test is not Boolean!");
          break;
        case CallK:
          break;
        case ReturnK:
          break;
        case AssignK:
          if(t->child[0]->type != t->child[1]->type)
            typeError(t,"ASSIGN: Different types!");
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp) {
        case OpK:
          if ((t->child[0]->type != Integer) || (t->child[1]->type != Integer))
            typeError(t,"OP applied to non-integer!");
          if ( (t->attr.op == LT) || (t->attr.op == LTE) || (t->attr.op == GT) || (t->attr.op == GTE) || (t->attr.op == EQ) || (t->attr.op == NEQ) )
            t->type = Boolean;
          else
            t->type = Integer;
          break;
        case ConstK:
          break;
        case IdK:
          break;
        default:
          break;
        }
        break;
    case DeclK:
      switch (t->kind.decl) {
        case FunK:
          break;
        case VarK:
          if(t->type == Void)
            typeError(t,"Variable can't be of type void!");
          break;
        case ParamK:
          break;
        default:
          break;
      }
      break;
    case TypeK:
      switch (t->kind.type) {
        case Void:
          break;
        case Integer:
          break;
        default:
          break;
      }
      break;
    default:
      break;
    }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}
