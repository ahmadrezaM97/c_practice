#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "../include/vector.h"

// A simple struct for testing with complex types
typedef struct {
    int id;
    double value;
} TestStruct;

int test_setup(void) {
    return 0; // Success
}

int test_teardown(void) {
    return 0; // Success
}

void test_vector_init_and_free(void) {
    Vector v;
    vec_init(&v, TestStruct);
    CU_ASSERT_EQUAL(v.len, 0);
    CU_ASSERT_EQUAL(v.cap, 0);
    CU_ASSERT_EQUAL(v.elem_size, sizeof(TestStruct));
    CU_ASSERT_PTR_NULL(v.data_ptr);
    vec_free(&v);
    // After freeing, check that the pointer is nulled out
    CU_ASSERT_PTR_NULL(v.data_ptr);
}

void test_vector_push_and_get(void) {
    Vector v;
    vec_init(&v, int);

    vec_push(&v, int, 10);
    vec_push(&v, int, 20);
    vec_push(&v, int, 30);

    CU_ASSERT_EQUAL(v.len, 3);
    CU_ASSERT_TRUE(v.cap >= 3);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 0), 10);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 1), 20);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 2), 30);

    vec_free(&v);
}


void test_vector_erase(void) {
    Vector v;
    vec_init(&v, int);

    // 10, 20, 30, 40, 50
    for (int i = 1; i <= 5; i++) {
        vec_push(&v, int, i * 10);
    }
    CU_ASSERT_EQUAL(v.len, 5);

    // Erase the middle element (30)
    // Vector becomes: 10, 20, 40, 50
    vec_erase(&v, 2);
    CU_ASSERT_EQUAL(v.len, 4);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 0), 10);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 1), 20);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 2), 40); // 30 is gone, 40 is here
    CU_ASSERT_EQUAL(*vec_get(&v, int, 3), 50);

    vec_free(&v);
}

void test_vector_edge_cases(void) {
    Vector v;
    vec_init(&v, int);

    // Test growth by pushing many elements
    int num_elements = 1000;
    for (int i = 0; i < num_elements; i++) {
        vec_push(&v, int, i);
    }
    CU_ASSERT_EQUAL(v.len, num_elements);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 999), 999);
    
    // Erase the last element
    vec_erase(&v, 999);
    CU_ASSERT_EQUAL(v.len, 999);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 998), 998);

    // Erase the first element
    vec_erase(&v, 0);
    CU_ASSERT_EQUAL(v.len, 998);
    CU_ASSERT_EQUAL(*vec_get(&v, int, 0), 1); // Element '1' is now at the start

    vec_free(&v);
}


void test_vector_of_structs(void) {
    Vector v;
    vec_init(&v, TestStruct);

    TestStruct s1 = {1, 3.14};
    TestStruct s2 = {2, 2.71};

    vec_push(&v, TestStruct, s1);
    vec_push(&v, TestStruct, s2);
    
    CU_ASSERT_EQUAL(v.len, 2);
    
    TestStruct* retrieved_s1 = vec_get(&v, TestStruct, 0);
    CU_ASSERT_EQUAL(retrieved_s1->id, 1);
    CU_ASSERT_DOUBLE_EQUAL(retrieved_s1->value, 3.14, 0.001);

    TestStruct* retrieved_s2 = vec_get(&v, TestStruct, 1);
    CU_ASSERT_EQUAL(retrieved_s2->id, 2);
    CU_ASSERT_DOUBLE_EQUAL(retrieved_s2->value, 2.71, 0.001);

    vec_free(&v);
}


int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("Vector_Suite", test_setup, test_teardown);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests to the suite
    if ((NULL == CU_add_test(suite, "test_vector_init_and_free", test_vector_init_and_free)) ||
        (NULL == CU_add_test(suite, "test_vector_push_and_get", test_vector_push_and_get)) ||
        (NULL == CU_add_test(suite, "test_vector_erase", test_vector_erase)) ||
        (NULL == CU_add_test(suite, "test_vector_edge_cases", test_vector_edge_cases)) ||
        (NULL == CU_add_test(suite, "test_vector_of_structs", test_vector_of_structs)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }


    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
