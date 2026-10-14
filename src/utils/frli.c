#include "frli.h"
#include "backend/logger.h"
#include <stdlib.h>
#include <string.h>

FreeListStatus frli_create(void **entries, uint8_t **used, size_t *capacity,
                           size_t *count, size_t type_size, size_t num,
                           const char *label) {
  *entries = calloc(num, type_size);
  *used = calloc(num, sizeof(uint8_t));
  *capacity = num;
  *count = 0;

  if (!*entries || !*used) {
    logger_add(LoggerFlag_Error, "Couldn't create new free list: %s\n", label);
    free(*entries);
    free(*used);
    *entries = NULL;
    *used = NULL;
    *capacity = 0;
    return FreeListStatus_AllocFail;
  }
  return FreeListStatus_Success;
}

FreeListStatus frli_empty(void *entries, uint8_t *used, size_t capacity,
                          size_t *count, size_t type_size) {
  memset(entries, 0, capacity * type_size);
  memset(used, 0, capacity);
  *count = 0;
  return FreeListStatus_Success;
}

FreeListStatus frli_free(void **entries, uint8_t **used, size_t *capacity,
                         size_t *count) {
  free(*entries);
  free(*used);
  *entries = NULL;
  *used = NULL;
  *capacity = 0;
  *count = 0;
  return FreeListStatus_Success;
}

void *frli_new_entry(void **entries, uint8_t *used, size_t *capacity,
                     size_t *count, size_t type_size, size_t *index,
                     const char *label) {

  if (*entries == NULL || used == NULL || *capacity == 0) {
    logger_add(LoggerFlag_Error,
               "Free list '%s' not initialized, new entry aborted. "
               "(Entries: <%p>, used: <%p>, capacity: %lu, count: %lu)",
               label, *entries, (void *)used, *capacity, *count);
    return NULL;
  }

  if (*count >= *capacity) {
    logger_add(LoggerFlag_Error, "Free list '%s' reached max capacity", label);
    return NULL;
  }

  for (size_t i = 0; i < *capacity; i++) {
    if (used[i])
      continue;

    used[i] = 1; // claim immediately
    (*count)++;

    char *slot = (char *)(*entries) + (i * type_size);
    memset(slot, 0, type_size);

    if (index)
      *index = i;
    return slot;
  }
  return NULL;
}

FreeListStatus frli_remove(void *entries, uint8_t *used, size_t capacity,
                           size_t *count, size_t type_size, void *entry,
                           const char *label) {
  char *base = (char *)entries;
  char *slot = (char *)entry;

  if (slot < base || slot >= base + capacity * type_size ||
      (size_t)(slot - base) % type_size != 0) {
    logger_add(LoggerFlag_Warning, "Entry not found in '%s'.", label);
    return FreeListStatus_UnfoundEntry;
  }
  return frli_remove_at_index(entries, used, capacity, count, type_size,
                              (size_t)(slot - base) / type_size, label);
}

FreeListStatus frli_remove_at_index(void *entries, uint8_t *used,
                                    size_t capacity, size_t *count,
                                    size_t type_size, size_t index,
                                    const char *label) {
  if (index >= capacity)
    return FreeListStatus_OutOfBound;

  if (!used[index]) {
    logger_add(LoggerFlag_Warning, "Slot %lu in '%s' is already free.", index,
               label);
    return FreeListStatus_UnfoundEntry; // double-remove guard
  }

  memset((char *)entries + (index * type_size), 0, type_size);
  used[index] = 0;
  (*count)--;
  return FreeListStatus_Success;
}

// Dense helpers: src is treated as src_count contiguous valid entries.
FreeListStatus frli_append(const void *src_entries, size_t src_count,
                           void **dest_entries, uint8_t *dest_used,
                           size_t *dest_capacity, size_t *dest_count,
                           size_t type_size, const char *label) {
  if (*dest_count + src_count > *dest_capacity) {
    logger_add(LoggerFlag_Error,
               "Appending %lu entries to '%s' (count %lu) exceeds capacity %lu.",
               src_count, label, *dest_count, *dest_capacity);
    return FreeListStatus_OutOfBound;
  }

  memcpy((char *)(*dest_entries) + (*dest_count * type_size), src_entries,
         src_count * type_size);
  memset(dest_used + *dest_count, 1, src_count);
  *dest_count += src_count;
  return FreeListStatus_Success;
}

FreeListStatus frli_replace(const void *src_entries, size_t src_count,
                            void **dest_entries, uint8_t *dest_used,
                            size_t *dest_capacity, size_t *dest_count,
                            size_t type_size, const char *label) {
  if (src_count > *dest_capacity) {
    logger_add(LoggerFlag_Error,
               "Source (%lu) exceeds capacity (%lu) of '%s'.", src_count,
               *dest_capacity, label);
    return FreeListStatus_OutOfBound;
  }

  memcpy(*dest_entries, src_entries, src_count * type_size);
  memset(dest_used, 0, *dest_capacity);
  memset(dest_used, 1, src_count);
  *dest_count = src_count;
  return FreeListStatus_Success;
}

FreeListStatus frli_clone(const void *src_entries, size_t src_count,
                          void **dest_entries, uint8_t **dest_used,
                          size_t *dest_capacity, size_t *dest_count,
                          size_t type_size, const char *label) {
  void *new_entries = calloc(src_count, type_size);
  uint8_t *new_used = calloc(src_count, sizeof(uint8_t));
  if (!new_entries || !new_used) {
    logger_add(LoggerFlag_Error, "Couldn't clone to %s.", label);
    free(new_entries);
    free(new_used);
    return FreeListStatus_AllocFail;
  }

  memcpy(new_entries, src_entries, src_count * type_size);
  memset(new_used, 1, src_count);

  free(*dest_entries);
  free(*dest_used);
  *dest_entries = new_entries;
  *dest_used = new_used;
  *dest_capacity = src_count;
  *dest_count = src_count;
  return FreeListStatus_Success;
}
