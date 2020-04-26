#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include "beargit.h"
#include "util.h"

/* printf/fprintf calls in this tester will NOT go to file. */

#undef printf
#undef fprintf

/* The suite initialization function.
 * You'll probably want to delete any leftover files in .beargit from previous
 * tests, along with the .beargit directory itself.
 *
 * You'll most likely be able to share this across suites.
 */
int init_suite(void)
{
    // preps to run tests by deleting the .beargit directory if it exists
    fs_force_rm_beargit_dir();
    unlink("TEST_STDOUT");
    unlink("TEST_STDERR");
    return 0;
}

/* You can also delete leftover files after a test suite runs, but there's
 * no need to duplicate code between this and init_suite
 */
int clean_suite(void)
{
    return 0;
}

/* Simple test of fread().
 * Reads the data previously written by testFPRINTF()
 * and checks whether the expected characters are present.
 * Must be run after testFPRINTF().
 */
void simple_sample_test(void)
{
    // This is a very basic test. Your tests should likely do more than this.
    // We suggest checking the outputs of printfs/fprintfs to both stdout
    // and stderr. To make this convenient for you, the tester replaces
    // printf and fprintf with copies that write data to a file for you
    // to access. To access all output written to stdout, you can read
    // from the "TEST_STDOUT" file. To access all output written to stderr,
    // you can read from the "TEST_STDERR" file.
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
}

/* Test of branch command.

 * This suite is testing three things. First it checks that initialy
 * there's only master branch visible in the command output.
 *
 * Then it adds another branch and checks if the new branch is visible
 * with the '*' mark as the current branch.
 *
 * Then it checkouts to master to check if the '*' mark is near the
 * master branch.
 *
 * In the end it creates a valid commit and then checkouts that
 * commit. The branch command should display all branches available
 * without '*' mark near any of them (as we're in the detached state).
 */
void branch_test(void)
{
  int retval;
  const int LINE_SIZE = 512;
  char line[LINE_SIZE];
  FILE *fstdout;

  retval = beargit_init();
  CU_ASSERT(0==retval);

  // Test branch output initially
  retval = beargit_branch();
  CU_ASSERT(0==retval);

  fstdout = fopen("TEST_STDOUT", "r");
  CU_ASSERT_PTR_NOT_NULL(fstdout);

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
  CU_ASSERT(!strcmp(line, "* master\n"));

  CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

  CU_ASSERT(feof(fstdout));
  fclose(fstdout);

  // Clean up TEST_STDOUT file
  fstdout = fopen("TEST_STDOUT", "w");
  fclose(fstdout);

  // Test branch output after creating new branch
  retval = beargit_checkout("dev", 1);
  CU_ASSERT(0==retval);

  retval = beargit_branch();
  CU_ASSERT(0==retval);

  fstdout = fopen("TEST_STDOUT", "r");
  CU_ASSERT_PTR_NOT_NULL(fstdout);

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
  CU_ASSERT(!strcmp(line, "  master\n"));

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
  CU_ASSERT(!strcmp(line, "* dev\n"));

  CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

  CU_ASSERT(feof(fstdout));
  fclose(fstdout);

  // Clean up TEST_STDOUT file
  fstdout = fopen("TEST_STDOUT", "w");
  fclose(fstdout);

  // Test branch output after checking out a commit
  retval = beargit_commit("GO BEARS!");
  CU_ASSERT(0==retval);

  retval = beargit_checkout("1666666666666666666666666666666666666661", 0);
  CU_ASSERT(0==retval);

  retval = beargit_branch();
  CU_ASSERT(0==retval);

  fstdout = fopen("TEST_STDOUT", "r");
  CU_ASSERT_PTR_NOT_NULL(fstdout);

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
  CU_ASSERT(!strcmp(line, "  master\n"));

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
  CU_ASSERT(!strcmp(line, "  dev\n"));

  CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

  CU_ASSERT(feof(fstdout));
  fclose(fstdout);
}

/* Test of checkout command
 *
 * This suit is testing checkout command. First it test checking out to
 * a particular valid commit id. When commit id does not exists the call
 * should return 1.
 */
void checkout_test(void)
{
  int retval, i;
  FILE *fstdout;
  const int LINE_SIZE = 512;
  char line[LINE_SIZE];

  retval = beargit_init();
  CU_ASSERT(0==retval);

  // Test checkout a commit id that doesn't exist
  retval = beargit_checkout("6666666666666666666666666666666666666661", 0);
  CU_ASSERT(1==retval);
  read_string_from_file("TEST_STDERR", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, "ERROR: Commit 6666666666666666666666666666666666666661 does not exist\n"));
  fstdout = fopen("TEST_STDERR", "w");
  fclose(fstdout);

  // Test checkout a commit id that does exist
  retval = beargit_add("asdf.txt");
  CU_ASSERT(0==retval);

  retval = beargit_commit("GO BEARS!");
  CU_ASSERT(0==retval);

  retval = beargit_checkout("6666666666666666666666666666666666666661", 0);
  CU_ASSERT(0==retval);
  read_string_from_file(".beargit/.current_branch", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, ""));
  read_string_from_file(".beargit/.prev", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, "6666666666666666666666666666666666666661"));

  // Checkout branch that does not exist
  retval = beargit_checkout("dev", 0);
  CU_ASSERT(1==retval);
  for (i = 0; *(line+i) != '\0'; i++)
    *(line+i) = '\0';
  read_string_from_file("TEST_STDERR", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, "ERROR: No branch dev exists\n"));

  retval = beargit_checkout("dev", 1);
  CU_ASSERT(0==retval);
  read_string_from_file(".beargit/.current_branch", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, "dev"));

  retval = beargit_checkout("master", 0);
  CU_ASSERT(0==retval);
  read_string_from_file(".beargit/.current_branch", line, LINE_SIZE);
  CU_ASSERT(!strcmp(line, "master"));

}


struct commit {
  char msg[MSG_SIZE];
  struct commit* next;
};


void free_commit_list(struct commit** commit_list) {
  if (*commit_list) {
    free_commit_list(&((*commit_list)->next));
    free(*commit_list);
  }

  *commit_list = NULL;
}

void run_commit(struct commit** commit_list, const char* msg) {
    int retval = beargit_commit(msg);
    CU_ASSERT(0==retval);

    struct commit* new_commit = (struct commit*)malloc(sizeof(struct commit));
    new_commit->next = *commit_list;
    strcpy(new_commit->msg, msg);
    *commit_list = new_commit;
}

void simple_log_test(void)
{
    struct commit* commit_list = NULL;
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    run_commit(&commit_list, "GO BEARS!1");
    run_commit(&commit_list, "GO BEARS!2");
    run_commit(&commit_list, "GO BEARS!3");

    retval = beargit_log();
    CU_ASSERT(0==retval);

    struct commit* cur_commit = commit_list;

    const int LINE_SIZE = 512;
    char line[LINE_SIZE];

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);

    while (cur_commit != NULL) {
      char refline[LINE_SIZE];

      // First line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strcmp(line,"\n"));

      // Second line is commit -- don't check the ID.
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strncmp(line,"commit", strlen("commit")));

      // Third line is msg
      sprintf(refline, "    %s\n", cur_commit->msg);
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, refline);

      cur_commit = cur_commit->next;
    }

    // Last line is empty
    CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
    CU_ASSERT(!strcmp(line,"\n"));

    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    // It's the end of output
    CU_ASSERT(feof(fstdout));
    fclose(fstdout);

    free_commit_list(&commit_list);
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int cunittester()
{
   CU_pSuite pSuite = NULL;
   CU_pSuite pSuite2 = NULL;
   CU_pSuite pSuite3 = NULL;
   CU_pSuite pSuite4 = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Suite_1", init_suite, clean_suite);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Add tests to the Suite #1 */
   if (NULL == CU_add_test(pSuite, "Simple Test #1", simple_sample_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   pSuite2 = CU_add_suite("Suite_2", init_suite, clean_suite);
   if (NULL == pSuite2) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Add tests to the Suite #2 */
   if (NULL == CU_add_test(pSuite2, "Log output test", simple_log_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add a suite to the registry */
   pSuite3 = CU_add_suite("Suite_3", init_suite, clean_suite);
   if (NULL == pSuite3) {
     CU_cleanup_registry();
     return CU_get_error();
   }

   /* Add test named branch_test to Suite 3 */
   if (NULL == CU_add_test(pSuite3, "Branch output test", branch_test)) {
     CU_cleanup_registry();
     return CU_get_error();
   }

   /* add a suite to the registry */
   pSuite4 = CU_add_suite("Suite_4", init_suite, clean_suite);
   if (NULL == pSuite4) {
     CU_cleanup_registry();
     return CU_get_error();
   }

   /* Add test named checkout_test to Suite 4 */
   if (NULL == CU_add_test(pSuite4, "Checkout test", checkout_test)) {
     CU_cleanup_registry();
     return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
