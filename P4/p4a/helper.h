#ifndef _HELPER_H
#define _HELPER_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 20
#define check(exp, msg) if(exp) {} else {\
   printf("%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
   exit(1);}

void *Malloc(size_t size) {
  void *r = malloc(size);
  assert(r);
  return r;
}

char *Strdup(const char *s) {
  void *r = strdup(s);
  assert(r);
  return r;
}

char *parseURL(const char *link)
{
	char *url = strdup(link);
	char *saveptr;
	char *token = strtok_r(url, "/", &saveptr);
	char *word = token;
	while(token != NULL)
	{
		word = token;
		token = strtok_r(NULL, "/", &saveptr);	
	} 
	return word;
}

int compare(const void* line1, const void* line2)
{
	char *str1 = (char *) line1;
	char *str2 = (char *) line2;
	//printf("%s %s %d\n", str1, str2, strcmp(str1, str2));
	return strcmp(str1, str2);
}

int isDifferent(char actual[100][50], char expected[100][50], int size)
{
	int i = 0;
	for(i = 0; i < size; ++i)
	{
		if(strcmp(actual[i], expected[i]) != 0)
			return 1;
	}
	return 0;
}

void print(char buffer[100][50], int size)
{
	int i = 0;
	for(i = 0; i < size; ++i)
	{
		printf("%s", buffer[i]);
	}
}

void printData(char actual[100][50], int size1, char expected[100][50], int size2)
{
	printf("Actual Output (sorted):\n\n");
	print(actual, size1);
	printf("\nExpected Output (sorted): \n\n");
	print(expected, size2);
}

int compareOutput(char actual[100][50], int size1, char *file)
{
	char expected[100][50];
	FILE *fp = fopen(file, "r");	
	if(fp == NULL)
		fprintf(stderr, "Unable to open the file %s", file);
	int size2 = 0;
	while(fgets(expected[size2], 50, fp) != NULL)
	{
		size2++;
	}
        qsort(actual, size1, 50, compare);
	qsort(expected, size2, 50, compare);
	fclose(fp);
	if(size1 != size2)
	{
		printf("wrong size\n");
		printData(actual, size1, expected, size2);
		return 1;
	}
	else if(isDifferent(actual, expected, size1))
	{
		printf("mismatch\n");
		printData(actual, size1, expected, size2);
		return 1;
	}
	return 0;
}

#endif
