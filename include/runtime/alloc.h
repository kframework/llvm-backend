#ifndef ALLOC_H
#define ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

// The maximum single allocation size in bytes.
// A contiguous area larger than that size cannot be allocated in any arena.
extern const size_t BLOCK_SIZE;

// allocates exactly requested bytes into the young generation
void* koreAlloc(size_t requested);
// allocates enough space for a string token whose raw size is requested into the young generation.
// rounds up to the nearest 8 bytes and always allocates at least 16 bytes
void* koreAllocToken(size_t requested);
// allocates exactly requested bytes into the old generation
void* koreAllocOld(size_t requested);
// allocates enough space for a string token whose raw size is requested into the old generation.
// rounds up to the nearest 8 bytes and always allocates at least 16 bytes
void* koreAllocTokenOld(size_t requested);
// allocates exactly requested bytes into the not garbage-collected arena
void* koreAllocNoGC(size_t requested);
// swaps the two semispace of the young generation as part of garbage collection
// if the swapOld flag is set, it also swaps the two semispaces of the old generation
void koreAllocSwap(bool swapOld);
// resizes the last allocation into the young generation
void* koreResizeLastAlloc(void* oldptr, size_t newrequest, size_t oldrequest);

void* koreReallocNoGC(void*, size_t, size_t);

void koreFree(void*, size_t);

#ifdef ALLOC_DBG
#define MEM_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define MEM_LOG(...)
#endif

#ifdef __cplusplus
}
#endif

#endif // ALLOC_H
