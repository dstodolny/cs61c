#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


/* beargit add <filename>
 *
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 *
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {
  FILE *findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  int exist = 0;

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      exist = 1;
      continue;
    }
    fprintf(fnewindex, "%s\n", line);
  }

  fclose(findex);
  fclose(fnewindex);

  if (exist == 0) {
    fs_rm(".beargit/.newindex");
    fprintf(stderr, "ERROR: File %s not tracked\n", filename);
    return 1;
  }

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
  const char *sp = go_bears;
  const char *mp = msg;

  while (*msg) {
    mp = msg;
    while (*mp++ == *sp++) {
      if (*sp == '\0')
        return 1;
    }
    sp = go_bears;
    msg++;
  }
  return 0;
}

long int to_decimal(char *str)
{
  int len = strlen(str);
  int power = 1;
  int num = 0;
  int i;

  for (i = len - 1; i >= 0; i--) {
    num += ((int)str[i] - '0') * power;
    power = power * 3;
  }
  return num;
}

void reverse(char *s)
{
  int len = strlen(s);
  char *p1 = s;
  char *p2 = s + len - 1;

  while (p1 < p2) {
    char tmp = *p1;
    *p1++ = *p2;
    *p2-- = tmp;
  }
}

void from_decimal(long int n, char *str)
{
  int i = 0;

  while (n > 0) {
    str[i++] = (char) ((n % 3) + '0');
    n /= 3;
  }
  str[i] = '\0';
  reverse(str);
}

void decode(char *id)
{
  while(*id) {
    switch (*id)
      {
      case '6':
        *id = '0';
        break;

      case 'c':
        *id = '2';
        break;
      }
    id++;
  }
}

void encode(char *id)
{
  while(*id) {
    switch (*id)
      {
      case '0':
        *id = '6';
        break;

      case '2':
        *id = 'c';
        break;
      }
    id++;
  }
}

void next_commit_id(char* commit_id) {
  int i;
  long int n;
  char *pt;
  char newid[strlen(commit_id)];
  const char *last_id = "cccccccccccccccccccccccccccccccccccccccc";

  if (*commit_id == '0' || strcmp(commit_id, last_id) == 0) {
    strcpy(commit_id, "6666666666666666666666666666666666666661");
  } else {
    // skip preceding '6'
    pt = commit_id;
    while (*pt++ == '6');
    pt--;

    strcpy(newid, pt);
    decode(newid);
    n = to_decimal(newid);
    from_decimal(n+1, newid);
    encode(newid);

    // left pad with '6'
    pt = commit_id;
    i = 0;
    for (i = 0; i < COMMIT_ID_BYTES - strlen(newid); i++, pt++)
      *pt = '6';
    strcpy(pt, newid);
  }
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  // create new dir
  char dirname[COMMIT_ID_SIZE+9];
  sprintf(dirname, ".beargit/%s", commit_id);
  fs_mkdir(dirname);

  // copy old .prev and .index to the new dir
  char destination[COMMIT_ID_SIZE+20];
  sprintf(destination, "%s/.prev", dirname);
  fs_cp(".beargit/.prev", destination);
  sprintf(destination, "%s/.index", dirname);
  fs_cp(".beargit/.index", destination);

  // copy all tracked files to the new dir
  FILE* findex = fopen(".beargit/.index", "r");
  char filename[FILENAME_SIZE];
  while(fgets(filename, sizeof(filename), findex)) {
    strtok(filename, "\n");
    sprintf(destination, "%s/%s", dirname, filename);
    fs_cp(filename, destination);
  }
  fclose(findex);

  // store msg and new .prev
  sprintf(destination, "%s/.msg", dirname);
  write_string_to_file(destination, msg);
  write_string_to_file(".beargit/.prev", commit_id);

  return 0;
}

/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {
  FILE* findex = fopen(".beargit/.index", "r");

  int count;
  char line[FILENAME_SIZE];

  count = 0;

  printf("Tracked files:\n\n");
  while(fgets(line, sizeof(line), findex)) {
    printf("  %s", line);
    count++;
  }
  printf("\n%d files total\n", count);
  fclose(findex);

  return 0;
}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

int beargit_log() {
  char commit_id[COMMIT_ID_SIZE];
  char commit_msg[MSG_SIZE];
  char destination[COMMIT_ID_SIZE+20];
  const char *first_id = "0000000000000000000000000000000000000000";
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);

  if (strcmp(commit_id, first_id) == 0) {
    fprintf(stderr, "ERROR: There are no commits! \n");
    return 1;
  }

  printf("\n");
  while (strcmp(commit_id, first_id) != 0) {
    sprintf(destination, ".beargit/%s/.msg", commit_id);
    read_string_from_file(destination, commit_msg, MSG_SIZE);

    printf("commit %s\n    %s\n\n", commit_id, commit_msg);
    sprintf(destination, ".beargit/%s/.prev", commit_id);
    read_string_from_file(destination, commit_id, COMMIT_ID_SIZE);
  }

  return 0;
}
