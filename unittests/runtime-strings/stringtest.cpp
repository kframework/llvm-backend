#include<boost/test/unit_test.hpp>
#include<gmp.h>
#include<mpfr.h>
#include<cstdint>
#include<cstdlib>
#include<cstring>
#include<cmath>

#include "runtime/header.h"
#include "runtime/alloc.h"

#define KCHAR char
extern "C" {
  bool hook_STRING_gt(string *, string *);
  bool hook_STRING_ge(string *, string *);
  bool hook_STRING_lt(string *, string *);
  bool hook_STRING_le(string *, string *);
  bool hook_STRING_eq(string *, string *);
  bool hook_STRING_ne(string *, string *);
  string * hook_STRING_concat(string *, string *);
  mpz_ptr hook_STRING_length(string *);
  string * hook_STRING_chr(mpz_t);
  mpz_ptr hook_STRING_ord(string *);
  string * hook_STRING_substr(string *, mpz_t, mpz_t);
  mpz_ptr hook_STRING_find(string *, string *, mpz_ptr);
  mpz_ptr hook_STRING_rfind(string *, string *, mpz_ptr);
  mpz_ptr hook_STRING_findChar(string *, string *, mpz_ptr);
  mpz_ptr hook_STRING_rfindChar(string *, string *, mpz_ptr);
  mpz_ptr hook_STRING_string2int(string *);
  floating *hook_STRING_string2float(string *);
  mpz_ptr hook_STRING_string2base(string *, mpz_t);
  string * hook_STRING_float2string(floating *);
  string * hook_STRING_int2string(mpz_t);
  string * hook_STRING_replaceAll(string *, string *, string *);
  string * hook_STRING_replace(string *, string *, string *, mpz_t);
  string * hook_STRING_replaceFirst(string *, string *, string *);
  mpz_ptr hook_STRING_countAllOccurrences(string *, string *);
  string * hook_STRING_transcode(string *, string *, string *);
  string * makeString(const KCHAR *, int64_t len = -1);
  stringbuffer *hook_BUFFER_empty();
  stringbuffer *hook_BUFFER_concat(stringbuffer *, string *);
  string *hook_BUFFER_toString(stringbuffer *);

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

  void add_hash64(void*, uint64_t) {}
}

BOOST_AUTO_TEST_SUITE(StringTest)

  BOOST_AUTO_TEST_CASE(gt) {
    auto a = makeString("hello");
    auto b = makeString("he");
    auto c = makeString("hf");
    auto d = makeString("");

    BOOST_CHECK_EQUAL(false, hook_STRING_gt(a,a));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(a,b));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(a,c));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(a,d));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(b,a));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(b,b));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(b,c));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(b,d));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(c,a));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(c,b));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(c,c));
    BOOST_CHECK_EQUAL(true,  hook_STRING_gt(c,d));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(d,a));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(d,b));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(d,c));
    BOOST_CHECK_EQUAL(false, hook_STRING_gt(d,d));
  }

BOOST_AUTO_TEST_CASE(ge) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(a,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(a,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(a,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(a,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(b,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(b,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(b,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(b,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(c,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(c,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(c,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(c,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(d,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(d,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_ge(d,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ge(d,d));
}

BOOST_AUTO_TEST_CASE(lt) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  BOOST_CHECK_EQUAL(false, hook_STRING_lt(a,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(a,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(a,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(a,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(b,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(b,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(b,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(b,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(c,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(c,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(c,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(c,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(d,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(d,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_lt(d,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_lt(d,d));
}

BOOST_AUTO_TEST_CASE(le) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  BOOST_CHECK_EQUAL(true,  hook_STRING_le(a,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(a,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(a,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(a,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(b,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(b,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(b,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(b,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(c,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(c,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(c,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_le(c,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(d,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(d,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(d,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_le(d,d));
}

BOOST_AUTO_TEST_CASE(eq) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  BOOST_CHECK_EQUAL(true,  hook_STRING_eq(a,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(a,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(a,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(a,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(b,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_eq(b,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(b,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(b,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(c,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(c,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_eq(c,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(c,d));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(d,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(d,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_eq(d,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_eq(d,d));
}

BOOST_AUTO_TEST_CASE(ne) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  BOOST_CHECK_EQUAL(false, hook_STRING_ne(a,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(a,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(a,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(a,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(b,a));
  BOOST_CHECK_EQUAL(false, hook_STRING_ne(b,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(b,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(b,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(c,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(c,b));
  BOOST_CHECK_EQUAL(false, hook_STRING_ne(c,c));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(c,d));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(d,a));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(d,b));
  BOOST_CHECK_EQUAL(true,  hook_STRING_ne(d,c));
  BOOST_CHECK_EQUAL(false, hook_STRING_ne(d,d));
}

BOOST_AUTO_TEST_CASE(concat) {
  auto a = makeString("hello");
  auto b = makeString("he");
  auto c = makeString("hf");
  auto d = makeString("");

  auto emptyCatR = hook_STRING_concat(a, d);
  BOOST_CHECK_EQUAL(0, memcmp(emptyCatR->data, a->data, len(emptyCatR)));
  BOOST_CHECK_EQUAL(len(emptyCatR), len(a));

  auto emptyCatL = hook_STRING_concat(d, a);
  BOOST_CHECK_EQUAL(0, memcmp(emptyCatL->data, a->data, len(emptyCatL)));
  BOOST_CHECK_EQUAL(len(emptyCatL), len(a));

  auto catAll = hook_STRING_concat(hook_STRING_concat(a,b), c);
  auto expected = makeString("hellohehf");
  BOOST_CHECK_EQUAL(0, memcmp(catAll->data, expected->data, len(catAll)));
  BOOST_CHECK_EQUAL(len(catAll), len(expected));
}


BOOST_AUTO_TEST_CASE(chr) {
  mpz_t a, b;
  mpz_init_set_ui(a, 65);
  mpz_init_set_ui(b, 32);

  const string * A = hook_STRING_chr(a);
  const string * space = hook_STRING_chr(b);

  BOOST_CHECK_EQUAL(A->data[0], 'A');
  BOOST_CHECK_EQUAL(len(A), 1);
  BOOST_CHECK_EQUAL(space->data[0], ' ');
  BOOST_CHECK_EQUAL(len(space), 1);
}

BOOST_AUTO_TEST_CASE(length) {
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_length(makeString("hello")), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_length(makeString("")), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_length(makeString("worl")), 4), 0);
}

BOOST_AUTO_TEST_CASE(ord) {
  mpz_ptr result = hook_STRING_ord(makeString("A"));
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 65), 0);
  result = hook_STRING_ord(makeString(" "));
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 32), 0);
  result = hook_STRING_ord(makeString("\xff"));
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 255), 0);
  BOOST_CHECK_THROW(hook_STRING_ord(makeString("")), std::invalid_argument);
  BOOST_CHECK_THROW(hook_STRING_ord(makeString("AA")), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(substr) {
  auto catAll = makeString("hellohehf");

  mpz_t _2, _9, _6, _0, _4, _7, _40, _8, _10, _1024, _4096;
  mpz_init_set_si(_2, 2);
  mpz_init_set_si(_9, 9);
  mpz_init_set_si(_6, 6);
  mpz_init_set_si(_0, 0);
  mpz_init_set_si(_4, 4);
  mpz_init_set_si(_7, 7);
  mpz_init_set_si(_40, 40);
  mpz_init_set_si(_8, 8);
  mpz_init_set_si(_10, 10);
  mpz_init_set_si(_1024, 1024);
  mpz_init_set_si(_4096, 4096);
  BOOST_CHECK_EQUAL(memcmp(hook_STRING_substr(catAll, _2, _9)->data, "llohehf", 7), 0);
  BOOST_CHECK_EQUAL(memcmp(hook_STRING_substr(catAll, _2, _6)->data, "lloh", 4), 0);
  BOOST_CHECK_EQUAL(memcmp(hook_STRING_substr(catAll, _0, _4)->data, "hell", 4), 0);
  BOOST_CHECK_EQUAL(memcmp(hook_STRING_substr(catAll, _6, _9)->data, "ehf", 3), 0);
  BOOST_CHECK_THROW(hook_STRING_substr(catAll, _7, _40), std::invalid_argument);
  BOOST_CHECK_THROW(hook_STRING_substr(catAll, _8, _40), std::invalid_argument);
  BOOST_CHECK_EQUAL(memcmp(hook_STRING_substr(catAll, _8, _9)->data, "f", 1), 0);
  BOOST_CHECK_EQUAL(len(hook_STRING_substr(catAll, _9, _9)), 0);
BOOST_CHECK_THROW(hook_STRING_substr(catAll, _8, _7), std::invalid_argument);
  BOOST_CHECK_THROW(hook_STRING_substr(catAll, _7, _10), std::invalid_argument);
  BOOST_CHECK_THROW(hook_STRING_substr(catAll, _1024, _4096), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(find) {
  auto haystack = makeString("hellollo");
  auto needle = makeString("llo");
  auto needle2 = makeString("hf");

  mpz_t a, b, c, d, e, f, g;
  mpz_init_set_si(a, 0);
  mpz_init_set_si(b, 1);
  mpz_init_set_si(c, 2);
  mpz_init_set_si(d, 3);
  mpz_init_set_si(e, 4);
  mpz_init_set_si(f, 5);
  mpz_init_set_si(g, 6);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, a), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, b), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, c), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, d), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, e), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, f), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle, g), -1), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_find(haystack, needle2, d), -1), 0);
}

BOOST_AUTO_TEST_CASE(findChar) {
  auto haystack = makeString("Hello world");
  auto needle = makeString("Hd");

  mpz_t a, b;
  mpz_init_set_si(a, 0);
  mpz_init_set_si(b, 1);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_findChar(haystack, needle, a), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_findChar(haystack, needle, b), 10), 0);
}

BOOST_AUTO_TEST_CASE(rfind) {
  auto haystack = makeString("hellollo");
  auto needle = makeString("llo");
  auto needle2 = makeString("hf");
  auto needle3 = makeString("hello");
  auto needle4 = makeString("lol");

  mpz_t a, b, c, d, e, f, g;
  mpz_init_set_si(a, 0);
  mpz_init_set_si(b, 1);
  mpz_init_set_si(c, 2);
  mpz_init_set_si(d, 3);
  mpz_init_set_si(e, 4);
  mpz_init_set_si(f, 5);
  mpz_init_set_si(g, 6);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, a), -1), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, b), -1), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, c), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, d), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, e), 2), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, f), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle, g), 5), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle2, d), -1), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle3, a), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfind(haystack, needle4, g), 3), 0);
}

BOOST_AUTO_TEST_CASE(rfindChar) {
  auto haystack = makeString("Hello world");
  auto needle = makeString("Hd");

  mpz_t a, b;
  mpz_init_set_si(a, 10);
  mpz_init_set_si(b, 9);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfindChar(haystack, needle, a), 10), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_rfindChar(haystack, needle, b), 0), 0);
}


BOOST_AUTO_TEST_CASE(int2string) {
  mpz_t a;
  mpz_init_set_ui(a, 10);

  auto res = hook_STRING_int2string(a);
  BOOST_CHECK_EQUAL(2, len(res));
  BOOST_CHECK_EQUAL(0, memcmp(res->data, "10", 2));

  mpz_set_ui(a, 1234);
  res = hook_STRING_int2string(a);
  BOOST_CHECK_EQUAL(4, len(res));
  BOOST_CHECK_EQUAL(0, memcmp(res->data, "1234", 4));

  mpz_set_si(a, -1234);
  res = hook_STRING_int2string(a);
  BOOST_CHECK_EQUAL(5, len(res));
  BOOST_CHECK_EQUAL(0, memcmp(res->data, "-1234", 5));

  mpz_set_ui(a, 0);
  res = hook_STRING_int2string(a);
  BOOST_CHECK_EQUAL(1, len(res));
  BOOST_CHECK_EQUAL(0, memcmp(res->data, "0", 1));
}

BOOST_AUTO_TEST_CASE(string2int) {
  auto _0 = makeString("0");
  auto _10 = makeString("10");
  auto neg10 = makeString("-10");
  auto neg10000 = makeString("-10000");
  auto plus1000 = makeString("+1000");

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2int(_0), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2int(_10), 10), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2int(neg10), -10), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2int(neg10000), -10000), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2int(plus1000), 1000), 0);
}

BOOST_AUTO_TEST_CASE(string2base) {
  auto _0 = makeString("0");
  auto _10 = makeString("10");
  auto neg10 = makeString("-10");
  auto ff = makeString("ff");

  mpz_t int10;
  mpz_init_set_ui(int10, 10);
  mpz_t int16;
  mpz_init_set_ui(int16, 16);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(_0, int10), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(_10, int10), 10), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(neg10, int10), -10), 0);
  BOOST_CHECK_THROW(hook_STRING_string2base(ff, int10), std::invalid_argument);

  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(_0, int16), 0), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(_10, int16), 16), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(neg10, int16), -16), 0);
  BOOST_CHECK_EQUAL(mpz_cmp_si(hook_STRING_string2base(ff, int16), 255), 0);
}

BOOST_AUTO_TEST_CASE(string2float) {
  auto _float = makeString("8.0f");
  floating *result;
  result = hook_STRING_string2float(_float);
  
  BOOST_CHECK_EQUAL(24, mpfr_get_prec(result->f));
  BOOST_CHECK_EQUAL(8, result->exp);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 8.0), 0);

  string *result2 = hook_STRING_float2string(result);
  std::string resultSTL = std::string(result2->data, len(result2));
  BOOST_CHECK_EQUAL(resultSTL, "0.800000000e1f");

  _float = makeString("+Infinity");
  result = hook_STRING_string2float(_float);

  BOOST_CHECK_EQUAL(53, mpfr_get_prec(result->f));
  BOOST_CHECK_EQUAL(11, result->exp);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, INFINITY), 0);

  _float = makeString("-Infinity");
  result = hook_STRING_string2float(_float);

  BOOST_CHECK_EQUAL(53, mpfr_get_prec(result->f));
  BOOST_CHECK_EQUAL(11, result->exp);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, -INFINITY), 0);

  _float = makeString("Infinityf");
  result = hook_STRING_string2float(_float);

  BOOST_CHECK_EQUAL(24, mpfr_get_prec(result->f));
  BOOST_CHECK_EQUAL(8, result->exp);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, INFINITY), 0);

  _float = makeString("Infinityp50x10");
  result = hook_STRING_string2float(_float);

  BOOST_CHECK_EQUAL(50, mpfr_get_prec(result->f));
  BOOST_CHECK_EQUAL(10, result->exp);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, INFINITY), 0);
}

BOOST_AUTO_TEST_CASE(replace) {
  auto replacee = makeString("hello world hello world hello world he worl");
  auto matcher = makeString("hello");
  auto replacer = makeString("goodbye");

  mpz_t a, b, c, d, e;
  mpz_init_set_si(a, 0);
  mpz_init_set_si(b, 1);
  mpz_init_set_si(c, 2);
  mpz_init_set_si(d, 3);
  mpz_init_set_si(e, 4);

  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replaceAll(replacee, matcher, replacer),
        makeString("goodbye world goodbye world goodbye world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replace(replacee, matcher, replacer, a),
        makeString("hello world hello world hello world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replace(replacee, matcher, replacer, b),
        makeString("goodbye world hello world hello world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replace(replacee, matcher, replacer, c),
        makeString("goodbye world goodbye world hello world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replace(replacee, matcher, replacer, d),
        makeString("goodbye world goodbye world goodbye world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replace(replacee, matcher, replacer, e),
        makeString("goodbye world goodbye world goodbye world he worl")));
  BOOST_CHECK_EQUAL(true,
      hook_STRING_eq(hook_STRING_replaceFirst(replacee, matcher, replacer),
        makeString("goodbye world hello world hello world he worl")));
}

BOOST_AUTO_TEST_CASE(countAllOccurrences) {
  auto replacee = makeString("hello world hello world hello world he worl");
  auto matcher = makeString("hello");
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_countAllOccurrences(replacee, matcher), 3), 0);

  replacee = makeString("hel world hel world heo world he worl");
  matcher = makeString("hello");
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_countAllOccurrences(replacee, matcher), 0), 0);

  replacee = makeString("hel world hel world hello world he worl");
  matcher = makeString("hello");
  BOOST_CHECK_EQUAL(mpz_cmp_ui(hook_STRING_countAllOccurrences(replacee, matcher), 1), 0);
}

BOOST_AUTO_TEST_CASE(buffer_empty) {
  auto buf = hook_BUFFER_empty();
  BOOST_CHECK_EQUAL(16, len(buf->contents));
  BOOST_CHECK_EQUAL(0, buf->strlen);
}

BOOST_AUTO_TEST_CASE(buffer_concat) {
  auto buf = hook_BUFFER_empty();
  size_t totalLen = 0;
  for (int i = 0; i < 10000; i++) {
    size_t len = rand() % 1000;
    totalLen += len;
    auto str = static_cast<string *>(malloc(sizeof(string) + len));
    set_len(str, len);
    memset(str->data, 'a', len);
    hook_BUFFER_concat(buf, str);
  }
  auto result = hook_BUFFER_toString(buf);
  auto expected = static_cast<string *>(malloc(sizeof(string) + totalLen));
  set_len(expected, totalLen);
  memset(expected->data, 'a', totalLen);
  BOOST_CHECK_EQUAL(totalLen, len(result));
  BOOST_CHECK_EQUAL(0, memcmp(result->data, expected->data, totalLen));
}

BOOST_AUTO_TEST_CASE(transcode) {
  auto foo = makeString("foo");
  auto fooUTF16LE = makeString("f\0o\0o\0", 6);
  auto fooUTF16BE = makeString("\0f\0o\0o", 6);
  auto fooUTF32LE = makeString("f\0\0\0o\0\0\0o\0\0\0", 12);
  auto fooUTF32BE = makeString("\0\0\0f\0\0\0o\0\0\0o", 12);
  auto UTF8 = makeString("UTF-8");
  auto UTF16LE = makeString("UTF-16LE");
  auto UTF16BE = makeString("UTF-16BE");
  auto UTF32LE = makeString("UTF-32LE");
  auto UTF32BE = makeString("UTF-32BE");
  auto result = hook_STRING_transcode(foo, UTF8, UTF16LE);
  BOOST_CHECK_EQUAL(1, hook_STRING_eq(result, fooUTF16LE));
  result = hook_STRING_transcode(foo, UTF8, UTF16BE);
  BOOST_CHECK_EQUAL(1, hook_STRING_eq(result, fooUTF16BE));
  result = hook_STRING_transcode(foo, UTF8, UTF32LE);
  BOOST_CHECK_EQUAL(1, hook_STRING_eq(result, fooUTF32LE));
  result = hook_STRING_transcode(foo, UTF8, UTF32BE);
  BOOST_CHECK_EQUAL(1, hook_STRING_eq(result, fooUTF32BE));
}

BOOST_AUTO_TEST_SUITE_END()
