#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "src/utils.h"
#include "src/tables.h"
#include "src/translate_utils.h"
#include "src/translate.h"

const char* TMP_FILE = "test_output.txt";
const int BUF_SIZE = 1024;

/****************************************
 *  Helper functions
 ****************************************/

int do_nothing() {
  return 0;
}

int init_log_file() {
  set_log_file(TMP_FILE);
  return 0;
}

int check_lines_equal(char **arr, int num) {
  char buf[BUF_SIZE];

  FILE *f = fopen(TMP_FILE, "r");
  if (!f) {
    CU_FAIL("Could not open temporary file");
    return 0;
  }
  for (int i = 0; i < num; i++) {
    if (!fgets(buf, BUF_SIZE, f)) {
      CU_FAIL("Reached end of file");
      return 0;
    }
    CU_ASSERT(!strncmp(buf, arr[i], strlen(arr[i])));
  }
  fclose(f);
  return 0;
}

/****************************************
 *  Test cases for translate_utils.c
 ****************************************/

void test_translate_reg() {
  CU_ASSERT_EQUAL(translate_reg("$0"), 0);
  CU_ASSERT_EQUAL(translate_reg("$at"), 1);
  CU_ASSERT_EQUAL(translate_reg("$v0"), 2);
  CU_ASSERT_EQUAL(translate_reg("$a0"), 4);
  CU_ASSERT_EQUAL(translate_reg("$a1"), 5);
  CU_ASSERT_EQUAL(translate_reg("$a2"), 6);
  CU_ASSERT_EQUAL(translate_reg("$a3"), 7);
  CU_ASSERT_EQUAL(translate_reg("$t0"), 8);
  CU_ASSERT_EQUAL(translate_reg("$t1"), 9);
  CU_ASSERT_EQUAL(translate_reg("$t2"), 10);
  CU_ASSERT_EQUAL(translate_reg("$t3"), 11);
  CU_ASSERT_EQUAL(translate_reg("$s0"), 16);
  CU_ASSERT_EQUAL(translate_reg("$s1"), 17);
  CU_ASSERT_EQUAL(translate_reg("$3"), -1);
  CU_ASSERT_EQUAL(translate_reg("asdf"), -1);
  CU_ASSERT_EQUAL(translate_reg("hey there"), -1);
}

void test_translate_num() {
  long int output;

  CU_ASSERT_EQUAL(translate_num(&output, "35", -1000, 1000), 0);
  CU_ASSERT_EQUAL(output, 35);
  CU_ASSERT_EQUAL(translate_num(&output, "145634236", 0, 9000000000), 0);
  CU_ASSERT_EQUAL(output, 145634236);
  CU_ASSERT_EQUAL(translate_num(&output, "0xC0FFEE", -9000000000, 9000000000), 0);
  CU_ASSERT_EQUAL(output, 12648430);
  CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 72), 0);
  CU_ASSERT_EQUAL(output, 72);
  CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 71), -1);
  CU_ASSERT_EQUAL(translate_num(&output, "72", 72, 150), 0);
  CU_ASSERT_EQUAL(output, 72);
  CU_ASSERT_EQUAL(translate_num(&output, "72", 73, 150), -1);
  CU_ASSERT_EQUAL(translate_num(&output, "35x", -100, 100), -1);
}

/****************************************
 *  Test cases for tables.c
 ****************************************/

void test_table_1() {
  int retval;

  SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
  CU_ASSERT_PTR_NOT_NULL(tbl);

  retval = add_to_table(tbl, "abc", 8);
  CU_ASSERT_EQUAL(retval, 0);
  retval = add_to_table(tbl, "efg", 12);
  CU_ASSERT_EQUAL(retval, 0);
  retval = add_to_table(tbl, "q45", 16);
  CU_ASSERT_EQUAL(retval, 0);
  retval = add_to_table(tbl, "q45", 24);
  CU_ASSERT_EQUAL(retval, -1);
  retval = add_to_table(tbl, "bob", 14);
  CU_ASSERT_EQUAL(retval, -1);

  retval = get_addr_for_symbol(tbl, "abc");
  CU_ASSERT_EQUAL(retval, 8);
  retval = get_addr_for_symbol(tbl, "q45");
  CU_ASSERT_EQUAL(retval, 16);
  retval = get_addr_for_symbol(tbl, "ef");
  CU_ASSERT_EQUAL(retval, -1);

  free_table(tbl);

  char* arr[] = { "Error: name 'q45' already exists in table.",
    "Error: address is not a multiple of 4." };
  check_lines_equal(arr, 2);

  SymbolTable* tbl2 = create_table(SYMTBL_NON_UNIQUE);
  CU_ASSERT_PTR_NOT_NULL(tbl2);

  retval = add_to_table(tbl2, "q45", 16);
  CU_ASSERT_EQUAL(retval, 0);
  retval = add_to_table(tbl2, "q45", 24);
  CU_ASSERT_EQUAL(retval, 0);

  free_table(tbl2);
}

void test_table_2() {
  int retval, max = 100;

  SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
  CU_ASSERT_PTR_NOT_NULL(tbl);

  char buf[10];
  for (int i = 0; i < max; i++) {
    sprintf(buf, "%d", i);
    retval = add_to_table(tbl, buf, 4 * i);
    CU_ASSERT_EQUAL(retval, 0);
  }

  for (int i = 0; i < max; i++) {
    sprintf(buf, "%d", i);
    retval = get_addr_for_symbol(tbl, buf);
    CU_ASSERT_EQUAL(retval, 4 * i);
  }

  free_table(tbl);
}

void test_translate_inst() {
  int retval;

  FILE *f = fopen(TMP_FILE, "w");
  fclose(f);

  f = fopen(TMP_FILE, "a");

  char *argsR1[6] = {"$s0", "$a0", "$a1"};
  char *argsR1err1[6] = {"s0", "$a0", "$a1"};
  char *argsR1err2[6] = {"$s0", "$a0"};
  char *argsR2[6] = {"$s0", "$a0", "10"};
  char *argsR2err1[6] = {"$s0", "$a0"};
  char *argsR2err2[6] = {"$s0", "$a0", "32"};
  char *argsR2err3[6] = {"$s0", "$a0", "-1"};
  char *argsJ1[6] = {"$s0"};
  char *argsJ1err1[6] = {"$s0", "$a0"};
  char *argsJ1err2[6] = {"s0"};
  char *argsI1[6] = {"$s0", "$a0", "10000"};
  char *argsI1err1[6] = {"$s0", "$a0", "32768"};
  char *argsI1err2[6] = {"$s0", "$a0", "-10"};
  char *argsI1neg[6] = {"$s0", "$a0", "-10"};
  char *argsI2[6] = {"$s0", "10000"};
  char *argsI2err1[6] = {"$s0"};
  char *argsI2err2[6] = {"$s0", "-10"};
  char *argsI3[6] = {"$s0", "4", "$a0"};
  char *argsI3err1[6] = {"$s0", "4"};
  char *argsI3err2[6] = {"$s0", "32768", "$a0"};
  char *argsI4[6] = {"$s0", "-4", "$a0"};
  char *argsB1[6] = {"$s0", "$a0", "fun"};
  char *argsB1err1[6] = {"$s0", "$a0", "fun2"};
  char *argsB1err2[6] = {"$s0", "a0", "fun"};
  char *argsJ2[6] = {"printf"};
  char *argsJ2err[6] = {"printf", "$0"};
  char *argsJ3[6] = {"scanf"};

  SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
  SymbolTable* reltbl = create_table(SYMTBL_NON_UNIQUE);
  CU_ASSERT_PTR_NOT_NULL(symtbl);
  retval = add_to_table(symtbl, "fun", 20);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "addu", argsR1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "addu", argsR1err1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "addu", argsR1err2, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "or", argsR1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "slt", argsR1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "sltu", argsR1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "sll", argsR2, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "sll", argsR2err1, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "sll", argsR2err2, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "sll", argsR2err3, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "jr", argsJ1, 1, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "jr", argsJ1err1, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "jr", argsJ1err2, 1, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "addiu", argsI1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "addiu", argsI1err1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "addiu", argsI1neg, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "ori", argsI1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "ori", argsI1err2, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "lui", argsI2, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "lui", argsI2err1, 1, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "lui", argsI2err2, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "lb", argsI3, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "lb", argsI3err1, 2, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "lb", argsI3err2, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "lbu", argsI3, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "lw", argsI3, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "sb", argsI3, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "sw", argsI4, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "beq", argsB1, 3, 4, symtbl, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "beq", argsB1err1, 3, 4, symtbl, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "beq", argsB1err2, 3, 4, symtbl, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "bne", argsB1, 3, 28, symtbl, NULL);
  CU_ASSERT_EQUAL(retval, 0);

  retval = translate_inst(f, "j", argsJ2, 1, 28, NULL, reltbl);
  CU_ASSERT_EQUAL(retval, 0);
  retval = get_addr_for_symbol(reltbl, "printf");
  CU_ASSERT_EQUAL(retval, 28);

  retval = translate_inst(f, "j", argsJ2err, 2, 28, NULL, reltbl);
  CU_ASSERT_EQUAL(retval, -1);

  retval = translate_inst(f, "jal", argsJ3, 1, 32, NULL, reltbl);
  CU_ASSERT_EQUAL(retval, 0);
  retval = get_addr_for_symbol(reltbl, "scanf");
  CU_ASSERT_EQUAL(retval, 32);

  retval = translate_inst(f, "xxx", argsR1, 3, 0, NULL, NULL);
  CU_ASSERT_EQUAL(retval, -1);

  free_table(symtbl);
  free_table(reltbl);
  fclose(f);
}

void test_write_pass_one() {
  int retval;

  FILE *f = fopen(TMP_FILE, "w");
  fclose(f);

  f = fopen(TMP_FILE, "a");

  char *args_addu[6] = {"$s0", "$a0", "$a1"};
  char *args_li1[6] = {"$s0", "65536"};
  char *args_li2[6] = {"$s0", "65535"};
  char *args_li3[6] = {"$s0", "-1"};
  char *args_li4[6] = {"$s0", "-32769"};
  char *args_li5[6] = {"$s0", "-2147483648"};
  char *args_lierr1[6] = {"$s0"};
  char *args_lierr2[6] = {"$s0", "-2147483649" };
  char *args_lierr3[6] = {"$s0", "4294967296" };
  char *args_blt[6] = {"$s0", "$a0", "loop"};
  char *args_blterr[6] = {"$s0", "$a0"};

  retval = write_pass_one(f, "addu", args_addu, 3);
  CU_ASSERT_EQUAL(retval, 1);

  retval = write_pass_one(f, "li", args_li1, 2);
  CU_ASSERT_EQUAL(retval, 2);

  retval = write_pass_one(f, "li", args_li2, 2);
  CU_ASSERT_EQUAL(retval, 1);

  retval = write_pass_one(f, "li", args_li3, 2);
  CU_ASSERT_EQUAL(retval, 1);

  retval = write_pass_one(f, "li", args_li4, 2);
  CU_ASSERT_EQUAL(retval, 2);

  retval = write_pass_one(f, "li", args_li5, 2);
  CU_ASSERT_EQUAL(retval, 2);

  retval = write_pass_one(f, "li", args_lierr1, 1);
  CU_ASSERT_EQUAL(retval, 0);

  retval = write_pass_one(f, "li", args_lierr2, 2);
  CU_ASSERT_EQUAL(retval, 0);

  retval = write_pass_one(f, "li", args_lierr3, 2);
  CU_ASSERT_EQUAL(retval, 0);

  retval = write_pass_one(f, "blt", args_blt, 3);
  CU_ASSERT_EQUAL(retval, 2);

  retval = write_pass_one(f, "blt", args_blterr, 2);
  CU_ASSERT_EQUAL(retval, 0);

  fclose(f);
}


/****************************************
 *  Add your test cases here
 ****************************************/

int main(int argc, char** argv) {
  CU_pSuite pSuite1 = NULL, pSuite2 = NULL, pSuite3 = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  /* Suite 1 */
  pSuite1 = CU_add_suite("Testing translate_utils.c", NULL, NULL);
  if (!pSuite1) {
    goto exit;
  }
  if (!CU_add_test(pSuite1, "test_translate_reg", test_translate_reg)) {
    goto exit;
  }
  if (!CU_add_test(pSuite1, "test_translate_num", test_translate_num)) {
    goto exit;
  }

  /* Suite 2 */
  pSuite2 = CU_add_suite("Testing tables.c", init_log_file, NULL);
  if (!pSuite2) {
    goto exit;
  }
  if (!CU_add_test(pSuite2, "test_table_1", test_table_1)) {
    goto exit;
  }
  if (!CU_add_test(pSuite2, "test_table_2", test_table_2)) {
    goto exit;
  }

  /* Suite 3 */
  pSuite3 = CU_add_suite("Testing translate.c", NULL, NULL);
  if (!pSuite3) {
    goto exit;
  }
  if (!CU_add_test(pSuite3, "test_translate_inst", test_translate_inst)) {
    goto exit;
  }
  if (!CU_add_test(pSuite3, "test_write_pass_one", test_write_pass_one)) {
    goto exit;
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

 exit:
  CU_cleanup_registry();
  return CU_get_error();;
}
