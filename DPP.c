#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum {
  INIT,
  EATING,
  THINKING,
  WAITING,
  TERMINATED,
} philState_t;
void *philosopher(void *arg);
void acquireFork(unsigned philID, unsigned forkID);
void releaseForks(unsigned forkID1, unsigned forkID2);
pthread_t *phils;
pthread_mutex_t contLock, *forks;
sem_t initReady, procPermit;
unsigned numPhil, seed, cont = 0;

int main(int argc, char const *argv[]) {
  if (argc != 4) {
    perror("Usage: DPP [number of philosophers] [random seed] [time limit].\n");
    return EXIT_FAILURE;
  }
  numPhil = atoi(argv[1]);
  seed = atoi(argv[2]);
  int timeLim = atoi(argv[3]);
  pthread_mutex_init(&contLock, NULL);
  sem_init(&initReady, 0, 0);
  sem_init(&procPermit, 0, 0);
  printf("%d %d %d\n", numPhil, seed, timeLim);
  phils = (pthread_t*)malloc(numPhil * sizeof(pthread_t));
  forks = (pthread_mutex_t*)malloc(numPhil * sizeof(pthread_mutex_t));
  size_t i;
  for (i = 0; i < numPhil; ++i) {
    pthread_mutex_init(&forks[i], NULL);
    unsigned *arg = (unsigned*)malloc(sizeof(unsigned));
    *arg = i;
    pthread_create(&phils[i], NULL, philosopher, (void*)arg);
    sem_wait(&initReady);
  }
  cont = 1;
  for (i = 0; i < numPhil; ++i) {
    sem_post(&procPermit);
  }
  sleep(timeLim);
  pthread_mutex_lock(&contLock);
  cont = 0;
  pthread_mutex_unlock(&contLock);
  for (i = 0; i < numPhil; ++i) {
    pthread_join(phils[i], NULL);
  }
  return EXIT_SUCCESS;
}

void *philosopher(void *arg) {
  philState_t state = INIT;
  unsigned id = *((unsigned*)arg);
  free(arg);
  printf("Creating thread %d\n", id);
  unsigned leftForkID = id;
  unsigned rightForkID = (numPhil + id - 1) % numPhil;
  sem_post(&initReady);
  sem_wait(&procPermit);
  printf("Philosopher %d: received condv, proceed.\n", id);
  while (1) {
    srandom(seed);
    state = THINKING;
    usleep(random() % 10000000 + 1);
    pthread_mutex_lock(&contLock);
    if (!cont) {
      state = TERMINATED;
      pthread_mutex_unlock(&contLock);
      pthread_exit(NULL);
    }
    pthread_mutex_unlock(&contLock);
    state = WAITING;
    acquireFork(id, leftForkID);
    acquireFork(id, rightForkID);
    state = EATING;
    usleep(random() % 10000000 + 1);
    releaseForks(leftForkID, rightForkID);
    pthread_mutex_lock(&contLock);
    if (!cont) {
      state = TERMINATED;
      pthread_mutex_unlock(&contLock);
      pthread_exit(NULL);
    }
    pthread_mutex_unlock(&contLock);
  }
  state = TERMINATED;
  pthread_exit(NULL);
}

void acquireFork(unsigned philID, unsigned forkID) {
  printf ("Philosopher %d: contend for fork %d\n", philID, forkID);
  pthread_mutex_lock(&forks[forkID]);
  printf ("Philosopher %d: got fork %d\n", philID, forkID);
}

void releaseForks(unsigned forkID1, unsigned forkID2) {
  pthread_mutex_unlock(&forks[forkID1]);
  pthread_mutex_unlock(&forks[forkID2]);
  printf ("Fork %d, %d released.\n", forkID1, forkID2);
}
