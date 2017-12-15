
#ifndef _CGEN_H_
#define _CGEN_H_

#include "globals.h"
#include "symtab.h"

#define number_temp 10

typedef enum {BucketL, TempAddr, TempValue, Label, Const} AddressKind;

typedef enum {FunctionOP, EndFunctionOP, HaltOP, CallOP, LabelOP, IF_fOP, GotoOP, ParamOP, ReturnOP, AssignOP, SumOP, SubOP, MultOP, DivOP, LtOP, LteOP, GtOP, GteOP, EqOP, NeqOP} OperationKind;

typedef enum {regT, immediateT, addressIT} OperatingType;

typedef enum {sumAO, sumiAO, subAO, subiAO, multAO, divAO, lwAO, lrAO, loadiAO, swAO, swrAO, andAO, orAO, xorAO, norAO, notAO, sltAO, sleAO, sgtAO, sgeAO, eqAO, neqAO, srlAO, sllAO, beqAO, bnqAO, jAO, jrAO, jalAO, nopAO, hltAO, moveAO, inAO, outAO, delayOutAO, delayNotOutAO, hdToMIAO, RFtoHDAO, HDtoRFAO, HDtoMDAO, dataToHDAO, setMultiprogAO, setAddrCSAO, execProcessAO, setQuantumAO, setPCProcessAO, getPCProcessAO, setProcessAO} AssemblyOperation;

typedef struct OperatingT
  { OperatingType type;
    int op;
  }*Operating;

typedef struct AssemblyList
  { AssemblyOperation ao;
    int memlocI;
    struct OperatingT *result, *op1, *op2;
    struct AssemblyList *next;
  }*Assembly;

/**************************************************/
/*******   Code three address Quadruplets   *******/
/**************************************************/

typedef struct AddressDataUnion
  { AddressKind addressKind;
    union{ BucketList bPointer;
          int nTemp;
          int label;
          int constant;} addr;
  } *AddressData;

typedef struct ThreeAddressQuadElement
  { OperationKind op;
    int instructionMemory;
    struct AddressDataUnion *addr1, *addr2, *result;
    struct ThreeAddressQuadElement *next;
    struct AssemblyList *objectCode;
  } *AddressQuadElement;

typedef struct ThreeAddressQuad //Intermediate code
  { struct ThreeAddressQuadElement *first, *last;
  } *AddressQuad;

typedef struct StackElementNode
  { struct ThreeAddressQuadElement *addr;
    struct StackElementNode *previous;
  } *StackElement;

typedef struct StackNode
  { struct StackElementNode *top;
  } *Stack;

void codeGen(TreeNode * syntaxTree, FILE * codefile);
void print_address_quad_element(AddressQuadElement addr);

#endif
