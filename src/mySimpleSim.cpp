
/*

The project is developed as part of Computer Architecture class
Project Name: Functional/Pipeline Simulator for simpleRISC Processor

Developer's Name: Kumar Ayush
Developer's Email id: 2015eeb1060@iitrpr.ac.in
Date: 16.10.2016

*/

/* mySimpleSim.cpp
   Purpose of this file: implementation file for mySimpleSim
*/

#include "mySimpleSim.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <deque>
#include <math.h>

using namespace std;
//Register file
static unsigned int R[16];
//memory
static unsigned char MEM[4000];
int PC;
int gt,eq;
bool hasConflict(unsigned int A, unsigned int B);
bool branchOccur(unsigned int instruction);
class Buffer {
public:
bool hasData;

//flags

//intermediate datapath and control path signals
unsigned int instruction_word;
unsigned int operand1;
unsigned int operand2;

unsigned int aluResult;
unsigned int ldResult;

unsigned int imm;
unsigned int immx;
int isSt;
int isLd;
int isBeq;
 int isBgt;
 int isRet;
 int isImmediate;
 int isWb;
 int isUBranch;
 int isCall;
 int isAdd;
 int isSub;
 int isCmp;
 int isMul;
 int isDiv;
 int isMod;
 int isLsl;
 int isLsr;
 int isAsr;
 int isOr;
 int isAnd;
 int isNot;
 int isMov;
 int isBranchTaken;
int branchTarget;
unsigned int branchPC;
void setZero();
};
void Buffer::setZero() {
  branchTarget = 0;
  branchPC = 0;
isSt=0;
isLd=0;
isBeq=0;
isBgt=0;
isRet=0;
isImmediate=0;
isWb=0;
isUBranch=0;
isCall=0;
isAdd=0;
isSub=0;
isCmp=0;
isMul=0;
isDiv=0;
isMod=0;
isLsl=0;
isLsr=0;
isAsr=0;
isOr=0;
isAnd=0;
isNot=0;
isMov=0;
isBranchTaken=0;
hasData = 0;
}
Buffer *IFOF = new Buffer();
Buffer *OFEX = new Buffer();
Buffer *EXMA = new Buffer();
Buffer *MARW = new Buffer();
void run_simplesim() {
  PC = -4;
  int window = 1;
  IFOF->setZero();
  OFEX->setZero();
  EXMA->setZero();
  MARW->setZero();
  deque<unsigned int> prevIns;
  int cases=1;
  int valFetch = 1;
  int completion[5];
  int nop = 0;
  while(1) {
    fill_n(completion, 5, 0);
    if (MARW->hasData){
      write_back();
      completion[0] = 1;
    }

    if (!nop) {
      if (MARW->isBranchTaken) {
        PC = MARW->branchPC;
        valFetch = 1;
        PC -= 4;
        MARW->setZero();
      } else if (valFetch) {
        PC += 4;
      }
    }

    if (MARW->hasData) {
      MARW->setZero();
    }

    if (EXMA->hasData) {
      mem();
      completion[1] = 1;
      EXMA->setZero();
    }

    if (OFEX->hasData ) {
      execute();
      completion[2] = 1;
      if (EXMA->isBranchTaken) {
        nop = 3;
        IFOF->setZero();
      }
      OFEX->setZero();
    }

    if (IFOF->hasData && !nop) {
      decode();
      completion[3] = 1;
      IFOF->setZero();
    }

    if (valFetch && !nop && PC >= 0) {
      fetch();
      completion[4] = 1;
    }
    if (PC < 0) {
      valFetch = 0;
    }
    if ((IFOF->instruction_word == -1 || IFOF->instruction_word == 0xffffffff) && !nop) {
      valFetch = 0;
      if (prevIns.size() > 0) {
        prevIns.pop_front();
      }
    } else if (!nop) {
      for (int i = prevIns.size() - 1; i >= 0; --i) {
        if(hasConflict(IFOF->instruction_word, prevIns[i])) {
          nop = 4 - (prevIns.size() - 1 - i);
          break;
        }
      }
      if (prevIns.size() == 3) {
        prevIns.pop_front();
      }
      prevIns.push_back(IFOF->instruction_word);
    }
    if (nop && prevIns.size() == 3 && window) {
      prevIns.pop_front();
      window = 0;
    } else if (nop && prevIns.size() > 0) {
      prevIns.pop_front();
    }

    if (nop) {
      prevIns.push_back(0x68000000);
    }
    nop--;
    nop = max(nop,0);

    if (!completion[0] && !completion[1] && !completion[2] && !completion[3] &&
       !completion[4] && !valFetch && !nop) {
         break;
    }

    cout<<cases<<"   "<<nop<<"  "<<endl;
    printf("%x   %d\n",IFOF->instruction_word,PC);
    /*
    for (int i = 0; i < 5; ++i) {          ///printing register values
      cout<<"Register value at "<<i<<" is "<<R[i]<<endl;
    }
    cout<<endl<<endl;
    */
    cases++;
  }
  for (int i = 0; i < 16; ++i) {          ///printing register values
    cout<<"Register value at "<<i<<" is "<<R[i]<<endl;
  }
  write_data_memory ();
}

// it is used to set the reset values
//reset all registers and memory content to 0
void reset_proc() {
    PC = 0;
    gt = 0;
    eq = 0;
    for(int i = 0;i < 16;++i) {     /////making all registees vakue 0
      R[i] = 0;
    }
    R[14]=4000-8;
    unsigned int minuso = -1;
    for(int i = 0;i < 4000;i += 1) {   //////initialising memory to zero
      write_word(MEM,i,-1);
    }
}

//load_program_memory reads the input memory, and pupulates the instruction
// memory
void load_program_memory(char *file_name) {
  FILE *fp;
  unsigned int address, instruction;
  fp = fopen(file_name, "r");
  if(fp == NULL) {
    printf("Error opening input mem file\n");
    exit(1);
  }
  while(fscanf(fp, "%x %x", &address, &instruction) != EOF) {
    write_word(MEM, address, instruction);
  }
  fclose(fp);
}

//writes the data memory in "data_out.mem" file
void write_data_memory() {
  FILE *fp;
  unsigned int i;
  fp = fopen("data_out.mem", "w");
  if(fp == NULL) {
    printf("Error opening dataout.mem file for writing\n");
    return;
  }

  for(i=0; i < 4000; i = i + 4){
    fprintf(fp, "%x %x\n", i, read_word(MEM, i));
  }
  fclose(fp);
}

//reads from the instruction memory and updates the instruction register
void fetch() {
  IFOF->instruction_word = read_word(MEM,PC);
  IFOF->hasData = 1;
}
//reads the instruction register, reads operand1, operand2 fromo register file, decides the operation to be performed in execute stage
void decode() {
  unsigned int instruction_word = IFOF->instruction_word;
  OFEX->instruction_word = instruction_word;
  OFEX->imm = instruction_word & 0xFFFF;
  OFEX->immx = OFEX->imm;
  unsigned int u = (instruction_word & 0x10000) >> 16;
  unsigned int h = (instruction_word & 0x20000) >> 17;
  if (h) {         /////calculating immx
    OFEX->immx = OFEX->imm << 16;
  } else if (u == 0 && ((OFEX->imm & 0x8000) >> 15)) {
    OFEX->immx = OFEX->imm | 0xFFFC0000;
  }
  OFEX->branchTarget = (instruction_word & 0x7FFFFFF) << 2;    ////calculating BRANCHTARGHET
  if ((OFEX->branchTarget & 0x10000000) >> 28) {
    OFEX->branchTarget += 0x1E0000000;
  }
  OFEX->branchTarget += PC;

  unsigned int opcode = (instruction_word & 0xF8000000) >> 27;

  OFEX->isRet = (opcode == 0x14);
  OFEX->isSt = (opcode == 0xF);
  if(OFEX->isRet) {               /////initialising operand1 and operand2
    OFEX->operand1 = R[15];
  } else {
    OFEX->operand1 = R[((instruction_word & 0x3C0000) >> 18)];
  }
  if(!(OFEX->isSt)) {
    OFEX->operand2 = R[((instruction_word & 0x3C000) >> 14)];
  } else {
    OFEX->operand2 = R[((instruction_word & 0x3C00000) >> 22)];
  }
  IFOF->setZero();
  OFEX->hasData = 1;
}
//executes the ALU operation based on ALUop
void execute() {

  unsigned int instruction_word = OFEX->instruction_word;
  EXMA->instruction_word = instruction_word;
  unsigned int opcode = (instruction_word & 0xF8000000) >> 27;
  EXMA->isRet = OFEX->isRet;
  EXMA->isSt = OFEX->isSt;
  if(OFEX->isRet) {               /////initialising operand1 and operand2
    EXMA->operand1 = R[15];
  } else {
    EXMA->operand1 = R[((instruction_word & 0x3C0000) >> 18)];
  }
  if(!(OFEX->isSt)) {
    EXMA->operand2 = R[((instruction_word & 0x3C000) >> 14)];
  } else {
    EXMA->operand2 = R[((instruction_word & 0x3C00000) >> 22)];
  }
  if(OFEX->isRet) {
    EXMA->branchPC = EXMA->operand1;
  } else {
    EXMA->branchPC = OFEX->branchTarget;
  }
  EXMA->isBeq = opcode == 0x10;     ////////////initialising CONTROL SIGNALS
  EXMA->isCmp = opcode == 0x5;
  EXMA->isBgt = opcode == 0x11;
  EXMA->isLd = opcode == 0xE;
  EXMA->isImmediate = (instruction_word & 0x4000000) >> 26;
  EXMA->isCall = opcode == 19;
  EXMA->isAdd = (opcode == 0) || (opcode == 14) || (opcode == 15);
  EXMA->isSub = opcode == 0x1;
  EXMA->isCmp = opcode == 0x5;
  EXMA->isMul = opcode == 0x2;
  EXMA->isDiv = opcode == 0x3;
  EXMA->isMod = opcode == 0x4;
  EXMA->isLsl = opcode == 0xA;
  EXMA->isLsr = opcode == 0xB;
  EXMA->isAsr = opcode == 0xC;
  EXMA->isOr = opcode == 0x7;
  EXMA->isAnd = opcode == 0x6;
  EXMA->isNot = opcode == 0x8;
  EXMA->isMov = opcode == 0x9;
  EXMA->isWb = (opcode==0) || (EXMA->isSub) || (EXMA->isMul) || (EXMA->isDiv) ||
   (EXMA->isMod) || (EXMA->isAnd) || (EXMA->isOr) || (EXMA->isNot) ||
   (EXMA->isMov) || (EXMA->isLd) || (EXMA->isLsl) || (EXMA->isLsr)
    || (EXMA->isAsr) || (EXMA->isCall);  ///////BASED ON FORMULA OF isWb

  EXMA->isUBranch = (opcode == 18) || (opcode == 19) || (opcode == 20);

  unsigned int op1 = EXMA->operand1;
  unsigned int op2;
  if (EXMA->isImmediate) {
    op2 = OFEX->immx;
  } else {
    op2 = EXMA->operand2;
  }

  //EXMA->operand2 = OFEX->operand2;
  /////////////ALU////////
  if(EXMA->isAdd) {
    EXMA->aluResult = op1 + op2;
  } else if (EXMA->isSub) {
    EXMA->aluResult = op1 - op2;
  } else if (EXMA->isMul) {
    EXMA->aluResult = op1 * op2;
  } else if (EXMA->isDiv) {
    EXMA->aluResult = op1 / op2;
  } else if (EXMA->isMod) {
    EXMA->aluResult = op1 % op2;
  } else if (EXMA->isLsl) {
    EXMA->aluResult = op1 << op2;
  } else if (EXMA->isLsr) {
    EXMA->aluResult = op1 >> op2;
  } else if(EXMA->isAsr) {
    EXMA->aluResult = (unsigned int)((signed int)op1 >> op2);
  } else if (EXMA->isOr) {
    EXMA->aluResult = op1 | op2;
  } else if (EXMA->isAnd) {
    EXMA->aluResult = op1 & op2;
  } else if (EXMA->isNot) {
    EXMA->aluResult = !(op2);
  } else if (EXMA->isMov) {
    EXMA->aluResult = op2;
  } else if(EXMA->isCmp) {
    if (op1 == op2) {
      eq = 1;
    } else {
      eq = 0;
    }
    if (op1 > op2) {
      gt = 1;
    } else {
      gt = 0;
    }
  }
  if ((EXMA->isBeq && eq) || (EXMA->isBgt && gt) || EXMA->isUBranch) {
    EXMA->isBranchTaken = 1;
  }
OFEX->setZero();
EXMA->hasData = 1;
}
//perform the memory operation
void mem() {
  MARW->isBranchTaken = EXMA->isBranchTaken;
  MARW->instruction_word = EXMA->instruction_word;
  MARW->isWb = EXMA->isWb;
  MARW->isCall = EXMA->isCall;
  MARW->isLd = EXMA->isLd;
  MARW->isCall = EXMA->isCall;
  MARW->aluResult = EXMA->aluResult;
  MARW->branchPC = EXMA->branchPC;
  if (EXMA->isLd) {
    MARW->ldResult = read_word (MEM, EXMA->aluResult+500);    //////////500 ASSIGNS DIFFERNENT MEMORY BLOCK FOR LOAD AND STORE
  } else if (EXMA->isSt) {
    write_word (MEM, EXMA->aluResult+500, EXMA->operand2);
  }
  MARW->hasData = 1;
EXMA->setZero();
}
//writes the results back to register file
void write_back() {
  int sourceOperand;
  if (!MARW->isLd && !MARW->isCall) {
    sourceOperand = MARW->aluResult;
    //cout<<"Here  "<<MARW->aluResult<<endl;
  } else if (MARW->isLd && (!MARW->isCall)) {
    sourceOperand = MARW->ldResult;
  } else {
    sourceOperand = PC + 4;
  }
  if (MARW->isWb) {
    if (MARW->isCall) {
      R[15] = sourceOperand;      /////////RETIURN ADDRESS
    } else {
      R[((MARW->instruction_word & 0x3C00000) >> 22)] = sourceOperand;
      //cout<<"SO  "<<sourceOperand<<endl;
    }
  }
MARW->hasData = 0;
}
bool hasConflict(unsigned int A, unsigned int B) {
  unsigned int opcodeA = (A & 0xF8000000) >> 27;
  unsigned int opcodeB = (B & 0xF8000000) >> 27;
  int isBeq = opcodeA == 0x10;
  int isBgt = opcodeA == 0x11;
  int isCall = opcodeA == 19;
  int isB = opcodeA == 18;
  int isNop = opcodeA == 13;
  if (isBeq || isBgt || isCall || isB || isNop) {
    return false;
  }
  isBeq = opcodeB == 0x10;
  isBgt = opcodeB == 0x11;
  isB = opcodeB == 18;
  int isRet = opcodeB == 20;
  int isSt = opcodeB == 15;
  int isCmp = opcodeB == 5;
  isNop = opcodeB == 13;
  if (isBeq || isBgt || isB || isRet || isSt || isCmp || isNop) {
    return false;
  }
  isSt = opcodeA == 15;
  isRet = opcodeA == 20;
  isCall = opcodeB == 19;
  unsigned int src1 = ((A & 0x3C0000) >> 18);
  unsigned int src2 = ((A & 0x3C000) >> 14);
  if (isSt) {
    src1 = ((A & 0x3C00000) >> 22);
  }
  if (isRet) {
    src2 = R[15];
  }
  unsigned int dest = ((B & 0x3C00000) >> 22);
  if (isCall) {
    dest = R[15];
  }
  bool hasSrc2 = true;
  if (!isSt && ((A & 0x4000000) >> 26)) {
    hasSrc2 = false;
  }
  int isMov = opcodeA == 0x9;
  int isNot = opcodeA == 0x8;
  if (src1 == dest && !isMov && !isNot) {
    return true;
  } else if (hasSrc2 == true && src2 == dest) {
    return true;
  }
  return false;
}
bool branchOccur(unsigned int instruction) {
  unsigned int opcode = (instruction & 0xF8000000) >> 27;
  int isUBranch = (opcode == 18) || (opcode == 19) || (opcode == 20);
  int isBeq = opcode == 0x10;
  int isBgt = opcode == 0x11;
  if ((isBeq && eq) || (isBgt && gt) || isUBranch) {
    return true;
  }
  return false;
}
unsigned int read_word(unsigned char *mem, unsigned int address) {
  unsigned int *data;
  data =  (unsigned int*) (mem + address);
  return *data;
}

void write_word(unsigned char *mem, unsigned int address, unsigned int data) {
  unsigned int *data_p;
  data_p = (unsigned int*) (mem + address);
  *data_p = data;
}
