#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/user.h>

#include "elfhash.h"

#define ZZZ_KEEP_GNU_HASH 1

/*wraper functions for 32bit/64bit*/
static int is_valid_elf(char *base)
{
 if(is_32bit_elf(base))
    return is_valid_elf32(base);
 if(is_64bit_elf(base))
    return is_valid_elf64(base);
 return -1;
}

static int has_gnuhash(char *base)
{
 if(is_32bit_elf(base))
   return has_gnuhash32(base);
 if(is_64bit_elf(base))
   return has_gnuhash64(base);
 return -1;
}

static int rename_func(char *base, const char *old_func, const char *new_func)
{
 if(is_32bit_elf(base))
   return rename_func32(base, old_func, new_func);
 if(is_64bit_elf(base))
   return rename_func64(base, old_func, new_func);
 return -1;
}

static int rehash(char *base, const char *new_func)
{
 if(is_32bit_elf(base))
    return rehash32(base, new_func);
 if(is_64bit_elf(base))
    return rehash64(base, new_func);
 return -1;
}
static int convert_gnu_to_sysv(char *base, unsigned long size, unsigned long gap)
{
 if(is_32bit_elf(base))
    return convert_gnu_to_sysv32(base, size, gap);
 if(is_64bit_elf(base))
    return convert_gnu_to_sysv64(base, size, gap);
 return -1;
}
static void elfhash_listcontent(char* base)
{
 if(is_32bit_elf(base))
    elfhash_listcontent32(base);
 if(is_64bit_elf(base))
    elfhash_listcontent64(base);
}
unsigned long elfhash_compute_gap(void*base)
{
 if(is_32bit_elf(base))
    return elfhash_compute_gap32(base);
 if(is_64bit_elf(base))
    return elfhash_compute_gap64(base);
 return -1;
}
#ifdef ZZZ_KEEP_GNU_HASH
static int elfhash_dump_gnuhash(char *base, const char *test_str)
{
 if(is_32bit_elf(base))
   return dump_gnuhash32(base, test_str);
 if(is_64bit_elf(base))
   return dump_gnuhash64(base, test_str);
 return -1;
}
#endif

static void elfhash_usage(const char*name)
{
  printf("Manipulate and convert elf hash section and replace elf dynamic symbols\n");
  printf("\t\t\t---By Cjacker <cjacker@gmail.com>\n\n");
  printf("Usage: %s <options> file\n\n", name);
  printf("Options:\n");
  printf("  -l  list elf contents\n\n");
  printf("  -r  rehash gnu.hash\n\n");
  printf("  -f  oldname -t newname \n");
  printf("      replace the dynamic symbol from oldname to newname and rehash the elf\n\n");
  printf("  -s  name_to_search \n");
  printf("      search the symbol name\n\n");
  printf("  -h  display this message\n");
  exit(0);
}


struct globalArgs_t {
  int list;  //-l  list elf contents
  int help;  //-h  help
  int reh;   //-r  rehash
  char *old_func; //-f old function be renamed.
  char *new_func; //-t new function rename to.
  char *search_func; //-t new function rename to.
  char **inputFiles; // input file;
  int numInputFiles; // count of input file, should always be 1. this is we limited.
} globalArgs;


static const char *optString = "lhrcf:s:t:";

int main(int argc, char**argv)
{
  if(argc == 1) {
    elfhash_usage(argv[0]);
  }

  int opt = 0;
  globalArgs.list = 0;
  globalArgs.help = 0;
  globalArgs.reh = 0;
  globalArgs.old_func = NULL;
  globalArgs.new_func = NULL;
  globalArgs.search_func = NULL;
  globalArgs.inputFiles = NULL;
  globalArgs.numInputFiles = 0;

  opt = getopt( argc, argv, optString );
  while( opt != -1 ) {
    switch( opt ) {
    case 'l': 
      globalArgs.list = 1;
      break;
    case 'r':
      globalArgs.reh = 1;
      break;
    case 'f':
      globalArgs.old_func = optarg;
      break;
    case 't':
      globalArgs.new_func = optarg;
      break;
    case 's':
      globalArgs.search_func = optarg;
      break;
    case 'h':
      elfhash_usage(argv[0]);
      break;
    default:
      //never here.
      break;
    }
    opt = getopt( argc, argv, optString );
  }
  globalArgs.inputFiles = argv + optind;
  globalArgs.numInputFiles = argc - optind;

  if(globalArgs.numInputFiles != 1) { 
    fprintf(stderr,"Please supply one elf file each time\n");
    exit(0);
  }

  // -f and -t must be used at same time.
  if((!globalArgs.old_func)^(!globalArgs.new_func)) {
    fprintf(stderr,"Please use -f and -t at the same time\n");
    exit(0);
  }

  if(globalArgs.old_func && globalArgs.new_func 
     && (strlen(globalArgs.old_func) != strlen(globalArgs.new_func))) {
    fprintf(stderr, "The old and new function name must be same length!\n");
    exit(0);
  }


  /* ** file init */
  const char *filename = globalArgs.inputFiles[0];
  // const char *filename = "/home/vargo/work/tmp/libacdoec/arm/libacodec.so";


  int fd = open(filename, O_RDWR);
  if(fd == -1) {
    fprintf(stderr, "file '%s' cannot be opened (%s).\n", filename, strerror(errno));
    return 0;
  }
  int rc;
  struct stat s;
  rc = fstat(fd, &s);
  unsigned long size = s.st_size;
  //NOTE, here we map it as readonly.
  char *base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0); 
  if(!base) {
    fprintf(stderr, "file '%s' is not readable (%s).\n", filename, strerror(errno));
    return 0;
  }


  if(!is_valid_elf(base)) {
    close(fd);
    exit(1);
  }
  
  if(is_32bit_elf(base))
    printf("%s is a 32bit elf\n", filename);
  else if(is_64bit_elf(base))
    printf("%s is a 64bit elf\n", filename);
  else
    exit(1);
 
  //if there is .gnu.hash exist, convert it except you use -l option
  if(has_gnuhash(base) && !globalArgs.list) {

#ifdef ZZZ_KEEP_GNU_HASH

	  // print gnu hash info
	  // elfhash_dump_gnuhash(base);
	  // munmap(base, size);
	  // base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    // int ret = rename_func(base, globalArgs.old_func, globalArgs.new_func);
    // //after rename, rehash
    // if(ret)
    //   rehash(base, globalArgs.new_func);
	  // close(fd);

	  // return 0;
#else
    /* Calculate gap */
    unsigned long gap = elfhash_compute_gap(base);

    /* Grow the file */
    munmap(base, size);
    size += gap;
    rc = ftruncate(fd, size);
    if(rc) {
      return 1;
    }
    base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(!base) {
      return 1;
    }
    convert_gnu_to_sysv(base, size, gap);
#endif
  }
  //process options.
  if(globalArgs.list) {
     /* show elf contents */
    elfhash_listcontent(base);
  } else if(globalArgs.reh) {
    /* rehash */
    //remap it to write.
    munmap(base, size);
    base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    //rehash
    rehash(base, "");
  } else if(globalArgs.old_func || globalArgs.new_func) {
      munmap(base, size);
      base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
      int ret = rename_func(base, globalArgs.old_func , globalArgs.new_func);
      //after rename, rehash
      if(ret)
        rehash(base, globalArgs.new_func);
  } else if(globalArgs.search_func) {
    elfhash_dump_gnuhash(base, globalArgs.search_func);
  }
 
  close(fd);
  return 0;
}
