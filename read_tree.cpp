//**********************************************************************************
//  read_files.cpp 
//  This will be a generic utility to identify all files specified 
//  by a provided file spec with wildcards
//  This is intended as a template for reading all files in current directory,
//  then performing some task on them.  The print statement at the end
//  can be replaced with a function call to perform the desired operation
//  on each discovered file.
//  
//  Written by:  Derell Licht
//**********************************************************************************

#include <windows.h>
#include <stdio.h>
#include <string>
#include <memory> //  unique_ptr

//  remove this define to remove all the debug messages
// #define  DESPERATE

#include "common.h"
#ifndef _lint
#include "conio_min.h"
#endif
#include "qualify.h"

//lint -esym(534, FindClose)  // Ignoring return value of function
//lint -e818  variable could be declared as pointing to const
//lint -e10   Expecting '}'

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' could be made static
//lint -e714  Symbol '_CRT_glob' not referenced
int _CRT_glob = 0 ;

wchar_t tempstr[MAX_PATH+1];

//lint -esym(843, show_all)
bool show_all = true ;

static wchar_t formstr[50];

//  name of drive+path without filenames
static TCHAR base_path[MAX_FILE_LEN+1] ;
static unsigned base_len ;  //  length of base_path

//lint -e129  declaration expected, identifier ignored
static std::unique_ptr<conio_min> console ;

//************************************************************
static wchar_t dirpath[MAX_PATH_LEN];
unsigned level;

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   dirs *brothers {nullptr};
   dirs *sons{nullptr};
   wchar_t *name{nullptr};
   uchar attrib{};
   ULONGLONG dirsize{};
   ULONGLONG dirsecsize{};
   ULONGLONG subdirsize{};
   ULONGLONG subdirsecsize{};
   unsigned files{};
   unsigned directs{};
   unsigned subfiles{};
   unsigned subdirects{};
   bool     is_multi_byte {};
   uint     mb_len {};
};

dirs *top = NULL;

//*****************************************************************
//  this was used for debugging directory-tree read and build
//*****************************************************************
#ifdef  DESPERATE
void debug_dump(wchar_t *fname, wchar_t *msg)
{
   // syslog("l%u %s: %s\n", level, fname, msg) ;  //  debug dump
   console->dputsf(L"l%u %s: %s\n", level, fname, msg) ;  //  debug dump
}
#endif

//**********************************************************
//  allocate struct for dir listing                         
//  NOTE:  It is assumed that the caller will               
//         initialize the name[], ext[], attrib fields!!    
//**********************************************************
static dirs *new_dir_node (void)
{
   //  switching this statement from malloc() to new()
   //  changes total exe size from 70,144 to 179,200 !!!
   //   70144 ->     32256   45.99%    win64/pe     ndir64.exe
   // dirs *dtemp = (dirs *) malloc(sizeof(dirs)) ;
   // if (dtemp == NULL) {
   //    return NULL ;
   // }
   //     179200 ->     72704   40.57%    win64/pe     ndir64.exe
   dirs *dtemp = new dirs ;
   ZeroMemory(dtemp, sizeof (struct dirs));  //lint !e668
   // memset ((wchar_t *) dtemp, 0, sizeof (struct dirs));  //lint !e668
   // dtemp->dirsecsize = clbytes;
   // dtemp->subdirsecsize = clbytes;
   return dtemp;
}

//**********************************************************
//  recursive routine to read directory tree
//**********************************************************
static int read_dir_tree (dirs * cur_node)
{
   dirs *dtail = 0;
   TCHAR *strptr;
   HANDLE handle;
   uint slen ;
   bool done ;  //, result;
   DWORD err;
   // ULONGLONG file_clusters, clusters;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATA fdata ; //  long-filename file struct

   // if (((dircount % 50) == 0)  &&  _kbhit()) {
   //    result = _getch() ;
   //    //  check for ESCAPE character
   //    if (result == 27) {
   //       // error_exit(DATA_OKAY, NULL) ;
   //       early_abort = 1 ;
   //    }
   // }
   // //  if early_abort flag gets set, return with "success" flag,
   // //  but without reading anything further...
   // if (early_abort) 
   //    return 0;

   // pattern_update() ;
   //  Insert next subtree level.
   //  if level == 0, this is first call, and
   //  dirpath is already complete
   if (level > 0) {
      //  insert new path name
      strptr = wcsrchr (dirpath, L'\\');
      strptr++;
      *strptr = 0;
      slen = wcslen (dirpath);
      wcscat (dirpath, cur_node->name);
      wcscat (dirpath, L"\\*");
   }
   else {
      slen = wcslen (dirpath);
   }

   //  first, build tree list for current level
   level++;
#ifdef  DESPERATE
   if (level > 100) {
      exit(1);
   }
#endif   

#ifdef  DESPERATE
debug_dump(dirpath, L"entry") ;
#endif
   err = 0;
   handle = FindFirstFile(dirpath, &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      err = GetLastError ();
      if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(dirpath, L"FindFirstFile denied") ;
#endif
         ;                     //  continue reading
      }
      else {
#ifdef  DESPERATE
console->dputsf(L"%s: FindFindFirst: %s\n", dirpath, get_system_message (err));
#endif
         // _stprintf (tempstr, "path [%s]\n", dirpath);
         // nputs (0xA, tempstr);
         // _stprintf (tempstr, "FindFirst: %s\n", get_system_message ());
         // nputs (0xA, tempstr);
         return (int) err ;
      }
   }

   //  loop on find_next
   done = false;
   while (!done) {
      if (!err) {
         //  we found a directory
         if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            bool cut_dot_dirs;

            //  skip '.' and '..', but NOT .ncftp (for example)
            if (wcscmp(fdata.cFileName, L".")  == 0  ||
                wcscmp(fdata.cFileName, L"..") == 0) {
               cut_dot_dirs = true;
            }
            else {
               cut_dot_dirs = false;
            }

            if (!cut_dot_dirs) {
               cur_node->directs++;
               cur_node->subdirects++;

               dirs *dtemp = new_dir_node ();
               if (cur_node->sons == NULL)
                  cur_node->sons = dtemp;
               else
                  dtail->brothers = dtemp;   //lint !e613  NOLINT
               dtail = dtemp;
               // if (!n.ucase) 
               //    strlwr(ff.name) ;
               
               //  convert Unicode filenames to UTF8
               dtemp->mb_len = wcslen(fdata.cFileName) ;
               dtemp->name = (TCHAR *) new TCHAR[dtemp->mb_len + 1] ;  //lint !e737
               wcscpy (dtemp->name, (TCHAR *) fdata.cFileName);  //  NOLINT

               dtemp->attrib = (uchar) fdata.dwFileAttributes;
               // dtail->directs++ ;
            }                   //  if this is not a DOT directory
         }                      //  if this is a directory

         //  we found a normal file
         else {
            // printf("%9ld %04X %s\n", fdata.file_size, fdata.attrib, fdata.fname) ;
            //  convert file size
            // u64toul iconv;
            // iconv.u[0] = fdata.nFileSizeLow;
            // iconv.u[1] = fdata.nFileSizeHigh;
            // cur_node->dirsize += iconv.i;
            // cur_node->subdirsize += iconv.i;
            // 
            // clusters = iconv.i / clbytes;
            // if ((iconv.i % clbytes) > 0)  //lint !e79
            //    clusters++;
            // 
            // file_clusters = clusters * clbytes;
            // cur_node->dirsecsize += file_clusters;
            // cur_node->subdirsecsize += file_clusters;
            // 
            // cur_node->files++;
            // cur_node->subfiles++;
         }                      //  if entry is a file
      }                         //  if no errors detected

      //******************************************
      //  search for another file
      //******************************************
      if (FindNextFile(handle, &fdata) == 0) {
         // done = 1;
         err = GetLastError ();
         if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(fdata.cFileName, L"denied") ;
#else
            ;                     //  continue reading
#endif
         }
         else if (err == ERROR_NO_MORE_FILES) {
#ifdef  DESPERATE
console->dputsf(L"FindNextFileW: no more files\n") ;
#endif
            done = true ;
         }
         else {
#ifdef  DESPERATE
wsprintf (tempstr, L"FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
            done = true ;
         }
      } else {
         err = 0 ;
      }
   }  //  while reading files from directory

#ifdef  DESPERATE
debug_dump(dirpath, L"close") ;
#endif
   FindClose (handle);

   //  next, build tree lists for subsequent levels (recursive)
   dirs *ktemp = cur_node->sons;
   while (ktemp != NULL) {
#ifdef  DESPERATE
debug_dump(ktemp->name, L"call read_dir_tree") ;
#endif
      read_dir_tree (ktemp);
      cur_node->subdirsize += ktemp->subdirsize;
      cur_node->subdirsecsize += ktemp->subdirsecsize;

      cur_node->subfiles += ktemp->subfiles;
      cur_node->subdirects += ktemp->subdirects;
      ktemp = ktemp->brothers;
   }

   //  when done, strip name from path and restore '\*.*'
   wcscpy (&dirpath[slen], L"*");  //lint !e669  string overrun??

   //  restore the level number
   level--;
   return 0;   //lint !e438
}

//**********************************************************
static int build_dir_tree (wchar_t *tpath)
{
   // int result ;
   TCHAR *strptr;
   level = 0;

   //  Extract base path from first filespec,
   //  and strip off filename
   wcscpy (base_path, tpath);
   strptr = wcsrchr (base_path, L'\\');
   if (strptr != 0)
      *(++strptr) = 0;          //  strip off filename

   // get_disk_info (base_path);

   //  allocate struct for dir listing
   top = new_dir_node ();

   //  Extract base path from first filespec,
   //  and strip off filename
   wcscpy (base_path, tpath);
   strptr = wcsrchr (base_path, L'\\');
   strptr++;                    //  skip past backslash, to filename
   *strptr = 0;                 //  strip off filename
   base_len = wcslen (base_path);

   //  derive root path name
   if (wcslen (base_path) == 3) {
      top->name = (TCHAR *) new TCHAR[8] ;
      wcscpy (top->name, L"<root>");
   }
   else {
      wcscpy (tempstr, base_path);
      tempstr[base_len - 1] = 0; //  strip off tailing backslash
      strptr = wcsrchr (tempstr, L'\\');
      strptr++;                 //  skip past backslash, to filename

      top->name = (TCHAR *) new TCHAR[wcslen (strptr) + 1];
      wcscpy (top->name, strptr);
   }

   // top->attrib = 0 ;   //  top-level dir is always displayed

   wcscpy (dirpath, tpath);

   // pattern_init(_T("wait; reading directory ")) ;
   // int result = 
   read_dir_tree (top);
#ifdef  DESPERATE
debug_dump(L"exit", L"returned from read_dir_tree") ;
#endif
   // pattern_reset() ;
   return 0;
}

//**********************************************************
//  Note: lstr contains form string plus filename
//**********************************************************
static void display_tree_filename (wchar_t *lformstr, dirs *ktemp)
{
   console->dputsf(L"%s %s\n", lformstr, ktemp->name) ;
}

//**********************************************************
//  recursive routine to display directory tree
//  do all subroutines, then go to next.
//**********************************************************
static void display_dir_tree (dirs * ktop)
{
   dirs *ktemp = ktop;
   if (ktop == NULL)
      return;

   //  next, build tree lists for subsequent levels (recursive)
   while (ktemp != NULL) {
      //  first, build tree list for current level
      if (level == 0) {
         formstr[0] = (wchar_t) 0;
      }
      else {
         if (ktemp->brothers == (struct dirs *) NULL) {
            formstr[level - 1] = (wchar_t) '\\';   //lint !e743 
            formstr[level] = (wchar_t) NULL;
         }
         else {
            formstr[level - 1] = (wchar_t) '+';   //lint !e743 
            formstr[level] = (wchar_t) NULL;
         }
      }

      //*****************************************************************
      //                display data for this level                      
      //*****************************************************************
      display_tree_filename (formstr, ktemp);

      //  build tree string for deeper levels
      if (level > 0) {
         if (ktemp->brothers == NULL)
            formstr[level - 1] = ' ';
         else
            formstr[level - 1] = '|' ; //lint !e743 
      }                         //  if level > 1

      //  process any sons
      level++;
         display_dir_tree(ktemp->sons);
      formstr[--level] = (wchar_t) 0;  //  NOLINT

      //  goto next brother
      ktemp = ktemp->brothers;
   }                            //  while not done listing directories
}

//********************************************************************************
//  this solution is from:
//  https://github.com/coderforlife/mingw-unicode-main/
//********************************************************************************
#if defined(__GNUC__) && defined(_UNICODE)

#ifndef __MSVCRT__
#error Unicode main function requires linking to MSVCRT
#endif

#include <wchar.h>
#include <stdlib.h>

extern int _CRT_glob;
extern 
#ifdef __cplusplus
"C" 
#endif
void __wgetmainargs(int*,wchar_t***,wchar_t***,int,int*);

#ifdef MAIN_USE_ENVP
int wmain(int argc, wchar_t *argv[], wchar_t *envp[]);
#else
int wmain(int argc, wchar_t *argv[]);
#endif

int main() 
{
   wchar_t **enpv, **argv;
   int argc, si = 0;
   __wgetmainargs(&argc, &argv, &enpv, _CRT_glob, &si); // this also creates the global variable __wargv
#ifdef MAIN_USE_ENVP
   return wmain(argc, argv, enpv);
#else
   return wmain(argc, argv);
#endif
}

#endif //defined(__GNUC__) && defined(_UNICODE)

//**********************************************************************************
wchar_t file_spec[MAX_PATH_LEN+1] = L"" ;

// int main(int argc, wchar_t **argv)
int wmain(int argc, wchar_t *argv[])
{
   console = std::make_unique<conio_min>() ;
   if (!console->init_okay()) {  //lint !e530
      wprintf(L"console init failed\n");
      return 1 ;
   }
   int idx, result ;
   for (idx=1; idx<argc; idx++) {
      wchar_t *p = argv[idx] ;
      wcsncpy(file_spec, p, MAX_PATH_LEN);
      file_spec[MAX_PATH_LEN] = 0 ;
   }

   if (file_spec[0] == 0) {
      wcscpy(file_spec, L".");
   }

   uint qresult = qualify(file_spec) ;
   if (qresult == QUAL_INV_DRIVE) {
      console->dputsf(L"%s: 0x%X\n", file_spec, qresult);
      return 1 ;
   }
   console->dputsf(L"input file spec: %s\n", file_spec);

   //  Extract base path from first filespec, and strip off filename.
   //  base_path becomes useful when one wishes to perform
   //  multiple searches in one path.
   wcscpy(base_path, file_spec) ;
   wchar_t *strptr = wcsrchr(base_path, '\\') ;
   if (strptr != 0) {
       strptr++ ;  //lint !e613  skip past backslash, to filename
      *strptr = 0 ;  //  strip off filename
   }
   base_len = wcslen(base_path) ;
   // printf("base path: %s\n", base_path);
   
   result = build_dir_tree(file_spec) ;
   if (result < 0) {
      console->dputsf(L"build_dir_tree: %s, %s\n", file_spec, strerror(-result));
      return 1 ;
   }

   //  show the tree that we read
   display_dir_tree(top);
   return 0;
}

