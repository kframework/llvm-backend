#include<cstdbool>
#include<cstdint>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cassert>
#include "runtime/alloc.h"
#include "runtime/header.h"
#include "runtime/arena.h"
#include "runtime/collect.h"

extern "C" {

char **young_alloc_ptr(void);
char **old_alloc_ptr(void);
char* youngspace_ptr(void);
char* oldspace_ptr(void);

static bool is_gc = false;
bool collect_old = false;
#ifndef GC_DBG
static uint8_t num_collection_only_young = 0;
#endif

size_t numBytesLiveAtCollection[1 << AGE_WIDTH];
static char *last_alloc_ptr;

bool during_gc() {
  return is_gc;
}

size_t get_size(uint64_t hdr, uint16_t layout) {
  if (!layout) {
    size_t size = (len_hdr(hdr)  + sizeof(blockheader) + 7) & ~7;
    return hdr == NOT_YOUNG_OBJECT_BIT ? 8 : size < 16 ? 16 : size;
  } else {
    return size_hdr(hdr);
  }
}

void migrate(block** blockPtr) {
  block* currBlock = *blockPtr;
  uintptr_t intptr = (uintptr_t)currBlock;
  if (intptr & 1) {
    return;
  }
  const uint64_t hdr = currBlock->h.hdr;
  initialize_migrate();
  uint16_t layout = layout_hdr(hdr);
  size_t lenInBytes = get_size(hdr, layout);
  block** forwardingAddress = (block**)(currBlock + 1);
  if (!hasForwardingAddress) {
    block *newBlock;
    if (shouldPromote || (isInOldGen && collect_old)) {
      newBlock = (block *)koreAllocOld(lenInBytes);
    } else {
      newBlock = (block *)koreAlloc(lenInBytes);
    }
#ifdef GC_DBG
    numBytesLiveAtCollection[oldAge] += lenInBytes;
#endif
    memcpy(newBlock, currBlock, lenInBytes);
    migrate_header(newBlock);
    *forwardingAddress = newBlock;
    currBlock->h.hdr |= FWD_PTR_BIT;
    *blockPtr = newBlock;
  } else {
    *blockPtr = *forwardingAddress;
  }
}

// call this function instead of migrate on objects directly referenced by shared objects (like collection nodes)
// that are not tracked by gc
void migrate_once(block** blockPtr) {
  block* currBlock = *blockPtr;
  uintptr_t intptr = (uintptr_t)currBlock;
  if (intptr & 1) {
    return;
  }
  if (youngspace_collection_id() == getArenaSemispaceIDOfObject((void *)currBlock) ||
      oldspace_collection_id() == getArenaSemispaceIDOfObject((void *)currBlock)) {
    migrate(blockPtr);
  }
}

static void migrate_string_buffer(stringbuffer** bufferPtr) {
  stringbuffer* buffer = *bufferPtr;
  const uint64_t hdr = buffer->h.hdr;
  const uint64_t cap = len(buffer->contents);
  initialize_migrate();
  if (!hasForwardingAddress) {
    stringbuffer *newBuffer;
    string *newContents;
    if (shouldPromote || (isInOldGen && collect_old)) {
      newBuffer = (stringbuffer *)koreAllocOld(sizeof(stringbuffer));
      newContents = (string *)koreAllocTokenOld(sizeof(string) + cap);
    } else {
      newBuffer = (stringbuffer *)koreAlloc(sizeof(stringbuffer));
      newContents = (string *)koreAllocToken(sizeof(string) + cap);
    }
#ifdef GC_DBG
    numBytesLiveAtCollection[oldAge] += cap + sizeof(stringbuffer) + sizeof(string);
#endif
    memcpy(newContents, buffer->contents, sizeof(string) + buffer->strlen);
    memcpy(newBuffer, buffer, sizeof(stringbuffer));
    migrate_header(newBuffer);
    newBuffer->contents = newContents;
    *(stringbuffer **)(buffer->contents) = newBuffer;
    buffer->h.hdr |= FWD_PTR_BIT;
  }
  *bufferPtr = *(stringbuffer **)(buffer->contents);
}

static void migrate_mpz(mpz_ptr *mpzPtr) {
  mpz_hdr *intgr = struct_base(mpz_hdr, i, *mpzPtr);
  const uint64_t hdr = intgr->h.hdr;
  initialize_migrate();
  if (!hasForwardingAddress) {
    mpz_hdr *newIntgr;
    string *newLimbs;
    bool hasLimbs = intgr->i->_mp_alloc > 0;
#ifdef GC_DBG
    numBytesLiveAtCollection[oldAge] += sizeof(mpz_hdr);
#endif
    if (hasLimbs) {
      string *limbs = struct_base(string, data, intgr->i->_mp_d);
      size_t lenLimbs = len(limbs);

#ifdef GC_DBG
      numBytesLiveAtCollection[oldAge] += lenLimbs + sizeof(string);
#endif

      assert(intgr->i->_mp_alloc * sizeof(mp_limb_t) == lenLimbs);

      if (shouldPromote || (isInOldGen && collect_old)) {
        newIntgr = struct_base(mpz_hdr, i, koreAllocIntegerOld(0));
        newLimbs = (string *) koreAllocTokenOld(sizeof(string) + lenLimbs);
      } else {
        newIntgr = struct_base(mpz_hdr, i, koreAllocInteger(0));
        newLimbs = (string *) koreAllocToken(sizeof(string) + lenLimbs);
      }
      memcpy(newLimbs, limbs, sizeof(string) + lenLimbs);
    } else {
      if (shouldPromote || (isInOldGen && collect_old)) {
        newIntgr = struct_base(mpz_hdr, i, koreAllocIntegerOld(0));
      } else {
        newIntgr = struct_base(mpz_hdr, i, koreAllocInteger(0));
      }
    }
    memcpy(newIntgr, intgr, sizeof(mpz_hdr));
    migrate_header(newIntgr);
    if (hasLimbs) {
      newIntgr->i->_mp_d = (mp_limb_t *)newLimbs->data;
    }
    *(mpz_ptr *)(&intgr->i->_mp_d) = newIntgr->i;
    intgr->h.hdr |= FWD_PTR_BIT;
  }
  *mpzPtr = *(mpz_ptr *)(&intgr->i->_mp_d);
}

static void migrate_floating(floating **floatingPtr) {
  floating_hdr *flt = struct_base(floating_hdr, f, *floatingPtr);
  const uint64_t hdr = flt->h.hdr;
  initialize_migrate();
  if (!hasForwardingAddress) {
    floating_hdr *newFlt;
    string *newLimbs;
    string *limbs = struct_base(string, data, flt->f.f->_mpfr_d-1);
    size_t lenLimbs = len(limbs);

#ifdef GC_DBG
    numBytesLiveAtCollection[oldAge] += sizeof(floating_hdr) + sizeof(string) + lenLimbs;
#endif

    assert(((flt->f.f->_mpfr_prec + mp_bits_per_limb - 1) / mp_bits_per_limb) * sizeof(mp_limb_t) <= lenLimbs);

    if (shouldPromote || (isInOldGen && collect_old)) {
      newFlt = struct_base(floating_hdr, f, koreAllocFloatingOld(0));
      newLimbs = (string *) koreAllocTokenOld(sizeof(string) + lenLimbs);
    } else {
      newFlt = struct_base(floating_hdr, f, koreAllocFloating(0));
      newLimbs = (string *) koreAllocToken(sizeof(string) + lenLimbs);
    }
    memcpy(newLimbs, limbs, sizeof(string) + lenLimbs);
    memcpy(newFlt, flt, sizeof(floating_hdr));
    migrate_header(newFlt);
    newFlt->f.f->_mpfr_d = (mp_limb_t *)newLimbs->data+1;
    *(floating **)(flt->f.f->_mpfr_d) = &newFlt->f;
    flt->h.hdr |= FWD_PTR_BIT;
  }
  *floatingPtr = *(floating **)(flt->f.f->_mpfr_d);
}

static void migrate_child(void* currBlock, layoutitem *args, unsigned i, bool ptr) {
  layoutitem *argData = args + i;
  void *arg = ((char *)currBlock) + argData->offset;
  switch(argData->cat) {
  case MAP_LAYOUT:
    migrate_map(ptr ? *(map**)arg : arg);
    break;
  case LIST_LAYOUT:
    migrate_list(ptr ? *(list**)arg : arg);
    break;
  case SET_LAYOUT:
    migrate_set(ptr ? *(set**)arg : arg);
    break;
  case STRINGBUFFER_LAYOUT:
    migrate_string_buffer((stringbuffer **)arg);
    break;
  case SYMBOL_LAYOUT:
  case VARIABLE_LAYOUT:
    migrate((block **)arg);
    break;
  case INT_LAYOUT:
    migrate_mpz((mpz_ptr *)arg);
    break;
  case FLOAT_LAYOUT:
    migrate_floating((floating **)arg);
    break;
  case BOOL_LAYOUT:
  default: //mint
    break;
  }
}

static char* evacuate(char* scan_ptr, char** alloc_ptr) {
  block *currBlock = (block *)scan_ptr;
  const uint64_t hdr = currBlock->h.hdr;
  uint16_t layoutInt = layout_hdr(hdr);
  if (layoutInt) {
    layout *layoutData = getLayoutData(layoutInt);
    for (unsigned i = 0; i < layoutData->nargs; i++) {
      migrate_child(currBlock, layoutData->args, i, false);
    }
  }
  return movePtr(scan_ptr, get_size(hdr, layoutInt), *alloc_ptr);
}

// Contains the decision logic for collecting the old generation.
// For now, we collect the old generation every 50 young generation collections.
static bool shouldCollectOldGen() {
#ifdef GC_DBG
  return true;
#else
  if (++num_collection_only_young == 50) {
    num_collection_only_young = 0;
    return true;
  }

  return false;
#endif
}

void migrateRoots();

void initStaticObjects(void) {
  map m = map();
  list l = list();
  set s = set();
  setKoreMemoryFunctionsForGMP();
}

void koreCollect(void** roots, uint8_t nroots, layoutitem *typeInfo) {
  is_gc = true;
  collect_old = shouldCollectOldGen();
  MEM_LOG("Starting garbage collection\n");
#ifdef GC_DBG
  if (!last_alloc_ptr) {
    last_alloc_ptr = youngspace_ptr();
  }
  char *current_alloc_ptr = *young_alloc_ptr();
#endif
  koreAllocSwap(collect_old);
#ifdef GC_DBG
  for (int i = 0; i < 2048; i++) {
    numBytesLiveAtCollection[i] = 0;
  }
#endif
  for (int i = 0; i < nroots; i++) {
    migrate_child(roots, typeInfo, i, true);
  }
  migrateRoots();
  char *scan_ptr = youngspace_ptr();
  if (scan_ptr != *young_alloc_ptr()) {
    MEM_LOG("Evacuating young generation\n");
    while(scan_ptr) {
      scan_ptr = evacuate(scan_ptr, young_alloc_ptr());
    }
  }
  scan_ptr = oldspace_ptr();
  if (scan_ptr != *old_alloc_ptr()) {
    MEM_LOG("Evacuating old generation\n");
    while(scan_ptr) {
      scan_ptr = evacuate(scan_ptr, old_alloc_ptr());
    }
  }
#ifdef GC_DBG
  ssize_t numBytesAllocedSinceLastCollection = ptrDiff(current_alloc_ptr, last_alloc_ptr);
  assert(numBytesAllocedSinceLastCollection >= 0);
  fwrite(&numBytesAllocedSinceLastCollection, sizeof(ssize_t), 1, stderr);
  last_alloc_ptr = *young_alloc_ptr();
  fwrite(numBytesLiveAtCollection, 
      sizeof(numBytesLiveAtCollection[0]),
      sizeof(numBytesLiveAtCollection) / sizeof(numBytesLiveAtCollection[0]),
      stderr);
#endif
  MEM_LOG("Finishing garbage collection\n");
  is_gc = false;
}

void freeAllKoreMem() {
  koreCollect(nullptr, 0, nullptr);
}

}
