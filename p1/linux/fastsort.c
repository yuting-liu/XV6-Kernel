#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 129
// global variable
int sortIndex =  0;

struct pair {
	char * sentence;
	char * word;
};

int compstr(const void *a, const void *b) {
	return strcmp(((struct pair*)a) -> word, ((struct pair*)b) -> word);
}

int main(int argc, char *argv[]) {
	// file op
	FILE *fp;
	struct pair *list;
	// fast sort
	if(argc == 2) {
		// read in file
		fp = fopen(argv[1], "r");
		sortIndex = 1;
		if(fp == NULL) {
			fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
			exit(EXIT_FAILURE);
		}
	}
	// get the number of words to be sorted by
	else if (argc == 3){
		fp = fopen(argv[2], "r");
		if(fp == NULL) {
			fprintf(stderr, "Error: Cannot open file %s\n", argv[2]);
			exit(EXIT_FAILURE);
		}
		
		// get 
		char substr[5];
		memcpy(substr, &argv[1][1], strlen(argv[1]) - 1);
		sortIndex = atoi(substr);
		if(sortIndex <= 0) {
			fprintf(stderr, "Error: Bad command line parameters\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		fprintf(stderr, "Error: Bad command line parameters\n");
		exit(EXIT_FAILURE);
	}
	
	// count line number in file
	int lineNum = 0;
	char str[MAX_LEN];
	// get line number of file
	while(fgets(str, MAX_LEN, fp) != NULL) {
		lineNum++;
	}
	// move file stream back to begin
	fseek(fp, 0, SEEK_SET);
	
	// allocate memory for list
	list = malloc(lineNum * sizeof(struct pair));
	if(list == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(EXIT_FAILURE);
	}
	// read in first line
	int i;
	for(i = 0; i < lineNum; ++i) {
		if(fgets(str, MAX_LEN, fp) != NULL) {
			// copy token to list.setence
			int len = strlen(str) + 1;
			if(len > 128) {
				fprintf(stderr, "Line too long\n");
				exit(1);
			}
			list[i].sentence = (char*)malloc(len * sizeof(char));
			if(list[i].sentence == NULL) {
				fprintf(stderr, "malloc failed\n");
				exit(EXIT_FAILURE);
			}
			strcpy(list[i].sentence, str);
			
			// chop the setence into tokens
			// empty line
			if(strlen(str) == 1) {
				list[i].word = (char*)malloc(2 * sizeof(char));
				if(list[i].word == NULL) {
					fprintf(stderr, "malloc failed\n");
					exit(EXIT_FAILURE);
				}
				strcpy(list[i].word, str);
			}
			else {
				char * copy, * token, * last_token = NULL;
				copy = (char*)malloc(len * sizeof(char));
				strcpy(copy, str);
				int count = sortIndex;
				int token_len = 0;
				token = strtok(copy, " \n");
				count--;
				while(count > 0 && !(token == NULL)) {
					token_len = strlen(token) + 1;
					last_token = (char*)malloc(token_len * sizeof(char));
					if(last_token == NULL) {
						fprintf(stderr, "malloc failed\n");
						exit(EXIT_FAILURE);
					}
					strcpy(last_token, token);
					token = strtok(NULL, " \n");
					count--;
				}
				if(token == NULL) {
					token = (char*)malloc(token_len * sizeof(char));
					if(token == NULL) {
						fprintf(stderr, "malloc failed\n");
						exit(EXIT_FAILURE);
					}
					strcpy(token, last_token);
				}
				list[i].word = (char*)malloc((strlen(token) + 1) * sizeof(char));
				if(list[i].word == NULL) {
					fprintf(stderr, "malloc failed\n");
					exit(EXIT_FAILURE);
				}
				strcpy(list[i].word, token);
			}
		}
	}
	// sort the file
	qsort(list, lineNum, sizeof(struct pair), compstr);
	for(i = 0; i < lineNum; ++i) {
		printf("%s", list[i].sentence);
	}
	free(list);
	fclose(fp);
	exit(EXIT_SUCCESS);
}
