#include "globals.h"

#include "util.h"
#include "parse.h"
#include "analyze.h"
#include "cgen.h"

/* allocate global variables */
int lineno = 0;
FILE * source;
FILE * listing;

//vezes 1024 --> desloca 10 casas decimais

/* allocate and set tracing flags */
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = FALSE;
int TraceAnalyze = TRUE;
int TraceCode = TRUE;

int Error = FALSE;

int BIOS = FALSE;
int SO = FALSE;
int posSector;
int locationMD;
FAT fat;

static void fillElementFAT(int i){
  Program aux, t = fat.p;
  if(t != NULL){
    while(t->next != NULL) t = t->next;
    aux = (Program) malloc(sizeof(struct program));
    aux->usage = 1;
    aux->name = i - 1;
    aux->inicio = 1024*(i + 1);
    aux->next = NULL;
    t->next = aux;
  } else{
    fat.p = (Program) malloc(sizeof(struct program));
    fat.p->usage = 1;
    fat.p->name = i;
    fat.p->inicio = 1024*(i + 1);
    fat.p->next = NULL;
  }
}

static void reexecuteCompiler(int i){
  Program aux;
  fillElementFAT(i);
  lineno = 0;
  posSector = i + 1;
  fat.qtd_prog++;
  fat.prox_pos = 1024*(i+2);
}

static void initializeFAT(){
  fat.qtd_prog = 0;
  fat.tam_prog = 1024;
  fat.prox_pos = 1024;
  fat.p = NULL;
}

static void readFile(char* name){
  source = fopen(name, "r");
  fprintf(listing,"\nC- COMPILATION: %s\n",name);
  if (source==NULL){
    fprintf(stderr,"File %s not found\n", name);
    exit(1);
  }
}

static FILE * writeToFile(char *name){
  FILE * code;
  if (! Error){
    code = fopen(name,"w");
    if (code == NULL)
    { printf("Unable to open %s\n",name);
      exit(1);
    }
  }
  return code;
}

static TreeNode * syntaxTreeParse(){
  TreeNode *syntaxTree = parse();
  if (! Error){
    if (TraceParse) {
      fprintf(listing,"\nSyntax tree:\n");
      printTree(syntaxTree);
    }
    if (TraceAnalyze) fprintf(listing,"\nBuilding Symbol Table...\n");
    buildSymtab(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nChecking Types...\n");
    typeCheck(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nType Checking Finished\n");
  }
  return syntaxTree;
}

static void generateCode(TreeNode *syntaxTree, FILE * codefile){
  if (! Error)
    codeGen(syntaxTree, codefile);
}

static void compilerSato(char *name, FILE * codefile){
  TreeNode *syntaxTree;
  readFile(name);
  syntaxTree = syntaxTreeParse();
  locationMD = getLocation();
  generateCode(syntaxTree, codefile);
  fclose(source);
}

static void FATinHD(FILE *codefile){
  Program aux = fat.p;
  int cont = 0, programa = 0;
  fprintf(codefile, "//TAB\n\n");
  fprintf(codefile, "ram[%d] = %d; //QTD_PROG\n", cont++, fat.qtd_prog);
  fprintf(codefile, "ram[%d] = %d; //TAM_PROG\n", cont++, fat.tam_prog);
  fprintf(codefile, "ram[%d] = %d; //PROX_POS\n\n", cont++, fat.prox_pos);
  while(aux != NULL){
    fprintf(codefile, "//PROGRAM %d\n", programa++);
    fprintf(codefile, "ram[%d] = %d; //USO\n", cont++, aux->usage);
    fprintf(codefile, "ram[%d] = %d; //NOME\n", cont++, aux->name);
    fprintf(codefile, "ram[%d] = %d; //INICIO\n", cont++, aux->inicio);
    fprintf(codefile, "ram[%d] = %d; //BASE MI\n", cont++, aux->tamMI);
    fprintf(codefile, "ram[%d] = %d; //BASE MD\n\n", cont++, aux->tamMD);
    aux = aux->next;
  }
}

static void compilerPrograms (int N, FILE * codefile){ //Quantidade de programas
  int i;
  char file[120];
  char name[20][120] = {"buscaBinaria", "fatorial", "fibonacci", "mdc",
                        "media", "mediana", "mmc", "palindromo",
                        "potencia", "selectionSort"};
  for(i = 0; i < N; i++){
    fprintf(codefile, "//Algorithm: %s\n\n", name[i]);
    reexecuteCompiler(i+2);
    strcpy(file, "Algoritmos/");
    strcat(file, name[i]);
    strcat(file, ".cm");
    compilerSato(file, codefile);
    fprintf(codefile, "\n\n\n\n");
  }
}

int main( int argc, char * argv[] )
{
  FILE * codefile;
  char fileRead[120];
  char * fileWrite;
  int fnlen;
  listing = stdout; /* send listing to screen */
  initializeFAT();
  switch (argc) {
    case 1:
      //Gerar código da BIOS
      BIOS = TRUE;
      codefile = writeToFile("SO/BIOS.sato");
      fprintf(codefile, "//BIOS\n\n");
      compilerSato("SO/BIOS.cm", codefile);
      fclose(codefile);
      BIOS = FALSE;
      //Gerar código do Sistema Operacional
      /*SO = TRUE;
      reexecuteCompiler(0);
      codefile = writeToFile("SO/SO.sato");
      fprintf(codefile, "//SO\n\n");
      compilerSato("SO/SO.cm", codefile);
      //fclose(codefile);
      SO = FALSE;*/
      SO = TRUE;
      reexecuteCompiler(0);
      codefile = writeToFile("Algoritmos/hd.sato");
      fprintf(codefile, "//SO\n\n");
      compilerSato("SO/SO.cm", codefile);
      fprintf(codefile, "\n\n\n\n");
      //fclose(codefile);
      SO = FALSE;
      //Gerar código do HD
      compilerPrograms(10, codefile);
      FATinHD(codefile);
      //fclose(codefile);
      break;
    case 2:
      //Arquivo de leitura
      strcpy(fileRead, argv[1]);
      if(strchr(fileRead, '.') == NULL)
        strcat(fileRead,".cm");
      //Arquivo de escrita
      fnlen = strcspn(fileRead, ".");
      fileWrite = (char*)calloc(fnlen+6, sizeof(char));
      strncpy(fileWrite, fileRead, fnlen);
      strcat(fileWrite, ".sato");
      //Abrir arquivo de escrita
      codefile = writeToFile(fileWrite);
      //Leitura do arquivo de escrita e geração do código objeto
      compilerSato(fileRead, codefile);
      fclose(codefile);
      break;
    default:
      exit(1);
      break;
  }
  return 0;
}
