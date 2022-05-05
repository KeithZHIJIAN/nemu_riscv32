#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

static int pos = 0;
static char ops[] = "+-*/";
static inline int choose(int n) { return rand() % n; }

static inline void gen_rand_expr(int *curr, int *success)
{
  if (*success == 0)
    return;
  switch (choose(3))
  {
  case 0:;
    int temp = rand() % 10;
    char num = temp + '0';
    buf[pos++] = num;
    *curr = temp;
    break;
  case 1:
    buf[pos++] = '(';
    gen_rand_expr(curr, success);
    buf[pos++] = ')';
    break;
  default:;
    int val1 = 0;
    gen_rand_expr(&val1, success);
    int check_point = pos;
    buf[pos++] = ops[rand() % 4];
    int val2 = 0;
    gen_rand_expr(&val2, success);
    break;
  }
  buf[pos] = '\0';
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++)
  {

    int success = 1;
    do
    {
      pos = 0;
      success = 1;
      int curr = 0;
      gen_rand_expr(&curr, &success);
    } while (!success);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0)
      continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    if (fscanf(fp, "%d", &result) != 1)
    {
      fprintf(stderr, "fscanf failed\n");
      exit(1);
    }
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
