#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/http.h"

// Test fixture - setup
int test_setup(void) {
  return 0; // Success
}

// Test fixture - teardown
int test_teardown(void) {
  return 0; // Success
}

// // Test case for HTTP_Request_to_string
// void test_HTTP_Request_to_string_basic(void) {
//     // Initialize a basic HTTP request
//     http_request_t *req = HTTP_Request_init("GET", "/index.html", "HTTP/1.1",
//     "localhost:4221"); CU_ASSERT_PTR_NOT_NULL(req);

//     // Convert request to string
//     char *actual = HTTP_Request_to_string(req, "localhost:4221");
//     CU_ASSERT_PTR_NOT_NULL(actual);

//     // Expected format of the HTTP request
//     const char expected[] =
//         "GET /index.html HTTP/1.1\r\n"
//         "Host: localhost:4221\r\n"
//         "User-Agent: curl/7.64.1\r\n"
//         "Accept: */*\r\n"
//         "\r\n";

//     // Compare the actual output with expected
//     CU_ASSERT_STRING_EQUAL(actual, expected);

//     // Cleanup
//     HTTP_Request_free(req);
//     free(req);
//     free(actual);
// }

int main() {
  // Initialize CUnit test registry
  // if (CUE_SUCCESS != CU_initialize_registry()) {
  //     return CU_get_error();
  // }

  // // Create suite
  // CU_pSuite suite = CU_add_suite("HTTP_Request_to_string_suite", test_setup,
  // test_teardown); if (NULL == suite) {
  //     CU_cleanup_registry();
  //     return CU_get_error();
  // }

  // // Add tests to suite
  // if (NULL == CU_add_test(suite, "test basic HTTP request to string",
  // test_HTTP_Request_to_string_basic)) {
  //     CU_cleanup_registry();
  //     return CU_get_error();
  // }

  // // Run tests
  // CU_basic_set_mode(CU_BRM_VERBOSE);
  // CU_basic_run_tests();
  // CU_cleanup_registry();

  // return CU_get_error();
  return 0;
}