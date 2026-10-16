#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include "backend/registry.h"

/*
    Small primes

    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
   73, 79, 83, 89, 97

    Medium primes

    101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
   179, 181, 191, 193, 197, 199 211, 223, 227, 229, 233, 239, 241, 251, 257,
   263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349

    Large primes

    353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439,
   443, 449, 457, 461, 463, 467, 479, 487, 491, 499 509, 521, 523, 541, 547,
   557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641,
   643, 647, 653, 659, 661

    Very large primes

    673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769,
   773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877,
   881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983,
   991, 997

 */

typedef enum {
  HashTableStatus_Success,
  HashTableStatus_AllocFail,
  HashTableStatus_UnfoundEntry,
  HashTableStatus_NotInit,
  HashTableStatus_MissingCallback,
  HashTableStatus_UndefError,
} HashTableStatus;

typedef enum {
  HashTableBucketState_Empty,
  HashTableBucketState_Tombstone,
  HashTableBucketState_Occupied,
} HashTableBucketState;

typedef uint32_t hash_t;

typedef bool (*hsht_bucket_compare)(const void *, const void *);
typedef hash_t (*hsht_hash_generator)(const void *);
typedef HashTableBucketState (*hsht_bucket_get_state)(const void *);
typedef void (*hsht_bucket_set_state)(const void *, const HashTableBucketState);
typedef void *(*hsht_hash_get_key)(const void *);

typedef struct {
  const void *key;
  void *entry;
} HashTableOccupiedSlot;

typedef struct {

  const char *label;
  void *entries;
  size_t capacity;
  size_t count;
  size_t bucket_size;

  // callback used to compare two entries during the linear probing
  hsht_bucket_compare comparator;
  // callback resposible for generating the hash from the given key
  hsht_hash_generator generator;
  // callback used to check if a bucket is either occupied, tombstone or empty
  hsht_bucket_get_state get_bucket_state;
  // callback used to mark a bucket as occupied or not
  hsht_bucket_set_state set_bucket_state;
  // callback responsible to retrieve the key from the bucket
  hsht_hash_get_key get_key;

} HashTable;

typedef struct {
  const char *label;
  const size_t capacity;
  const size_t bucket_size;

  hsht_bucket_compare comparator_callback;
  hsht_hash_generator generator_callback;
  hsht_bucket_get_state get_bucket_state_callback;
  hsht_bucket_set_state set_bucket_state_callback;
  hsht_hash_get_key get_key_callback;

} HashTableDescriptor;

static inline hash_t hsht_hash_ptr(const void *ptr) {
  uintptr_t key = (uintptr_t)ptr;

#if UINTPTR_MAX > 0xFFFFFFFF
  key = (uint32_t)(key ^ (key >> 32));
#endif

  uint32_t h = (uint32_t)key;

  h = (~h) + (h << 15);
  h = h ^ (h >> 12);
  h = h + (h << 2);
  h = h ^ (h >> 4);
  h = h * 2057;
  h = h ^ (h >> 16);

  return h;
}

static inline hash_t hsht_hash_id(uint32_t key) {
  key = ~key + (key << 15);
  key = key ^ (key >> 12);
  key = key + (key << 2);
  key = key ^ (key >> 4);
  key = key * 2057;
  key = key ^ (key >> 16);
  return key;
}

static inline hash_t hsht_hash_key(const char *key) {
  if (!key)
    return 0;
  uint32_t hash = 2166136261u;
  for (; *key; key++)
    hash = (hash ^ (uint8_t)(*key)) * 16777619u;
  return hash;
}

static inline hash_t hsht_hash_djb2(const char *key) {

  unsigned long hash = 5381;
  int c;

  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

// 32-bit FNV-1a hash
static inline uint32_t hsht_hash_fnv1a32(const void *data, size_t len) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t hash = 2166136261u; // FNV offset basis
  for (size_t i = 0; i < len; i++) {
    hash ^= bytes[i];
    hash *= 16777619u; // FNV prime
  }
  return hash;
}

typedef enum {
  HashTableNewFlag_None = 0,
  HashTableNewFlag_FixedCapacity = 1,
} HashTableNewFlag;

HashTableStatus hsht_create(HashTable *, const HashTableDescriptor *);

void *hsht_new_entry(HashTable *, const void *, const HashTableNewFlag);

HashTableStatus hsht_remove_entry(HashTable *, const void *);

HashTableStatus hsht_expand(HashTable *, const size_t);

void *hsht_find(HashTable *, const void *, size_t *);

HashTableStatus hsht_empty(void *, size_t *, size_t, const char *);

HashTableStatus hsht_free(HashTable *, const char *);

static inline size_t hsht_count(HashTable *tb) { return tb->count; }

#endif
