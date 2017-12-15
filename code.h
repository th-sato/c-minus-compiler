
#ifndef _CODE_H_
#define _CODE_H_

#include "symtab.h"
#include "globals.h"
#include "cgen.h"

#define registerZero 0 //Registrador com valor constante zero
#define registerTBR_Prog 1 //Início dos registradores do tipo T
#define registerSBR_Prog 11 //Início dos registradores do tipo S
#define registerINOUT 20 //registrador para impressão
#define registerRV 21 //Registrador para guardar valor de retorno
#define size_registerS_programs 10

//Context Switch
#define registerTBR_CS 22
#define registerSBR_CS 26
#define size_registerS_CS 6

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
#define DLY_OUT 14
#define DLY_NOT_OUT 15
#define END_BIOS 16
#define HD_LOAD_INST_MEM 17
#define RF_to_HD 18
#define HD_to_RF 19
#define HD_to_MD 20
#define WRITE_DATA_HD 21
#define SET_MULTIPROG 22
#define SET_QUANTUM 23
#define SET_ADDR_CS 24
#define SET_PC_PROCESS 25
#define GET_PC_PROCESS 26
#define EXEC_PROCESS_X 27
#define SET_PROCESS 28

typedef enum {OTHER, MAIN, INPUT, OUTPUT, HDtoMI, RFtoHD, HDtoRF, HDtoMD,
              dataToHD, setMultiprog, setAddrCS, executeProc, setQuantum,
              getPCProcess, returnMain, setProcess} Functions;
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
  {"HDtoMI", HDtoMI},
  {"RFtoHD", RFtoHD},
  {"HDtoRF", HDtoRF},
  {"HDtoMD", HDtoMD},
  {"dataToHD", dataToHD},
  {"setMultiprogramming", setMultiprog},
  {"setAddrContextSwitch", setAddrCS},
  {"executeProcess", executeProc},
  {"setQuantum", setQuantum},
  {"getPCProcess", getPCProcess},
  {"returnMain", returnMain},
  {"setProcess", setProcess}
};

static int NKEYS = (sizeof(functionName)/sizeof(functionMap));

void codeGenerationAssembly(AddressQuad code, FILE * codefile);

#endif
