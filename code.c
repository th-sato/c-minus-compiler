#include "code.h"

static int nextToBeReplacedRegS;
static int memoryInstruction; //Posição memória de instruções
static BucketList functionActual;
static Param_Funct params_function;

//Shift dos registradores
static int registerTBR;
static int registerSBR;
static int size_registerS;

static void shiftRegistersProg(){
  registerTBR = registerTBR_Prog;
  registerSBR = registerSBR_Prog;
  size_registerS = num_registers_type;
}

static void shiftRegistersSO(){
  registerTBR = registerTBR_SO;
  registerSBR = registerSBR_SO;
  size_registerS = num_registers_type;
}

static void initializeCodeAssembly(){
  shiftRegistersProg();
  memoryInstruction = 0;
  functionActual = NULL;
  params_function = NULL;
}

static void fillCertainFieldsFAT(){
  Program t = fat.p;
  if(t != NULL){
    while(t->next != NULL) t = t->next;
    t->tamMI = memoryInstruction;
    t->tamMD = locationMD;
  }
}

static int keyFromString(char *key){
  int i;
  functionMap element;
  for (int i = 0; i < NKEYS; i++) {
    element = functionName[i];
    if(strcmp(element.key,key) == 0)
      return element.val;
  }
  return OTHER;
}

static void initializeRegisterS(){
  nextToBeReplacedRegS = registerSBR;
}

static int useRegisterS(){
  int pos = nextToBeReplacedRegS;
  if(nextToBeReplacedRegS < registerSBR + (size_registerS - 1)) nextToBeReplacedRegS++;
  else nextToBeReplacedRegS = registerSBR;
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
  int registerCONST = useRegisterS();
  loadi(instructionM, registerCONST, immed);
  storeWord(instructionM, registerCONST, constant);
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

static void in_instruction(AddressQuadElement instruction, int regPos){
  addAssemblyElement(instruction, createElement(inAO, regOperating(regPos), NULL, NULL));
}

static void out_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(outAO, regOperating(registerOUT), NULL, NULL));
  addAssemblyElement(instruction, createElement(delayAO, NULL, NULL, NULL));
}

static void delay_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(delayAO, NULL, NULL, NULL));
}

//MI[$1] = HD[$2][$3]
static void HD_transfer_MI_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(hd_transfer_miAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

//HD[$2][$3] = $1
static void saveRF_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(save_rf_hdAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

//$1 = HD[$2][$3]
static void recoveryRF_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(rec_rf_hdAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

//HD[$2][$3] = RF[$1]
static void saveRF_indirect_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(save_rf_hd_indAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

//RF[$1] = HD[$2][$3]
static void recoveryRF_indirect_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(rec_rf_hd_indAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

static void setMultiprog_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(set_multiprogAO, immediateOperating(params_function->param1), NULL, NULL));
}

static void setQuantum_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(set_quantumAO, regOperating(params_function->param1), NULL, NULL));
}

static void setAddrCS_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(set_addr_csAO, regOperating(params_function->param1), NULL, NULL));
}

static void setNumProg_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(set_num_progAO, regOperating(params_function->param1), NULL, NULL));
}

static void getPCProcess_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(get_pc_processAO, regOperating(params_function->param1), NULL, NULL));
}

static void executeProc_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(exec_procAO, regOperating(params_function->param1), regOperating(params_function->param2), regOperating(params_function->param3)));
}

/*
static void send_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(sendAO, immediateOperating(SOURCE_DATA), regOperating(params_function->param1), regOperating(params_function->param2)));
}*/

static void send_instruction(AddressQuadElement instruction){
  addAssemblyElement(instruction, createElement(sendAO, regOperating(params_function->param1), regOperating(params_function->param2), NULL));
}

static void receive_instruction(AddressQuadElement instruction, int regPos){
  addAssemblyElement(instruction, createElement(receiveAO, regOperating(regPos), NULL, NULL));
}

static void fillParameters(Operating paramOutput, AddressQuadElement element){
  switch (params_function->key) {
    case PARAM1: //MI or Data
      params_function->param1 = useRegisterS();
      assignReg(element, params_function->param1, paramOutput->op);
      break;
    case PARAM2: //SECTOR
      params_function->param2 = useRegisterS();
      assignReg(element, params_function->param2, paramOutput->op);
      break;
    case PARAM3: //TRACK
      params_function->param3 = useRegisterS();
      assignReg(element, params_function->param3, paramOutput->op);
      break;
    default:
      fprintf(listing, "ERROR in fillParameters()\n");
      break;
  }
}

static void instructionMult(AddressQuadElement element, Operating result, Operating op1, Operating op2){ //Somente valores positivos
  int ret;
  int regContador = useRegisterS();
  int regResultado = useRegisterS();
  loadi(element, regContador, 0);
  loadi(element, regResultado, 0);
  ret = memoryInstruction;
  addAssemblyElement(element, createElement(beqAO, op2, immediateOperating(0), addrITOperating(ret+4)));
  addAssemblyElement(element, createElement(sumAO, regOperating(regResultado), regOperating(regResultado), op1));
  addAssemblyElement(element, createElement(sumiAO, regOperating(regContador), regOperating(regContador), immediateOperating(1)));
  addAssemblyElement(element, createElement(bnqAO, regOperating(regContador), op2, addrITOperating(ret)));
  assignReg(element, result->op, regResultado);
  //addAssemblyElement(element, createElement(multAO, result, op1, op2));
}

static void instructionDiv(AddressQuadElement element, Operating result, Operating op1, Operating op2){ //Somente valores positivos
  int ret;
  int regSomador = useRegisterS();
  int regResultado = useRegisterS();
  int regComparador = useRegisterS();
  loadi(element, regSomador, 0);
  loadi(element, regResultado, 0); //Resultado
  ret = memoryInstruction;
  addAssemblyElement(element, createElement(sumAO, regOperating(regSomador), regOperating(regSomador), op2));
  addAssemblyElement(element, createElement(sumiAO, regOperating(regResultado), regOperating(regResultado), immediateOperating(1)));
  addAssemblyElement(element, createElement(sleAO, regOperating(regComparador),regOperating(regSomador), op1));
  addAssemblyElement(element, createElement(bnqAO, regOperating(regComparador), regOperating(registerZero), addrITOperating(ret)));
  addAssemblyElement(element, createElement(subiAO, regOperating(regResultado), regOperating(regResultado), immediateOperating(1)));
  assignReg(element, result->op, regResultado);
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
  int pos = useRegisterS();
  if(b->vector){
    if(isItParameter(b->name)) loadWord(instructionM, pos, b->memloc);
    else loadi(instructionM, pos, b->memloc);
  }
  else loadWord(instructionM, pos, b->memloc);
  return pos;
}

static Operating checkOperator(AddressQuadElement instructionM, AddressData ad){
  int registerCONST;
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
      registerCONST = useRegisterS();
      loadi(instructionM, registerCONST, ad->addr.constant);
      return regOperating(registerCONST);
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
  int registerCONST;
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
          registerCONST = useRegisterS();
          loadi(element, registerCONST, value->op);
          storeReg(element, registerCONST, addr);
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

static void paramSystemCallOrIO(BucketList functionCall, AddressQuadElement element){
  Operating paramOutput = checkOperator(element, element->addr1);
  switch (keyFromString(functionCall->name)) {
    case OUTPUT:
      assignReg(element, registerOUT, paramOutput->op);
      break;
    case HD_TRANSF_MI:
    case HD_WRITE:
    case HD_READ:
    case SAVE_RF:
    case RECOVERY_RF:
    case EXEC_PROC:
    case SEND_DATA:
      fillParameters(paramOutput, element);
      break;
    case SET_MULTIPROG:
    case SET_QUANTUM:
    case SET_ADDR_CS:
    case SET_NUM_PROG:
      params_function->param1 = useRegisterS();
      assignReg(element, params_function->param1, paramOutput->op);
      break;
    /*case RECEIVE_DATA:
    case GET_PC_PROCESS:
    case RETURN_MAIN:
      break;*/
    default:
      fprintf(listing, "Error: Call Function!\n");
      break;
  }
}

static void systemCallOrIO(AddressQuad code, AddressQuadElement element){
  switch (keyFromString(element->addr1->addr.bPointer->name)) {
    case INPUT:
      in_instruction(element, registerTBR + element->result->addr.nTemp);
      break;
    case OUTPUT:
      out_instruction(element);
      break;
    case MAIN:
      jump(element, searchFunction(code, element->addr1->addr.bPointer));
      break;
    case DELAY:
      delay_instruction(element);
      break;
    case HD_TRANSF_MI:
      HD_transfer_MI_instruction(element);
      break;
    case HD_WRITE:
      saveRF_instruction(element);
      break;
    case HD_READ:
      params_function->param1 = registerTBR + element->result->addr.nTemp;
      recoveryRF_instruction(element);
      break;
    case SAVE_RF:
      saveRF_indirect_instruction(element);
      break;
    case RECOVERY_RF:
      recoveryRF_indirect_instruction(element);
      break;
    case SET_MULTIPROG:
      setMultiprog_instruction(element);
      break;
    case SET_QUANTUM:
      setQuantum_instruction(element);
      break;
    case SET_ADDR_CS:
      setAddrCS_instruction(element);
      break;
    case SET_NUM_PROG:
      setNumProg_instruction(element);
      break;
    case EXEC_PROC:
      executeProc_instruction(element);
      break;
    case GET_PC_PROCESS:
      params_function->param1 = registerTBR + element->result->addr.nTemp;
      getPCProcess_instruction(element);
      break;
    case RETURN_MAIN:
      jump(element, 0);
      break;
    case SEND_DATA:
      send_instruction(element);
      break;
    case RECEIVE_DATA:
      receive_instruction(element, registerTBR + element->result->addr.nTemp);
      break;
    default: //Função criado pelo usuário
      storeI(element, memoryInstruction+3, element->addr1->addr.bPointer->memloc);
      jump(element, searchFunction(code, element->addr1->addr.bPointer));
      if(element->result != NULL)
        assignReg(element, registerTBR + element->result->addr.nTemp, registerRV);
      break;
  }
}

static void exitFunction(AddressQuadElement instructionM){
  int registerAR = useRegisterS();
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

static int checkNameFunction(char *name){
  switch (keyFromString(name)) {
    case INPUT:
    case OUTPUT:
    case DELAY:
    case HD_TRANSF_MI:
    case HD_WRITE:
    case HD_READ:
    case SAVE_RF:
    case RECOVERY_RF:
    case SET_MULTIPROG:
    case SET_QUANTUM:
    case SET_ADDR_CS:
    case SET_NUM_PROG:
    case EXEC_PROC:
    case GET_PC_PROCESS:
    case RETURN_MAIN:
    case SEND_DATA:
    case RECEIVE_DATA:
      return 0; //Função criada manualmente
      break;
    default:
      return 1; //Funções do programa
      break;
  }
}

static void correctJumps(AddressQuad code){
  AddressQuadElement element = code->first;
  while (element != NULL) {
    switch (element->op) {
      case CallOP:
        if(checkNameFunction(element->addr1->addr.bPointer->name))
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
    //Instruções adicionadas
    case delayAO: fprintf(listing, "delay"); break;
    case hd_transfer_miAO: fprintf(listing, "hd_transfer_mi"); break;
    case save_rf_hdAO: fprintf(listing, "save_rf_in_hd"); break;
    case rec_rf_hdAO: fprintf(listing, "rec_rf_from_hd"); break;
    case save_rf_hd_indAO: fprintf(listing, "save_rf_in_hd_indirect"); break;
    case rec_rf_hd_indAO: fprintf(listing, "rec_rf_from_hd_indirect"); break;
    case set_multiprogAO: fprintf(listing, "setMultiprog"); break;
    case set_quantumAO: fprintf(listing, "setQuantum"); break;
    case set_addr_csAO: fprintf(listing, "setAddrCS"); break;
    case set_num_progAO: fprintf(listing, "setNumProg"); break;
    case exec_procAO: fprintf(listing, "execProc"); break;
    case get_pc_processAO: fprintf(listing, "getPC_Process"); break;
    case sendAO: fprintf(listing, "send"); break;
    case receiveAO: fprintf(listing, "receive"); break;
    default: fprintf(listing, "ERROR"); break;
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
  fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 5'd0, 6'd%d", ALU, RD, RS, RT, function);
}

void print_arithmetic_operations_immediate(FILE * codefile, int function, int RD, int RS, int immed){
  fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 10'd%d, 6'd%d", ALU, RD, RS, immed, function);
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
      fprintf(codefile, "6'd%d, 5'd%d, 1'b0, 20'd%d", LW, assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case loadiAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd%d", LI, assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case lrAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 16'd0", LR,  assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case swAO:
      fprintf(codefile, "6'd%d, 5'd%d, 1'b0, 20'd%d", SW, assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case swrAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 16'd0", SR, assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case beqAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 16'd%d", BEQ, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case bnqAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 16'd%d", BNQ, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case jAO:
      fprintf(codefile, "6'd%d, 6'd0, 20'd%d", JUMP, assemblyPrint->result->op);
      break;
    case jrAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", JR, assemblyPrint->result->op);
      break;
    case nopAO:
      fprintf(codefile, "6'd%d, 26'd0", NOP);
      break;
    case hltAO:
        fprintf(codefile, "6'd%d, 26'd0", HLT);
      break;
    case inAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", IN, assemblyPrint->result->op);
      break;
    case outAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", OUT, assemblyPrint->result->op);
      break;
    case delayAO:
      fprintf(codefile, "6'd%d, 26'd0", DLY);
      break;
    case hd_transfer_miAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", HD_TRANSFER_MI, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case save_rf_hdAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", SAVE_RF_HD, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case rec_rf_hdAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", REC_RF_HD, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case save_rf_hd_indAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", SAVE_RF_HD_IND, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case rec_rf_hd_indAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", REC_RF_HD_IND, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case set_multiprogAO:
      fprintf(codefile, "6'd%d, 5'd0, 21'd%d", SET_MP, assemblyPrint->result->op);
      break;
    case set_quantumAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", SET_QTM, assemblyPrint->result->op);
      break;
    case set_addr_csAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", SET_ADDRCS, assemblyPrint->result->op);
      break;
    case set_num_progAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", SET_NUMPROG, assemblyPrint->result->op);
      break;
    case exec_procAO:
      fprintf(codefile, "6'd%d, 5'd%d, 5'd%d, 5'd%d, 11'd0", EXEC_PROCESS, assemblyPrint->result->op, assemblyPrint->op1->op, assemblyPrint->op2->op);
      break;
    case get_pc_processAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", GET_PCPROCESS, assemblyPrint->result->op);
      break;
    case sendAO:
      fprintf(codefile, "6'd%d, 5'%d, 5'd%d, 16'd0", SEND, assemblyPrint->result->op, assemblyPrint->op1->op);
      break;
    case receiveAO:
      fprintf(codefile, "6'd%d, 5'd%d, 21'd0", RECEIVE, assemblyPrint->result->op);
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
      fprintf(codefile, "ram[%d] <= {", posSector*1024 + aux2->memlocI);
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
  List paramList = NULL;
  initializeCodeAssembly();
  params_function = (Param_Funct) malloc(sizeof(Param_Funct));
  if(SO == TRUE) shiftRegistersSO();
  else shiftRegistersProg();
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
        if(checkNameFunction(element->addr1->addr.bPointer->name) == 1)
          initializeRegisterS();
        functionCall = NULL;
        paramList = NULL;
        systemCallOrIO(code, element);
        break;
      case IF_fOP: //Ok
         if_instruction(element);
         break;
      case GotoOP: //Ok
        jump(element, element->result->addr.label);
        break;
      case ParamOP: //OK
        if(functionCall == NULL){ //Nova função sendo chamada
          functionCall = foundCall(element); //Determinar a função
          if(strcmp(functionCall->name, "HD_Read") == 0)
            params_function->key = PARAM2;
          else params_function->key = PARAM1;
          paramList = functionCall->param->list; //Guarda a lista de parâmetros
        }
        if(paramList == NULL){ //Função criada manualmente
          if(strcmp(functionCall->name, "setMultiprog") != 0){
            paramSystemCallOrIO(functionCall, element);
            params_function->key++;
          }
          else params_function->param1 = element->addr1->addr.constant;
        }
        else{ //Função criada no próprio código em cminus
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
  if(!BIOS)
    fillCertainFieldsFAT();
}
