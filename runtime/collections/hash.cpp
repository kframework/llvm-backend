#include "runtime/header.h"

extern "C" {
  void map_hash(map *, void *);
  void list_hash(list *, void *);
  void set_hash(set *, void *);
  void int_hash(mpz_ptr, void *);
  void float_hash(floating *, void *);

  static thread_local uint32_t hash_length;
  static thread_local uint32_t hash_depth;
  static constexpr uint32_t HASH_THRESHOLD = 5;
  static constexpr uint32_t HASH_LENGTH_THRESHOLD = 1024;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

__attribute__ ((always_inline)) void add_hash8(void *h, uint8_t data) {
    size_t *hash = (size_t *)h;
    *hash = ((*hash) ^ ((size_t)data)) * 1099511628211UL;
    hash_length++;
  }

  __attribute__ ((always_inline)) void add_hash64(void *h, uint64_t data) {
    uint8_t *buf = (uint8_t *)&data;
    add_hash8(h, buf[0]);
    add_hash8(h, buf[1]);
    add_hash8(h, buf[2]);
    add_hash8(h, buf[3]);
    add_hash8(h, buf[4]);
    add_hash8(h, buf[5]);
    add_hash8(h, buf[6]);
    add_hash8(h, buf[7]);
  }

  __attribute__ ((always_inline)) void add_hash_str(void *h, char *data, size_t len) {
    if (len + hash_length > HASH_LENGTH_THRESHOLD) {
      len = HASH_LENGTH_THRESHOLD - hash_length;
    }
    for (size_t i = 0; i < len; i++) {
      add_hash8(h, data[i]);
    }
  }

#pragma GCC diagnostic pop

  size_t hash_k(block *term) {
    size_t hash = 14695981039346656037ULL;
    hash_length = 0;
    k_hash(term, &hash);

    return hash;
  }

  bool hash_enter() {
    bool result = hash_depth < HASH_THRESHOLD && hash_length < HASH_LENGTH_THRESHOLD;
    hash_depth = hash_depth + 1;
    return result;
  }

  void hash_exit() {
    hash_depth = hash_depth - 1;
  }

  void k_hash(block *arg, void *h) {
    if (hash_enter()) {
      uint64_t argintptr = (uint64_t)arg;
      if (is_leaf_block(argintptr)) {
        add_hash64(h, argintptr);
      } else {
        uint64_t arghdrcanon = arg->h.hdr & HDR_MASK;
        if (uint16_t arglayout = layout(arg)) {
          add_hash64(h, arghdrcanon);
          layout *layoutPtr = getLayoutData(arglayout);
          uint8_t length = layoutPtr->nargs;
          for (uint8_t i = 0; i < length; i++) {
            uint64_t offset = layoutPtr->args[i].offset;
            uint16_t cat = layoutPtr->args[i].cat;
            switch(cat) {
            case MAP_LAYOUT: {
              map *mapptr = (map *)(argintptr+offset);
              map_hash(mapptr, h);
              break;
            }
            case LIST_LAYOUT: {
              list *listptr = (list *)(argintptr+offset);
              list_hash(listptr, h);
              break;
            }
            case SET_LAYOUT: {
              set *setptr = (set *)(argintptr+offset);
              set_hash(setptr, h);
              break;
            }
            case INT_LAYOUT: {
              mpz_ptr *intptrptr = (mpz_ptr *)(argintptr+offset);
              int_hash(*intptrptr, h);
              break;
            }
            case FLOAT_LAYOUT: {
              floating **floatptrptr = (floating **)(argintptr+offset);
              float_hash(*floatptrptr, h);
              break;
            }
            case BOOL_LAYOUT: {
              bool *boolptr = (bool *)(argintptr+offset);
              add_hash8(h, *boolptr);
              break;
            }
            case SYMBOL_LAYOUT:
            case VARIABLE_LAYOUT: {
              block **childptrptr = (block **)(argintptr+offset);
              k_hash(*childptrptr, h);
              break;
            }
            default:
              abort();
            }
          }
        } else {
          string *str = (string *)arg;
          add_hash_str(h, str->data, len(arg));
        }
      }
    }
    hash_exit();
  }
}
