#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include "runtime/alloc.h"
#include "runtime/header.h"
#include "runtime/arena.h"

char **young_alloc_ptr(void);
char **old_alloc_ptr(void);
char* youngspace_ptr(void);
char* oldspace_ptr(void);
char youngspace_collection_id(void);
char oldspace_collection_id(void);
void map_foreach(void *, void(block**));
void set_foreach(void *, void(block**));
void list_foreach(void *, void(block**));

static bool is_gc = false;
static bool collect_old = false;
static uint8_t num_collection_only_young = 0;

bool during_gc() {
  return is_gc;
}

static size_t get_size(uint64_t hdr, uint16_t layout) {
  if (!layout) {
    size_t size = (len_hdr(hdr)  + sizeof(blockheader) + 7) & ~7;
    return hdr == NOT_YOUNG_OBJECT_BIT ? 8 : size < 16 ? 16 : size;
  } else {
    return size_hdr(hdr);
  }
}

static void migrate(block** blockPtr) {
  block* currBlock = *blockPtr;
  uintptr_t intptr = (uintptr_t)currBlock;
  if (intptr & 1) {
    return;
  }
  const uint64_t hdr = currBlock->h.hdr;
  bool isNotInYoungGen = hdr & NOT_YOUNG_OBJECT_BIT;
  bool hasAged = hdr & YOUNG_AGE_BIT;
  if (isNotInYoungGen && !(hasAged && collect_old)) {
    return;
  }
  bool shouldPromote = !isNotInYoungGen && hasAged;
  uint64_t mask = shouldPromote ? NOT_YOUNG_OBJECT_BIT : YOUNG_AGE_BIT;
  bool hasForwardingAddress = hdr & FWD_PTR_BIT;
  uint16_t layout = layout_hdr(hdr);
  size_t lenInBytes = get_size(hdr, layout);
  block** forwardingAddress = (block**)(currBlock + 1);
  if (!hasForwardingAddress) {
    block *newBlock;
    if (shouldPromote || (hasAged && collect_old)) {
      newBlock = koreAllocOld(lenInBytes);
    } else {
      newBlock = koreAlloc(lenInBytes);
    }
    memcpy(newBlock, currBlock, lenInBytes);
    newBlock->h.hdr |= mask;
    *forwardingAddress = newBlock;
    currBlock->h.hdr |= FWD_PTR_BIT;
    *blockPtr = newBlock;
  } else {
    *blockPtr = *forwardingAddress;
  }
}

// call this function instead of migrate on objects directly referenced by shared objects (like collection nodes)
// that are not tracked by gc
static void migrate_once(block** blockPtr) {
  block* currBlock = *blockPtr;
  if (youngspace_collection_id() == getArenaSemispaceIDOfObject((void *)currBlock) ||
      oldspace_collection_id() == getArenaSemispaceIDOfObject((void *)currBlock)) {
    migrate(blockPtr);
  }
}

static void migrate_string_buffer(stringbuffer** bufferPtr) {
  stringbuffer* buffer = *bufferPtr;
  const uint64_t hdr = buffer->contents->h.hdr;
  bool isNotInYoungGen = hdr & NOT_YOUNG_OBJECT_BIT;
  bool hasAged = hdr & YOUNG_AGE_BIT;
  if (isNotInYoungGen && !(hasAged && collect_old)) {
    return;
  }
  bool shouldPromote = !isNotInYoungGen && hasAged;
  uint64_t mask = shouldPromote ? NOT_YOUNG_OBJECT_BIT : YOUNG_AGE_BIT;
  bool hasForwardingAddress = hdr & FWD_PTR_BIT;
  if (!hasForwardingAddress) {
    stringbuffer *newBuffer;
    string *newContents;
    if (shouldPromote || (hasAged && collect_old)) {
      newBuffer = koreAllocOld(sizeof(stringbuffer));
      newBuffer->capacity = buffer->capacity; // contents is written below
      newContents = koreAllocTokenOld(sizeof(string) + buffer->capacity);
    } else {
      newBuffer = koreAlloc(sizeof(stringbuffer));
      newBuffer->capacity = buffer->capacity; // contents is written below
      newContents = koreAllocToken(sizeof(string) + buffer->capacity);
    }
    memcpy(newContents, buffer->contents, len(buffer->contents));
    newContents->h.hdr |= mask;
    newBuffer->contents = newContents;
    *(stringbuffer **)(buffer->contents) = newBuffer;
    buffer->contents->h.hdr |= FWD_PTR_BIT;
  }
  *bufferPtr = *(stringbuffer **)(buffer->contents);
}

static void migrate_mpz(mpz_ptr *mpzPtr) {
  integer *intgr = struct_base(integer, i, *mpzPtr);
  const uint64_t hdr = intgr->h.hdr;
  bool isNotInYoungGen = hdr & NOT_YOUNG_OBJECT_BIT;
  bool hasAged = hdr & YOUNG_AGE_BIT;
  if (isNotInYoungGen && !(hasAged && collect_old)) {
    return;
  }
  bool shouldPromote = !isNotInYoungGen && hasAged;
  uint64_t mask = shouldPromote ? NOT_YOUNG_OBJECT_BIT : YOUNG_AGE_BIT;
  bool hasForwardingAddress = hdr & FWD_PTR_BIT;
  if (!hasForwardingAddress) {
    integer *newIntgr;
    string *newLimbs;
    string *limbs = struct_base(string, data, intgr->i->_mp_d);
    size_t lenLimbs = len(limbs);

    assert(intgr->i->_mp_alloc * sizeof(mp_limb_t) == lenLimbs);

    if (shouldPromote || (hasAged && collect_old)) {
      newIntgr = struct_base(integer, i, koreAllocIntegerOld(0));
      newLimbs = (string *) koreAllocTokenOld(sizeof(string) + lenLimbs);
    } else {
      newIntgr = struct_base(integer, i, koreAllocInteger(0));
      newLimbs = (string *) koreAllocToken(sizeof(string) + lenLimbs);
    }
    memcpy(newLimbs, limbs, sizeof(string) + lenLimbs);
    memcpy(newIntgr, intgr, sizeof(integer));
    newIntgr->h.hdr |= mask;
    newIntgr->i->_mp_d = (mp_limb_t *)newLimbs->data;
    *(mpz_ptr *)(intgr->i->_mp_d) = newIntgr->i;
    intgr->h.hdr |= FWD_PTR_BIT;
  }
  *mpzPtr = *(mpz_ptr *)(intgr->i->_mp_d);
}

static void migrate_floating(floating **floatingPtr) {
  floating *flt = *floatingPtr;
  const uint64_t hdr = flt->h.hdr;
  bool isNotInYoungGen = hdr & NOT_YOUNG_OBJECT_BIT;
  bool hasAged = hdr & YOUNG_AGE_BIT;
  if (isNotInYoungGen && !(hasAged && collect_old)) {
    return;
  }
  bool shouldPromote = !isNotInYoungGen && hasAged;
  uint64_t mask = shouldPromote ? NOT_YOUNG_OBJECT_BIT : YOUNG_AGE_BIT;
  bool hasForwardingAddress = hdr & FWD_PTR_BIT;
  if (!hasForwardingAddress) {
    floating *newFlt;
    string *newLimbs;
    string *limbs = struct_base(string, data, flt->f->_mpfr_d-1);
    size_t lenLimbs = len(limbs);

    assert(((flt->f->_mpfr_prec + mp_bits_per_limb - 1) / mp_bits_per_limb) * sizeof(mp_limb_t) <= lenLimbs);

    if (shouldPromote || (hasAged && collect_old)) {
      newFlt = (floating *) koreAllocFloatingOld(0);
      newLimbs = (string *) koreAllocTokenOld(sizeof(string) + lenLimbs);
    } else {
      newFlt = (floating *) koreAllocFloating(0);
      newLimbs = (string *) koreAllocToken(sizeof(string) + lenLimbs);
    }
    memcpy(newLimbs, limbs, sizeof(string) + lenLimbs);
    memcpy(newFlt, flt, sizeof(floating));
    newFlt->h.hdr |= mask;
    newFlt->f->_mpfr_d = (mp_limb_t *)newLimbs->data+1;
    *(floating **)(flt->f->_mpfr_d) = newFlt;
    flt->h.hdr |= FWD_PTR_BIT;
  }
  *floatingPtr = *(floating **)(flt->f->_mpfr_d);
}

static char* evacuate(char* scan_ptr, char **alloc_ptr) {
  block *currBlock = (block *)scan_ptr;
  const uint64_t hdr = currBlock->h.hdr;
  uint16_t layoutInt = layout_hdr(hdr);
  if (layoutInt) {
    layout *layoutData = getLayoutData(layoutInt);
    for (unsigned i = 0; i < layoutData->nargs; i++) {
      layoutitem *argData = layoutData->args + i;
      void *arg = ((char *)currBlock) + argData->offset;
      switch(argData->cat) {
      case MAP_LAYOUT:
        map_foreach(arg, migrate_once);
        break;
      case LIST_LAYOUT:
        list_foreach(arg, migrate_once); 
        break;
      case SET_LAYOUT:
        set_foreach(arg, migrate_once);
        break;
      case STRINGBUFFER_LAYOUT:
        migrate_string_buffer(arg);
        break;
      case SYMBOL_LAYOUT: 
      case VARIABLE_LAYOUT:
        migrate(arg);
        break;
      case INT_LAYOUT:
        migrate_mpz(arg);
        break;
      case FLOAT_LAYOUT:
        migrate_floating(arg);
        break;
      case BOOL_LAYOUT:
      default: //mint
        break;
      }
    }
  }
  return movePtr(scan_ptr, get_size(hdr, layoutInt), *alloc_ptr);
}

// Contains the decision logic for collecting the old generation.
// For now, we collect the old generation every 50 young generation collections.
static bool shouldCollectOldGen() {
  if (++num_collection_only_young == 50) {
    num_collection_only_young = 0;
    return true;
  }

  return false;
}

void koreCollect(block** root) {
  is_gc = true;
  collect_old = shouldCollectOldGen();
  MEM_LOG("Starting garbage collection\n");
  koreAllocSwap(collect_old);
  migrate(root);
  char *scan_ptr = youngspace_ptr();
  MEM_LOG("Evacuating young generation\n");
  while(scan_ptr) {
    scan_ptr = evacuate(scan_ptr, young_alloc_ptr());
  }
  scan_ptr = oldspace_ptr();
  if (scan_ptr != *old_alloc_ptr()) {
    MEM_LOG("Evacuating old generation\n");
    while(scan_ptr) {
      scan_ptr = evacuate(scan_ptr, old_alloc_ptr());
    }
  }
  MEM_LOG("Finishing garbage collection\n");
  is_gc = false;
}
