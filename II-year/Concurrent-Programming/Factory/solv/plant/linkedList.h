// Author: Mateusz Burza
#ifndef MB_LINKEDLIST_H
#define MB_LINKEDLIST_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "../common/err.h"
#include "../common/plant.h"

#define LIST_OK (0)
#define LIST_ERROR (-1)
#define LIST_DUPLICATE (1)

#define T_ADDED (0)
#define T_WANTED (1)
#define T_DONE_OK (2)
#define T_DONE_ERROR (3)

typedef struct i_task_t {
  task_t* task;
  pthread_cond_t is_ready;

  short task_status;
} i_task_t;

typedef struct node_t {
  i_task_t* i_task;

  struct node_t* next;
  struct node_t* prev;
} node_t;

typedef struct list_t {
  size_t size;

  node_t* head;
  node_t* tail;
} list_t;


int list_init(list_t* list);

bool is_empty(list_t* list);

void list_destroy(list_t* list);

size_t size(list_t* list);

node_t* back(list_t* list);

node_t* front(list_t* list);

void link_nodes(node_t* base_node, node_t* old_node, node_t* new_node);

// If there exists a task with id `task->id`, then does nothing.
// (returns `LIST_DUPLICATE`)
int emplace_back(list_t* list, task_t* task);

// Here `itask->task->id` should be already unique in the list.
int emplace_back_i(list_t* list, i_task_t* i_task);

// When list is empty then does nothing.
// Caller should call `free` on apropriate `i_task`, calls `free` on last node.
void pop_back(list_t* list);

// Caller should guarantee that `node` is neither `head` nor `tail`.
// Calls `free` on `node`.
void remove_node(list_t* list, node_t* node);

// If can't find such i_task, returns `NULL`.
i_task_t* get_i_task(list_t* list, task_t* task);

#endif //MB_LINKEDLIST_H