/************************************************************
* Student name and No.: WANG Zixu 3035140067
* Development platform: 64-bit Ubuntu 16.04 LTS and
*                       64-bit Ubuntu 14.04 LTS
* Last modified date: Nov 14, 2016
* Compilation: gcc DPP.c -o DPP -Wall -pthread
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

// Get philosopher's left fork ID
#define LEFT_FORK_ID(philID) (philID)

// Get philosopher's right fork ID
#define RIGHT_FORK_ID(philID) ((numPhil + philID - 1) % numPhil)

// Get philosopher's smaller fork ID
#define SMALLER_FORK_ID(philID) (LEFT_FORK_ID(philID)<RIGHT_FORK_ID(philID)\
                                  ? LEFT_FORK_ID(philID) : RIGHT_FORK_ID(philID))

// Get philosopher's larger fork ID
#define LARGER_FORK_ID(philID) (LEFT_FORK_ID(philID)<RIGHT_FORK_ID(philID)\
                                  ? RIGHT_FORK_ID(philID) : LEFT_FORK_ID(philID))

// Define -1 as a fork's free
#define FREE -1

// Enumerate of a philosopher's state
typedef enum {
  EATING,
  THINKING,
  WAITING,
  TERMINATED,
} philState_t;

// Philosopher's thread
void *philosopher(void *arg);

// Think event
void think(unsigned philID);

// Eat event
void eat(unsigned philID);

// A philosopher acquires forks
void acquireForks(unsigned philID);

// A philosopher releases forks
void releaseForks(unsigned philID);

// Watcher's thread
void *watcher(void *arg);

// Initializing the table
void init();

// After initialization, start the simulation
void start();

// When time's up, terminate the simulation
void terminate();

// pthread handler for watcher and philosophers
pthread_t watcherHandler, *phils;

// Array of philosopher states
// Value range: states in enum philState_t
philState_t *philStates;

// Array of fork states
// Value range: FREE (-1) for availiable,
// 0 ~ numPhil-1 for holder philosopher ID
int *forkStates;

// Mutex lock for continue flag, states array, and fork access
pthread_mutex_t contLock, stateLock, *forks;

// Semaphore as conditional variable for
// thread initialization ready, philosopher proceed permit,
// and watcher proceed permit
sem_t initReady, procPermit, watcherPermit;

// Number of philosophers, random seed, and continue flag
unsigned numPhil, seed, cont = 0;

// Main thread: read arguments and run simulation
int main(int argc, char const *argv[]) {
  // Check for execution arguments
  if (argc != 4) {
    // If wrong num of arguments, prompt usage and exit
    fprintf(stderr, "Usage: DPP [number of philosophers] [random seed] [time limit].\n");
    return EXIT_FAILURE;
  }
  // Read arguments
  numPhil = atoi(argv[1]);
  seed = atoi(argv[2]);
  int timeLim = atoi(argv[3]);
  // Initialize everything
  init();
  // When initialization done,
  // signal philosophers and watcher to start simulation
  start();
  // Run simulation for at least timeLim seconds
  sleep(timeLim);
  // When time's up,
  // signal philosophers and watcher to stop
  terminate();
  return EXIT_SUCCESS;
}

// Philosopher thread: cycle between thinking and eating
void *philosopher(void *arg) {
  // Get ID
  unsigned id = *((unsigned*)arg);
  free(arg);
  // Signal main thread that initialization done
  sem_post(&initReady);
  // Wait for permission to proceed
  sem_wait(&procPermit);
  // Main cycle for philosopher
  while (1) {
    think(id);
    eat(id);
  }
  // Code actually out of reach. Ensure thread exit
  pthread_exit(NULL);
}

// Watcher thread: periodically check and report states
void *watcher(void *arg) {
  // Signal main thread that initialization done
  sem_post(&initReady);
  // Wait for permission to proceed
  sem_wait(&watcherPermit);
  // Main cycle for watcher
  while (1) {
    // Lock the states to read
    pthread_mutex_lock(&stateLock);
    // Record number of states
    size_t numThinking = 0, numEating = 0, numWaiting = 0, numTerm = 0;
    size_t numUse = 0, numAvail = 0;
    // Print header
    printf("Philo   State             Fork    Held by\n");
    // Iterate through philosophers and forks for states
    size_t i;
    for (i = 0; i < numPhil; ++i) {
      // Print philosopher ID
      printf("[%2lu]:   ", i);
      // For different philosopher states,
      // print accordingly and record number
      switch (philStates[i]) {
        case THINKING:
          printf("%-18s", "Thinking");
          numThinking++;
          break;
        case EATING:
          printf("%-18s", "Eating");
          numEating++;
          break;
        case WAITING:
          printf("%-18s", "Waiting");
          numWaiting++;
          break;
        case TERMINATED:
          printf("%-18s", "Terminated");
          numTerm++;
      }
      // Print fork ID
      printf("[%2lu]:   ", i);
      // For different fork states,
      // print accordingly and record number
      if (forkStates[i] == FREE) {
        printf("Free\n");
        numAvail++;
      } else {
        printf("%4d\n", forkStates[i]);
        numUse++;
      }
    }
    // Print summary info
    printf("Th=%2lu Wa=%2lu Ea=%2lu         ", numThinking, numWaiting, numEating);
    printf("Use=%2lu  Avail=%2lu\n\n\n", numUse, numAvail);
    if (numTerm == numPhil) {
      // If all philosophers are terminated, exit
      pthread_mutex_unlock(&stateLock);
      pthread_exit(NULL);
    }
    // Release lock for states
    pthread_mutex_unlock(&stateLock);
    // Sleep for 500 miliseconds (=500000 microseconds)
    usleep(500000);
  }
}

// Philosopher thinking
void think(unsigned id) {
  // Lock state to update state
  pthread_mutex_lock(&stateLock);
  // Update state to THINKING
  philStates[id] = THINKING;
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
  // Think for random time in range
  // [1 microsecond, 10 seconds (=10000000 microseconds)]
  usleep(random() % 10000000 + 1);
  // Lock continue flag to check whether to continue
  // Get stateLock first to avoid deadlock between
  // stateLock and contLock using resourse hierarchy
  pthread_mutex_lock(&stateLock);
  pthread_mutex_lock(&contLock);
  if (!cont) {
    // If not to continue
    // Update state to TERMINATED
    philStates[id] = TERMINATED;
    // Release lock for continue flag
    pthread_mutex_unlock(&contLock);
    // Release lock for states
    pthread_mutex_unlock(&stateLock);
    // Exit
    pthread_exit(NULL);
  }
  // Release lock for continue flag
  pthread_mutex_unlock(&contLock);
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
}

// Philosopher eating
void eat(unsigned id) {
  // Lock state to update state
  pthread_mutex_lock(&stateLock);
  // Update state to WAITING
  philStates[id] = WAITING;
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
  // Try to acquire forks
  acquireForks(id);
  // Forks acquired
  // Eat for random time in range
  // [1 microsecond, 10 seconds (=10000000 microseconds)]
  usleep(random() % 10000000 + 1);
  // Done eating, release forks
  releaseForks(id);
}

// Acquire forks before eating
// Use resource hierarchy solution to avoid deadlock:
// Each fork is identified by a unique fork ID
// Each philosopher must acquire forks by the order of fork ID
void acquireForks(unsigned philID) {
  // Try to acquire the fork with smaller ID
  pthread_mutex_lock(&forks[SMALLER_FORK_ID(philID)]);
  // Fork with smaller ID acquired, lock states to
  // update fork state
  pthread_mutex_lock(&stateLock);
  forkStates[SMALLER_FORK_ID(philID)] = philID;
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
  // Try to acquire the fork with larger ID
  pthread_mutex_lock(&forks[LARGER_FORK_ID(philID)]);
  // Fork with smaller ID acquired, lock states to
  // update fork state as well as philosopher state
  pthread_mutex_lock(&stateLock);
  forkStates[LARGER_FORK_ID(philID)] = philID;
  philStates[philID] = EATING;
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
}

// Release forks after eating
// Order here doesn't matter
void releaseForks(unsigned philID) {
  // Lock states to update fork and philosopher states
  pthread_mutex_lock(&stateLock);
  // Release left fork and update state to free
  pthread_mutex_unlock(&forks[LEFT_FORK_ID(philID)]);
  forkStates[LEFT_FORK_ID(philID)] = FREE;
  // Release right fork and update state to free
  pthread_mutex_unlock(&forks[RIGHT_FORK_ID(philID)]);
  forkStates[RIGHT_FORK_ID(philID)] = FREE;
  // Lock continue flag to check whether to continue
  pthread_mutex_lock(&contLock);
  if (!cont) {
    // If not to continue
    // Update state to TERMINATED
    philStates[philID] = TERMINATED;
    // Release lock for continue flag
    pthread_mutex_unlock(&contLock);
    // Release lock for states
    pthread_mutex_unlock(&stateLock);
    // Exit
    pthread_exit(NULL);
  }
  // Release lock for continue flag
  pthread_mutex_unlock(&contLock);
  // If continue, update state to thinking
  philStates[philID] = THINKING;
  // Release lock for states
  pthread_mutex_unlock(&stateLock);
}

// Initializing everything.
// Initialize locks and semaphores, allocate memory,
// create and organize threads
// Post condition: locks and semaphores initialized and
// all threads ready and waiting for permit to proceed
void init() {
  // Initialize mutex locks
  pthread_mutex_init(&contLock, NULL);
  pthread_mutex_init(&stateLock, NULL);
  // Initialize semaphores
  sem_init(&initReady, 0, 0);
  sem_init(&procPermit, 0, 0);
  sem_init(&watcherPermit, 0, 0);
  // Allocate memory
  phils = (pthread_t*)malloc(numPhil * sizeof(pthread_t));
  forks = (pthread_mutex_t*)malloc(numPhil * sizeof(pthread_mutex_t));
  philStates = (philState_t*)malloc(numPhil * sizeof(philState_t));
  forkStates = (int*)malloc(numPhil * sizeof(int));
  // Apply random seed
  srandom(seed);
  // Create philosopher threads
  size_t i;
  for (i = 0; i < numPhil; ++i) {
    // Initialize forks
    pthread_mutex_init(&forks[i], NULL);
    forkStates[i] = FREE;
    // Record current philosopher ID
    unsigned *arg = (unsigned*)malloc(sizeof(unsigned));
    *arg = i;
    // Create philosopher thread
    pthread_create(&phils[i], NULL, philosopher, (void*)arg);
    // Wait for philosopher thread to initialize
    sem_wait(&initReady);
  }
  // Create watcher thread
  pthread_create(&watcherHandler, NULL, watcher, NULL);
  // Wait for watcher thread to initialize
  sem_wait(&initReady);
  // All threads are ready and waiting at here
}

// After initialization ready, start the simulation
void start() {
  // Set continue flag
  cont = 1;
  // Signal all philosophers to proceed
  size_t i;
  for (i = 0; i < numPhil; ++i) {
    sem_post(&procPermit);
  }
  // After all philosophers are set off, signal watcher to proceed
  sem_post(&watcherPermit);
}

// When time's up, terminate the simulation
void terminate() {
  // Lock continue flag to unset the flag
  pthread_mutex_lock(&contLock);
  cont = 0;
  // Release lock for continue flag
  pthread_mutex_unlock(&contLock);
  // Wait for all philosopher threads to terminate
  size_t i;
  for (i = 0; i < numPhil; ++i) {
    pthread_join(phils[i], NULL);
  }
  // Wait for watcher thread to terminate
  pthread_join(watcherHandler, NULL);
  // Cleaning up
  // Destroy locks
  pthread_mutex_destroy(&contLock);
  pthread_mutex_destroy(&stateLock);
  for (i = 0; i < numPhil; ++i) {
    pthread_mutex_destroy(&forks[i]);
  }
  // Destroy semaphores
  sem_destroy(&initReady);
  sem_destroy(&procPermit);
  sem_destroy(&watcherPermit);
  // Release memory
  free(phils);
  free(forks);
  free(philStates);
  free(forkStates);
  // Done
}
