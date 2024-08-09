#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "ls_vm.h"

START_TEST(test_new_string) {
  LsVM *vm = ls_new_vm(NULL);

  // Allocate a string.
  LsValue strval = ls_new_string(vm, "Hello world!");
  LsObj *strobj = ls_val2obj(strval);
  (void)strobj;

  // Object is well initialized.
  ck_assert_int_eq(strobj->type, LS_OBJ_STRING);
  ck_assert(!strobj->is_dark);
  ck_assert_ptr_null(strobj->next);

  LsObjString *str = (LsObjString *)strobj;
  // Check string specific fields.
  ck_assert_ptr_eq(&str->obj, strobj);
  ck_assert_int_eq(str->length, 12);

  // VM Internal state is ok.
  // +1 for null terminated byte.
  ck_assert_int_eq(vm->bytes_allocated, sizeof(LsObjString) + str->length + 1);
  ck_assert_int_eq(vm->next_gc, 0);
  ck_assert_ptr_eq(vm->first_obj, strobj);

  // Free pointer.
  ls_free(vm, strobj);

  // Free VM.
  ls_free_vm(vm);
}
END_TEST

static Suite *alloc_suite(void) {
  Suite *s = suite_create("ls_vm");
  TCase *tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_new_string);
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
