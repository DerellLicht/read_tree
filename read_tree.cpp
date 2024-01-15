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
#include <stdlib.h>  //  PATH_MAX

//  remove this define to remove all the debug messages
// #define  DBG_TRACE

#include "common.h"
#include "qualify.h"

//lint -esym(534, FindClose)  // Ignoring return value of function
//lint -esym(534, __builtin_va_start, vsprintf)
//lint -esym(818, filespec, argv)  //could be declared as pointing to const
//lint -e10  Expecting '}'

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' could be made static
//lint -e714  Symbol '_CRT_glob' not referenced
int _CRT_glob = 0 ;

uint filecount = 0 ;

char tempstr[MAX_PATH+1];

static char formstr[50];

//lint -esym(613, ftail)  //  Possible use of null pointer in left argument to operator '->' 
//************************************************************
#define  MAX_EXT_LEN    6
//lint -esym(751, ffdata_t)  // local typedef not referenced
typedef struct ffdata {
   FILETIME       ft ;
   ULONGLONG      fsize ;
   char           *filename ;
   char           ext[MAX_EXT_LEN+1];
   struct ffdata  *next ;
} ffdata_t, *ffdata_p;

//************************************************************
static char dirpath[PATH_MAX];
unsigned level;

static char dir_full_path[PATH_MAX];
static uint dir_fpath_len = 0 ;

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   dirs *brothers;
   dirs *sons;
   char *name;
   char *fpath ;
   ffdata_p ftop ;
};

dirs *top = NULL;

//*****************************************************************
//  this was used for debugging directory-tree read and build
//*****************************************************************
#ifdef  DBG_TRACE
void debug_dump(char *fname, char *msg)
{
   syslog("l%u %s: %s\n", level, fname, msg) ;  //  debug dump
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
   dirs *dtemp = (dirs *) malloc(sizeof(dirs)) ;
   if (dtemp == NULL) {
      return NULL ;
   }
   //     179200 ->     72704   40.57%    win64/pe     ndir64.exe
   // dirs *dtemp = new dirs ;
   ZeroMemory(dtemp, sizeof (struct dirs));  //lint !e668
   return dtemp;
}

//**********************************************************
//  recursive routine to read directory tree
//**********************************************************
static int read_dir_tree (dirs * cur_node)
{
   dirs *dtail = 0;
   char *strptr;
   HANDLE handle;
   size_t dir_path_len ;
   uint slen ;
   bool done ;
   DWORD err;
   // ULONGLONG file_clusters, clusters;
   WIN32_FIND_DATA fdata ; //  long-filename file struct
   // WIN32_FIND_DATAW fdata ; //  long-filename file struct
   ffdata *ftemp;
   ffdata *ftop  = NULL;
   ffdata *ftail = NULL;

   //  Insert next subtree level.
   //  if level == 0, this is first call, and
   //  dirpath is already complete
   if (level > 0) {
      //  insert new path name
      strptr = strrchr(dirpath, '\\');
      if (strptr == NULL) {
         printf("no path end found: %s\n", dirpath);
         dir_path_len = strlen(dirpath);
      }
      else {
         strptr++;
         *strptr = 0;
         dir_path_len = strlen(dirpath);
         strcat(dirpath, cur_node->name);
         strcpy(dir_full_path, dirpath);
         dir_fpath_len = strlen(dir_full_path) ;
         strcat(dirpath, "\\*");
      }
   }
   else {
      // printf("dirpath: %s\n", dirpath);
      dir_path_len = strlen(dirpath);
      strcpy(dir_full_path, base_path);
      dir_fpath_len = strlen(dir_full_path) ;
   }

   //  first, build tree list for current level
   level++;

#ifdef  DBG_TRACE
debug_dump(dirpath, "entry") ;
#endif
   err = 0;
   
   handle = FindFirstFile(dirpath, &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      err = GetLastError ();
      if (err == ERROR_ACCESS_DENIED) {
#ifdef  DBG_TRACE
debug_dump(dirpath, "FindFirstFile denied") ;
#endif
         ;                     //  continue reading
      }
      else {
#ifdef  DBG_TRACE
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
         return (int) err ;
      }
   }

   //  loop on find_next
   done = false;
   while (!done) {
      //  this err will only occur when unreadable files are encounter in a folder
      if (err == 0) {
         //  we found a directory
         if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            bool cut_dot_dirs;

            // printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.fname) ;
            //  skip '.' and '..', but NOT .ncftp (for example)
            if (strcmp(fdata.cFileName, ".")  == 0  ||
                strcmp(fdata.cFileName, "..") == 0) {
               cut_dot_dirs = true;
            }
            else {
               cut_dot_dirs = false;
            }

            if (!cut_dot_dirs) {
               dirs *dtemp = new_dir_node ();
               if (cur_node->sons == NULL)
                  cur_node->sons = dtemp;
               else
                  dtail->brothers = dtemp;   //lint !e613
               dtail = dtemp;
               
               dtemp->name = (char *) malloc(strlen ((char *) fdata.cFileName) + 1);
               strcpy (dtemp->name, (char *) fdata.cFileName);
               slen = dir_fpath_len + 1 + strlen(fdata.cFileName) + 2 ;
               dtemp->fpath = (char *) malloc(slen);
               
               sprintf(dtemp->fpath, "%s\\%s\\", dir_full_path, fdata.cFileName);
               // printf("<%s>\n", dtemp->fpath);
            }                   //  if this is not a DOT directory
         }                      //  if this is a directory

         //  we found a normal file
         else {
            //  In this tree scanner, we will build a file list for each folder
            // ftemp = new ffdata;
            ftemp = (struct ffdata *) malloc(sizeof(ffdata)) ;
            if (ftemp == NULL) {
               return -errno;
            }
            memset((char *) ftemp, 0, sizeof(ffdata));

            // ftemp->attrib = (uchar) fdata.dwFileAttributes;

            //  convert file time
            // if (n.fdate_option == FDATE_LAST_ACCESS)
            //    ftemp->ft = fdata.ftLastAccessTime;
            // else if (n.fdate_option == FDATE_CREATE_TIME)
            //    ftemp->ft = fdata.ftCreationTime;
            // else
            //    ftemp->ft = fdata.ftLastWriteTime;
            ftemp->ft = fdata.ftLastAccessTime;

            //  convert file size
            u64toul iconv;
            iconv.u[0] = fdata.nFileSizeLow;
            iconv.u[1] = fdata.nFileSizeHigh;
            ftemp->fsize = iconv.i;

            ftemp->filename = (char *) malloc(strlen ((char *) fdata.cFileName) + 1);
            strcpy (ftemp->filename, (char *) fdata.cFileName);

            strptr = strrchr (ftemp->filename, '.');
            if (strptr != NULL) {
               strptr++ ;  //  skip past period
               if (strlen (strptr) <= MAX_EXT_LEN) {
                  strcpy (ftemp->ext, strptr);
               }
               else {
                  ftemp->ext[0] = 0;  //  no extension found
               }
            }
            else {
               ftemp->ext[0] = 0;  //  no extension found
            }
            
            //****************************************************
            //  add the structure to the file list
            //****************************************************
            if (ftop == NULL) {
               ftop = ftemp;
            }
            else {
               ftail->next = ftemp;
            }
            ftail = ftemp;
         }                      //  if entry is a file
      }                         //  if no errors detected

      //******************************************
      //  search for another file
      //******************************************
      if (FindNextFile(handle, &fdata) == 0) {
         // done = true;
         err = GetLastError ();
         if (err == ERROR_ACCESS_DENIED) {
#ifdef  DBG_TRACE
debug_dump(fdata.cFileName, "denied") ;
#else
            ;                     //  continue reading
#endif
         }
         else if (err == ERROR_NO_MORE_FILES) {
            done = true ;
         }
         else {
#ifdef  DBG_TRACE
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
            done = true ;
         }
      } else {
         err = 0 ;
      }
   }  //  while reading files from directory
   
#ifdef  DBG_TRACE
debug_dump(dirpath, "close") ;
#endif
   FindClose (handle);

   //  how do we tack ftemp onto current folder ??
   cur_node->ftop = ftop ;

   //  next, build tree lists for subsequent levels (recursive)
   dirs *ktemp = cur_node->sons;
   while (ktemp != NULL) {
#ifdef  DBG_TRACE
if (ktemp == 0) 
   debug_dump("[NULL]", "call read_dir_tree") ;
else
   debug_dump(ktemp->name, "call read_dir_tree") ;
#endif
      read_dir_tree (ktemp);  //lint !e534
      // cur_node->subdirsize += ktemp->subdirsize;
      // cur_node->subdirsecsize += ktemp->subdirsecsize;

      // cur_node->subfiles += ktemp->subfiles;
      // cur_node->subdirects += ktemp->subdirects;
      ktemp = ktemp->brothers;
   }

   //  when done, strip name from path and restore '\*.*'
   strcpy(&dirpath[dir_path_len], "*");  //lint !e669  string overrun??

   //  restore the level number
   level--;
   return 0;
}

//**********************************************************
static int build_dir_tree (char *tpath)
{
   int result ;
   char *strptr;
   level = 0;
   
// tpath: D:\SourceCode\Git\read_folder_tree\*
// bpath: D:\SourceCode\Git\read_folder_tree
// dirpath: D:\SourceCode\Git\read_folder_tree\*
   // printf("tpath: %s\n", tpath);
   // printf("bpath: %s\n", base_path);

   //  allocate top-level struct for dir listing
   top = new_dir_node ();

   //  derive root path name
   if (strlen (base_path) == 3) {
      top->name = (char *) malloc(8) ;
      if (top->name == 0) {
         return ERROR_OUTOFMEMORY ;
      }
      strcpy (top->name, "<root>");
   }
   else {
      strcpy (tempstr, base_path);
      //tempstr[base_len - 1] = 0; //  strip off tailing backslash
      strptr = strrchr (tempstr, '\\');
      if (strptr == NULL) {
         printf("no path end found: %s\n", dirpath);
         return ERROR_FILE_NOT_FOUND ;
      }
      strptr++;                 //  skip past backslash, to filename

      // printf("L%u top folder: %s\n", level, strptr);
      top->name = (char *) malloc(strlen (strptr) + 1);
      if (top->name == NULL) {
         return ERROR_OUTOFMEMORY ;
      }
      strcpy (top->name, strptr);
   }

   strcpy (dirpath, tpath);
   strcpy(dir_full_path, base_path);
   dir_fpath_len = strlen(dir_full_path) ;
   
   DWORD slen = strlen(base_path);
   top->fpath = (char *) malloc(slen+2);
   sprintf(top->fpath, "%s\\", base_path);

   result = read_dir_tree (top);
   return result ;
}  //lint !e818

//***********************************************************************
//  This is the function which actually does the work in the application
//***********************************************************************
static void execute_file_operation(char *full_path, ffdata_p ftemp)
{
   printf("%s\\%s\n", full_path, ftemp->filename);
}
      
//**********************************************************
static void display_file_list(char *full_path, ffdata_p ftop)
{
   //  now, do something with the files that you found   
   for (ffdata *ftemp = ftop; ftemp != NULL; ftemp = ftemp->next) {
      execute_file_operation(full_path, ftop);
   }
   puts("");
}

//**********************************************************
static void display_tree_filename (char *frmstr, dirs const * const ktemp)
{
   if (ktemp->fpath == NULL) {
      printf("%s[%s]\n", frmstr, "<NULL>") ;
   }
   else {
      printf("%s[%s]\n", frmstr, ktemp->fpath) ;
   }

   if (ktemp->ftop != NULL) {
      display_file_list(ktemp->fpath, ktemp->ftop);
   }
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
         formstr[0] = (char) NULL;
      }
      else {
         if (ktemp->brothers == (struct dirs *) NULL) {
            formstr[level - 1] = (char) '\\';   //lint !e743 
            formstr[level] = (char) NULL;
         }
         else {
            formstr[level - 1] = (char) '+';   //lint !e743 
            formstr[level] = (char) NULL;
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
      formstr[--level] = (char) NULL;

      //  goto next brother
      ktemp = ktemp->brothers;
   }                            //  while not done listing directories
}

//**********************************************************************************
char file_spec[PATH_MAX+1] = "" ;

int main(int argc, char **argv)
{
   int idx, result ;
   for (idx=1; idx<argc; idx++) {
      char *p = argv[idx] ;
      strncpy(file_spec, p, PATH_MAX);
      file_spec[PATH_MAX] = 0 ;
   }

   if (file_spec[0] == 0) {
      strcpy(file_spec, ".");
   }

   uint qresult = qualify(file_spec) ;
   if (qresult == QUAL_INV_DRIVE) {
      printf("%s: 0x%X\n", file_spec, qresult);
      return 1 ;
   }
   // printf("file spec: %s\n", file_spec);

   //  Extract base path from first filespec, and strip off filename.
   //  base_path becomes useful when one wishes to perform
   //  multiple searches in one path.
   strcpy(base_path, file_spec) ;
   char *strptr = strrchr(base_path, '\\') ;
   if (strptr != 0) {
       // strptr++ ;  //lint !e613  skip past backslash, to filename
      *strptr = 0 ;  //  strip off filename
   }
   base_len = strlen(base_path) ;
   // printf("base path: %s\n", base_path);
   
   result = build_dir_tree(file_spec) ;
   // result = read_files(file_spec);
   if (result < 0) {
      printf("build_dir_tree: %s, %s\n", file_spec, strerror(-result));
      return 1 ;
   }

   //  now display the resulting directory tree
   display_dir_tree(top);
   return 0;
}

