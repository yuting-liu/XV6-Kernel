#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "test for function getprocs()!\n");
  printf(1, "\n");
  
  int ppid;
  printf(1, "getprocs() returns result: %d\n", getprocs());
  ppid = getpid();
  printf(1, "current pid is: %d\n", ppid);
  printf(1, "\n");
  
  printf(1, "another getprocs() returns result: %d\n", getprocs());
  ppid = getpid();
  printf(1, "current pid is: %d\n", ppid);
  printf(1, "\n");
 
  exit();
}
