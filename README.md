# XV6-Operating System
===
Author: Yuting Liu

In this project, different advanced features and functions are implementedin XV6 operating system.

## Section 1.  Process Surport
New system calls are implemented in system for process management.

(1) `int getprocs(void)`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;returns the number of processes that exist in the system at the time of the call.

(2) `int getpinfo(struct pstat *)`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;returns some basic information about each running process, including how long it has run at each priority (measured in clock ticks) and its process ID.

(3) Adding deference null pointer to help setting up the two-level page table.

## Section 2. Command Line Intepreter (Shell)

Different necessary commands are build for linux environment

### Basic shell

The basic shell is an interactive loop: it repeatedly prints a prompt "whoosh> " (note the space after the greater-than sign), parses the input, executes the command specified on that line of input, and waits for the command to finish. This is repeated until the user types "exit". 

### Build-in commands

Implement different commands, including `exit`, `pwd`, `cd`, `path`, and `ls`

### Redirection commands
Redirect the output into specific output file.
example: `ls > file.txt`

## Section 3. Priority-based Scheduler

Implemented the Multi-level Feedback Queue in XV6 kerenel.

The basic idea is simple: assign each running process a priority, which is an integer number, in this case either 1 (low priority) or 2 (high priority). At any given instance, the scheduler should run processes that have the high priority. If there are two or more processes that have the same high priority, the scheduler should round-robin between them. A low-priority (level 1) process does NOT run as long as there are high-priority jobs available to run.

(1) `int setpri(int num)` which sets the priority for the calling process.

## Section 4. Shared Memory
Implemented the share-memory page to processes that are interested in communicating through memory.

(1) `void *shmem_access(int page_number)`, which should make a shared page available to the process calling it. The page_number argument can range from 0 through 3, and allows for four different pages to be used in this manner.

(2) `int shmem_count(int page_number)`, returns the number of processes currently are sharing a particular shared page.

## Section 5. Multi-threading
Implement differnt multi-threading support system call. Build up the thread library with different functions for multi-threading programming and OS management.

(1) `clone()`, a new system call to create a new thread

(2) `join()` to wait for a thread

(3) `thread_create(void (*start_routine)(void*), void *arg)`, call `malloc()` to create a new user stack, use `clone()` to create the child thread and get it running.

(4) `thread_join()` should also be used, which calls the underlying `join()` system call, frees the user stack, and then returns.

(5) `lock_init`, `lock_acquire()`, and `lock_release()` to control the thread library hardware, using X86 atomic exchange to faciliate spin lock.


## Section 6. File System

### File System Checker
A checker reads in a file system image and makes sure that it is consistent. When it isn't, the checker takes steps to fix the problems it sees; however, we won't be doing any fixes this time to keep your life a little simpler.

### File System with Small File Optimization
The basic idea is that if you have a small file (say, just a few bytes in size), instead of allocating a data block for it, instead just store the data itself inside the inode, thus speeding up access to the small file (as well as saving some disk space). Because the inode has some number of slots for block addresses (specifically, NDIRECT for direct pointers, and 1 more for an indirect pointer), you should be able to store (NDIRECT + 1) * 4 bytes in the inode for these small files (52 bytes or less).
