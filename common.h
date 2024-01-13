//**********************************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  common.cpp - common library functions
//                                                                 
//  Written by:   Daniel D. Miller  (the derelict)                 
//  
//**********************************************************************************

//lint -esym(756, u64)  global typedef not referenced

typedef  unsigned long long   u64 ;
typedef  unsigned char        uchar ;
typedef  unsigned long        ulong ;
typedef  unsigned int         uint ;
typedef  unsigned char        u8  ;
typedef  unsigned short       u16 ;
typedef  unsigned int         u32 ;

//  this definition was excluded by WINNT.H
#define FILE_ATTRIBUTE_VOLID  0x00000008

#ifndef PATH_MAX
#define  PATH_MAX       260
#endif

#define  LOOP_FOREVER   true

#define  MAXLINE        1024

#define  MAX_EXT_SIZE   8

typedef union ul2uc_u {
   unsigned       ul ;
   unsigned short us[2] ;
   unsigned char  uc[4] ;
} ul2uc_t;

union u64toul {
   ULONGLONG i ;
   ulong u[2] ;
};

//lint -esym(552, base_len)   Symbol not accessed
//lint -esym(759, base_len)   header declaration for symbol could be moved from header to module
extern char base_path[PATH_MAX] ;
extern unsigned base_len ;  //  length of base_path

//  debug functions
int hex_dump(u8 *bfr, int bytes, unsigned addr);
int hex_dump(u8 *bfr, int bytes);

// ULLONG_MAX = 18,446,744,073,709,551,615
#define  MAX_ULL_COMMA_LEN  26
char *convert_to_commas(ULONGLONG uli, char *outstr);

