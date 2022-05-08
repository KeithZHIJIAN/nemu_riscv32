#include "expr.h"
#include "watchpoint.h"
#include <isa.h>
#include <memory/vaddr.h>

#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>

void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_help(char *args);

// static int cmd_test(char *args)
// {
//   FILE *fp = fopen("/home/zj/ics2020/nemu/tools/gen-expr/input", "r");
//   int bufferLength = 256;
//   char buffer[bufferLength]; /* not ISO 90 compatible */

//   while (fgets(buffer, bufferLength, fp))
//   {
//     word_t ret = 0;
//     char p[256];
//     sscanf(buffer, "%d %s", &ret, p);
//     bool t = true;
//     printf("ans is %d, %d, expression is %s, expr gets\n", ret, expr(p, &t), p);
//   }

//   fclose(fp);
//   return 0;
// }

static int cmd_si(char *args)
{
  int steps = (args == NULL) ? 1 : atoi(args);
  cpu_exec(steps);
  return 0;
}

static int cmd_info(char *args)
{
  if (strcmp(args, "r\0") == 0)
    isa_reg_display();
  else if (strcmp(args, "w\0") == 0)
    watchpoints_display();
  return 0;
}

static int cmd_d(char *args)
{
  if (args == NULL)
  {
    printf("d command needs an argument.\n");
    return 0;
  }
  int wp_no = 0;
  sscanf(args, "%d", &wp_no);
  free_wp(wp_no);
  return 0;
}

static int cmd_w(char *args)
{
  if (args == NULL)
  {
    printf("w command needs an argument.\n");
    return 0;
  }
  WP *wp = new_wp(args);
  if (wp == NULL)
  {
    printf("Failed to create watchpoint.\n");
  }
  return 0;
}

static int cmd_x(char *args)
{
  if (args == NULL)
  {
    return 0;
  }
  int n;
  word_t exprs;
  sscanf(args, "%d%x", &n, &exprs);
  for (int i = 0; i < n; i++)
  {
    printf("0x%8x\t", exprs + i * 32);
    word_t data = vaddr_read(exprs + i * 32, 4);
    for (int j = 0; j < 4; j++)
    {
      printf("%02x ", data & 0xff);
      data >>= 8;
    }
    printf("\n");
  }
  return 0;
}

static struct
{
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* TODO: Add more commands */
    {"si", "Step through a single instruction", cmd_si},
    {"info", "List information about the argument", cmd_info},
    {"x", "Display the memory contents at a given address", cmd_x},
    {"d", "Delete a watchpoint", cmd_d},
    {"w", "Create a watchpoint", cmd_w},
    // {"test", "Test the gen-expr", cmd_test},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop()
{
  if (is_batch_mode())
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
