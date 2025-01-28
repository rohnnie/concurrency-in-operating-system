#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

//stdio is for i/o
// stdlib is for general utilities 
//string is for string operations
// pthread is for thread operations
// assert is like the library for loop invariants, but in C
// sys/time is for datetime operations


#define NUM_BUCKETS 5     // Buckets in hash table
#define NUM_KEYS 100000   // Number of keys inserted per thread
int num_threads = 1;      // Number of threads (configurable)
int keys[NUM_KEYS];
//defining keys array with size 100000 (100,000)


pthread_spinlock_t spinlock;

typedef struct _bucket_entry {
  int key;
  int val;
  struct _bucket_entry *next;
  //similar to LLs, nested DS that points to the next "bucket entry"
} bucket_entry;

bucket_entry *table[NUM_BUCKETS];

//table has 5 slots or entries

void panic(char *msg) {
  printf("%s\n", msg);
  exit(1);
}

//printing function

double now() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
  	//whole number of seconds in interval is tv_sec
  	//additional fraction as microseconds is tv_usec
    //basically, it's seconds + microseconds 
    //we're only dividing usec by 1000000, because we're converting it to seconds (10^-6)
}


// Inserts a key-value pair into the table
void insert(int key, int val) {
  int i = key % NUM_BUCKETS;
  //pthread_mutex_lock(&mutex[i]);
  pthread_spin_lock(&spinlock);

  //i can have any value ranging from 0 to 4, because NUM_BUCKETS is 5
  bucket_entry *e = (bucket_entry *) malloc(sizeof(bucket_entry));
  // creating a bucket_entry
  if (!e) panic("No memory to allocate bucket!");

  //if e is false, use panic to return an error message
  e->next = table[i];
  //e->next is pointing to table[i], let's say i==3. then it just means that table[3] now contains 
  //e->next. it means that it's just hashed to that slot in the table
  e->key = key;
  //storing the key in e 
  e->val = val;
  //storing the value in e 
  table[i] = e;
  //storing e as a whole into table[3]

  pthread_spin_unlock(&spinlock);
}

// Retrieves an entry from the hash table by key
// Returns NULL if the key isn't found in the table
bucket_entry * retrieve(int key) {
    int i = key % NUM_BUCKETS;
    pthread_spin_lock(&spinlock);
  bucket_entry *b;
  for (b = table[key % NUM_BUCKETS]; b != NULL; b = b->next) {
    //for a particular table[i], we're traversing the chain
    if (b->key == key) 
    {
    pthread_spin_unlock(&spinlock);
    return b;
    }
    //return the bucket entry that matches the key
  pthread_spin_unlock(&spinlock);
      }
  return NULL;
}

void * put_phase(void *arg) {
  long tid = (long) arg;
  //thread id
  int key = 0;

  // If there are k threads, thread i inserts
  //      (i, i), (i+k, i), (i+k*2)
  for (key = tid ; key < NUM_KEYS; key += num_threads) {
    insert(keys[key], tid);
    //inserting keys with value as thread id
  }

  pthread_exit(NULL);
}

void * get_phase(void *arg) {
  long tid = (long) arg;
  //tid = thread id
  int key = 0;
  long lost = 0;

  for (key = tid ; key < NUM_KEYS; key += num_threads) {
    if (retrieve(keys[key]) == NULL) lost++;
  }
  printf("[thread %ld] %ld keys lost!\n", tid, lost);

  pthread_exit((void *)lost);
}

int main(int argc, char **argv) {
  long i;
  pthread_t *threads;
  double start, end;

  if (argc != 2) {
    panic("usage: ./parallel_hashtable <num_threads>");
  }
  if ((num_threads = atoi(argv[1])) <= 0) {
    panic("must enter a valid number of threads to run");
  }

  srandom(time(NULL));
  for (i = 0; i < NUM_KEYS; i++)
    keys[i] = random();

  threads = (pthread_t *) malloc(sizeof(pthread_t)*num_threads);
  //this is what stores threads.
  if (!threads) {
    panic("out of memory allocating thread handles");
  }

  // Insert keys in parallel
  start = now();
    pthread_spin_init(&spinlock,PTHREAD_PROCESS_SHARED);
  for (i = 0; i < num_threads; i++) {
    //num_threads = 1 in this case
    pthread_create(&threads[i], NULL, put_phase, (void *)i);
  }

/*pthread_create starts a new thread in the calling process, it starts execution by 
calling put_phase, and i is the argument passed to put_phase*/
// Barrier
  for (i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
    /*The pthread_join() function waits for the thread specified by thread to
    terminate.  If that thread has already terminated, then	pthread_join()
    returns immediately.*/
  }
  end = now();

  printf("[main] Inserted %d keys in %f seconds\n", NUM_KEYS, end - start);

  // Reset the thread array
  memset(threads, 0, sizeof(pthread_t)*num_threads);

  // Retrieve keys in parallel
  start = now();
  for (i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, get_phase, (void *)i);
  }

  // Collect count of lost keys
  long total_lost = 0;
  long *lost_keys = (long *) malloc(sizeof(long) * num_threads);
  for (i = 0; i < num_threads; i++) {
    pthread_join(threads[i], (void **)&lost_keys[i]);
    /*If  retval  is  not NULL, then pthread_join() copies the exit status of
       the target thread (i.e., the value that the target thread  supplied  to
       pthread_exit(3)) into the location pointed to by retval*/
    total_lost += lost_keys[i];
  }
  end = now();

  printf("[main] Retrieved %ld/%d keys in %f seconds\n", NUM_KEYS - total_lost, NUM_KEYS, end - start);

    pthread_spin_destroy(&spinlock);

  return 0;
}
