#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/str.h"


// Test case for String_split
void test_string_split(void) {

  String *src = String_make("apple remerge germany berlin");
  String *del = String_make(" ");

  Vector *res = String_split(src, del);

  char *expected_res[] = {"apple", "redmerge", "germany", "berlin"};

  for (int i = 0; i < res->len; i++) {
    String *item = vec_get(res, String, i);
    CU_ASSERT_STRING_EQUAL(item->data, expected_res[i]);
    // printf("%s\n", item->data);
  }
}

int main() {
  // Initialize CUnit test registry
  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  // Create suite
  CU_pSuite suite = CU_add_suite("String", 0, 0);
  if (NULL == suite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  // Add tests to suite
  if (NULL == CU_add_test(suite, "String Split", test_string_split)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  // Run tests
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}