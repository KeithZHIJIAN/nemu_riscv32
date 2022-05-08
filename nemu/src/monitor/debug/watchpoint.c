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

WP *new_wp()
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
      printf("Watchpoint triggerred! No %d changed from %d to %d\n", wp->NO, wp->prev_val, curr_val);
      wp->prev_val = curr_val;
      *stop = true;
    }

    wp = wp->next;
  }
}

void watchpoints_display()
{
  WP *wp = head;
  printf("| No \t\t| What \t\t| Value \t|\n");
  while (wp != NULL)
  {
    printf("| %d \t\t| %s \t\t| %d \t|\n", wp->NO, wp->expr, wp->prev_val);
    wp = wp->next;
  }
}
