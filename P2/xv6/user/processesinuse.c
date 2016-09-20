#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {\
   printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
   exit();}
#define PROC 7

void spin()
{
	int i = 0, j = 0;
	while(1)
	for(j = 0; j < 10000000;++j)
	{
		i = j % 11;
	}
}
int
main(int argc, char *argv[])
{
   struct pstat st;
   int count = 0;
   int i;
   int pid[NPROC];
   while(i < PROC)
   {
        pid[i] = fork();
	if(pid[i] == 0)
        {
		spin();
		exit();
        }
	i++;
   }
   sleep(500);
   check(getpinfo(&st) == 0, "getpinfo");
   printf(1, "\n**** PInfo ****\n");
   for(i = 0; i < NPROC; i++) {
      if (st.inuse[i]) {
	 count++;
         printf(1, "pid: %d hticks: %d lticks: %d\n", st.pid[i], st.hticks[i], st.lticks[i]);
      }
   }
   for(i = 0; i < PROC; i++)
   {
	kill(pid[i]);
   }
   printf(1,"Number of processes in use %d\n", count);
   check(count == 10, "getpinfo should return 10 processes in use\n");
   printf(1, "Should print 1 then 2");
   exit();
}
