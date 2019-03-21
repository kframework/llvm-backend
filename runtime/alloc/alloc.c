#include <gmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "runtime/alloc.h"
#include "runtime/header.h"
#include "runtime/arena.h"


REGISTER_ARENA(youngspace, 0);
REGISTER_ARENA(oldspace, 1);
REGISTER_ARENA(nogcspace, 2);

char *youngspace_ptr() {
  return arenaStartPtr(&youngspace);
}

char *oldspace_ptr() {
  return arenaStartPtr(&oldspace);
}

char **young_alloc_ptr() {
  return arenaEndPtr(&youngspace);
}

char **old_alloc_ptr() {
  return arenaEndPtr(&oldspace);
}

char youngspace_collection_id() {
  return getArenaCollectionSemispaceID(&youngspace);
}

char oldspace_collection_id() {
  return getArenaCollectionSemispaceID(&oldspace);
}

void koreAllocSwap(bool swapOld) {
  arenaSwapAndReset(&youngspace);
  if (swapOld) {
    arenaSwapAndReset(&oldspace);
  }
}

void freeAllKoreMem() {
  freeAllMemory();
  arenaReset(&youngspace);
  arenaReset(&oldspace);
  arenaReset(&nogcspace);
}

void setKoreMemoryFunctionsForGMP() {
   mp_set_memory_functions(koreAllocMP, koreReallocMP, koreFree);
}

__attribute__ ((always_inline)) void* koreAlloc(size_t requested) {
  return arenaAlloc(&youngspace, requested);
}

__attribute__ ((always_inline)) void* koreAllocToken(size_t requested) {
  size_t size = (requested + 7) & ~7;
  return arenaAlloc(&youngspace, size < 16 ? 16 : size);
}

__attribute__ ((always_inline)) void* koreAllocOld(size_t requested) {
  return arenaAlloc(&oldspace, requested);
}

__attribute__ ((always_inline)) void* koreAllocTokenOld(size_t requested) {
  size_t size = (requested + 7) & ~7;
  return arenaAlloc(&oldspace, size < 16 ? 16 : size);
}

__attribute__ ((always_inline)) void* koreAllocNoGC(size_t requested) {
  return arenaAlloc(&nogcspace, requested);
}

void* koreResizeLastAlloc(void* oldptr, size_t newrequest, size_t last_size) {
  newrequest = (newrequest + 7) & ~7;
  last_size = (last_size + 7) & ~7;
  if (oldptr != *arenaEndPtr(&youngspace) - last_size) {
    MEM_LOG("May only reallocate last allocation. Tried to reallocate %p to %zd\n", oldptr, newrequest);
    exit(255);
  }
  ssize_t increase = newrequest - last_size;
  if (arenaResizeLastAlloc(&youngspace, increase)) {
    return oldptr;
  } else {
    void* newptr = koreAlloc(newrequest);
    memcpy(newptr,oldptr,last_size);
    return newptr;
  }
}

void* koreAllocMP(size_t requested) {
  string* new = (string *) koreAllocToken(sizeof(string) + requested);
  set_len(new, requested);
  return new->data;
}

void* koreReallocMP(void* ptr, size_t old_size, size_t new_size) {
  string* new = (string *) koreAllocToken(sizeof(string) + new_size);
  size_t min = old_size > new_size ? new_size : old_size;
  memcpy(new->data, ptr, min);
  set_len(new, new_size);
  return new->data;
}

void koreFree(void* ptr, size_t size) {}

__attribute__ ((always_inline)) void* koreAllocInteger(size_t requested) {
  integer *result = (integer *) koreAlloc(sizeof(integer));
  set_len(result, sizeof(integer) - sizeof(blockheader));
  return &result->i;
}

__attribute__ ((always_inline)) void* koreAllocFloating(size_t requested) {
  floating_hdr *result = (floating_hdr *) koreAlloc(sizeof(floating_hdr));
  set_len(result, sizeof(floating_hdr) - sizeof(blockheader));
  return &result->f;
}

__attribute__ ((always_inline)) void* koreAllocIntegerOld(size_t requested) {
  integer *result = (integer *) koreAllocOld(sizeof(integer));
  set_len(result, sizeof(integer) - sizeof(blockheader));
  return &result->i;
}

__attribute__ ((always_inline)) void* koreAllocFloatingOld(size_t requested) {
  floating_hdr *result = (floating_hdr *) koreAllocOld(sizeof(floating_hdr));
  set_len(result, sizeof(floating_hdr) - sizeof(blockheader));
  return &result->f;
}
