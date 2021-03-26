#include<boost/test/unit_test.hpp>
#include<gmp.h>
#include<mpfr.h>
#include<cstdint>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<dlfcn.h>

#include "runtime/header.h"
#include "runtime/alloc.h"

#define KCHAR char
#define TYPETAG(type) "Lbl'Hash'ffi'Unds'" #type "{}"
extern "C" {

  struct point {
    int x;
    int y;
  };

  struct point2 {
    struct point p;
  };

#define NUM_SYMBOLS 6
  const char * symbols[NUM_SYMBOLS] = {TYPETAG(struct), TYPETAG(uint), TYPETAG(sint), TYPETAG(pointer), "inj{SortBytes{}, SortKItem{}}", "inj{SortFFIType{}, SortKItem{}}"};

  char * getTerminatedString(string * str);

  uint32_t getTagForSymbolName(const char *s) {
    for (int i = 0; i < NUM_SYMBOLS; i++) {
      if (0 == strcmp(symbols[i], s)) {
        return i + 1;
      }
    }

    return 0;
  }

  struct blockheader getBlockHeaderForSymbol(uint32_t tag) {
    return blockheader {tag};
  }

  void add_hash64(void*, uint64_t) {}

  size_t hash_k(block * kitem) {
    return (size_t) kitem;
  }

  bool hash_enter(void) {
    return true;
  }

  void hash_exit(void) {}

  void k_hash(block *, void *) {}

  bool during_gc() {
    return false;
  }

  void printConfigurationInternal(writer *file, block *subject, const char *sort, bool) {}
  void sfprintf(writer *, const char *, ...) {}

  bool hook_KEQUAL_eq(block * lhs, block * rhs) {
    return lhs->h.hdr == rhs->h.hdr;
  }

  mpz_ptr hook_FFI_address(string * fn);
  string * hook_FFI_call(mpz_t addr, list * args, list * types, block * ret);
  string * hook_FFI_call_variadic(mpz_t addr, list * args, list * fixtypes, list * vartypes, block * ret);

  mpz_ptr hook_FFI_bytes_address(string * bytes);
  block * hook_FFI_free(block * kitem);
  block * hook_FFI_freeAll(void);
  block * hook_FFI_bytes_ref(string * bytes);
  string * hook_FFI_alloc(block * kitem, mpz_t size, mpz_t align);
  bool hook_FFI_allocated(block * kitem);

  string * makeString(const KCHAR *, int64_t len = -1);

  list hook_LIST_element(block * value);
  list hook_LIST_concat(list * l1, list * l2);
  list hook_LIST_unit();
  size_t hook_LIST_size_long(list * l);
  block * hook_LIST_get_long(list * l, ssize_t idx);

  mpz_ptr move_int(mpz_t i) {
    mpz_ptr result = (mpz_ptr)malloc(sizeof(__mpz_struct));
    *result = *i;
    return result;
  }

  floating *move_float(floating *i) {
    floating *result = (floating *)malloc(sizeof(floating));
    *result = *i;
    return result;
  }

  block D1 = {{1}};
  block * DUMMY1 = &D1;
}

struct FfiTestFixture {

  ~FfiTestFixture() {
    hook_FFI_freeAll();
  }
};

BOOST_FIXTURE_TEST_SUITE(FfiTest, FfiTestFixture)

BOOST_AUTO_TEST_CASE(address) {
  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("fakeFunction");
  addr = hook_FFI_address(fn);
  BOOST_CHECK_EQUAL(0, mpz_cmp_ui(addr, 0));
}

BOOST_AUTO_TEST_CASE(call) {
  /* int timesTwo(int x) */
  int x = -3;
  string * xargstr = makeString((char *) &x, sizeof(int)); 

  block * xarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  xarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(xarg->children, &xargstr, sizeof(string *));

  list args = hook_LIST_element(xarg);
  block * type_sint = leaf_block(getTagForSymbolName(TYPETAG(sint)));

  block * argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}, SortKItem{}}"));
  memcpy(argtype->children, &type_sint, sizeof(block *));

  list types = hook_LIST_element(argtype);

  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);

  string * bytes = hook_FFI_call(addr, &args, &types, type_sint);

  BOOST_CHECK(bytes != NULL);

  int ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* unsigned int utimesTwo(unsigned int x) */
  x = 4;
  xargstr = makeString((char *) &x, sizeof(int)); 

  memcpy(xarg->children, &xargstr, sizeof(string *));

  args = hook_LIST_element(xarg);
  block * type_uint = leaf_block(getTagForSymbolName(TYPETAG(uint)));
  memcpy(argtype->children, &type_uint, sizeof(block *));

  types = hook_LIST_element(argtype);

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_uint);

  BOOST_CHECK(bytes != NULL);

  ret = *(unsigned int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* int times(int x, int y) */
  int y = 4;
  string * yargstr = makeString((char *) &y, sizeof(int));

  memcpy(argtype->children, &type_sint, sizeof(block *));

  block * yarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  yarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(yarg->children, &yargstr, sizeof(string *));

  list yargs = hook_LIST_element(yarg);

  args = hook_LIST_concat(&args, &yargs);
  types = hook_LIST_concat(&types, &types);

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * y);

  /* struct point constructPoint(int x, int y) */
  block * structType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  list * structFields = static_cast<list *>(koreAlloc(sizeof(list)));
  list tmp = hook_LIST_element(argtype);
  tmp = hook_LIST_concat(&tmp, &tmp);
  //TODO: explain why this is ok
  memcpy((void *)structFields, &tmp, sizeof(list));

  memcpy(structType->children, &structFields, sizeof(list *));

  fn = makeString("constructPoint");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, structType);

  struct point p = *(struct point *) bytes->data;
  BOOST_CHECK_EQUAL(p.x, x);
  BOOST_CHECK_EQUAL(p.y, y);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 1);

  /* void increaseX(void) */
  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 2);

  /* struct point {
   *  int x;
   *  int y;
   * }
   *
   * int timesPoint(struct point p) */
  p = {.x = 2, .y = 5};
  string * pargstr = makeString((char *) &p, sizeof(struct point));

  block * parg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  parg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(parg->children, &pargstr, sizeof(string *));

  args = hook_LIST_element(parg);

  block * new_argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  new_argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}, SortKItem{}}"));
  memcpy(new_argtype->children, &structType, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p.x * p.y);

  /* struct point2 {
   *  struct point p;
   * }
   *
   * int timesPoint2(struct point2 p) */
  struct point2 p2 = {.p = p};
  string * pargstr2 = makeString((char *) &p2, sizeof(struct point2));

  memcpy(parg->children, &pargstr2, sizeof(string *));

  args = hook_LIST_element(parg);

  block * structType2 = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType2->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  block * structArgType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structArgType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}, SortKItem{}}"));
  memcpy(structArgType->children, &structType, sizeof(block *));

  list * structFields2 = static_cast<list *>(koreAlloc(sizeof(list)));
  list tmp2 = hook_LIST_element(structArgType);
  //TODO: explain why this is ok
  memcpy((void *)structFields2, &tmp2, sizeof(list));

  memcpy(structType2->children, &structFields2, sizeof(list *));
  
  memcpy(new_argtype->children, &structType2, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint2");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);

  /* Make sure there is no double free */
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);

  /* int pointerTest(int * x) */
  x = 2;
  mpz_t s1, align;
  mpz_init_set_ui(s1, sizeof(int));
  mpz_init_set_ui(align, 16);
  string * b1 = hook_FFI_alloc(DUMMY1, s1, align);
  memcpy(b1->data, &x, sizeof(int));

  mpz_ptr addr1 = hook_FFI_bytes_address(b1);
  uintptr_t address1 = mpz_get_ui(addr1);

  string * ptrargstr = makeString((char *) &address1, sizeof(uintptr_t *));

  block * ptrarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  ptrarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(ptrarg->children, &ptrargstr, sizeof(string *));

  args = hook_LIST_element(ptrarg);
  block * type_pointer = leaf_block(getTagForSymbolName(TYPETAG(pointer)));

  memcpy(argtype->children, &type_pointer, sizeof(block *));

  types = hook_LIST_element(argtype);

  fn = makeString("pointerTest");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);

  hook_FFI_free(DUMMY1);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x);
}

BOOST_AUTO_TEST_CASE(call_variadic) {
  /* int addInts(int x, ...) */
  int n = 1;
  string * nargstr = makeString((char *) &n, sizeof(int)); 

  block * narg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  narg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(narg->children, &nargstr, sizeof(string *));

  list args = hook_LIST_element(narg);

  int arg1 = 1;
  string * arg1str = makeString((char *) &arg1, sizeof(int)); 

  block * arg1block = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  arg1block->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(arg1block->children, &arg1str, sizeof(string *));

  list arg1list = hook_LIST_element(arg1block);

  args = hook_LIST_concat(&args, &arg1list);

  block * type_sint = leaf_block(getTagForSymbolName(TYPETAG(sint)));

  block * fixargtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  fixargtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}, SortKItem{}}"));
  memcpy(fixargtype->children, &type_sint, sizeof(block *));

  block * varargtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  varargtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}, SortKItem{}}"));
  memcpy(varargtype->children, &type_sint, sizeof(block *));

  list fixtypes = hook_LIST_element(fixargtype);
  list vartypes = hook_LIST_element(varargtype);

  string * fn = makeString("addInts");
  mpz_ptr addr = hook_FFI_address(fn);

  string * bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  int ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, arg1);

  /* addInts with 2 var args */
  n = 2;
  nargstr = makeString((char *) &n, sizeof(int));
  memcpy(narg->children, &nargstr, sizeof(string *));
  args = hook_LIST_element(narg);

  arg1 = 20;
  arg1str = makeString((char *) &arg1, sizeof(int));
  memcpy(arg1block->children, &arg1str, sizeof(string *));
  arg1list = hook_LIST_element(arg1block);
  args = hook_LIST_concat(&args, &arg1list);

  int arg2 = 15;
  string * arg2str = makeString((char *) &arg2, sizeof(int));

  block * arg2block = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  arg2block->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}, SortKItem{}}"));
  memcpy(arg2block->children, &arg2str, sizeof(string *));

  list arg2list = hook_LIST_element(arg2block);

  args = hook_LIST_concat(&args, &arg2list);

  vartypes = hook_LIST_element(varargtype);
  vartypes = hook_LIST_concat(&vartypes, &vartypes);

  bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, arg1 + arg2);

  /* addInts with 0 var args */
  n = 0;
  nargstr = makeString((char *) &n, sizeof(int));
  memcpy(narg->children, &nargstr, sizeof(string *));
  args = hook_LIST_element(narg);

  vartypes = hook_LIST_unit();

  bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 0);
}

BOOST_AUTO_TEST_CASE(alloc) {
  mpz_t s1, s2;
  mpz_init_set_ui(s1, 1);
  mpz_init_set_ui(s2, 16);

  string * b1 = hook_FFI_alloc(DUMMY1, s1, s2);
  BOOST_CHECK(0 != b1);

  string * b2 = hook_FFI_alloc(DUMMY1, s1, s2);
  BOOST_CHECK_EQUAL(b1, b2);
}

BOOST_AUTO_TEST_CASE(free) {
  mpz_t s1, s2;
  mpz_init_set_ui(s1, 1);
  mpz_init_set_ui(s2, 16);

  hook_FFI_alloc(DUMMY1, s1, s2);
  hook_FFI_free(DUMMY1);
}

BOOST_AUTO_TEST_CASE(bytes_ref) {
  mpz_t s1, s2;
  mpz_init_set_ui(s1, 1);
  mpz_init_set_ui(s2, 16);

  string * b1 = hook_FFI_alloc(DUMMY1, s1, s2);

  block * i2 = hook_FFI_bytes_ref(b1);

  BOOST_CHECK_EQUAL(DUMMY1, i2);
}

BOOST_AUTO_TEST_CASE(allocated) {
  mpz_t s1, s2;
  mpz_init_set_ui(s1, 1);
  mpz_init_set_ui(s2, 16);

  hook_FFI_alloc(DUMMY1, s1, s2);
  BOOST_CHECK_EQUAL(true, hook_FFI_allocated(DUMMY1));

  block * i2 = (block *) 2;
  BOOST_CHECK_EQUAL(false, hook_FFI_allocated(i2));
}

BOOST_AUTO_TEST_CASE(alignment) {
  mpz_t s1, s2;
  mpz_init_set_ui(s1, 1);
  mpz_init_set_ui(s2, 32768);
  string *b1 = hook_FFI_alloc(DUMMY1, s1, s2);
  BOOST_CHECK_EQUAL(((uintptr_t)b1) % 32768, 0);
}

BOOST_AUTO_TEST_SUITE_END()
