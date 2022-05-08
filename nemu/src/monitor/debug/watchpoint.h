#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint
{
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  int prev_val;
  char expr[128];

} WP;

void trace_watchpoints();
void watchpoints_display();
WP *new_wp();
void free_wp(int wp_no);
#endif
