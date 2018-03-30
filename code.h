
#ifndef _CODE_H_
#define _CODE_H_

#include "symtab.h"
#include "globals.h"
#include "cgen.h"

#define registerZero 0 //Registrador com valor constante zero
#define registerOUT 6 //registrador para impressão
#define registerRV 7 //Registrador para guardar valor de retorno
#define registerTBR_Prog 8 //Início dos registradores do tipo T
#define registerSBR_Prog 14 //Início dos registradores do tipo S
#define registerTBR_SO 20 //Início dos registradores do tipo T
#define registerSBR_SO 26 //Início dos registradores do tipo S
#define num_registers_type 6 //Número de registradores para cada tipo

#define ALU 0
#define LW 1
#define LI 2
#define LR 3
#define SW 4
#define SR 5
#define BEQ 6
#define BNQ 7
#define JUMP 8
#define JR 9
#define NOP 10
#define HLT 11
#define IN 12
#define OUT 13
#define DLY 14
//Novas instruções
#define HD_TRANSFER_MI 15
#define SAVE_RF_HD 16
#define REC_RF_HD 17
#define SAVE_RF_HD_IND 18
#define REC_RF_HD_IND 19
#define SET_MP 20
#define SET_QTM 21
#define SET_ADDRCS 22
#define SET_NUMPROG 23
#define EXEC_PROCESS 24
#define GET_PCPROCESS 25

typedef enum {OTHER, MAIN, INPUT, OUTPUT, DELAY, HD_TRANSF_MI, HD_WRITE,
              HD_READ, SAVE_RF, RECOVERY_RF, SET_MULTIPROG, SET_QUANTUM,
              SET_ADDR_CS, SET_NUM_PROG, EXEC_PROC, GET_PC_PROCESS,
              RETURN_MAIN} Functions;
typedef enum {PARAM1, PARAM2, PARAM3} POS_Key;
//typedef enum {POS_MI, SECTOR, TRACK} POS_Key;

typedef struct{
  char *key;
  Functions val;
} functionMap;

typedef struct {
  int param1; //Sector
  int param2; //Track
  int param3; //MI or Data
  POS_Key key;
} *Param_Funct;


//Mapeamento das funções
static functionMap functionName[]= {
  {"main", MAIN},
  {"input", INPUT},
  {"output", OUTPUT},
  {"delay", DELAY},
  {"HD_Transfer_MI", HD_TRANSF_MI},
  {"HD_Write", HD_WRITE},
  {"HD_Read", HD_READ},
  {"saveRF_in_HD", SAVE_RF},
  {"recoveryRF", RECOVERY_RF},
  {"setMultiprog", SET_MULTIPROG},
  {"setQuantum", SET_QUANTUM},
  {"setAddrCS", SET_ADDR_CS},
  {"setNumProg", SET_NUM_PROG},
  {"execProcess", EXEC_PROC},
  {"getPC_Process", GET_PC_PROCESS},
  {"returnMain", RETURN_MAIN}
};

static int NKEYS = (sizeof(functionName)/sizeof(functionMap));

void codeGenerationAssembly(AddressQuad code, FILE * codefile);

#endif
