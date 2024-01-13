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
#include <tchar.h>
#include <errno.h>

//  remove this define to remove all the debug messages
// #define  DESPERATE

#include "common.h"
#include "read_tree.h"
#include "qualify.h"

//lint -esym(534, FindClose)  // Ignoring return value of function
//lint -esym(818, filespec, argv)  //could be declared as pointing to const
//lint -e10  Expecting '}'

WIN32_FIND_DATA fdata ; //  long-filename file struct

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' could be made static
//lint -e714  Symbol '_CRT_glob' not referenced
int _CRT_glob = 0 ;

uint filecount = 0 ;

char tempstr[MAX_PATH+1];

//lint -esym(843, show_all)
bool show_all = true ;

static char formstr[50];

//************************************************************
static char dirpath[PATH_MAX];
unsigned level;

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   dirs *brothers;
   dirs *sons;
   char *name;
   uchar attrib;
   ULONGLONG dirsize;
   ULONGLONG dirsecsize;
   ULONGLONG subdirsize;
   ULONGLONG subdirsecsize;
   unsigned files;
   unsigned directs;
   unsigned subfiles;
   unsigned subdirects;
   bool     is_multi_byte ;
   uint     mb_len ;
};

dirs *top = NULL;


//************************************************************
ffdata *ftop  = NULL;
ffdata *ftail = NULL;

//*****************************************************************
//  this was used for debugging directory-tree read and build
//*****************************************************************
#ifdef  DESPERATE
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
   // memset ((char *) dtemp, 0, sizeof (struct dirs));  //lint !e668
   // dtemp->dirsecsize = clbytes;
   // dtemp->subdirsecsize = clbytes;
   return dtemp;
}

//**********************************************************************************
int read_files(char *filespec)
{
   int done, fn_okay ;
   HANDLE handle;
   ffdata *ftemp;

   handle = FindFirstFile (filespec, &fdata);
   //  according to MSDN, Jan 1999, the following is equivalent
   //  to the preceding... unfortunately, under Win98SE, it's not...
   // handle = FindFirstFileEx(target[i], FindExInfoStandard, &fdata, 
   //                      FindExSearchNameMatch, NULL, 0) ;
   if (handle == INVALID_HANDLE_VALUE) {
      return -errno;
   }

   //  loop on find_next
   done = 0;
   while (!done) {
      if (!show_all) {
         if ((fdata.dwFileAttributes & 
            (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)) != 0) {
            fn_okay = 0 ;
            goto search_next_file;
         }
      }
      //  filter out directories if not requested
      if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_VOLID) != 0)
         fn_okay = 0;
      else if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
         fn_okay = 1;
      //  For directories, filter out "." and ".."
      else if (fdata.cFileName[0] != '.') //  fn=".something"
         fn_okay = 1;
      else if (fdata.cFileName[1] == 0)   //  fn="."
         fn_okay = 0;
      else if (fdata.cFileName[1] != '.') //  fn="..something"
         fn_okay = 1;
      else if (fdata.cFileName[2] == 0)   //  fn=".."
         fn_okay = 0;
      else
         fn_okay = 1;

      if (fn_okay) {
         // printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.cFileName) ;
         // printf("%9ld %04X %s\n", fdata.file_size, fdata.attrib, fdata.cFileName) ;
         filecount++;

         //****************************************************
         //  allocate and initialize the structure
         //****************************************************
         // ftemp = new ffdata;
         ftemp = (struct ffdata *) malloc(sizeof(ffdata)) ;
         if (ftemp == NULL) {
            return -errno;
         }
         memset((char *) ftemp, 0, sizeof(ffdata));

         //  convert filename to lower case if appropriate
         // if (!n.ucase)
         //    strlwr(fblk.name) ;

         ftemp->attrib = (uchar) fdata.dwFileAttributes;

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

         ftemp->dirflag = ftemp->attrib & FILE_ATTRIBUTE_DIRECTORY;

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
      }  //  if file is parseable...

search_next_file:
      //  search for another file
      if (FindNextFile (handle, &fdata) == 0)
         done = 1;
   }

   FindClose (handle);
   return 0;
}

//**********************************************************
//  recursive routine to read directory tree
//**********************************************************
static int read_dir_tree (dirs * cur_node)
{
   dirs *dtail = 0;
   char *strptr;
   HANDLE handle;
   int slen, done, result;
   DWORD err;
   // ULONGLONG file_clusters, clusters;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATAW fdata ; //  long-filename file struct

   // if (((dircount % 50) == 0)  &&  _kbhit()) {
   //    result = _getch() ;
   //    //  check for ESCAPE character
   //    if (result == 27) {
   //       // error_exit(DATA_OKAY, NULL) ;
   //       early_abort = 1 ;
   //    }
   // }
   //  if early_abort flag gets set, return with "success" flag,
   //  but without reading anything further...
   // if (early_abort) 
   //    return 0;

   // pattern_update() ;
   //  Insert next subtree level.
   //  if level == 0, this is first call, and
   //  dirpath is already complete
   if (level > 0) {
      //  insert new path name
      strptr = _tcsrchr (dirpath, '\\');
      strptr++;
      *strptr = 0;
      slen = _tcslen (dirpath);
      _tcscat (dirpath, cur_node->name);
      _tcscat (dirpath, "\\*.*");
   }
   else
      slen = _tcslen (dirpath);

   //  first, build tree list for current level
   level++;

   // if (n.lfn_off) {
   //    save_sfn_base_path(dirpath);
   // }

#ifdef  DESPERATE
debug_dump(dirpath, "entry") ;
#endif
   err = 0;
   WCHAR wfilespec[MAX_PATH+1];
   result = MultiByteToWideChar(CP_ACP, 0, dirpath, -1, wfilespec, (int) _tcslen(dirpath)+1);
   if (result == 0) {
      syslog("%s: a2u failed: %u\n", dirpath, (unsigned) GetLastError());
      return -1;
   }
   
   handle = FindFirstFileW(wfilespec, &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      err = GetLastError ();
      if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(dirpath, "FindFirstFile denied") ;
#endif
         ;                     //  continue reading
      }
      else {
#ifdef  DESPERATE
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
         // sprintf (tempstr, "path [%s]\n", dirpath);
         // nputs (0xA, tempstr);
         // sprintf (tempstr, "FindFirst: %s\n", get_system_message ());
         // nputs (0xA, tempstr);
         return err ;
      }
   }

   //  loop on find_next
   done = 0;
   while (!done) {
      if (!err) {
         //  we found a directory
         if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            bool cut_dot_dirs;

            // printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.fname) ;
            //  skip '.' and '..', but NOT .ncftp (for example)
            if (fdata.cFileName[0] != '.')
               cut_dot_dirs = false;
            else if (fdata.cFileName[1] == 0)
               cut_dot_dirs = true;
            else if (fdata.cFileName[1] == '.' && fdata.cFileName[2] == 0)
               cut_dot_dirs = true;
            else
               cut_dot_dirs = false;

            if (!cut_dot_dirs) {
               cur_node->directs++;
               cur_node->subdirects++;

               dirs *dtemp = new_dir_node ();
               if (cur_node->sons == NULL)
                  cur_node->sons = dtemp;
               else
                  dtail->brothers = dtemp;   //lint !e613
               dtail = dtemp;
               // if (!n.ucase) 
               //    strlwr(ff.name) ;
               
               //  convert Unicode filenames to UTF8
               dtemp->mb_len = wcslen(fdata.cFileName) ;
               int bufferSize ;
               if (fdata.cFileName[0] > 255) {
                  SetConsoleOutputCP(CP_UTF8);
                  bufferSize = WideCharToMultiByte(CP_UTF8, 0, fdata.cFileName, -1, NULL, 0, NULL, NULL);
                  dtail->name = (TCHAR *) malloc(bufferSize + 1); //lint !e732
                  WideCharToMultiByte(CP_UTF8, 0, fdata.cFileName, -1, dtail->name, bufferSize, NULL, NULL);
                  dtail->is_multi_byte = true ;
               }
               else {
                  bufferSize = WideCharToMultiByte(CP_UTF8, 0, fdata.cFileName, -1, NULL, 0, NULL, NULL);
                  dtail->name = (TCHAR *) malloc(bufferSize + 1);  //lint !e732
                  WideCharToMultiByte(CP_ACP, 0, fdata.cFileName, -1, dtail->name, bufferSize, NULL, NULL);
               }

               dtail->attrib = (uchar) fdata.dwFileAttributes;
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
            
            //  In this tree scanner, we will build a file list for each folder
         }                      //  if entry is a file
      }                         //  if no errors detected

      //******************************************
      //  search for another file
      //******************************************
      if (FindNextFileW(handle, &fdata) == 0) {
         // done = 1;
         err = GetLastError ();
         if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(fdata.cFileName, "denied") ;
#else
            ;                     //  continue reading
#endif
         }
         else if (err == ERROR_NO_MORE_FILES) {
            done = 1 ;
         }
         else {
#ifdef  DESPERATE
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
            done = 1 ;
         }
      } else {
         err = 0 ;
      }
   }  //  while reading files from directory

#ifdef  DESPERATE
debug_dump(dirpath, "close") ;
#endif
   FindClose (handle);

   //  next, build tree lists for subsequent levels (recursive)
   dirs *ktemp = cur_node->sons;
   while (ktemp != NULL) {
#ifdef  DESPERATE
if (ktemp == 0) 
   debug_dump("[NULL]", "call read_dir_tree") ;
else
   debug_dump(ktemp->name, "call read_dir_tree") ;
#endif
      read_dir_tree (ktemp);
      cur_node->subdirsize += ktemp->subdirsize;
      cur_node->subdirsecsize += ktemp->subdirsecsize;

      cur_node->subfiles += ktemp->subfiles;
      cur_node->subdirects += ktemp->subdirects;
      ktemp = ktemp->brothers;
   }

   //  when done, strip name from path and restore '\*.*'
   _tcscpy (&dirpath[slen], "*.*");  //lint !e669  string overrun??

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

   //  Extract base path from first filespec,
   //  and strip off filename
   _tcscpy (base_path, tpath);
   strptr = _tcsrchr (base_path, '\\');
   if (strptr != 0)
      *(++strptr) = 0;          //  strip off filename

   // get_disk_info (base_path);

   //  allocate struct for dir listing
   top = new_dir_node ();

   //  Extract base path from first filespec,
   //  and strip off filename
   _tcscpy (base_path, tpath);
   strptr = _tcsrchr (base_path, '\\');
   strptr++;                    //  skip past backslash, to filename
   *strptr = 0;                 //  strip off filename
   base_len = _tcslen (base_path);

   //  derive root path name
   if (_tcslen (base_path) == 3) {
      top->name = (char *) malloc(8) ;
      if (top->name == 0) {
         return -ENOMEM ;
      }
      _tcscpy (top->name, "<root>");
   }
   else {
      _tcscpy (tempstr, base_path);
      tempstr[base_len - 1] = 0; //  strip off tailing backslash
      strptr = _tcsrchr (tempstr, '\\');
      strptr++;                 //  skip past backslash, to filename

      top->name = (char *) malloc(_tcslen (strptr) + 1);
      if (top->name == 0) {
         return -ENOMEM ;
      }
      _tcscpy (top->name, strptr);
   }

   // top->attrib = 0 ;   //  top-level dir is always displayed

   _tcscpy (dirpath, tpath);

   result = read_dir_tree (top);
#ifdef  DESPERATE
debug_dump("exit", "returned from read_dir_tree") ;
#endif
   return result ;
}

//**********************************************************
//  Note: lstr contains form string plus filename
//**********************************************************
static void display_tree_filename (char *formstr, dirs *ktemp)
{
   // char levelstr[PATH_MAX];
   // sprintf (levelstr, "%s%s", frmstr, ktemp->name);
//    int wlen = _tcslen(ktemp->name);
//    //  why is mb_len == 0 on base folder??
//    if (ktemp->mb_len == 0) {
//       //  this won't work for Unicode targets
//       ktemp->mb_len = wlen ;
//    }
//    uint slen = ktemp->mb_len;
   
   //  calculate required padding spaces
   // int frmlen = _tcslen(frmstr);
   // uint splen = 0 ;

   printf("%s %s\n", formstr, ktemp->name) ;
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
         //  show file/directory sizes only
          //   if (ktemp->dirsize != ktemp->subdirsize ||   //lint !e777
          //      ktemp->dirsecsize != ktemp->subdirsecsize) {   //lint !e777
          //      // dsize.convert  (ktemp->dirsize);
          //      // dssize.convert (ktemp->dirsecsize);
          //      // sdsize.convert (ktemp->subdirsize);
          // 
          //      //  now, print the normal directory
          //      // sprintf (tempstr, "%11s", dsize.putstr ());
          //      // nputs (dtree_colors[level], tempstr);
          // 
          //      // sprintf(tempstr, "%11s %14s", dssize_ptr, sdsize_ptr) ;
          //      // sprintf (tempstr, "%11s %14s", dssize.putstr (),
          //      //   sdsize.putstr ());
          //      // nputs (dtree_colors[level], tempstr);
          //      printf("%I64u, %I64u, %I64u ", ktemp->dirsize, ktemp->dirsecsize, ktemp->subdirsize) ;
          //   }
          // 
          //   /*  no subdirectories are under this one  */
          //   else {
          //      //  now, print the normal directory
          //      // sprintf(tempstr, "            %14s", sdsize_ptr) ;
          //      // sdsize.convert (ktemp->subdirsize);
          //      // sprintf (tempstr, "%14s", sdsize.putstr ());
          //      // nputs (dtree_colors[level], tempstr);
          //      printf("%I64u ", ktemp->subdirsize) ;
          //   }                   /* end  else !(ktemp->nsdi) */
          // 
          //   // sprintf(tempstr, "%14s", sdssize_ptr) ;
          //   // sdssize.convert (ktemp->subdirsecsize);
          //   // sprintf (tempstr, "%14s", sdssize.putstr ());
          //   // nputs (dtree_colors[level], tempstr);
          //   printf("%I64u ", ktemp->subdirsecsize) ;
          //   puts("");

      //  build tree string for deeper levels
      if (level > 0) {
         if (ktemp->brothers == NULL)
            formstr[level - 1] = ' ';
         else
            formstr[level - 1] = '|' ; //lint !e743 
      }                         //  if level > 1

      //  process any sons
      level++;
      // if (level <= tree_level_limit) {
         display_dir_tree(ktemp->sons);
      // }
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
       strptr++ ;  //lint !e613  skip past backslash, to filename
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

   //  now, do something with the files that you found   
   // printf("filespec: %s, %u found\n", file_spec, filecount);
   // if (filecount > 0) {
   //    puts("");
   //    for (ffdata *ftemp = ftop; ftemp != NULL; ftemp = ftemp->next) {
   //       printf("%s\n", ftemp->filename);
   //    }
   // 
   // }
   //  now display the resulting directory tree
   display_dir_tree(top);
   return 0;
}

