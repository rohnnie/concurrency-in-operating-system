# Parallel Hashtable Implementation with Pthreads

## Overview
This project modifies a non-thread-safe hash table implementation to support multi-threaded operations using **pthreads**. The goal is to ensure correctness while maintaining performance across multiple threads. Various synchronization mechanisms like **mutexes** and **spinlocks** are employed, and parallelization strategies are explored for both insertion and retrieval operations.

## Background
The original hash table implementation is not thread-safe and loses data when multiple threads perform operations simultaneously. This project addresses these issues, compares performance under different synchronization mechanisms, and optimizes the implementation.

  
