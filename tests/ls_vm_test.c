#include <stdlib.h>

#include <check.h>

#include "ls_vm.h"

START_TEST(test_vm_allocate) {
  LsVM *vm = ls_new_vm(NULL);
  ck_assert_int_eq(vm->bytes_allocated, 0);
  ck_assert_int_eq(vm->next_gc, 0);

  // Allocate a pointer.
  char *c = ls_allocate(vm, char);
  *c = 'f';

  // Internal state is ok.
  ck_assert_int_eq(vm->bytes_allocated, sizeof(char));
  ck_assert_int_eq(vm->next_gc, 0);

  // Free pointer.
  ls_free(vm, c);

  // Free VM.
  ls_free_vm(vm);
}
END_TEST

static Suite *alloc_suite(void) {
  Suite *s = suite_create("ls_vm");
  TCase *tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_vm_allocate);
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
