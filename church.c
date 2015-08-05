/*
 * church.c - Church numerals (was test14.c in lambda-c)
 *
 * $Id: church.c,v 1.2 2003/02/20 15:37:54 tominaga Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "lambda.h"

Lexp
Cchurch_num(int n) {
     Lexp l0;
     char *a, *p;
     size_t llen;
     int i;

     /* for n = 2, |(L 1.(L 2.|(1 (1 |2|))|))\0| */
     llen = 10 + 3*n + 1 + n + 3;
     if ((a = malloc(llen)) == NULL) {
	  /* this library cannot fail; return something */
	  l0 = Lstr2Lexp("1");
	  return l0;
     }

     p = a;
     strncpy(p, "(L 1.(L 2.", 10); p += 10;
     for (i = 0; i < n; i++) {
	  *p++ = '(';
	  *p++ = '1';
	  *p++ = ' ';
     }
     *p++ = '2';
     for (i = 0; i < n; i++)
	  *p++ = ')';
     *p++ = ')';
     *p++ = ')';
     *p++ = '\0';

     assert(p - a == llen);

     l0 = Lstr2Lexp(a);

     free(a);
     return l0;
}

static int napplication;

static int
count_app_handler(Cellidx ci, int descending) {
     if (!descending)
	  return 1;	/* do nothing, just continue */
     if (Ltype(ci) == APPL)
	  napplication++;
     return 1;		/* go deeper */
}

int
Ccount_app(Lexp l) {
     napplication = 0;
     LdfsLexp(l, count_app_handler);
     return napplication;
}

/* EOF */
