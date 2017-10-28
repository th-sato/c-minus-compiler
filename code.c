#include "code.h"

static int nextToBeReplacedRegS;
static int memoryInstruction = 0; //Posição memória de instruções
static BucketList functionActual = NULL;

static void initializeRegisterS(){
  nextToBeReplacedRegS = 0;
}

static int useRegisterS(){
  int pos = nextToBeReplacedRegS;
  if(nextToBeReplacedRegS < size_registerS - 1) nextToBeReplacedRegS++;
  else nextToBeReplacedRegS = 0;
  return pos;
}

static Operating createOperating(OperatingType type, int op){
  Operating p = (Operating)malloc(sizeof(struct OperatingT));
  p->type = type;
  p->op = op;
  return p;
}

static Operating immediateOperating(int value){
  return createOperating(immediateT, value);
}

static Operating regOperating(int regPos){
  return createOperating(regT, regPos);
}

static Operating addrITOperating(int addrIT){
  return createOperating(addressIT, addrIT);
}

static Assembly createElement(AssemblyOperation ao, Operating result, Operating op1, Operating op2){
  Assembly newElement = (Assembly)malloc(sizeof(struct AssemblyList));
  newElement->ao = ao;
  newElement->result = result;
  newElement->memlocI = memoryInstruction++;
  newElement->op1 = op1;
  newElement->op2 = op2;
  newElement->next = NULL;
  return newElement;
}

static void addAssemblyElement(AddressQuadElement instruction, Assembly newElement){
  Assembly aux = instruction->objectCode;
  if(aux == NULL) instruction->objectCode = newElement;
  else{
    while(aux->next != NULL){
      aux = aux->next;
    }
    aux->next = newElement;
  }
}

//halt: termina processamento
static void instructionHalt(AddressQuadElement instructionM){
  addAssemblyElement(instructionM, createElement(hltAO, NULL, NULL, NULL));
}

//ldi $R, imediato
//loadi: Registrador[pos] = valor imediato
static void loadi (AddressQuadElement instructionM, int registerPos, int value){
  addAssemblyElement(instructionM, createElement(loadiAO, regOperating(registerPos), immediateOperating(value), NULL));
}

//lw $1, CONST	| $1 = Memory[CONST]
static void loadWord (AddressQuadElement instructionM, int registerPosW, int constR){
  addAssemblyElement(instructionM, createElement(lwAO, regOperating(registerPosW), immediateOperating(constR), NULL));
}

//lwr $1, $2	| $1 = Memory[$2]
static void loadReg (AddressQuadElement instructionM, int registerPosW, int registerPosR){
  addAssemblyElement(instructionM, createElement(lrAO, regOperating(registerPosW), regOperating(registerPosR), NULL));
}

//sw $1, CONST	| Memory[CONST] = $1
static void storeWord (AddressQuadElement instructionM, int registerPosR, int posW){
  addAssemblyElement(instructionM, createElement(swAO, regOperating(registerPosR), immediateOperating(posW), NULL));
}

//swi IMEDIATO, CONST	| Memory[CONST] = IMEDIATO
static void storeI (AddressQuadElement instructionM, int immed, int constant){
  loadi(instructionM, registerCONST1, immed);
  storeWord(instructionM, registerCONST1, constant);
}

//swr $1, $2	| Memory[$2] = $1
static void storeReg(AddressQuadElement instruction, int posR, int posW){
  addAssemblyElement(instruction, createElement(swrAO, regOperating(posR), regOperating(posW), NULL));
}

//Registrador[pos1] = Registrador[pos2] + 0
static void assignReg(AddressQuadElement instruction, int pos1, int pos2){
  addAssemblyElement(instruction, createElement(sumiAO, regOperating(pos1), regOperating(pos2), immediateOperating(0)));
}

//jump immediate
static void jump(AddressQuadElement instruction, int immed){
  addAssemblyElement(instruction, createElement(jAO, addrITOperating(immed), NULL, NULL));
}

//jump register[pos]
static void jumpR(AddressQuadElement instruction, int pos){
  addAssemblyElement(instruction, createElement(jrAO, regOperating(pos), NULL, NULL));
}

//beq $1, $2, CONST
static void if_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(beqAO, regOperating(registerTBR + instruction->addr1->addr.nTemp), regOperating(registerZero), addrITOperating(instruction->result->addr.label)));
}

static void in_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(inAO, regOperating(registerINOUT), NULL, NULL));
  addAssemblyElement(instruction, createElement(delayNotOutAO, NULL, NULL, NULL));
}

static void out_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(outAO, regOperating(registerINOUT), NULL, NULL));
  addAssemblyElement(instruction, createElement(delayOutAO, regOperating(registerINOUT), NULL, NULL));
  addAssemblyElement(instruction, createElement(delayNotOutAO, NULL, NULL, NULL));
}

static void saveTempMemory(AddressQuadElement instruction){ //ARRUMAR
  /*int i;
  for(i=0; i<number_temp; i++){
    storeWordSPIncrease(instruction, registerTBR+i, registerSP);
  }*/
}

static void restoreTempBankR(AddressQuadElement instruction){ //ARRUMAR
  /*int i;
  for(i=number_temp-1; i>=0; i--){
    //sp = sp - 1;
    addAssemblyElement(instruction, createElement(subiAO, regOperating(registerSP), regOperating(registerSP), immediateOperating(1)));
    loadWord(instruction, regT+i, registerSP);
  }*/
}

static void instructionMult(AddressQuadElement element, Operating result, Operating op1, Operating op2){ //Somente valores positivos
  int ret;
  loadi(element, registerMD1, 0); //Contador
  loadi(element, registerMD2, 0); //Resultado
  ret = memoryInstruction;
  addAssemblyElement(element, createElement(sumAO, regOperating(registerMD2), regOperating(registerMD2), op1));
  addAssemblyElement(element, createElement(sumiAO, regOperating(registerMD1), regOperating(registerMD1), immediateOperating(1)));
  addAssemblyElement(element, createElement(bnqAO, regOperating(registerMD1), op2, addrITOperating(ret)));
  assignReg(element, result->op, registerMD2);
  //addAssemblyElement(element, createElement(multAO, result, op1, op2));
}

static void instructionDiv(AddressQuadElement element, Operating result, Operating op1, Operating op2){ //Somente valores positivos
  int ret;
  loadi(element, registerMD1, 0);
  loadi(element, registerMD2, 0); //Resultado
  ret = memoryInstruction;
  addAssemblyElement(element, createElement(sumAO, regOperating(registerMD1), regOperating(registerMD1), op2));
  addAssemblyElement(element, createElement(sumiAO, regOperating(registerMD2), regOperating(registerMD2), immediateOperating(1)));
  addAssemblyElement(element, createElement(sltAO, regOperating(registerMDR),regOperating(registerMD1), op1));
  addAssemblyElement(element, createElement(bnqAO, regOperating(registerMDR), regOperating(registerZero), addrITOperating(ret)));
  addAssemblyElement(element, createElement(subiAO, regOperating(registerMD2), regOperating(registerMD2), immediateOperating(1)));
  assignReg(element, result->op, registerMD2);
  //addAssemblyElement(element, createElement(divAO, result, op1, op2));
}

static bool isItParameter(char* paramToCompare){
  List l = functionActual->param->list;
  while(l!=NULL){
    if(strcmp(l->name, paramToCompare)==0)
      return true;
    l = l->next;
  }
  return false;
}

static int checkBucketL (AddressQuadElement instructionM, BucketList b){
  int pos = useRegisterS(b);
  pos += registerSBR;
  if(b->vector){
    if(isItParameter(b->name)) loadWord(instructionM, pos, b->memloc);
    else loadi(instructionM, pos, b->memloc);
  }
  else loadWord(instructionM, pos, b->memloc);
  return pos;
}

static Operating checkOperator(AddressQuadElement instructionM, AddressData ad){
  switch (ad->addressKind) {
    case BucketL:
      return regOperating(checkBucketL(instructionM, ad->addr.bPointer));
      break;
    case TempAddr: //Endereço do vetor
      if(instructionM->addr1 == ad || instructionM->addr2 == ad)
        loadReg(instructionM, registerTBR + ad->addr.nTemp, registerTBR + ad->addr.nTemp);
      return regOperating(registerTBR + ad->addr.nTemp);
      break;
    case TempValue:
      return regOperating(registerTBR + ad->addr.nTemp);
      break;
    case Const:
      if(instructionM->addr1 == ad){
        loadi(instructionM, registerCONST1, ad->addr.constant);
        return regOperating(registerCONST1);
      }
      else if (instructionM->addr2 == ad){
        loadi(instructionM, registerCONST2, ad->addr.constant);
        return regOperating(registerCONST2);
      }
      break;
    default:
      break;
  }
}

static void arithmeticOperations(AddressQuadElement element){
  AssemblyOperation ao;
  Operating op1 = checkOperator(element, element->addr1);
  Operating op2 = checkOperator(element, element->addr2);
  Operating result = checkOperator(element, element->result);
  switch (element->op) {
    case SumOP: ao = sumAO; break;
    case SubOP: ao = subAO; break;
    case LtOP: ao = sltAO; break;
    case LteOP: ao = sleAO; break;
    case GtOP: ao = sgtAO; break;
    case GteOP: ao = sgeAO; break;
    case EqOP: ao = eqAO; break;
    case NeqOP: ao = neqAO; break;
    default: break;
  }
  if(element->op == MultOP)
    instructionMult(element, result, op1, op2);
  else if (element->op == DivOP)
    instructionDiv(element, result, op1, op2);
  else addAssemblyElement(element, createElement(ao, result, op1, op2));
}

//Retornar posição do registrador com o seu valor
static Operating assignValue (AddressQuadElement instructionM, AddressData ad){ //Valor
  int pos;
  switch (ad->addressKind) {
    case BucketL:
      return regOperating (checkBucketL(instructionM, ad->addr.bPointer));
      break;
    case TempAddr:
      pos = registerTBR + instructionM->addr1->addr.nTemp;
      loadReg(instructionM, pos, pos);
      return regOperating(pos);
      break;
    case TempValue:
      return regOperating(registerTBR + ad->addr.nTemp);
      break;
    case Const:
      return immediateOperating(ad->addr.constant);
      break;
    default:
      break;
  }
}

//Retornar posição da memória ou do registrador que contém o endereço
static int assignResult (AddressQuadElement instructionM, AddressData ad){ //Endereço
  switch (ad->addressKind) {
    case BucketL:
      return ad->addr.bPointer->memloc;
      break;
    case TempAddr:
      return registerTBR + ad->addr.nTemp;
      break;
    default:
      break;
  }
}

static void assignInstruction(AddressQuadElement element){
  int addr = assignResult(element, element->result); //Posição na memória
  Operating value = assignValue(element, element->addr1);
  switch (element->result->addressKind) {
    case BucketL:
      switch (value->type) {
        case regT:
          storeWord(element, value->op, addr);
          break;
        case immediateT:
          storeI(element, value->op, addr);
          break;
        default:
          break;
      }
      break;
    case TempAddr:
      switch (value->type) {
        case regT:
          storeReg(element, value->op, addr);
          break;
        case immediateT:
          loadi(element, registerCONST1, value->op);
          storeReg(element, registerCONST1, addr);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static int searchFunction (AddressQuad code, BucketList b){
  AddressQuadElement aux = code->first;
  while(aux != NULL){
    if(aux->op == FunctionOP){
      if(aux->addr1->addr.bPointer == b)
        return aux->instructionMemory;
    }
    aux = aux->next;
  }
}

static BucketList foundCall (AddressQuadElement instructionM){
  AddressQuadElement aux = instructionM;
  while(aux!=NULL){
    if(aux->op == CallOP)
      return aux->addr1->addr.bPointer;
    aux = aux->next;
  }
}

static void writeParamInMemory(AddressQuadElement instructionM, int location){
  switch (instructionM->addr1->addressKind){
    case BucketL:
      storeWord(instructionM, checkBucketL(instructionM, instructionM->addr1->addr.bPointer), location);
      break;
    case TempAddr:
      loadReg(instructionM, registerTBR + instructionM->addr1->addr.nTemp, registerTBR + instructionM->addr1->addr.nTemp);
      storeWord(instructionM, registerTBR + instructionM->addr1->addr.nTemp, location);
      break;
    case TempValue:
      storeWord(instructionM, registerTBR + instructionM->addr1->addr.nTemp, location);
      break;
    case Const:
      storeI(instructionM, instructionM->addr1->addr.constant, location);
      break;
    default:
      break;
  }
}

static void returnInstruction(AddressQuadElement instructionM){
  switch (instructionM->result->addressKind) {
    case BucketL:
      assignReg(instructionM, registerRV, checkBucketL(instructionM, instructionM->result->addr.bPointer));
      break;
    case TempAddr: //Endereço do vetor
      loadReg(instructionM, registerRV, registerTBR + instructionM->result->addr.nTemp);
      break;
    case TempValue:
      assignReg(instructionM, registerRV, registerTBR + instructionM->result->addr.nTemp);
      break;
    case Const:
      loadi(instructionM, registerRV, instructionM->result->addr.constant);
      break;
    default:
      break;
  }
}

static void exitFunction(AddressQuadElement instructionM){
  loadWord(instructionM, registerAR, functionActual->memloc);
  jumpR(instructionM, registerAR);
}

static int changeJump(AddressQuad code, int nNext){
  int i;
  AddressQuadElement aux = code->first;
  for(i = 0; i < nNext; i++)
    aux = aux->next;
  while(aux->objectCode == NULL)
    aux = aux->next;
  return aux->objectCode->memlocI;
}

static void changeJumpCall(AddressQuad code, AddressQuadElement element){
  Assembly aux = element->objectCode;
  while(aux != NULL && aux->ao != jAO)
    aux = aux->next;
  aux->result->op = changeJump(code, aux->result->op);
}

static void correctJumps(AddressQuad code){
  AddressQuadElement element = code->first;
  char *name = NULL;
  while (element != NULL) {
    switch (element->op) {
      case CallOP:
        name = element->addr1->addr.bPointer->name;
        if(strcmp(name, "input")!=0 && strcmp(name, "output")!=0)
          changeJumpCall(code, element);
        break;
      case GotoOP:
        element->objectCode->result->op = changeJump(code, element->result->addr.label);
        break;
      case IF_fOP:
        element->objectCode->op2->op = changeJump(code, element->result->addr.label);
        break;
      default:
        break;
    }
    element = element->next;
  }
}

void print_assembly_operation(AssemblyOperation a){
  switch (a) {
    case sumAO: fprintf(listing, "sum"); break;
    case sumiAO: fprintf(listing, "sumi"); break;
    case subAO: fprintf(listing, "sub"); break;
    case subiAO: fprintf(listing, "subi"); break;
    case multAO: fprintf(listing, "mult"); break;
    case divAO: fprintf(listing, "div"); break;
    case lwAO: fprintf(listing, "lw"); break;
    case lrAO: fprintf(listing, "lr"); break;
    case loadiAO: fprintf(listing, "loadi"); break;
    case swAO: fprintf(listing, "sw"); break;
    case swrAO: fprintf(listing, "swr"); break;
    case andAO: fprintf(listing, "and"); break;
    case orAO: fprintf(listing, "or"); break;
    case xorAO: fprintf(listing, "xor"); break;
    case norAO: fprintf(listing, "nor"); break;
    case notAO: fprintf(listing, "not"); break;
    case sltAO: fprintf(listing, "slt"); break;
    case sleAO: fprintf(listing, "slte"); break;
    case sgtAO: fprintf(listing, "sgt"); break;
    case sgeAO: fprintf(listing, "sge"); break;
    case eqAO: fprintf(listing, "eq"); break;
    case neqAO: fprintf(listing, "neq"); break;
    case srlAO: fprintf(listing, "srl"); break;
    case sllAO: fprintf(listing, "sll"); break;
    case beqAO: fprintf(listing, "beq"); break;
    case bnqAO: fprintf(listing, "bnq"); break;
    case jAO: fprintf(listing, "jump"); break;
    case jrAO: fprintf(listing, "jr"); break;
    case jalAO: fprintf(listing, "jal"); break;
    case nopAO: fprintf(listing, "nop"); break;
    case hltAO: fprintf(listing, "hlt"); break;
    case moveAO: fprintf(listing, "move"); break;
    case inAO: fprintf(listing, "in"); break;
    case outAO: fprintf(listing, "out"); break;
    case delayOutAO: fprintf(listing, "delay out"); break;
    case delayNotOutAO: fprintf(listing, "delay not out"); break;
    default: break;
  }
}

void print_assembly_operating(Operating op){
  if(op!=NULL){
    switch (op->type) {
      case regT:
        fprintf(listing, "R%d", op->op);
        break;
      case immediateT:
        fprintf(listing, "I%d", op->op);
        break;
      case addressIT:
        fprintf(listing, "A:%d", op->op);
        break;
      default: break;
    }
  }
  else fprintf(listing, "_");
}

void print_assembly (AddressQuad code){
  AddressQuadElement aux = code->first;
  Assembly aux2;
  while(aux!=NULL){
    fprintf(listing, "\n");
    print_address_quad_element(aux);
    aux2 = aux->objectCode;
    while(aux2!=NULL){
      fprintf(listing, "[%d] ", aux2->memlocI);
      print_assembly_operation(aux2->ao);
      fprintf(listing, " ");
      print_assembly_operating(aux2->result);
      fprintf(listing, ", ");
      print_assembly_operating(aux2->op1);
      fprintf(listing, ", ");
      print_assembly_operating(aux2->op2);
      fprintf(listing, "\n");
      aux2 = aux2->next;
    }
    aux = aux->next;
  }
}

void print_arithmetic_operations(FILE * codefile, int function, int RD, int RS, int RT){
  fprintf(codefile, "6'd0, 5'd%d, 5'd%d, 5'd%d, 5'bxxxxx, 6'd%d", RD, RS, RT, function);
}

void print_arithmetic_operations_immediate(FILE * codefile, int function, int RD, int RS, int immed){
  fprintf(codefile, "6'd0, 5'd%d, 5'd%d, 10'd%d, 6'd%d", RD, RS, immed, function);
}

void print_instruction(Assembly assemblyPrint, FILE * codefile){
  switch (assemblyPrint->ao) {
    case sumAO:
      print_arithmetic_operations(codefile, 0, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case sumiAO:
      print_arithmetic_operations_immediate(codefile, 1, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case subAO:
      print_arithmetic_operations(codefile, 2, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case subiAO:
      print_arithmetic_operations_immediate(codefile, 3, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case sltAO:
      print_arithmetic_operations(codefile, 11, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case sleAO:
      print_arithmetic_operations(codefile, 12, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case sgtAO:
      print_arithmetic_operations(codefile, 13, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case sgeAO:
      print_arithmetic_operations(codefile, 14, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case eqAO:
      print_arithmetic_operations(codefile, 15, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case neqAO:
      print_arithmetic_operations(codefile, 16, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case multAO:
      print_arithmetic_operations(codefile, 17, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case divAO:
      print_arithmetic_operations(codefile, 18, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case lwAO:
      fprintf(codefile, "6'd1, 5'd%d, 1'bx, 20'd%d", assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case loadiAO:
      fprintf(codefile, "6'd2, 5'd%d, 21'd%d", assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case lrAO:
      fprintf(codefile, "6'd3, 5'd%d, 5'd%d, 16'd0", assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case swAO:
      fprintf(codefile, "6'd4, 5'd%d, 1'bx, 20'd%d", assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case swrAO:
      fprintf(codefile, "6'd5, 5'd%d, 5'd%d, 16'd0", assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case beqAO:
      fprintf(codefile, "6'd6, 5'd%d, 5'd%d, 16'd%d", assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case bnqAO:
      fprintf(codefile, "6'd7, 5'd%d, 5'd%d, 16'd%d", assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case jAO:
      fprintf(codefile, "6'd8, 6'bxxxxxx, 20'd%d", assemblyPrint->result->op);
      break;
    case jrAO:
      fprintf(codefile, "6'd9, 5'd%d, 21'd0", assemblyPrint->result->op);
      break;
    case nopAO:
      fprintf(codefile, "6'd10, 26'd0");
      break;
    case hltAO:
      fprintf(codefile, "6'd11, 5'd%d, 21'd0", registerINOUT);
      break;
    case inAO:
      fprintf(codefile, "6'd12, 5'd%d, 21'd0", assemblyPrint->result->op);
      break;
    case outAO:
      fprintf(codefile, "6'd13, 5'd%d, 21'd0", assemblyPrint->result->op);
      break;
    case delayOutAO:
      fprintf(codefile, "6'd14, 5'd%d, 5'd0, 10'd450, 6'd0", assemblyPrint->result->op);
      break;
    case delayNotOutAO:
      fprintf(codefile, "6'd15, 10'd0, 10'd450, 6'd0");
      break;
    default:
      fprintf(listing, "Error");
      break;
  }
}

void print_objectCode(AddressQuad code, FILE * codefile){
  AddressQuadElement aux = code->first;
  Assembly aux2;
  while(aux!=NULL){
    aux2 = aux->objectCode;
    while(aux2!=NULL){
      fprintf(codefile, "instructionM[%d] <= {", aux2->memlocI);
      print_instruction(aux2, codefile);
      fprintf(codefile, "};\n");
      aux2 = aux2->next;
    }
    aux = aux->next;
  }
}

void codeGenerationAssembly(AddressQuad code, FILE * codefile){
  AddressQuadElement element = code->first;
  BucketList functionCall = NULL;
  Operating paramOutput = NULL;
  List paramList = NULL;
  while(element != NULL && !Error){
    switch (element->op) {
      case FunctionOP: //OK
        functionActual = element->addr1->addr.bPointer;
        initializeRegisterS();
        break;
      case EndFunctionOP: //OK
        exitFunction(element);
        functionActual = NULL;
        break;
      case HaltOP: //OK
        instructionHalt(element);
        break;
      case CallOP://OK
        initializeRegisterS();
        functionCall = NULL;
        paramList = NULL;
        if(strcmp(element->addr1->addr.bPointer->name, "input")==0){
          in_instruction(element);
          assignReg(element, registerTBR + element->result->addr.nTemp, registerINOUT);
        }
        else if (strcmp(element->addr1->addr.bPointer->name, "output")==0)
          out_instruction(element);
        else if(strcmp(element->addr1->addr.bPointer->name, "main")!=0){
          storeI(element, memoryInstruction+3, element->addr1->addr.bPointer->memloc);
          jump(element, searchFunction(code, element->addr1->addr.bPointer));
          if(element->result != NULL)
            assignReg(element, registerTBR + element->result->addr.nTemp, registerRV);
        }
        else jump(element, searchFunction(code, element->addr1->addr.bPointer));
        break;
      case IF_fOP: //Ok
         if_instruction(element);
         break;
      case GotoOP: //Ok
        jump(element, element->result->addr.label);
        break;
      case ParamOP: //OK
        if(functionCall == NULL){
          functionCall = foundCall(element);
          paramList = functionCall->param->list;
        }
        if(paramList == NULL){
          if(strcmp(functionCall->name, "output")==0){
            paramOutput = checkOperator(element, element->addr1);
            assignReg(element, registerINOUT, paramOutput->op);
          }
          else fprintf(listing, "Error: Call Function!\n");
        }
        else{
          writeParamInMemory(element, paramList->location);
          paramList = paramList->next;
        }
        break;
      case ReturnOP: //Ok
        returnInstruction(element);
        exitFunction(element);
        break;
      case AssignOP: //Ok
        assignInstruction(element);
        break;
      case SumOP:
      case SubOP:
      case MultOP:
      case DivOP:
      case LtOP:
      case LteOP:
      case GtOP:
      case GteOP:
      case EqOP:
      case NeqOP:
        arithmeticOperations(element);
        break;
      default:
        break;
    }
    element = element->next;
  }
  correctJumps(code);
  print_assembly(code);
  print_objectCode(code, codefile);
}
