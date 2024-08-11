#include <check.h>
#include <stdlib.h>

#include "lex.h"
#include "utils.h"
#include "value.h"

static void test_lex_string(const char *tcase, const char *source,
                            const Token *expected) {
  (void)tcase;
  log_debug("====== %s ====== ...\n", tcase);

  Lexer l = {0};
  lexer_init(&l, source);

  Token tk = lex(&l);
  do {
    log_debug("token content (%ld bytes) '%.*s'\n", tk.length, (int)tk.length,
              tk.start_char);
    ck_assert_int_eq(tk.type, expected->type);
    ck_assert_int_eq(tk.length, expected->length);
    ck_assert_mem_eq(tk.start_char, expected->start_char, tk.length);
    ck_assert_int_eq(tk.line, expected->line);
    ck_assert_int_eq(tk.value, expected->value);
    if (expected->type == TOKEN_EOF || expected->type == TOKEN_ERROR)
      break;
    expected++;
    tk = lex(&l);
  } while (1);

  log_debug("====== %s ====== OK\n", tcase);
}

START_TEST(test_lex_number) {
  test_lex_string("double", "3.14",
                  (Token[]){
                      {TOKEN_NUMBER, "3.14", 4, 1, 3.14},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("negative double", "-100.1",
                  (Token[]){
                      {TOKEN_NUMBER, "-100.1", 6, 1, -100.1},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("double with leading white spaces", "    \t  3.14",
                  (Token[]){
                      {TOKEN_NUMBER, "3.14", 4, 1, 3.14},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("double with leading trailing spaces", "    \t  3.14    \t",
                  (Token[]){
                      {TOKEN_NUMBER, "3.14", 4, 1, 3.14},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("negative integer", "-100",
                  (Token[]){
                      {TOKEN_NUMBER, "-100", 4, 1, -100},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("integer", "100",
                  (Token[]){
                      {TOKEN_NUMBER, "100", 3, 1, 100},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("scientific notation (uppercase E)", "100E-3",
                  (Token[]){
                      {TOKEN_NUMBER, "100E-3", 6, 1, 0.1},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("scientific notation (lowercase e)", "100e+3",
                  (Token[]){
                      {TOKEN_NUMBER, "100e+3", 6, 1, 100000},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
  test_lex_string("hexadecimal", "0xdeadBEEF",
                  (Token[]){
                      {TOKEN_NUMBER, "0xdeadBEEF", 10, 1, 0xdeadbeef},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });

  test_lex_string("invalid range", "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
                  (Token[]){
                      {
                          TOKEN_ERROR,
                          "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
                          40,
                          1,
                          LEXERR_INVALID_NUM_RANGE,
                      },
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
}
END_TEST

START_TEST(test_lex_number_op) {
  test_lex_string("100 - 100", "100 - 100",
                  (Token[]){
                      {TOKEN_NUMBER, "100", 3, 1, 100},
                      {TOKEN_MINUS, "-", 1, 1, LS_NULL},
                      {TOKEN_NUMBER, "100", 3, 1, 100},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });

  test_lex_string("100 -100", "100 -100",
                  (Token[]){
                      {TOKEN_NUMBER, "100", 3, 1, 100},
                      {TOKEN_NUMBER, "-100", 4, 1, -100},
                      {TOKEN_EOF, "\0", 1, 1, LS_NULL},
                  });
}
END_TEST

static Suite *alloc_suite(void) {
  Suite *s = suite_create("lex");
  TCase *tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_lex_number);
  tcase_add_test(tc_core, test_lex_number_op);
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
