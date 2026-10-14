#ifndef _FREE_LIST_H_
#define _FREE_LIST_H_

#include <stddef.h>
#include <stdint.h>

typedef enum {
  FreeListStatus_Success,
  FreeListStatus_AllocFail,
  FreeListStatus_NotInit,
  FreeListStatus_OutOfBound,
  FreeListStatus_UnfoundEntry,
  FreeListStatus_UndefError
} FreeListStatus;

typedef struct {
  void *entries;
  uint8_t *used;      // 1 byte per slot: 1 = taken, 0 = free
  size_t capacity;
  size_t count;
  size_t type_size;
  const char *label;
} FreeList;

//             entries, used,       capacity, count,   type_size, num, label
FreeListStatus frli_create(void **, uint8_t **, size_t *, size_t *, size_t,
                           size_t, const char *);

FreeListStatus frli_empty(void *, uint8_t *, size_t, size_t *, size_t);

FreeListStatus frli_free(void **, uint8_t **, size_t *, size_t *);

void *frli_new_entry(void **, uint8_t *, size_t *, size_t *, size_t, size_t *,
                     const char *);

FreeListStatus frli_remove(void *, uint8_t *, size_t, size_t *, size_t, void *,
                           const char *);

FreeListStatus frli_remove_at_index(void *, uint8_t *, size_t, size_t *,
                                    size_t, size_t, const char *);

//  src_entries, src_count, dest_entries, dest_used, dest_capacity, dest_count
FreeListStatus frli_append(const void *, size_t, void **, uint8_t *, size_t *,
                           size_t *, size_t, const char *);

FreeListStatus frli_replace(const void *, size_t, void **, uint8_t *, size_t *,
                            size_t *, size_t, const char *);

FreeListStatus frli_clone(const void *, size_t, void **, uint8_t **, size_t *,
                          size_t *, size_t, const char *);
#endif
