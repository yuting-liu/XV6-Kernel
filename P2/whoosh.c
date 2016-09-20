#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LONG_LEN 1000
#define MAX_LEN 130

char error_message[30] = "An error has occurred\n";
char promp_message[10] = "whoosh> ";
int write_return, write_promp;
// char * env_args[] = {"PATH=/bin", "USER=me", NULL};
int main(int argc, char* argv[]) {
  // incorrect number of command line arguments
  if (argc != 1) {
    write_return = write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  
  int i;
  char *path_dir[128];
  int path_dir_count;
  path_dir_count = 1;
  while(1) {
    // loop print
    write_promp = write(STDOUT_FILENO, promp_message, strlen(promp_message));
		
    char line[MAX_LONG_LEN];
    // read in command line
    if(fgets(line, MAX_LONG_LEN, stdin) != NULL) {

    char * token, * line_cpy;
    char *list[128];
   
    path_dir[0] = "/bin/";
    
    if(strlen(line) > 129) {
      write_return = write(STDERR_FILENO, error_message, strlen(error_message));
      continue;
    }
    int len = strlen(line) + 1;
      
    int token_len = 0;
    // copy line
    line_cpy = (char * )malloc(len * sizeof(char));
    strcpy(line_cpy, line);

    // no command came in
    if(strcmp(line, "\n") == 0) {
      continue;
    }
    // all whitespaces
    int count_white;
    count_white = 0;
    while(isspace(line[count_white])) {
      count_white++;
    }
    if(count_white == len - 1) {
      continue;
    }
    // store str tokens into list
    token = strtok(line_cpy, " \n");
    token_len = strlen(token) + 1;
    list[0] = (char*) malloc(token_len * sizeof(char));
    strcpy(list[0], token);
    int count = 0;
    count = 0;
    while(!(token == NULL)) {
      token_len = strlen(token) + 1;
      list[count] = (char*)malloc(token_len * sizeof(char));
      strcpy(list[count], token);
      token = strtok(NULL, " \n");
      count++;
    }
    char path_buff[1024];
    int continue_flag;
    int redirection;
    // exit
    if(strcmp(list[0], "exit") == 0) {
      if(count != 1) {
        write_return = write(STDERR_FILENO, error_message, strlen(error_message));
        continue;
      }
      // incorrect length of command line argument 
      else {
        exit(0);
      }
    }
    // pwd
    else if(strcmp(list[0], "pwd") == 0) {
      // incorrect length of command line argment
      if(count != 1) {
        write_return = write(STDERR_FILENO, error_message, strlen(error_message));
        continue;
      }
      else if(getcwd(path_buff, sizeof(path_buff))) {
        strcat(path_buff, "\n");
	write_return = write(STDOUT_FILENO, path_buff, strlen(path_buff));
      }
      // getcwd error
      else {
	write_return = write(STDERR_FILENO, error_message, strlen(error_message));
	continue;
      }
    }
    // cd
    else if(strcmp(list[0], "cd") == 0) {
    // $ cd
      if(count == 1) {
	char * h_path;
	h_path = getenv("HOME");
	
	// change dir not successful
	if(chdir(h_path) != 0) {
	  printf("dfd\n");
	  write_return = write(STDERR_FILENO, error_message, strlen(error_message));
	  continue;
	}
      }
      else if(count == 2) {
        // $ cd dir
        if(chdir(list[1]) != 0) {
	  write_return = write(STDERR_FILENO, error_message, strlen(error_message));
          continue;
	  }
        }
        // incorrect number of command line arguments
        else {
          write_return = write(STDERR_FILENO, error_message, strlen(error_message));
          continue;
        }
      }
      // path
      else if(strcmp(list[0], "path") == 0) {
	
	for(i = 1; i < count; ++i) {
          // return zero when success
          path_dir[i] = (char*)malloc((strlen(list[i]) + 1) * sizeof(char));
          strcpy(path_dir[i], list[i]);
	  strcat(path_dir[i], "/");
          path_dir_count++;
	}
      }
      // non-built in command
      else {
	struct stat statbuf;
	char bin_dir[100];
	int has_exec;
	has_exec = 0;
	for(i = 0; i < path_dir_count; ++i) {
	  strcpy(bin_dir, path_dir[i]);
	  strcat(bin_dir, list[0]);
	  // printf("%s\n", bin_dir);
	  if(stat(bin_dir, &statbuf) == 0) {
	    strcpy(list[0], bin_dir);
	    has_exec = 1;
	    break;
	  }
	}
	if(has_exec == 0) {
	  write_return = write(STDERR_FILENO, error_message, strlen(error_message));
	  continue;
	}
        
        continue_flag = 0;
        redirection = 0;
        // redirection
        for(i = 0; i < count; ++i) {
          // ">" appeas at the illegal sposition
          if(strcmp(list[i], ">") == 0 && i != count - 2) {
            continue_flag = 1;
	    break;
          }
	  else if(strcmp(list[i], ">") == 0 && i == count - 2) {
            redirection = 1;
          }
        }
        
        // error format command
        if(continue_flag == 1) {
          write_return = write(STDERR_FILENO, error_message, strlen(error_message));
	  continue;
        }
	
        // decide whether or not redirection
	int pid = fork();
        int dup_err = 0, dup_out = 0, dup2_err = 0, dup2_out = 0;
	int fd_out = 0;
	int  fd_err = 0;

        // child
        if(pid == 0) {
	  list[count] = NULL;
          // redirection
          if(redirection) {
	    dup_err = dup(STDERR_FILENO);
            dup_out = dup(STDOUT_FILENO);
	    if(dup2_err < 0 || dup2_out < 0) {
		write_return = write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
	    }
            close(STDOUT_FILENO);
	    close(STDERR_FILENO);
	    
	    char copy[128];
	    strcpy(copy,list[count - 1]);
            
            fd_out = open(strcat(copy, ".out"), O_CREAT | O_TRUNC | O_WRONLY,
	      S_IRUSR | S_IWUSR);
	    fd_err = open(strcat(list[count - 1], ".err"), O_CREAT | O_TRUNC | O_WRONLY,
              S_IRUSR | S_IWUSR);
	    
	    if(fd_out < 0 || fd_err < 0) {
	      dup2_err = dup2(dup_err, STDERR_FILENO);
	      dup2_out = dup2(dup_out, STDOUT_FILENO);
	      close(dup_err);
	      close(dup_out);
	      write_return = write(STDERR_FILENO, error_message, strlen(error_message));
              exit(1);
	    }
	    
            // run command
            // change file descriptor to this one
	  
            // NULL last two elements
            list[count - 1] = NULL;
            list[count - 2] = NULL;
            
            execv(list[0], list);
            write_return = write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
          } 
          else {
	    
            // run command
            execv(list[0], list);
            write_return = write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
          }
          
	}
        // parent
        else if(pid > 0) {
	 
          if((int)wait(NULL)!= pid) {
            write_return = write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
          }
        }
        // fork error
        else {
          write_return = write(STDERR_FILENO, error_message, strlen(error_message));
     	  exit(1);
        }
      }
    }
  }
  exit(0);
}
