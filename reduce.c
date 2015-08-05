/*
 * reduce.c - interactive utility to beta-reduce lambda expressions
 *
 * Copyright (c) 2003 Kazuto Tominaga.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lambda.h"

enum {
  LEXPBUFSZ = 8192,
  MISCBUFSZ = 8192,
  MAXSTEP = 1000,
  MAXCELL = 1000,
};

int
main() {
  Lexp l;
  char lbuf[LEXPBUFSZ], buf[MISCBUFSZ];
  char *p;
  int maxstep, maxcell;
  int steps;

  Linit();
  for (;;) {
    fputs("lexp to reduce: ", stderr);
    fgets(lbuf, sizeof(buf), stdin);
    p = strchr(lbuf, '\n');
    if (*p == '\0') {
      fputs("lexp too long\n", stderr);
      continue;
    }
    if (p == lbuf)	/* empty line */
      break;
    *p = '\0';	/* chop newline */
    l = Lstr2Lexp(lbuf);

    fprintf(stderr, "max #steps to reduce [%d]: ", MAXSTEP);
    fgets(buf, sizeof(buf), stdin);
    p = strchr(buf, '\n');
    if (p == buf) {
      maxstep = MAXSTEP;
    } else {
      maxstep = atoi(buf);
    }

    fprintf(stderr, "max #cells to use [%d]: ", MAXCELL);
    fgets(buf, sizeof(buf), stdin);
    p = strchr(buf, '\n');
    if (p == buf) {
      maxcell = MAXCELL;
    } else {
      maxcell = atoi(buf);
    }

    steps = Lbeta(l, CANONICAL, maxstep, maxcell);
    LLexp2str(l, buf, sizeof(buf));
    printf("%s =(%d)=> %s\n", lbuf, steps, buf);
  }
  return 0;
}

/* EOF */
