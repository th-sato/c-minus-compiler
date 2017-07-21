
#ifndef _CODE_H_
#define _CODE_H_

#include "symtab.h"
#include "globals.h"
#include "cgen.h"

#define size_registerS 10

#define registerZero 0 //Registrador com valor constante zero
#define registerTBR 1 //Início dos registradores do tipo T
#define registerSBR 11 //Início dos registradores do tipo S
#define registerCONST1 21 //registrador para valores constantes
#define registerCONST2 22 //registrador para valores constantes
#define registerMDR 26 //Registrador para multiplicação/divisão (Comparação)
#define registerMD1 27 //Registrador para multiplicação/divisão
#define registerMD2 28 //Registrador para multiplicação/divisão
#define registerINOUT 29 //registrador para impressão
#define registerAR 30 //Registrador para end. retorno
#define registerRV 31 //Registrador para guardar valor de retorno

#define sizeMI 100

typedef struct RegisterSElement
  { bool used;
    BucketList savedVariable;
  } RegisterSElement;

typedef struct RegisterS
  { struct RegisterSElement vector[size_registerS];
    int nextToBeReplaced; //Próximo a ser substituído
  } RegisterS;

void codeGenerationAssembly(AddressQuad code, FILE * codefile);

#endif
