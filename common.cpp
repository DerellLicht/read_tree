//**********************************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  common.cpp - common library functions
//                                                                 
//  Written by:   Daniel D. Miller  (the derelict)                 
//  
//**********************************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  //  PATH_MAX

#include "common.h"

//lint -esym(755, LOOP_FOREVER, MAX_EXT_SIZE, MAXLINE)
//lint -esym(756, u16, u32, ul2uc_t)
//lint -esym(759, base_path, convert_to_commas)
//lint -esym(768, ul2uc_u::uc, ul2uc_u::ul, ul2uc_u::us)

//  name of drive+path without filenames
char base_path[PATH_MAX] ;
unsigned base_len ;  //  length of base_path

//**********************************************************************
//  Modify this to build entire string and print once.
//  This command has several forms:
//
//  - the basic form has too many arguments!!
//    bfr - data buffer to display
//    bytes - number of bytes (of bfr) to display
//    addr  - base address to display at beginning of line.
//            This helps with repeated calls to this function.
//    mode  - 0=output to printf, 1=output to syslog
//
//  - The other two forms take only buffer and length args,
//    and implicitly print to either printf or syslog.
//**********************************************************************
//lint -esym(714, hex_dump)
//lint -esym(759, hex_dump)
//lint -esym(765, hex_dump)
int hex_dump(u8 *bfr, int bytes, unsigned addr)
{
   int j, len ;
   char tail[40] ;
   char pstr[81] ;
   static bool hex_dump_active = false ;
   if (hex_dump_active)
      return 0;
   hex_dump_active = true ;

   tail[0] = 0 ;
   int idx = 0 ;
   int plen = 0 ;
   while (1) {
      int leftovers = bytes - idx ;
      if (leftovers > 16)
          leftovers = 16 ;

      plen = wsprintfA(pstr, "%05X:  ", addr+idx) ;  //lint !e737
      len = 0 ;
      for (j=0; j<leftovers; j++) {
         u8 chr = bfr[idx+j] ;
         plen += wsprintfA(&pstr[plen], "%02X ", chr) ;
         if (chr < 32) {
            len += wsprintfA(tail+len, ".") ;
         } else if (chr < 127) {
            len += wsprintfA(tail+len, "%c", chr) ;
         } else {
            len += wsprintfA(tail+len, "?") ;
         }
      }
      //  last, print fill spaces
      for (; j<16; j++) {
         plen += wsprintfA(&pstr[plen], "   ") ;
         len += wsprintfA(tail+len, " ") ;
      }

      // printf(" | %s |\n", tail) ;
      strcat(pstr, " | ") ;
      strcat(pstr, tail) ;
      strcat(pstr, " |") ;
      printf("%s\n", pstr) ;
      // syslog("%s\n", pstr) ;

      idx += leftovers ;
      if (idx >= bytes)
         break;
   }
   hex_dump_active = false ;
   return 0;
}  //lint !e818 

//**************************************************************************
int hex_dump(u8 *bfr, int bytes)
{
   return hex_dump(bfr, bytes, 0) ;
}

//*****************************************************************************
// ULLONG_MAX = 18,446,744,073,709,551,615
//*****************************************************************************
char *convert_to_commas(ULONGLONG uli, char *outstr)
{  //lint !e1066
   uint slen, j, inIdx ;
   char *strptr ;
   char temp_ull_str[MAX_ULL_COMMA_LEN+1] ;
   static char local_ull_str[MAX_ULL_COMMA_LEN+1] ;
   if (outstr == NULL) {
       outstr = local_ull_str ;
   }

   // sprintf(temp_ull_str, "%"PRIu64"", uli);
   // sprintf(temp_ull_str, "%llu", uli);
   sprintf(temp_ull_str, "%I64u", uli);
   // _ui64toa(uli, temp_ull_str, 10) ;
   slen = strlen(temp_ull_str) ;
   inIdx = --slen ;//  convert byte-count to string index 

   //  put NULL at end of output string
   strptr = outstr + MAX_ULL_COMMA_LEN ;
   *strptr-- = 0 ;   //  make sure there's a NULL-terminator

   for (j=0; j<slen; j++) {
      *strptr-- = temp_ull_str[inIdx--] ;
      if ((j+1) % 3 == 0)
         *strptr-- = ',' ;
   }
   *strptr = temp_ull_str[inIdx] ;

   //  copy string from tail-aligned to head-aligned
   strcpy(outstr, strptr) ;
   return outstr ;
}

