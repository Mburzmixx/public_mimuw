// Author: Mateusz Burza
#include "linkedList.h"

#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define W_AVAILABLE (0)
#define W_WORKING (1)
#define W_NOT_AVAILABLE (2)
#define W_WILL_COME (3)
#define W_WAITING (4)

typedef struct w_kit_t {
  i_task_t* i_task;
  int position;
  int station;

  pthread_cond_t work;
  atomic_bool end_of_work;
} w_kit_t;

atomic_bool is_plant_working = false;
// To prevent parallel executions of `init_plant`.
atomic_bool started_init = false;
atomic_bool end_of_tasks = false;

pthread_mutex_t mutex;
pthread_cond_t smth_to_add;
pthread_cond_t smth_to_do;
pthread_cond_t noone_waiting;
pthread_cond_t some_station;
pthread_cond_t some_worker;

list_t task_archive;
list_t tasks_to_do;
list_t tasks_to_add;

worker_t** workers;
short* worker_status;
w_kit_t* worker_kits;

int* stations_capacity;
int* people_on_stations;

int num_workers;
int num_stations;

int free_stations;
int free_workers;
int min_needed_workers;

int best_station;
int human_capital;

int num_waiting;
int num_processed;

pthread_t task_master;
pthread_t hr_master;
pthread_t* worker_threads;


static void* task_master_main(void* data);
static void* hr_master_main(void* data);
static void* worker_main(void* data);

static int max(int a, int b);
static int min(int a, int b);
static void cleanup_structures();


int init_plant(int* stations, int n_stations, int n_workers)
{
  if (atomic_exchange(&started_init, true)) {
    // This means that someone else called `init_plant`.
    return ERROR;
  }

  free_stations = 0;
  free_workers = 0;
  min_needed_workers = 1;
  best_station = 0;
  human_capital = 0;
  num_waiting = 0;
  num_processed = 0;

  atomic_store(&end_of_tasks, false);

  ASSERT_ZERO(pthread_mutex_init(&mutex, NULL));
  ASSERT_ZERO(pthread_cond_init(&smth_to_add, NULL));
  ASSERT_ZERO(pthread_cond_init(&smth_to_do, NULL));
  ASSERT_ZERO(pthread_cond_init(&noone_waiting, NULL));
  ASSERT_ZERO(pthread_cond_init(&some_station, NULL));
  ASSERT_ZERO(pthread_cond_init(&some_worker, NULL));

  bool mem_error_f = false;
  mem_error_f |= (list_init(&task_archive) != LIST_OK);
  mem_error_f |= (list_init(&tasks_to_do) != LIST_OK);
  mem_error_f |= (list_init(&tasks_to_add) != LIST_OK);

  num_stations = n_stations;
  num_workers = n_workers;
  human_capital = n_workers;

  // After checking for errors, I will initalize values in all arrays.
  workers = (worker_t**) calloc(num_workers, sizeof(worker_t*));
  mem_error_f |= (workers == NULL);

  worker_status = (short*) malloc(num_workers * sizeof(short));
  mem_error_f |= (worker_status == NULL);

  worker_kits = (w_kit_t*) malloc(num_workers * sizeof(w_kit_t));
  mem_error_f |= (worker_kits == NULL);

  if (worker_kits != NULL) {
    for (size_t i = 0; i < num_workers; i++) {
      ASSERT_ZERO(pthread_cond_init(&worker_kits[i].work, NULL));
      atomic_store(&worker_kits[i].end_of_work, false);
      
      worker_kits[i].position = 0;
      worker_kits[i].station = 0;
      worker_kits[i].i_task = NULL;
    }
  }

  stations_capacity = (int*) malloc(num_stations * sizeof(int));
  mem_error_f |= (stations_capacity == NULL);

  people_on_stations = (int*) calloc(num_stations, sizeof(int));
  mem_error_f |= (people_on_stations == NULL);

  worker_threads = (pthread_t*) malloc(num_workers * sizeof(pthread_t));
  mem_error_f |= (worker_threads == NULL);

  if (mem_error_f) {
    // Handle memory allocation errors
    cleanup_structures();
    atomic_store(&started_init, false);
    return ERROR;
  }

  for (size_t i = 0; i < num_stations; i++) {
    stations_capacity[i] = stations[i];
  }

  free_stations = num_stations;

  for (size_t i = 0; i < num_stations; i++) {
    best_station = max(best_station, stations_capacity[i]);
  }

  for (size_t i = 0; i < num_workers; i++) {
    worker_status[i] = W_WILL_COME;
  }

  // YAY!
  atomic_store(&is_plant_working, true);

  ASSERT_ZERO(pthread_create(&task_master, NULL, task_master_main, NULL));
  ASSERT_ZERO(pthread_create(&hr_master, NULL, hr_master_main, NULL));

  for (size_t i = 0; i < num_workers; i++) {
    int* worker_arg = (int*) malloc (sizeof(int));
    *worker_arg = i;
    ASSERT_ZERO(pthread_create(&worker_threads[i], NULL, worker_main, (void*) worker_arg));
  }

  return PLANTOK;
}

int destroy_plant(void)
{
  if (!atomic_load(&is_plant_working)) {
    return ERROR;
  }

  atomic_store(&is_plant_working, false);
  atomic_store(&started_init, false);
  
  ASSERT_ZERO(pthread_cond_signal(&smth_to_add));

  ASSERT_ZERO(pthread_join(hr_master, NULL));

  atomic_store(&end_of_tasks, false);

  cleanup_structures();
  return PLANTOK;
}

int add_worker(worker_t* w)
{
  if (!atomic_load(&is_plant_working) || w == NULL) {
    return ERROR;
  }

  ASSERT_ZERO(pthread_mutex_lock(&mutex));

  size_t position;
  bool found_a_slot = false;
  // If this worker hasn't been already registered,
  // then I should find a place for him.
  for (size_t i = 0; i < num_workers; i++) {
    if (workers[i] == NULL) {
      position = i;
      found_a_slot = true;
      break;
    }

    if (w->id == workers[i]->id) {
      // This means that this worker was already registered.
      ASSERT_ZERO(pthread_mutex_unlock(&mutex));
      return PLANTOK;
    }
  }

  if (!found_a_slot) {
    // Happens only if assumptions, about num_workers, were not met.
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return ERROR;
  }

  workers[position] = w;
  worker_status[position] = W_WAITING;
  ASSERT_ZERO(pthread_cond_signal(&worker_kits[position].work));

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));
  
  return PLANTOK;
}

int add_task(task_t* t)
{
  if (!atomic_load(&is_plant_working) || t == NULL) {
    return ERROR;
  }

  ASSERT_ZERO(pthread_mutex_lock(&mutex));
  
  int flag = emplace_back(&task_archive, t);

  if (flag == LIST_ERROR) {
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return ERROR;

  } else if (flag == LIST_DUPLICATE) {
    // If this task was already added.
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return PLANTOK;
  }

  node_t* back_node = back(&task_archive);
  i_task_t* processed_task = back_node->i_task;

  if (emplace_back_i(&tasks_to_add, processed_task) == LIST_ERROR) {
    // Deleting task from `task_archive` to keep the integral state.
    pop_back(&task_archive);
    free(processed_task);
    
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return ERROR;
  }

  ASSERT_ZERO(pthread_cond_signal(&smth_to_add));
  ASSERT_ZERO(pthread_mutex_unlock(&mutex));

  return PLANTOK;
}

int collect_task(task_t* t)
{
  if (!atomic_load(&is_plant_working) || t == NULL) {
    return ERROR;
  }

  ASSERT_ZERO(pthread_mutex_lock(&mutex));

  // This is the corresponding i_task.
  i_task_t* corresp_i_task = get_i_task(&task_archive, t);

  if (corresp_i_task == NULL) {
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return ERROR;
  }

  if (corresp_i_task->task_status == T_ADDED) {
    corresp_i_task->task_status = T_WANTED;
  }

  num_waiting++;

  while (corresp_i_task->task_status == T_WANTED) {
    ASSERT_ZERO(pthread_cond_wait(&corresp_i_task->is_ready, &mutex));
  }

  if(--num_waiting == 0) {
    ASSERT_ZERO(pthread_cond_signal(&noone_waiting));
  }

  if (corresp_i_task->task_status == T_DONE_ERROR) {
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return ERROR;
  }

  // In this case, `corresponding_i_task->task_status` is equal to `T_DONE_OK`.
  ASSERT_ZERO(pthread_mutex_unlock(&mutex));
  return PLANTOK;
}

// Called without (!) `mutex`, return the size of `tasks_to_add` post factum.
static inline size_t try_to_add_tasks(void)
{
  ASSERT_ZERO(pthread_mutex_lock(&mutex));

  time_t timestamp = time(NULL);
  node_t* curr_node = front(&tasks_to_add);
  node_t* next_node = NULL;
  size_t list_len = size(&tasks_to_add);

  for (size_t i = 0; i < list_len; i++) {
    next_node = curr_node->next;

    i_task_t* processed = curr_node->i_task;

    if (processed->task->start <= timestamp) {
      // I transfer this i_task to list `tasks_to_do`.
      remove_node(&tasks_to_add, curr_node);

      emplace_back_i(&tasks_to_do, processed);
      ASSERT_ZERO(pthread_cond_signal(&smth_to_do));
    }

    curr_node = next_node;
  }

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));
  return size(&tasks_to_add);
}

// Called without (!) `mutex,` clears tasks with status other than `T_WANTED`.
// Called only when plant is being destroyed.
static inline void clear_not_wanted_tasks(list_t* list)
{
  ASSERT_ZERO(pthread_mutex_lock(&mutex));

  node_t* curr_node = front(list);
  node_t* next_node = NULL;
  size_t list_len = size(list);

  for (size_t i = 0; i < list_len; i++) {
    next_node = curr_node->next;

    i_task_t* processed = curr_node->i_task;

    if (processed->task_status != T_WANTED) {
      remove_node(list, curr_node);

      // If noone wants it and plant is being destroyed
      // then I won't do this task.
      processed->task_status = T_DONE_ERROR;
      ASSERT_ZERO(pthread_cond_signal(&processed->is_ready));
    }

    curr_node = next_node;
  }

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));
}

static void* task_master_main(void* data)
{
  // `*data` is NULL.

  while (atomic_load(&is_plant_working)) {
    sleep(1);

    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    while (size(&tasks_to_add) == 0 && atomic_load(&is_plant_working)) {
      ASSERT_ZERO(pthread_cond_wait(&smth_to_add, &mutex));
    }
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    try_to_add_tasks();
  }

  // When factory is being shut-down.
  clear_not_wanted_tasks(&tasks_to_add);
  clear_not_wanted_tasks(&tasks_to_do);

  while (try_to_add_tasks() > 0) {
    sleep(1);
  }

  atomic_store(&end_of_tasks, true);

  ASSERT_ZERO(pthread_cond_signal(&smth_to_do));
  ASSERT_ZERO(pthread_cond_signal(&some_station));
  ASSERT_ZERO(pthread_cond_signal(&some_worker));

  return NULL;
}

// Called with `mutex`.
// Checks if there is a chance plant can complete this task.
static inline bool maybe_can_do_it(task_t* task)
{
  return task->capacity <= human_capital && task->capacity <= best_station;
}

// Called with `mutex`. Finds a station for given task.
// When succesfull, `*found = true`.
static inline size_t find_station(task_t* task, bool* found)
{
  if (free_stations == 0) {
    *found = false;
    return 0;
  }

  for (size_t i = 0; i < num_stations; i++) {
    if (people_on_stations[i] == 0 && task->capacity <= stations_capacity[i]) {
      *found = true;
      return i;
    }
  }

  *found = false;
  return 0;
}

// Called with `mutex`, puts used timestamp into `*timestamp`.
// Updates each workers status:
//   from `W_AVAILABLE` potentialy to  `W_NOT_AVAILABLE.
// (with respect to their shift)
// Returns number of workers with status `W_AVAILABLE`.
static inline size_t get_update_workers(time_t* timestamp)
{
  // I will refer to this time.
  *timestamp = time(NULL);
  size_t result = 0;

  for (size_t i = 0; i < num_workers; i++) {
    if (worker_status[i] == W_AVAILABLE && workers[i] != NULL) {

      if (*timestamp < workers[i]->end) {
        result++;
      } else {
        worker_status[i] = W_NOT_AVAILABLE;
        human_capital--;
        free_workers--;

        atomic_store(&worker_kits[i].end_of_work, true);
        ASSERT_ZERO(pthread_cond_signal(&worker_kits[i].work));
      }
    }
  }

  return result;
}

// Called with `mutex`.
// Given a `i_task` and a station, assigns workers.
// Updates `free_workers`, `free_stations` and `people_on_stations`.
static inline void assign_workers(i_task_t* i_task, size_t station)
{
  size_t index = 0;
  size_t capacity = i_task->task->capacity;

  people_on_stations[station] = capacity;

  for (size_t i = 0; i < num_workers && index < capacity; i++) {
    if (worker_status[i] == W_AVAILABLE) {
      free_workers--;
      worker_status[i] = W_WORKING;

      worker_kits[i].i_task = i_task;
      worker_kits[i].position = index;
      index++;

      worker_kits[i].station = station;

      ASSERT_ZERO(pthread_cond_signal(&worker_kits[i].work));
    }
  }

  free_stations--;
}

// Called without (!) `mutex`.
static inline void try_assign_tasks(void)
{
  ASSERT_ZERO(pthread_mutex_lock(&mutex));

  if (min_needed_workers == INT32_MAX) {
    min_needed_workers = 1;
  }

  while (size(&tasks_to_do) == 0 && atomic_load(&is_plant_working)) {
    ASSERT_ZERO(pthread_cond_wait(&smth_to_do, &mutex));
  }

  while (free_stations == 0 && atomic_load(&is_plant_working)) {
    ASSERT_ZERO(pthread_cond_wait(&some_station, &mutex));
  }

  while (free_workers < min_needed_workers && atomic_load(&is_plant_working)) {
    ASSERT_ZERO(pthread_cond_wait(&some_worker, &mutex));
  }
  // I have `mutex`.

  min_needed_workers = INT32_MAX;
  node_t* curr_node = front(&tasks_to_do);
  node_t* next_node = NULL;
  size_t list_size = size(&tasks_to_do);

  // I will use this timestamp for the entire process of assigning workers.
  time_t timestamp;
  size_t available_workers = get_update_workers(&timestamp);

  for (size_t i = 0; i < list_size; i++) {
    next_node = curr_node->next;
    task_t* curr_task = curr_node->i_task->task;
    bool found = false;

    if (maybe_can_do_it(curr_task)) {
      size_t station = find_station(curr_task, &found);

      if (found) {
        if (curr_task->capacity <= available_workers) {
          // If we have workers and a station.
          assign_workers(curr_node->i_task, station);
          available_workers -= curr_task->capacity;

          remove_node(&tasks_to_do, curr_node);
        } else {
          min_needed_workers = min(min_needed_workers, curr_task->capacity);
        }
      }
    } else {
      // Can't do this task
      curr_node->i_task->task_status = T_DONE_ERROR;
      ASSERT_ZERO(pthread_cond_signal(&curr_node->i_task->is_ready));

      remove_node(&tasks_to_do, curr_node);
    }

    // Go on
    curr_node = next_node;
  }

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));
}

static void* hr_master_main(void* data)
{
  // `*data` is NULL.
  while (!atomic_load(&end_of_tasks)) {
    sleep(1);

    try_assign_tasks();
  }
  // Plant is being destroyed.
  // Waiting for `task_master` to end his work.
  ASSERT_ZERO(pthread_join(task_master, NULL));

  // Adding tasks which were requested.
  while (0 < size(&tasks_to_do)) {
    ASSERT_ZERO(pthread_mutex_lock(&mutex));
    sleep(1);
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    try_assign_tasks();
  }

  // Waiting until everybody collects their task.
  ASSERT_ZERO(pthread_mutex_lock(&mutex));
  while (0 < num_waiting) {
    ASSERT_ZERO(pthread_cond_wait(&noone_waiting, &mutex));
  }

  // End all workers.
  for (size_t i = 0; i < num_workers; i++) {
    worker_status[i] = W_NOT_AVAILABLE;
    atomic_store(&worker_kits[i].end_of_work, true);
    ASSERT_ZERO(pthread_cond_signal(&worker_kits[i].work));
  }

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));

  // Join all workers.
  for (size_t i = 0; i < num_workers; i++) {
    ASSERT_ZERO(pthread_join(worker_threads[i], NULL));
  }
  
  return NULL;
}

static void* worker_main(void* data)
{
  int work_id = *((int*) data);
  free(data);

  ASSERT_ZERO(pthread_mutex_lock(&mutex));
  while (worker_status[work_id] == W_WILL_COME 
          && !atomic_load(&worker_kits[work_id].end_of_work)) {
    ASSERT_ZERO(pthread_cond_wait(&worker_kits[work_id].work, &mutex));
  }

  if (atomic_load(&worker_kits[work_id].end_of_work)) {
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    return NULL;
  }

  worker_t* me = workers[work_id];

  time_t now = time(NULL);
  if (now < me->start) {
    // If I have to wait in order to start.
    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    sleep(me->start - now);
    ASSERT_ZERO(pthread_mutex_lock(&mutex));
  }

  // Now I can work!
  worker_status[work_id] = W_AVAILABLE;
  free_workers++;

  ASSERT_ZERO(pthread_cond_signal(&some_worker));

  ASSERT_ZERO(pthread_mutex_unlock(&mutex));

  while (!atomic_load(&worker_kits[work_id].end_of_work)) {

    ASSERT_ZERO(pthread_mutex_lock(&mutex));
    while (worker_status[work_id] != W_WORKING
          && !atomic_load(&worker_kits[work_id].end_of_work)) {
      ASSERT_ZERO(pthread_cond_wait(&worker_kits[work_id].work, &mutex));
    }

    if (atomic_load(&worker_kits[work_id].end_of_work)) {
      // If I should end working.
      ASSERT_ZERO(pthread_mutex_unlock(&mutex));
      return NULL;
    }

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    // Begin to work on a given task.
    i_task_t* i_to_do = worker_kits[work_id].i_task;
    task_t* to_do = i_to_do->task;
    int pos = worker_kits[work_id].position;

    (to_do->results)[pos] = (me->work)(me, to_do, pos);

    ASSERT_ZERO(pthread_mutex_lock(&mutex));
    if (--people_on_stations[worker_kits[work_id].station] == 0) {
      // If I am the last at the station.
      i_to_do->task_status = T_DONE_OK;
      ASSERT_ZERO(pthread_cond_signal(&i_to_do->is_ready));

      free_stations++;
      ASSERT_ZERO(pthread_cond_signal(&some_station));
    }

    worker_kits[work_id].i_task = NULL;
    worker_kits[work_id].position = 0;
    worker_status[work_id] = W_AVAILABLE;
    free_workers++;
    ASSERT_ZERO(pthread_cond_signal(&some_worker));

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
  }

  return NULL;
}

static inline int max(int a, int b)
{
  return (a > b) ? a : b;
}

static inline int min(int a, int b)
{
  return (a > b) ? b : a;
}

static void cleanup_structures(void)
{
  ASSERT_ZERO(pthread_mutex_destroy(&mutex));
  ASSERT_ZERO(pthread_cond_destroy(&smth_to_add));
  ASSERT_ZERO(pthread_cond_destroy(&smth_to_do));
  ASSERT_ZERO(pthread_cond_destroy(&noone_waiting));
  ASSERT_ZERO(pthread_cond_destroy(&some_station));
  ASSERT_ZERO(pthread_cond_destroy(&some_worker));

  list_destroy(&task_archive);

  // Both of them should be empty.
  list_destroy(&tasks_to_add);
  list_destroy(&tasks_to_do);

  if (workers != NULL) {
    free(workers);
    workers = NULL;
  }

  if (worker_status != NULL) {
    free(worker_status);
    worker_status = NULL;
  }

  if (worker_kits != NULL) {
    for (size_t i = 0; i < num_workers; i++) {
      ASSERT_ZERO(pthread_cond_destroy(&worker_kits[i].work));
    }

    free(worker_kits);
    worker_kits = NULL;
  }

  if (stations_capacity != NULL) {
    free(stations_capacity);
    stations_capacity = NULL;
  }

  if (people_on_stations != NULL) {
    free(people_on_stations);
    people_on_stations = NULL;
  }

  if (worker_threads != NULL) {
    free(worker_threads);
    worker_threads = NULL;
  }
}
