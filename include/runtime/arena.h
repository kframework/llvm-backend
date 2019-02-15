#ifndef ARENA_H
#define ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

// An arena can be used to allocate objects that can then be deallocated all at
// once.
struct arena {
  char* first_block;
  char* block;
  char* block_start;
  char* block_end;
  char *first_collection_block;
  char allocation_semispace_id;
};

// Macro to define a new arena with the given ID. Supports IDs ranging from 0 to
// 127.
#define REGISTER_ARENA(name, id) \
  static struct arena name = { 0, 0, 0, 0, 0, id }

// Resets the given arena.
void arenaReset(struct arena *);

// Returns the given arena's current allocation semispace ID.
// Each arena has 2 semispace IDs one equal to the arena ID and the other equal
// to the 1's complement of the arena ID. At any time one of these semispaces
// is used for allocation and the other is used for collection.
char getArenaAllocationSemispaceID(const struct arena *);

// Returns the given arena's current collection semispace ID.
// See above for details.
char getArenaCollectionSemispaceID(const struct arena *);

// Returns the ID of the semispace where the given address was allocated.
// The behavior is undefined if called with an address that has not been allocated
// within an arena.
char getArenaSemispaceIDOfObject(void *);

// Allocates the requested number of bytes as a contiguous region and returns a
// pointer to the first allocated byte.
// If called with requested size greater than the maximun single allocation size,
// the space is allocated in a general (not garbage collected pool).
void *arenaAlloc(struct arena *, size_t);

// Resizes the last allocation as long as the resize does not require a new
// block allocation.
// Returns the address of the byte following the last newlly allocated byte when
// the resize succeeds, returns 0 otherwise.
void *arenaResizeLastAlloc(struct arena *, ssize_t);

// Exchanges the current allocation and collection semispaces and clears the new
// current allocation semispace. It is used before garbage collection.
void arenaSwapAndReset(struct arena *);

// Returns the address of the first byte that belongs in the given arena.
// Returns 0 if nothing has been allocated ever in that arena.
char *arenaStartPtr(const struct arena *);

// Returns a pointer to a location holding the address of last allocated
// byte in the given arena plus 1.
// This address is 0 if nothing has been allocated ever in that arena.
char **arenaEndPtr(struct arena *);

// Returns a pointer to the byte following the given number of bytes starting
// from the given pointer.
// Returns 0 if the result is out of bounds (exceeding the last allocated byte).
char *movePtr(char *, size_t, const char *);

// Deallocates all the memory allocated for registered arenas.
void freeAllMemory(void);

#ifdef __cplusplus
}
#endif

#endif // ARENA_H
