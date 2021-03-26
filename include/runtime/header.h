#ifndef RUNTIME_HEADER_H
#define RUNTIME_HEADER_H

#include <stdint.h>
#include <stddef.h>
#include <gmp.h>
#include <mpfr.h>

#include "config/macros.h"
#include "runtime/alloc.h"

#ifndef IMMER_TAGGED_NODE
#define IMMER_TAGGED_NODE 0
#endif
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>

// the actual length is equal to the block header with the gc bits masked out.

#define len(s) len_hdr((s)->h.hdr)
#define len_hdr(s) ((s) & LENGTH_MASK)
#define set_len(s, l) ((s)->h.hdr = (l) | ((l) > BLOCK_SIZE - sizeof(char *) ? NOT_YOUNG_OBJECT_BIT : 0))
#define size_hdr(s) ((((s) >> 32) & 0xff) * 8)
#define layout(s) layout_hdr((s)->h.hdr)
#define layout_hdr(s) ((s) >> LAYOUT_OFFSET)
#define tag_hdr(s) (s & TAG_MASK)
#define is_in_young_gen_hdr(s) (!((s) & NOT_YOUNG_OBJECT_BIT))
#define is_in_old_gen_hdr(s) \
        (((s) & NOT_YOUNG_OBJECT_BIT) && ((s) & AGE_MASK))
#define reset_gc(s) ((s)->h.hdr = (s)->h.hdr & ~(NOT_YOUNG_OBJECT_BIT | AGE_MASK | FWD_PTR_BIT))
#define struct_base(struct_type, member_name, member_addr) \
        ((struct_type *)((char *)(member_addr) - offsetof(struct_type, member_name)))
#define leaf_block(tag) ((block *)((((uint64_t)(tag)) << 32) | 1))
#define variable_block(tag) ((block *)((((uint64_t)(tag)) << 32) | 3))
#define is_leaf_block(block) (((uintptr_t)block) & 1)
#define is_variable_block(block) ((((uintptr_t)block) & 3) == 3)

extern "C" {
  // llvm: blockheader = type { i64 } 
  typedef struct blockheader {
    uint64_t hdr;
  } blockheader;

  // A value b of type block* is either a constant or a block.
  // if (((uintptr_t)b) & 3) == 3, then it is a bound variable and
  // ((uintptr_t)b) >> 32 is the debruijn index. If ((uintptr_t)b) & 3 == 1)
  // then it is a symbol with 0 arguments and ((uintptr_t)b) >> 32 is the tag
  // of the symbol. Otherwise, if ((uintptr_t)b) & 1 == 0 then it is a pointer to
  // a block.
  // llvm: block = type { %blockheader, [0 x i64 *] }
  typedef struct block {
    blockheader h;
    uint64_t *children[];
  } block;

  
  // llvm: string = type { %blockheader, [0 x i8] }
  typedef struct string {
    blockheader h;
    char data[];
  } string;
  
  // llvm: stringbuffer = type { i64, i64, %string* }
  typedef struct stringbuffer {
    blockheader h;
    uint64_t strlen;
    string *contents;
  } stringbuffer;

  typedef struct mpz_hdr {
    blockheader h;
    mpz_t i;
  } mpz_hdr;

  typedef struct floating {
    uint64_t exp; // number of bits in exponent range
    mpfr_t f;
  } floating;

  typedef struct floating_hdr {
    blockheader h;
    floating f;
  } floating_hdr;

  typedef struct layoutitem {
    uint64_t offset;
    uint16_t cat;
  } layoutitem;

  typedef struct layout {
    uint8_t nargs;
    layoutitem *args;
  } layout;

  typedef struct {
    FILE *file;
    stringbuffer *buffer;
  } writer;

  bool hook_KEQUAL_eq(block *, block *);
  bool during_gc(void);
  size_t hash_k(block *);
  void k_hash(block *, void *);
  bool hash_enter(void);
  void hash_exit(void);
}

class KElem {
public:
  KElem(block * elem) {
    this->elem = elem;
  }

  bool operator==(const KElem& other) const {
    return hook_KEQUAL_eq(this->elem, other.elem);
  }

  bool operator!=(const KElem& other) const {
    return !(*this == other);
  }

  operator block*() const {
    return elem;
  }

  block * elem;
};

struct kore_alloc_heap {

  template <typename... Tags>
  static void *allocate(size_t size, Tags...) {
    if (during_gc()) {
      return ::operator new(size);
    } else {
      string *result = (string *)koreAllocToken(size + sizeof(blockheader));
      set_len(result, size);
      return result->data;
    }
  }

  static void deallocate(size_t size, void *data) {
    if (during_gc()) {
      ::operator delete(data);
    }
  }
};

struct HashBlock {
  size_t operator()(const KElem &block) const noexcept {
    return hash_k(block);
  }
};

using list = immer::flex_vector<KElem, immer::memory_policy<immer::heap_policy<kore_alloc_heap>, immer::no_refcount_policy>>;
using map = immer::map<KElem, KElem, HashBlock, std::equal_to<KElem>, list::memory_policy>;
using set = immer::set<KElem, HashBlock, std::equal_to<KElem>, list::memory_policy>;

typedef struct mapiter {
  map::iterator curr;
  map *m;
} mapiter;

typedef struct setiter {
  set::iterator curr;
  set *s;
} setiter;

typedef floating *SortFloat;
typedef mpz_ptr SortInt;
typedef string *SortString;
typedef string *SortBytes;
typedef stringbuffer *SortStringBuffer;
typedef block *SortK;
typedef block *SortKItem;
typedef block *SortIOInt;
typedef block *SortIOFile;
typedef block *SortIOString;
typedef block *SortJSON;
typedef block *SortEndianness;
typedef block *SortSignedness;
typedef block *SortFFIType;
typedef list *SortList;
typedef map *SortMap;
typedef set *SortSet;


extern "C" {

  block *parseConfiguration(const char *filename);
  void printConfiguration(const char *filename, block *subject);
  void printStatistics(const char *filename, uint64_t steps);
  string *printConfigurationToString(block *subject);
  void printConfigurationToFile(FILE *, block *subject);
  void printConfigurationInternal(writer *file, block *subject, const char *sort, bool);
  mpz_ptr move_int(mpz_t);

  // The following functions have to be generated at kompile time
  // and linked with the interpreter.
  uint32_t getTagForSymbolName(const char *symbolname);
  struct blockheader getBlockHeaderForSymbol(uint32_t tag);
  bool isSymbolAFunction(uint32_t tag);
  bool isSymbolABinder(uint32_t tag);
  void storeSymbolChildren(block *symbol, void *children[]);
  void *evaluateFunctionSymbol(uint32_t tag, void *arguments[]);
  void *getToken(const char *sortname, uint64_t len, const char *tokencontents);
  layout *getLayoutData(uint16_t);
  uint32_t getInjectionForSortOfTag(uint32_t tag);

  bool hook_STRING_eq(SortString, SortString);

  const char *getSymbolNameForTag(uint32_t tag);
  const char *topSort(void);
  void printMap(writer *, map *, const char *, const char *, const char *);
  void printSet(writer *, set *, const char *, const char *, const char *);
  void printList(writer *, list *, const char *, const char *, const char *);
  void visitChildren(block *subject, writer *file,
      void visitConfig(writer *, block *, const char *, bool), 
      void visitMap(writer *, map *, const char *, const char *, const char *), 
      void visitList(writer *, list *, const char *, const char *, const char *), 
      void visitSet(writer *, set *, const char *, const char *, const char *), 
      void visitInt(writer *, mpz_t, const char *),
      void visitFloat(writer *, floating *, const char *),
      void visitBool(writer *, bool, const char *),
      void visitStringBuffer(writer *, stringbuffer *, const char *),
      void visitMInt(writer *, size_t *, size_t, const char *),
      void visitSeparator(writer *));

  void sfprintf(writer *, const char *, ...);

  stringbuffer *hook_BUFFER_empty(void);
  stringbuffer *hook_BUFFER_concat(stringbuffer *buf, string *s);
  stringbuffer *hook_BUFFER_concat_raw(stringbuffer *buf, char const *data, uint64_t n);
  string *hook_BUFFER_toString(stringbuffer *buf);

  size_t hook_SET_size_long(set *);

  mpz_ptr hook_MINT_import(size_t *i, uint64_t bits, bool isSigned);

  block *debruijnize(block *);
  block *incrementDebruijn(block *);
  block *alphaRename(block *);

  setiter set_iterator(set *);
  block *set_iterator_next(setiter *);
  mapiter map_iterator(map *);
  block *map_iterator_next(mapiter *);

  extern const uint32_t first_inj_tag, last_inj_tag;

}

std::string floatToString(const floating *);
void init_float2(floating *, std::string);

#endif // RUNTIME_HEADER_H
