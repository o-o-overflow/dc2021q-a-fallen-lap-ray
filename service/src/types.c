#include "types.h"

uint8_t opcode_to_num_inputs[] = {
   /* ADD */ 2,
   /* SUB */ 2,
   /* BRR */ 2,
   /* LT */ 2,
   /* EQ */ 2,
   /* DUP */ 1,
   /* NEG */ 1,
   /* MER (this is a bit of a lie, there's two arguments for MER, but this will simply things 
	       if the machine thinks there's only one instruction. */ 1,
   /* NTG */ 1,
   /* ITG */ 1,
   /* GT */ 2,
   /* SIL */ 2,
   /* CTG */ 2,
   /* RTD */ 2,
   /* ETG */ 1,
   /* MUL */ 2,
   /* XOR */ 2,
   /* AND */ 2,
   /* OR  */ 2,
   /* SHL */ 2,
   /* SHR */ 2,
   /* NEQ */ 2,
   /* OPN */ 2,
   /* RED */ 1,
   /* WRT */ 2,
   /* CLS */ 1,
   /* GTE */ 2,
   /* LTE */ 2,
   /* HLT */ 1,
   /* LOD */ 2,
   /* LS  */ 1,
   /* SDF */ 2,
   /* ULK */ 1,
   /* LSK */ 2,
   /* RND */ 1,
   /* NAR */ 1,
   /* ARF */ 2,
   /* AST */ 2,
};

/* Not actually used, please change code in instruction_store.c */
opcode_type privileged_opcodes[] = {
   OPN,
   RED,
   WRT,
   CLS,
   HLT,
   LOD,
   LS,
   SDF,
   ULK,
   LSK,
   RND,
};

#ifdef DEBUG
char* opcode_to_name[] = {
   "ADD",
   "SUB",
   "BRR",
   "LT",
   "EQ",
   "DUP",
   "NEG",
   "MER",
   "NTG",
   "ITG",
   "GT",
   "SIL",
   "CTG",
   "RTD",
   "ETG",
   "MUL",
   "XOR",
   "AND",
   "OR",
   "SHL",
   "SHR",
   "NEQ",
   "OPN",
   "RED",
   "WRT",
   "CLS",
   "GTE",
   "LTE",
   "HLT",
   "LOD",
   "LS",
   "SDF",
   "ULK",
   "LSK",
   "RND",
   "NAR",
   "ARF",
   "AST",
};
#endif
