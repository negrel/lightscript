#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "ls_value.h"
#include "ls_vm.h"

START_TEST(test_new_array) {
  LsVM *vm = ls_new_vm(NULL);

  // Allocate an array.
  LsValue arrval = ls_new_array(vm, 0);
  LsObj *arrobj = ls_val2obj(arrval);

  // Object is well initialized.
  ck_assert_int_eq(arrobj->type, LS_OBJ_ARRAY);
  ck_assert(!arrobj->is_dark);
  ck_assert_ptr_null(arrobj->next);

  LsObjArray *arr = (LsObjArray *)arrobj;
  // Check array specific fields.
  ck_assert_ptr_eq(&arr->obj, arrobj);
  ck_assert_int_eq(arr->elements.length, 0);
  ck_assert_int_eq(arr->elements.capacity, 0);
  ck_assert_ptr_null(arr->elements.data);

  // VM Internal state is ok.
  // +1 for null terminated byte.
  ck_assert_int_eq(vm->bytes_allocated, sizeof(LsObjArray));
  ck_assert_int_eq(vm->next_gc, 0);
  ck_assert_ptr_eq(vm->first_obj, arrobj);

  // Free pointer.
  ls_free(vm, arrobj);

  // Free VM.
  ls_free_vm(vm);
}
END_TEST

START_TEST(test_array_eq) {
  LsVM *vm = ls_new_vm(NULL);

  // Allocate a string.
  LsValue arrval = ls_new_array(vm, 100);
  LsValue arrval2 = ls_new_array(vm, 200);

  // Equal to itself.
  ck_assert(ls_val_same(arrval, arrval));
  ck_assert(ls_val_eq(arrval, arrval));

  // Not same nor equal.
  ck_assert(!ls_val_same(arrval, arrval2));
  ck_assert(!ls_val_eq(arrval, arrval2));

  // Not same nor equal (again).
  ck_assert(!ls_val_same(arrval, LS_TRUE));
  ck_assert(!ls_val_eq(arrval, LS_TRUE));

  // Free pointers.
  ls_free_obj(vm, ls_val2obj(arrval));
  ls_free_obj(vm, ls_val2obj(arrval2));

  // Free VM.
  ls_free_vm(vm);
}
END_TEST

static Suite *alloc_suite(void) {
  Suite *s = suite_create("ls_array");
  TCase *tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_new_array);
  tcase_add_test(tc_core, test_array_eq);
  suite_add_tcase(s, tc_core);

  return s;
}

int main(void) {
  Suite *suite = alloc_suite();
  SRunner *sr = srunner_create(suite);

  srunner_run_all(sr, CK_NORMAL);
  int number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
