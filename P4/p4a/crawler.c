#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>

#define SET_SIZE 100

char * (*fetch_fn)(char *url);
void (*edge_fn)(char *from, char *to);
// link_queue variables
char **link_buffer;
int link_fill_ptr, link_use_ptr, link_size, numfull;
// Store the number of items that are in the queue or being used
int item_count; 

// Conditional variables
pthread_cond_t link_empty, link_full, page_empty;
pthread_mutex_t link_m, page_m, hashset_m, count_m;

// Thread pools
pthread_t *downloader_pool, *parser_pool;
int downloader_size, parser_size;

struct list_node {
  struct list_node *next;
  char *link;
};

typedef struct list_node list_node_t;

struct hashset_node {
  list_node_t *head;
}; 

typedef struct hashset_node hashset_node_t; 

// Hashset
hashset_node_t hashset[SET_SIZE];

// page_queue variables
struct page_node {
  char *page;
  char *link;
  struct page_node *next;
};

typedef struct page_node page_node_t;

page_node_t *page_fill_ptr, *page_use_ptr;

// Methods for hashset
void create_set() {
  int i;
  for (i = 0; i < SET_SIZE; ++i) {
    hashset[i].head = malloc(sizeof(list_node_t));
  }
}

void destroy_list(list_node_t *node) {
  if (!node) {
    return;
  }
  
  destroy_list(node->next);
  free(node);
}

void destroy_set() {
  int i;
  for (i = 0; i < SET_SIZE; ++i) {
    destroy_list(hashset[i].head);
  }
}

// From https://en.wikipedia.org/wiki/Fletcher%27s_checksum
uint32_t fletcher32 (uint16_t const *data, size_t words)
{
  uint32_t sum1 = 0xffff, sum2 = 0xffff;
  size_t tlen;
 
  while (words) {
    tlen = words >= 359 ? 359 : words;
    words -= tlen;
    do {
            sum2 += sum1 += *data++;
    } while (--tlen);
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }
  /* Second reduction step to reduce sums to 16 bits */
  sum1 = (sum1 & 0xffff) + (sum1 >> 16);
  sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  return sum2 << 16 | sum1;
}

int add(hashset_node_t *hashset, char* link) {
  uint32_t hash_code = fletcher32((uint16_t const *)link, sizeof(char*));
  
  list_node_t *item = hashset[hash_code % SET_SIZE].head;
  
  while (item->next) {
    if (strcmp(item->next->link, link) == 0) {
      return 0;
    }
    
    item = item->next;
  }
  
  item->next = malloc(sizeof(list_node_t));
  item->next->link = link;
  return 1;
}

// Methods for link_queue (fixed size)
void link_init(int size) {
  link_buffer = (char**) malloc(size * sizeof(char*)); 
  if (link_buffer == NULL) {
    printf("FATAL ERROR: malloc failed\n");
  }
  link_fill_ptr = 0;
  link_use_ptr = 0;
  numfull = 0;
  link_size = size;
  if (pthread_cond_init(&link_empty, NULL) != 0) {
    printf("FATAL ERROR: link_empty initialization failed\n");
  }
  if (pthread_cond_init(&link_full, NULL) != 0) {
    printf("FATAL ERROR: link_full initialization failed\n");
  }
  if (pthread_mutex_init(&link_m, NULL) != 0) {
    printf("FATAL ERROR: link_m initialization failed\n");
  }
}

void link_fill(char *link) {
  link_buffer[link_fill_ptr] = link;
  link_fill_ptr = (link_fill_ptr + 1) % link_size;
  numfull++;
}

char* link_get() {
  char* link = link_buffer[link_use_ptr];
  link_use_ptr = (link_use_ptr + 1) % link_size;
  numfull--;
  return link;
}

// Methods for page_queue (unbounded)
void page_init() {
  page_fill_ptr = malloc(sizeof(page_node_t));
  page_use_ptr = page_fill_ptr;
  if (pthread_cond_init(&page_empty, NULL) != 0) {
    printf("FATAL ERROR: page_empty initialization failed\n");
  }
  if (pthread_mutex_init(&page_m, NULL) != 0) {
    printf("FATAL ERROR: page_m initialization failed\n");
  }
}

void page_fill(char* page, char* link) {
  if ((page_fill_ptr->next = malloc(sizeof(page_node_t))) == NULL) {
    printf("FATAL ERROR: malloc failed\n");
    exit(1);
  }
  page_fill_ptr->next->page = page;
  page_fill_ptr->next->link = link;
  page_fill_ptr = page_fill_ptr->next;
}

page_node_t* page_get() {
  page_node_t *next = page_use_ptr->next;
  //free(page_use_ptr);
  page_use_ptr = next;
  return next;
}

void destroy_page_queue() {
  while (page_use_ptr) {
    page_node_t *next = page_use_ptr->next;
    free(page_use_ptr);
    page_use_ptr = next;
  }
}

void clean_up() {
  destroy_set();
  free(link_buffer);
  destroy_page_queue();
}

// Parser/Downloader
void* parser(void *arg) {  
  while (1) {
    // Get page from page_queue
    pthread_mutex_lock(&page_m);
    // When page_queue is empty, wait
    while (!page_use_ptr->next) {
      pthread_cond_wait(&page_empty, &page_m);
    }

    page_node_t *node = page_get();
    char* page = node->page;
    char* link = node->link;
    
    pthread_mutex_unlock(&page_m);
    
    // Parse page
    char* saveptr, *token;
    token = strtok_r(page, " \n\t", &saveptr);
    while (token) {
      if (strlen(token) > 5) {
        char subbuff[6];
        memcpy(subbuff, token, 5);
        subbuff[5] = '\0';
        // link found
        if (strcmp(subbuff, "link:") == 0) {
          edge_fn(link, &token[5]);
          pthread_mutex_lock(&hashset_m);
          int add_result = add(hashset, &token[5]);
          pthread_mutex_unlock(&hashset_m);                    
          
          if (add_result) {
            pthread_mutex_lock(&link_m);
            while (numfull == link_size) {
              pthread_cond_wait(&link_empty, &link_m);
            }
            link_fill(&token[5]);
            
            pthread_mutex_lock(&count_m);
            item_count++;
            pthread_mutex_unlock(&count_m);
            
            pthread_cond_signal(&link_full);
            pthread_mutex_unlock(&link_m);  
          }
        }
      }
      token = strtok_r(NULL, " \n\t", &saveptr);      
    }
    
    // After a page is parsed, the item count will decrease
    // free(page);
    pthread_mutex_lock(&count_m);
    item_count--;
    pthread_mutex_unlock(&count_m);
    
  }
  
  return NULL;
}

void* downloader(void *arg) {
  while(1) {
    // get link from link queue
    pthread_mutex_lock(&link_m);
    // when link queue is empty
    while(numfull == 0) {
      pthread_cond_wait(&link_full, &link_m);
    }
    // get link
    char *link = link_get();
    pthread_cond_signal(&link_empty);
    pthread_mutex_unlock(&link_m);
    
    // download page from link
    char *page = fetch_fn(link);
    
    // insert page into queue directly
    pthread_mutex_lock(&page_m);
    page_fill(page, link);
    // Update counts
    pthread_mutex_lock(&count_m);
    item_count++;
    pthread_mutex_unlock(&count_m);
    
    pthread_cond_signal(&page_empty);
    pthread_mutex_unlock(&page_m);
    
    pthread_mutex_lock(&count_m);
    item_count--;
    pthread_mutex_unlock(&count_m);
  
  }
  
  return NULL;
}

void *terminate(void *arg) {
  while (item_count)
    ; // spin

  int i;
  for (i = 0; i < downloader_size; ++i) {
    pthread_cancel(downloader_pool[i]);
  }
  for (i = 0; i < parser_size; ++i) {
    pthread_cancel(parser_pool[i]);
  }
  return NULL;
}

int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

  fetch_fn = _fetch_fn;
  edge_fn = _edge_fn;
  downloader_size = download_workers;
  parser_size = parse_workers;
  // Create two pools of threads: one for parser, one for downloader  
  downloader_pool = malloc(download_workers * sizeof(pthread_t));
  parser_pool = malloc(parse_workers * sizeof(pthread_t));
  pthread_t terminator;
  int i;
  
  // Create fixed size queue for links
  link_init(queue_size);
  page_init();
  create_set(hashset);
  add(hashset, start_url);
  link_fill(start_url);
  item_count = 1;
  pthread_mutex_init(&count_m, NULL);
  
  pthread_create(&terminator, NULL, &terminate, NULL);
  
  for (i = 0; i < download_workers; ++i) {
    pthread_create(&downloader_pool[i], NULL, &downloader, NULL);
  }
 
  for (i = 0; i < parse_workers; ++i) {
    pthread_create(&parser_pool[i], NULL, &parser, NULL);
  }
  
  pthread_join(terminator, NULL);
  
  free(downloader_pool);
  free(parser_pool);
  clean_up();
  
  return 0;
}
