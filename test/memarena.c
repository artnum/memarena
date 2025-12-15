
#include "../src/include/memarena.h"
#include <check.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ALIGNED_SIZE(x)                                                        \
  ((((x) + (MEMARENA_ALIGNMENT - 1)) / MEMARENA_ALIGNMENT) * MEMARENA_ALIGNMENT)
#define REGION_FREE_SPACE(r) ((r)->capacity - (r)->used)

START_TEST(test_memarena_init) {
  int ps = getpagesize();
  mem_arena_t *arena = mem_arena_new(0);
  ck_assert_ptr_nonnull(arena->tail);
  ck_assert_ptr_nonnull(arena->head);
  ck_assert_int_eq(arena->pagesize, ps);
  mem_arena_destroy(arena);
}
END_TEST

START_TEST(test_memarena_malloc) {
  int ps = getpagesize();
  size_t asize = ALIGNED_SIZE(sizeof(size_t));
  mem_arena_t *arena = mem_arena_new(0);
  ck_assert_ptr_nonnull(arena->tail);
  ck_assert_ptr_nonnull(arena->head);
  ck_assert_int_eq(arena->pagesize, ps);

  uint8_t *ptr = mem_alloc(arena, 12);
  ck_assert_int_eq(*(size_t *)(ptr - asize), 12);
  mem_free(arena, ptr);
  ck_assert_int_eq(*(size_t *)(ptr - asize), 0);

  mem_arena_destroy(arena);
}
END_TEST

START_TEST(test_memarena_realloc) {
  int ps = getpagesize();
  size_t asize = ALIGNED_SIZE(sizeof(size_t));
  /*** init and test init */
  mem_arena_t *arena = mem_arena_new(0);
  ck_assert_ptr_nonnull(arena->tail);
  ck_assert_ptr_nonnull(arena->head);
  ck_assert_int_eq(arena->pagesize, ps);

  /*** test realloc of last allocation, only size should grow, so the pointer
   * stay the same */
  uint8_t *ptr = mem_alloc(arena, 12);
  ck_assert_int_eq(*(size_t *)(ptr - asize), 12);
  uint8_t *ptr2 = mem_realloc(arena, ptr, 24);
  ck_assert_int_eq(*(size_t *)(ptr - asize), 24);
  ck_assert_ptr_eq(ptr, ptr2);
  mem_free(arena, ptr);
  ck_assert_int_eq(*(size_t *)(ptr - asize), 0);

  /*** test realloc with copying memory, we do 2 alloc so last alloc is not the
   * one we realloc, the realloc must copy memory to new pointer */

  /* alloc a pointer a fille with value */
  ptr = mem_alloc(arena, 24);
  for (int i = 0; i < 24; i++) {
    ptr[i] = i + 1;
  }

  /* add one alloc so previous alloc is not the last one */
  uint8_t *ptr3 = mem_alloc(arena, 12);
  ck_assert_int_eq(*(size_t *)(ptr3 - asize), 12);

  uint8_t *ptr4 = mem_realloc(arena, ptr, 48);
  ck_assert_int_eq(*(size_t *)(ptr4 - asize), 48);
  /* new ptr4 must be different as it's a new alloc */
  ck_assert_ptr_ne(ptr, ptr4);

  /* under the new pointer, values should be the same */
  ck_assert_int_eq(*(size_t *)(ptr - asize), 24);
  for (int i = 0; i < 24; i++) {
    ck_assert_int_eq(ptr4[i], i + 1);
  }

  mem_arena_destroy(arena);
}
END_TEST

START_TEST(test_memarena_memdup) {
  char ptr[100] = {0};

  for (int i = 0; i < 100; i++) {
    ptr[i] = (char)i + 1;
  }

  mem_arena_t *arena = mem_arena_new(0);
  ck_assert_ptr_nonnull(arena);
  void *cptr = mem_memdup(arena, ptr, sizeof(char) * 100);
  ck_assert_ptr_nonnull(cptr);
  for (int i = 0; i < 100; i++) {
    ck_assert_int_eq(ptr[i], ((char *)cptr)[i]);
  }
  mem_arena_destroy(arena);
}
END_TEST

Suite *test_memarena_suite(void) {
  Suite *s;
  s = suite_create("Memarena Test");

  TCase *tc_init = tcase_create("Init");
  tcase_add_test(tc_init, test_memarena_init);
  suite_add_tcase(s, tc_init);

  TCase *tc_malloc = tcase_create("Malloc");
  tcase_add_test(tc_malloc, test_memarena_malloc);
  suite_add_tcase(s, tc_malloc);

  TCase *tc_realloc = tcase_create("Realloc");
  tcase_add_test(tc_realloc, test_memarena_realloc);
  suite_add_tcase(s, tc_realloc);

  TCase *tc_memdup = tcase_create("Memdup");
  tcase_add_test(tc_memdup, test_memarena_memdup);
  suite_add_tcase(s, tc_memdup);
  return s;
}

int main(void) {
  int failed = 0;
  Suite *s;
  SRunner *sr = srunner_create(s);

  s = test_memarena_suite();
  sr = srunner_create(s);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_VERBOSE);
  failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
