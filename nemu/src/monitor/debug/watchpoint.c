#include "watchpoint.h"
#include "expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP *new_wp(char *args)
{
  WP *wp = free_;
  if (wp == NULL)
  {
    printf("No more free watchpoints!\n");
    assert(0);
  }
  free_ = wp->next;
  wp->next = head;
  head = wp;
  sscanf(args, "%s", wp->expr);
  bool t = true;
  wp->prev_val = expr(args, &t);
  if (t) // if expr is valid
  {
    printf("Watchpoint %d is watching %s.\n", wp->NO, wp->expr);
  }
  else
  {
    printf("Invalid expression: %s\n", wp->expr);
    free_wp(wp->NO);
    return NULL;
  }
  return wp;
}

void free_wp(int wp_no)
{
  WP *p = head;
  WP *prev = NULL;
  while (p != NULL && p->NO != wp_no)
  {
    prev = p;
    p = p->next;
  }
  if (p == NULL)
  {
    printf("Watchpoint not found!\n");
    return;
  }
  if (prev == NULL)
  {
    head = p->next;
  }
  else
  {
    prev->next = p->next;
  }
  p->next = free_;
  free_ = p;
  printf("Watchpoint %d is freed.\n", wp_no);
}

void trace_watchpoints(bool *stop)
{
  WP *wp = head;
  while (wp != NULL)
  {
    bool t = true;
    word_t curr_val = expr(wp->expr, &t);

    if (!t)
    {
      printf("expr error\n");
      return;
    }

    if (wp->prev_val != curr_val)
    {
      printf("Watchpoint %d triggerred! Value changed from 0x%08x to 0x%08x\n", wp->NO, wp->prev_val, curr_val);
      wp->prev_val = curr_val;
      *stop = true;
    }

    wp = wp->next;
  }
}

void watchpoints_display()
{
  WP *wp = head;
  printf("| No \t| What \t\t| Value \t|\n");
  while (wp != NULL)
  {
    printf("| %d \t| %-8s \t| 0x%08x \t|\n", wp->NO, wp->expr, wp->prev_val);
    wp = wp->next;
  }
}
