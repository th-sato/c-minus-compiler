#include "code.h"
#include "cgen.h"

static int tmpNumber;
static Stack stack = NULL;
static AddressQuadElement elementSaved = NULL;
static bool vector_temp[number_temp];
static int imIntermediateCode = 1;
AddressQuad CodeAddrQ = NULL;

/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);

static AddressQuadElement insertNewElementQuad (OperationKind op, AddressData add1, AddressData add2, AddressData res, int instructionMemory){
  AddressQuadElement newElement = (AddressQuadElement)malloc(sizeof(struct ThreeAddressQuadElement));
  newElement->op = op;
  newElement->instructionMemory = instructionMemory;
  newElement->addr1 = add1;
  newElement->addr2 = add2;
  newElement->result = res;
  newElement->objectCode = NULL;
  return newElement;
}

static void insertThreeAddressQuad (AddressQuadElement newElement){
  if(CodeAddrQ == NULL) {
    CodeAddrQ = (AddressQuad)malloc(sizeof(struct ThreeAddressQuad));
    CodeAddrQ->first = newElement;
    CodeAddrQ->last = newElement;
    CodeAddrQ->last->next = NULL;
  }
  else if(CodeAddrQ->first == CodeAddrQ->last){
    CodeAddrQ->last = newElement;
    CodeAddrQ->first->next = CodeAddrQ->last;
    CodeAddrQ->last->next = NULL;
  }
  else {
    CodeAddrQ->last->next = newElement;
    CodeAddrQ->last = CodeAddrQ->last->next;
    CodeAddrQ->last->next = NULL;
  }
}

static void insertFirstPosQuad(AddressQuadElement mainElement){
  if(CodeAddrQ == NULL) {
    CodeAddrQ = (AddressQuad)malloc(sizeof(struct ThreeAddressQuad));
    CodeAddrQ->first = mainElement;
    CodeAddrQ->last = mainElement;
    CodeAddrQ->last->next = NULL;
  }
  else if(CodeAddrQ->first == CodeAddrQ->last){
    CodeAddrQ->first = mainElement;
    CodeAddrQ->first->next = CodeAddrQ->last;
    CodeAddrQ->last->next = NULL;
  }
  else {
    mainElement->next = CodeAddrQ->first;
    CodeAddrQ->first = mainElement;
  }
}

static void insertMainThreeAddressQuad (){
  AddressData address = (AddressData)malloc(sizeof(struct AddressDataUnion));
  AddressData address2 = (AddressData)malloc(sizeof(struct AddressDataUnion));
  BucketList l = st_lookup ("main", "Global");
  address->addressKind = BucketL;
  address->addr.bPointer = l;
  address2->addressKind = Const;
  address2->addr.constant = l->param->number;
  insertFirstPosQuad(insertNewElementQuad(CallOP, address, address2, NULL,0));
}

static AddressData insertNameFunction(char *name){
  AddressData addr = (AddressData)malloc(sizeof(struct AddressDataUnion));
  addr->addressKind = BucketL;
  addr->addr.bPointer = st_lookup(name, "Global");
  return addr;
}

static void temporaryNotUsed(){
  int i = 0;
  bool found = false;
  while (i<number_temp && !found){
    if(!vector_temp[i]){
      tmpNumber = i;
      found = true;
    }
    i++;
  }
  if(i>number_temp){
    fprintf(listing, "Insufficient number of temp\n");
    Error = TRUE;
  }
}

static void updateTemporaryVector (int pos){
  vector_temp[pos] = false;
}

static AddressData insertTempData (AddressKind tp){
  AddressData target = (AddressData)malloc(sizeof(struct AddressDataUnion));
  temporaryNotUsed();
  target->addressKind = tp;
  target->addr.nTemp = tmpNumber;
  vector_temp[tmpNumber] = true;
  return target;
}

static AddressData returnAddressData(TreeNode *t){
  AddressData address = (AddressData)malloc(sizeof(struct AddressDataUnion));
  if(t != NULL){
    if(t->nodekind == ExpK){
      if(t->kind.exp == OpK){
        address->addressKind = TempValue;
        address->addr.nTemp = t->attr.temp;
        updateTemporaryVector(address->addr.nTemp);
      }
      else if(t->kind.exp == ConstK){
        address->addressKind = Const;
        address->addr.constant = t->attr.val;
      }
      else if (t->kind.exp == IdK){
        if(t->attr.vector){
          address->addressKind = TempAddr;
          address->addr.nTemp = t->attr.temp;
          updateTemporaryVector(address->addr.nTemp);
        }
        else{
          address->addressKind = BucketL;
          address->addr.bPointer = st_lookup(t->attr.name, t->attr.scope);
        }
      }
    }
    else if (t->nodekind == StmtK){
      if(t->kind.stmt == CallK){
        address->addressKind = TempValue;
        address->addr.nTemp = t->attr.temp;
        updateTemporaryVector(address->addr.nTemp);
      }
    }
    return address;
  }
}

static AddressData insertDataName (TreeNode *t){
  AddressData addr = (AddressData)malloc(sizeof(struct AddressDataUnion));
  addr->addressKind = BucketL;
  addr->addr.bPointer = st_lookup(t->attr.name, t->attr.scope);
  return addr;
}

static AddressData insertParamFunc (TreeNode *t){
  AddressData addr = (AddressData)malloc(sizeof(struct AddressDataUnion));
  BucketList l = st_lookup(t->attr.name, t->attr.scope);
  addr->addressKind = Const;
  addr->addr.constant = l->param->number;
  return addr;
}

static AddressData insertDataConst (int number){
  AddressData addr = (AddressData)malloc(sizeof(struct AddressDataUnion));
  addr->addressKind = Const;
  addr->addr.constant = number;
  return addr;
}

static void insertElementStack (AddressQuadElement addr){
  StackElement newStack = (StackElement)malloc(sizeof(struct StackElementNode));
  newStack->addr = addr;
  if(stack == NULL){
    stack = (Stack)malloc(sizeof(struct StackNode));
    newStack->previous = NULL;
    stack->top = newStack;
  }
  else{
    newStack->previous = stack->top;
    stack->top = newStack;
  }
}

static void removeElementStack (){
  if(stack != NULL){
    if(stack->top->previous == NULL)
      stack = NULL;
    else
      stack->top = stack->top->previous;
  }
}

static void updateStack(AddressData labelSaved){
  if(stack!=NULL){
    stack->top->addr->result = labelSaved;
    removeElementStack();
  }
}

static AddressData insertLabel (){
  AddressData labelAddr = (AddressData)malloc(sizeof(struct AddressDataUnion));
  labelAddr->addressKind = Label;
  //labelAddr->addr.label = labNumber++;
  labelAddr->addr.label = imIntermediateCode;
  //insertThreeAddressQuad(insertNewElementQuad(LabelOP, NULL, NULL, labelAddr, imIntermediateCode++));
  return labelAddr;
}

static AddressData insertTempDataToCall(TreeNode *t){
  BucketList b = st_lookup(t->attr.name, "Global");
  if(b->typeData == Void) return NULL;
  else if(b->typeData == Integer) return insertTempData(TempValue);
}

static void initializeVectorTemp(){
  int i;
  for(i=0; i<number_temp; i++)
    vector_temp[i] = false;
}

/* Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode * tree)
{ TreeNode * p1, * p2, * p3, *aux;
  AddressData addr1, addr2, result;
  AddressData labelReturnSaved;
  switch (tree->kind.stmt) {
      case IfK:
        p1 = tree->child[0];
        p2 = tree->child[1];
        p3 = tree->child[2];
        cGen(p1); /* generate code for test expression */
        addr1 = returnAddressData(p1);
        elementSaved = insertNewElementQuad(IF_fOP, addr1, NULL, NULL, imIntermediateCode++);
        insertThreeAddressQuad(elementSaved);
        insertElementStack(elementSaved);
        cGen(p2); /* recurse on then part */
        if(p3!=NULL){
          elementSaved = insertNewElementQuad(GotoOP, NULL, NULL, NULL, imIntermediateCode++);
          insertThreeAddressQuad(elementSaved);
          updateStack(insertLabel());
          insertElementStack(elementSaved);
          cGen(p3); /* recurse on else part */
        }
        updateStack(insertLabel());
        break;
      case WhileK:
        p1 = tree->child[0];
        p2 = tree->child[1];
        labelReturnSaved = insertLabel();
        cGen(p1);
        addr1 = returnAddressData(p1);
        elementSaved = insertNewElementQuad(IF_fOP, addr1, NULL, NULL, imIntermediateCode++);
        insertThreeAddressQuad(elementSaved);
        insertElementStack(elementSaved);
        cGen(p2);
        insertThreeAddressQuad(insertNewElementQuad(GotoOP, NULL, NULL, labelReturnSaved, imIntermediateCode++));
        updateStack(insertLabel());
        break;
      case CallK:
        p1 = tree->child[0];
        cGen(p1);
        aux = p1;
        while (aux!=NULL) {
          insertThreeAddressQuad(insertNewElementQuad(ParamOP, returnAddressData(aux), NULL, NULL,imIntermediateCode++));
          aux = aux->sibling;
        }
        result = insertTempDataToCall(tree);
        if(result!=NULL) tree->attr.temp = result->addr.nTemp;
        addr1 = insertDataName(tree);
        addr2 = insertParamFunc(tree);
        insertThreeAddressQuad(insertNewElementQuad(CallOP, addr1, addr2, result, imIntermediateCode++));
        break;
      case ReturnK:
        p1 = tree->child[0];
        cGen(p1);
        result = returnAddressData(p1);
        insertThreeAddressQuad(insertNewElementQuad(ReturnOP, NULL, NULL, result, imIntermediateCode++));
        break;
      case AssignK:
        p1 = tree->child[0];
        p2 = tree->child[1];
        cGen(p2);
        cGen(p1);
        result = returnAddressData(p1);
        addr1 = returnAddressData(p2);
        insertThreeAddressQuad(insertNewElementQuad(AssignOP, addr1, NULL, result, imIntermediateCode++));
        break;
      default:
         break;
    }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree)
{ TreeNode * p1, * p2;
  AddressData addr1, addr2, result;
  OperationKind op;
  switch (tree->kind.exp) {
    case OpK:
      p1 = tree->child[0];
      p2 = tree->child[1];
      cGen(p1);
      cGen(p2);
      addr1 = returnAddressData(p1);
      addr2 = returnAddressData(p2);
      result = insertTempData(TempValue);
      tree->attr.temp = result->addr.nTemp;
      switch (tree->attr.op) {
         case PLUS:
            op = SumOP;
            break;
         case MINUS:
            op = SubOP;
            break;
         case TIMES:
            op = MultOP;
            break;
         case OVER:
            op = DivOP;
            break;
         case LT:
            op = LtOP;
            break;
         case LTE:
            op = LteOP;
            break;
         case GT:
            op = GtOP;
            break;
         case GTE:
            op = GteOP;
            break;
         case EQ:
            op = EqOP;
            break;
         case NEQ:
            op = NeqOP;
            break;
         default:
            break;
      } /* case op */
      insertThreeAddressQuad(insertNewElementQuad(op, addr1, addr2, result, imIntermediateCode++));
      break;
    case ConstK:
      break;
    case IdK:
      if(tree->attr.vector){
        p1 = tree->child[0];
        cGen(p1);
        addr1 = insertDataName(tree);
        addr2 = returnAddressData(p1);
        result = insertTempData(TempAddr);
        tree->attr.temp = result->addr.nTemp;
        insertThreeAddressQuad(insertNewElementQuad(SumOP, addr1, addr2, result, imIntermediateCode++));
      }
      break;
    default:
      break;
  }
} /* genExp */

/* Procedure genDecl generates code at an declaration node */
static void genDecl( TreeNode * tree)
{ TreeNode *p;
  switch (tree->kind.decl) {
    case FunK:
      p = tree->child[1];
      insertThreeAddressQuad(insertNewElementQuad(FunctionOP, insertDataName(tree), NULL, NULL, imIntermediateCode++));
      cGen(p);
      if(strcmp(tree->attr.name,"main")==0)
        insertThreeAddressQuad(insertNewElementQuad(HaltOP, NULL, NULL, NULL, imIntermediateCode++));
      else insertThreeAddressQuad(insertNewElementQuad(EndFunctionOP, NULL, NULL, NULL, imIntermediateCode++));
      break;
    case VarK:
      break;
    case ParamK:
      break;
    default:
      break;
  }
} /* genDecl */

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree)
{ if (tree != NULL && !Error)
  { switch (tree->nodekind) {
      case StmtK:
        genStmt(tree);
        break;
      case ExpK:
        genExp(tree);
        break;
      case DeclK:
        genDecl(tree);
        break;
      default:
        break;
    }
    cGen(tree->sibling);
  }
}

static void print_address_data(AddressData addr){
  if(addr!=NULL){
    switch (addr->addressKind) {
      case BucketL:
        fprintf(listing, "%s", addr->addr.bPointer->name);
        break;
      case TempValue:
      case TempAddr:
        fprintf(listing, "t%d", addr->addr.nTemp);
        break;
      case Label:
        fprintf(listing, "(%d)", addr->addr.label);
        break;
      case Const:
        fprintf(listing, "%d", addr->addr.constant);
        break;
      default:
        break;
    }
  }
  else fprintf(listing, "_");
}

static void print_operation (OperationKind op){
  switch (op) {
    case FunctionOP:
      fprintf(listing, "function");
      break;
    case EndFunctionOP:
      fprintf(listing, "end_function");
      break;
    case HaltOP:
      fprintf(listing, "halt");
      break;
    case CallOP:
      fprintf(listing, "call");
      break;
    case LabelOP:
      fprintf(listing, "label");
      break;
    case IF_fOP:
      fprintf(listing, "if_f");
      break;
    case GotoOP:
      fprintf(listing, "goto");
      break;
    case ParamOP:
      fprintf(listing, "param");
      break;
    case ReturnOP:
      fprintf(listing, "return");
      break;
    case AssignOP:
      fprintf(listing, "assign");
      break;
    case SumOP:
      fprintf(listing, "sum");
      break;
    case SubOP:
      fprintf(listing, "sub");
      break;
    case MultOP:
      fprintf(listing, "mult");
      break;
    case DivOP:
      fprintf(listing, "div");
      break;
    case LtOP:
      fprintf(listing, "lt");
      break;
    case LteOP:
      fprintf(listing, "lte");
      break;
    case GtOP:
      fprintf(listing, "gt");
      break;
    case GteOP:
      fprintf(listing, "gte");
      break;
    case EqOP:
      fprintf(listing, "eq");
      break;
    case NeqOP:
      fprintf(listing, "neq");
      break;
    default:
      break;
  }
}

void print_address_quad_element(AddressQuadElement addr){
  fprintf(listing, "[");
  fprintf(listing, "%d", addr->instructionMemory);
  fprintf(listing, "] ");
  fprintf(listing, "(");
  print_operation (addr->op);
  fprintf(listing, ", ");
  print_address_data(addr->addr1);
  fprintf(listing, ", ");
  print_address_data(addr->addr2);
  fprintf(listing, ", ");
  print_address_data(addr->result);
  fprintf(listing, ")\n");
}

static void print_address_quad(){
  AddressQuadElement aux = CodeAddrQ->first;
  while(aux!=NULL){
    print_address_quad_element(aux);
    aux = aux->next;
  }
  fprintf(listing, "\n\n");
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, FILE * codefile)
{  initializeVectorTemp();
   /* generate code for CMinus program */
   cGen(syntaxTree);
   insertMainThreeAddressQuad();
   if(!Error){
     print_address_quad();
     codeGenerationAssembly(CodeAddrQ, codefile);
  }
}
