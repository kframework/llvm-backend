#include <boost/test/unit_test.hpp>
#include <gmp.h>
#include <mpfr.h>
#include <cmath>
#include <limits>

#include "runtime/header.h"

extern "C" {
  floating *hook_FLOAT_ceil(floating *);
  floating *hook_FLOAT_floor(floating *);
  floating *hook_FLOAT_round(floating *, mpz_t, mpz_t);
  mpz_ptr hook_FLOAT_float2int(floating *);
  floating *hook_FLOAT_int2float(mpz_t, mpz_t, mpz_t);
  floating *hook_FLOAT_sin(floating *);
  floating *hook_FLOAT_cos(floating *);
  floating *hook_FLOAT_tan(floating *);
  floating *hook_FLOAT_sec(floating *);
  floating *hook_FLOAT_csc(floating *);
  floating *hook_FLOAT_cot(floating *);
  floating *hook_FLOAT_asin(floating *);
  floating *hook_FLOAT_acos(floating *);
  floating *hook_FLOAT_atan(floating *);
  floating *hook_FLOAT_atan2(floating *, floating *);
  mpz_ptr hook_FLOAT_precision(floating *);
  mpz_ptr hook_FLOAT_exponentBits(floating *);
  mpz_ptr hook_FLOAT_exponent(floating *);
  bool hook_FLOAT_isNaN(floating *);
  floating *hook_FLOAT_maxValue(mpz_t, mpz_t);
  floating *hook_FLOAT_minValue(mpz_t, mpz_t);
  bool hook_FLOAT_gt(floating *, floating *);
  bool hook_FLOAT_ge(floating *, floating *);
  bool hook_FLOAT_lt(floating *, floating *);
  bool hook_FLOAT_le(floating *, floating *);
  bool hook_FLOAT_eq(floating *, floating *);
  bool hook_FLOAT_ne(floating *, floating *);
  floating *hook_FLOAT_abs(floating *);
  floating *hook_FLOAT_neg(floating *);
  floating *hook_FLOAT_min(floating *, floating *);
  floating *hook_FLOAT_max(floating *, floating *);
  floating *hook_FLOAT_add(floating *, floating *);
  floating *hook_FLOAT_sub(floating *, floating *);
  floating *hook_FLOAT_mul(floating *, floating *);
  floating *hook_FLOAT_div(floating *, floating *);
  floating *hook_FLOAT_rem(floating *, floating *);
  floating *hook_FLOAT_pow(floating *, floating *);
  floating *hook_FLOAT_root(floating *, mpz_t);
  floating *hook_FLOAT_log(floating *);
  floating *hook_FLOAT_exp(floating *);
  floating *hook_FLOAT_rat2float(mpz_t, mpz_t, mpz_t, mpz_t);
  bool hook_FLOAT_sign(floating *);

  floating *move_float(floating *i) {
    floating *result = (floating *)malloc(sizeof(floating));
    *result = *i;
    return result;
  }
}

static void set_float(floating *a, unsigned prec, unsigned exp, double val) {
  mpfr_init2(a->f, prec);
  a->exp = exp;
  mpfr_set_d(a->f, val, MPFR_RNDN);
}

BOOST_AUTO_TEST_SUITE(FloatTest)

BOOST_AUTO_TEST_CASE(ceil) {
  floating a[1];
  floating *result;
  set_float(a, 24, 8, 10.5);
  result = hook_FLOAT_ceil(a);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 11.0), 0);
}

BOOST_AUTO_TEST_CASE(floor) {
  floating a[1];
  floating *result;
  set_float(a, 24, 8, 10.5);
  result = hook_FLOAT_floor(a);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 10.0), 0);
}

BOOST_AUTO_TEST_CASE(round) {
  floating a[1];
  floating *result;
  mpz_t b, c;
  mpz_init_set_ui(b, 2);
  mpz_init_set_ui(c, 8);
  set_float(a, 53, 11, 10.5);
  result = hook_FLOAT_round(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 12.0), 0);
  set_float(a, 53, 11, 9.5);
  result = hook_FLOAT_round(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 8.0), 0);
  set_float(a, 53, 11, 10.5);
  mpz_set_ui(b, 24);
  result = hook_FLOAT_round(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 10.5), 0);
  set_float(a, 53, 11, 9.5);
  result = hook_FLOAT_round(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 9.5), 0);
}

BOOST_AUTO_TEST_CASE(float2int) {
  floating a[1];
  mpz_ptr result;
  set_float(a, 53, 11, 10.5);
  result = hook_FLOAT_float2int(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 10), 0);
  set_float(a, 53, 11, 9.5);
  result = hook_FLOAT_float2int(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 10), 0);
}

BOOST_AUTO_TEST_CASE(int2float) {
  mpz_t a, b, c;
  floating *result;
  mpz_init_set_ui(a, 9);
  mpz_init_set_ui(b, 2);
  mpz_init_set_ui(c, 8);
  result = hook_FLOAT_int2float(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 8.0), 0);
  mpz_set_ui(a, 11);
  result = hook_FLOAT_int2float(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 12.0), 0);
  mpz_set_ui(a, 10);
  result = hook_FLOAT_int2float(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 8.0), 0);
  mpz_set_ui(b, 24);
  result = hook_FLOAT_int2float(a, b, c);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, 10.0), 0);
}

BOOST_AUTO_TEST_CASE(trig) {
  floating a[1];
  floating *result;
  set_float(a, 53, 11, M_PI_4);
  result = hook_FLOAT_sin(a);
  double e = 0.000000000000001;
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), sin(M_PI_4), e);
  result = hook_FLOAT_cos(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), cos(M_PI_4), e);
  result = hook_FLOAT_tan(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 1.0, e);
  result = hook_FLOAT_sec(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 1.0/cos(M_PI_4), e);
  result = hook_FLOAT_csc(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 1.0/sin(M_PI_4), e);
  result = hook_FLOAT_cot(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 1.0, e);
  set_float(a, 53, 11, 0.0);
  result = hook_FLOAT_asin(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 0.0, e);
  result = hook_FLOAT_acos(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), M_PI_2, e);
  result = hook_FLOAT_atan(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 0, e);
  set_float(a, 53, 11, 1.0);
  result = hook_FLOAT_asin(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), M_PI_2, e);
  result = hook_FLOAT_acos(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), 0, e);
  result = hook_FLOAT_atan(a);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), M_PI_4, e);
  floating b[1];
  set_float(b, 53, 11, 0.0);
  result = hook_FLOAT_atan2(a, b);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), M_PI_2, e);
  set_float(a, 53, 11, -1.0);
  result = hook_FLOAT_atan2(a, b);
  BOOST_CHECK_CLOSE_FRACTION(mpfr_get_d(result->f, MPFR_RNDN), -M_PI_2, e);
}

BOOST_AUTO_TEST_CASE(precision) {
  floating a[1];
  mpz_ptr result;
  set_float(a, 2, 8, 0.0);
  result = hook_FLOAT_precision(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 2), 0);
  set_float(a, 24, 8, 0.0);
  result = hook_FLOAT_precision(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 24), 0);
}

BOOST_AUTO_TEST_CASE(exponentBits) {
  floating a[1];
  mpz_ptr result;
  set_float(a, 24, 8, 0.0);
  result = hook_FLOAT_exponentBits(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 8), 0);
  set_float(a, 53, 11, 0.0);
  result = hook_FLOAT_exponentBits(a);
  BOOST_CHECK_EQUAL(mpz_cmp_ui(result, 11), 0);
}

BOOST_AUTO_TEST_CASE(exponent) {
  floating a[1];
  mpz_ptr result;
  set_float(a, 24, 8, 0.0);
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, -127), 0);
  set_float(a, 24, 8, -0.0);
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, -127), 0);
  set_float(a, 24, 8, 1.0/0.0);
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, 128), 0);
  set_float(a, 24, 8, 0.0/0.0);
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, 128), 0);
  set_float(a, 24, 8, 4.0);
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, 2), 0);
  set_float(a, 24, 8, std::numeric_limits<float>::min());
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, -126), 0);
  set_float(a, 24, 8, std::numeric_limits<float>::denorm_min());
  result = hook_FLOAT_exponent(a);
  BOOST_CHECK_EQUAL(mpz_cmp_si(result, -127), 0);
}

BOOST_AUTO_TEST_CASE(isNaN) {
  floating a[1];
  set_float(a, 24, 8, 0.0);
  BOOST_CHECK(!hook_FLOAT_isNaN(a));
  set_float(a, 24, 8, -0.0);
  BOOST_CHECK(!hook_FLOAT_isNaN(a));
  set_float(a, 24, 8, 1.0 / 0.0);
  BOOST_CHECK(!hook_FLOAT_isNaN(a));
  set_float(a, 24, 8, 0.0 / 0.0);
  BOOST_CHECK(hook_FLOAT_isNaN(a));
}

BOOST_AUTO_TEST_CASE(maxValue) {
  mpz_t a, b;
  mpz_init_set_ui(a, 24);
  mpz_init_set_ui(b, 8);
  floating *result;
  result = hook_FLOAT_maxValue(a, b);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, std::numeric_limits<float>::max()), 0);
  mpz_init_set_ui(a, 53);
  mpz_init_set_ui(b, 11);
  result = hook_FLOAT_maxValue(a, b);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, std::numeric_limits<double>::max()), 0);
}

BOOST_AUTO_TEST_CASE(minValue) {
  mpz_t a, b;
  mpz_init_set_ui(a, 24);
  mpz_init_set_ui(b, 8);
  floating *result;
  result = hook_FLOAT_minValue(a, b);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, std::numeric_limits<float>::denorm_min()), 0);
  mpz_init_set_ui(a, 53);
  mpz_init_set_ui(b, 11);
  result = hook_FLOAT_minValue(a, b);
  BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, std::numeric_limits<double>::denorm_min()), 0);
}

BOOST_AUTO_TEST_CASE(lt) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_lt(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(!result);
      } else if (i < j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(!hook_FLOAT_lt(arr+i, nan));
    BOOST_CHECK(!hook_FLOAT_lt(nan, arr+i));
  }
  BOOST_CHECK(!hook_FLOAT_lt(nan, nan));
}

BOOST_AUTO_TEST_CASE(le) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_le(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(result);
      } else if (i <= j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(!hook_FLOAT_le(arr+i, nan));
    BOOST_CHECK(!hook_FLOAT_le(nan, arr+i));
  }
  BOOST_CHECK(!hook_FLOAT_le(nan, nan));
}

BOOST_AUTO_TEST_CASE(gt) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_gt(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(!result);
      } else if (i > j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(!hook_FLOAT_gt(arr+i, nan));
    BOOST_CHECK(!hook_FLOAT_gt(nan, arr+i));
  }
  BOOST_CHECK(!hook_FLOAT_gt(nan, nan));
}

BOOST_AUTO_TEST_CASE(ge) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_ge(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(result);
      } else if (i >= j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(!hook_FLOAT_ge(arr+i, nan));
    BOOST_CHECK(!hook_FLOAT_ge(nan, arr+i));
  }
  BOOST_CHECK(!hook_FLOAT_ge(nan, nan));
}

BOOST_AUTO_TEST_CASE(eq) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_eq(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(result);
      } else if (i == j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(!hook_FLOAT_eq(arr+i, nan));
    BOOST_CHECK(!hook_FLOAT_eq(nan, arr+i));
  }
  BOOST_CHECK(!hook_FLOAT_eq(nan, nan));
}

BOOST_AUTO_TEST_CASE(ne) {
  floating arr[6], nan[1];
  set_float(arr, 24, 8, -1.0/0.0);
  set_float(arr+1, 24, 8, -1.0);
  set_float(arr+2, 24, 8, -0.0);
  set_float(arr+3, 24, 8, 0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, 1.0/0.0);
  set_float(nan, 24, 8, 0.0/0.0);
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    for (size_t j = 0; j < sizeof(arr)/sizeof(arr[0]); j++) {
      bool result = hook_FLOAT_ne(arr+i, arr+j);
      if ((i == 2 && j == 3) || (i == 3 && j == 2)) {
        BOOST_CHECK(!result);
      } else if (i != j) {
        BOOST_CHECK(result);
      } else {
        BOOST_CHECK(!result);
      }
    }
  }
  for (size_t i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
    BOOST_CHECK(hook_FLOAT_ne(arr+i, nan));
    BOOST_CHECK(hook_FLOAT_ne(nan, arr+i));
  }
  BOOST_CHECK(hook_FLOAT_ne(nan, nan));
}

BOOST_AUTO_TEST_CASE(abs) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    floating *result = hook_FLOAT_abs(arr+i);
    float f = fabsf(ref[i]);
    if (f!=f) {
      BOOST_CHECK(mpfr_nan_p(result->f));
    } else {
      BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(log) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    floating *result = hook_FLOAT_log(arr+i);
    float f = logf(ref[i]);
    if (f!=f) {
      BOOST_CHECK(mpfr_nan_p(result->f));
    } else {
      BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(exp) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    floating *result = hook_FLOAT_exp(arr+i);
    float f = expf(ref[i]);
    if (f!=f) {
      BOOST_CHECK(mpfr_nan_p(result->f));
    } else {
      BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(neg) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    floating *result = hook_FLOAT_neg(arr+i);
    float f = -ref[i];
    if (f!=f) {
      BOOST_CHECK(mpfr_nan_p(result->f));
    } else {
      BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(min) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_min(arr+i, arr+j);
      float f = fminf(ref[i], ref[j]);
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(max) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_max(arr+i, arr+j);
      float f = fmaxf(ref[i], ref[j]);
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(add) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_add(arr+i, arr+j);
      float f = ref[i] + ref[j];
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(sub) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_sub(arr+i, arr+j);
      float f = ref[i] - ref[j];
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(mul) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_mul(arr+i, arr+j);
      float f = ref[i] * ref[j];
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(div) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_div(arr+i, arr+j);
      float f = ref[i] / ref[j];
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(rem) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_rem(arr+i, arr+j);
      float f = fmodf(ref[i], ref[j]);
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(pow) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    for (size_t j = 0; j < sizeof(ref)/sizeof(ref[0]); j++) {
      floating *result = hook_FLOAT_pow(arr+i, arr+j);
      float f = powf(ref[i], ref[j]);
      if (f!=f) {
        BOOST_CHECK(mpfr_nan_p(result->f));
      } else {
        BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(root) {
  floating arr[9];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  float ref[9];
  ref[0] = 0.0f;
  ref[1] = -0.0f;
  ref[2] = 1.0f/0.0f;
  ref[3] = -1.0f/0.0f;
  ref[4] = 1.0f;
  ref[5] = -1.0f;
  ref[6] = 3.0f;
  ref[7] = 0.5f;
  ref[8] = 0.0f/0.0f;
  mpz_t k;
  mpz_init_set_ui(k, 2);
  for (size_t i = 0; i < sizeof(ref)/sizeof(ref[0]); i++) {
    floating *result = hook_FLOAT_root(arr+i, k);
    float f = sqrt(ref[i]);
    if (f!=f) {
      BOOST_CHECK(mpfr_nan_p(result->f));
    } else {
      BOOST_CHECK_EQUAL(mpfr_cmp_d(result->f, f), 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(sign) {
  floating arr[10];
  set_float(arr+0, 24, 8, 0.0);
  set_float(arr+1, 24, 8, -0.0);
  set_float(arr+2, 24, 8, 1.0/0.0);
  set_float(arr+3, 24, 8, -1.0/0.0);
  set_float(arr+4, 24, 8, 1.0);
  set_float(arr+5, 24, 8, -1.0);
  set_float(arr+6, 24, 8, 3.0);
  set_float(arr+7, 24, 8, 0.5);
  set_float(arr+8, 24, 8, 0.0/0.0);
  BOOST_CHECK(!hook_FLOAT_sign(arr+0));
  BOOST_CHECK(hook_FLOAT_sign(arr+1));
  BOOST_CHECK(!hook_FLOAT_sign(arr+2));
  BOOST_CHECK(hook_FLOAT_sign(arr+3));
  BOOST_CHECK(!hook_FLOAT_sign(arr+4));
  BOOST_CHECK(hook_FLOAT_sign(arr+5));
  BOOST_CHECK(!hook_FLOAT_sign(arr+6));
  BOOST_CHECK(!hook_FLOAT_sign(arr+7));
  BOOST_CHECK(!hook_FLOAT_sign(arr+8));
}

BOOST_AUTO_TEST_CASE(rat2float) {
  mpz_t num, den, prec, exp;
  mpz_init_set_ui(num, 1);
  mpz_init_set_ui(den, 3);
  mpz_init_set_ui(prec, 53);
  mpz_init_set_ui(exp, 11);
  floating *result = hook_FLOAT_rat2float(num, den, prec, exp);
  mpfr_t ref;
  mpfr_init2(ref, 53);
  mpfr_set_d(ref, 0.33333333333333333333333333333333333, MPFR_RNDN);
  BOOST_CHECK_EQUAL(mpfr_cmp(ref, result->f), 0);
}

BOOST_AUTO_TEST_SUITE_END()
