// Author: Mateusz Burza
#include "linkedList.h"

int list_init(list_t* list)
{
  if (list == NULL) {
    return LIST_ERROR;
  }

  list->head = (node_t*) malloc(sizeof(node_t));
  if (list->head == NULL) {
    // Memory allocation error
    list = NULL;
    return LIST_ERROR;
  }

  list->tail = (node_t*) malloc(sizeof(node_t));
  if (list->tail == NULL) {
    // Memory allocation error
    free(list->head);
    list = NULL;
    return LIST_ERROR;
  }

  list->head->prev = NULL;
  list->head->next = list->tail;

  list->tail->prev = list->head;
  list->tail->next = NULL;

  list->size = 0;

  // Succesfull initialisation
  return LIST_OK;
}

bool is_empty(list_t* list)
{
  return list != NULL && list->size == 0;
}

void list_destroy(list_t* list)
{
  if (list == NULL) {
    return;
  }

  while (!is_empty(list)) {
      node_t* last = back(list);
      i_task_t* to_free = last->i_task;

      ASSERT_ZERO(pthread_cond_destroy(&to_free->is_ready));
      free(to_free);

      pop_back(list);
  }

  free(list->head);
  free(list->tail);

  list->head = NULL;
  list->tail = NULL;
}

size_t size(list_t* list)
{
  if (list == NULL) {
    return 0;
  }

  return list->size;
}

node_t* back(list_t* list)
{
  if (list == NULL || is_empty(list)) {
    return NULL;
  }

  return list->tail->prev;
}

node_t* front(list_t* list)
{
  if (list == NULL || is_empty(list)) {
    return NULL;
  }

  return list->head->next;
}

void link_nodes(node_t* base_node, node_t* old_node, node_t* new_node)
{
    if (base_node != NULL) {
        if (base_node->next == old_node) {
            base_node->next = new_node;
        } else {
            base_node->prev = new_node;
        }
    }
}

int emplace_back(list_t* list, task_t* task)
{
  node_t* curr_node = front(list);
  size_t list_size = size(list);

  for (size_t i = 0; i < list_size; i++) {
    if (curr_node->i_task->task->id == task->id) {
      return LIST_DUPLICATE;
    }

    curr_node = curr_node->next;
  }

  node_t* new_node = (node_t*) malloc(sizeof(node_t));

  if (new_node == NULL) {
    return LIST_ERROR;
  }

  new_node->i_task = (i_task_t*) malloc(sizeof(i_task_t));

  if (new_node->i_task == NULL) {
    free(new_node);
    return LIST_ERROR;
  }

  ASSERT_ZERO(pthread_cond_init(&new_node->i_task->is_ready, NULL));

  new_node->i_task->task = task;
  new_node->i_task->task_status = T_ADDED;

  new_node->next = list->tail;
  new_node->prev = list->tail->prev;

  link_nodes(list->tail->prev, list->tail, new_node);
  link_nodes(list->tail, list->tail->prev, new_node);

  list->size++;

  return LIST_OK;
}

int emplace_back_i(list_t* list, i_task_t* i_task)
{
  node_t* new_node = (node_t*) malloc(sizeof(node_t));

  if (new_node == NULL) {
    return LIST_ERROR;
  }

  new_node->i_task = i_task;
  new_node->next = list->tail;
  new_node->prev = list->tail->prev;

  link_nodes(list->tail->prev, list->tail, new_node);
  link_nodes(list->tail, list->tail->prev, new_node);

  list->size++;

  return LIST_OK;
}

void pop_back(list_t* list)
{
  if (is_empty(list)) {
    return;
  }

  node_t* to_remove = back(list);
  remove_node(list, to_remove);
}

void remove_node(list_t* list, node_t* node)
{
  link_nodes(node->prev, node, node->next);
  link_nodes(node->next, node, node->prev);

  free(node);

  list->size--;
}

i_task_t* get_i_task(list_t* list, task_t* task)
{
  if (is_empty(list) || task == NULL) {
    return NULL;
  }

  node_t* curr_node = front(list);
  size_t list_size = size(list);

  for (size_t i = 0; i < list_size; i++) {
    if (curr_node->i_task->task->id == task->id) {
        return curr_node->i_task;
    }

    curr_node = curr_node->next;
  }

  return NULL;
}
