#include "dining.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct dining {
  int capacity;
  bool cleaning;
  int seated_students;
  // dining hall mutex for atomic book keeping
  pthread_mutex_t m;
  // signal to the student and cleaning services when its safe to enter
  pthread_cond_t cv;
} dining_t;

dining_t* dining_init(int capacity) {
  dining_t* dining = malloc(sizeof(dining_t));
  dining->capacity = capacity;
  dining->cleaning = false;
  dining->seated_students = 0;
  pthread_mutex_init(&dining->m, NULL);
  pthread_cond_init(&dining->cv, NULL);
  return dining;
}

void dining_destroy(dining_t** dining) {
  pthread_mutex_destroy(&(*dining)->m);
  pthread_cond_destroy(&(*dining)->cv);
  free(*dining);
  *dining = NULL;
}

/**
 * If there is a room in the dining hall, this function returns. If students
 * cannot enter the dining hall, this function blocks until it becomes possible
 * to enter.
 */
void dining_student_enter(dining_t* d) {
  pthread_mutex_lock(&d->m);
  while ((d->cleaning) || (d->seated_students == d->capacity)) {
    pthread_cond_wait(&d->cv, &d->m);
  }
  d->seated_students++;
  pthread_mutex_unlock(&d->m);
}

/**
 * When a student leaves the dining hall
 */
void dining_student_leave(dining_t* d) {
  pthread_mutex_lock(&d->m);
  d->seated_students--;
  // If seats freed or cleaning may be waiting, wake someone
  pthread_cond_broadcast(&d->cv);
  pthread_mutex_unlock(&d->m);
}

/**
 * Students cannot be in the dining hall while the cleaning is taking place. The
 * function blocks until all students leave. Once the cleaning has begun,
 * students cannot enter the dining hall. Only one cleaning service provider can
 * work in the dining hall at a time.
 */
void dining_cleaning_enter(dining_t* d) {
  pthread_mutex_lock(&d->m);
  while (d->seated_students > 0) {
    pthread_cond_wait(&d->cv, &d->m);
  }
  d->cleaning = true;
  pthread_mutex_unlock(&d->m);
}

void dining_cleaning_leave(dining_t* d) {
  pthread_mutex_lock(&d->m);
  d->cleaning = false;
  pthread_cond_broadcast(&d->cv);
  pthread_mutex_unlock(&d->m);
}
