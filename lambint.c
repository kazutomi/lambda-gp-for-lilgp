#include <stdio.h>
#include <assert.h>

#include <lilgp.h>

#include "lambda.h"

/*
 * create new Var cell for variable #vi (internal form = negative)
 */

Lexp
Icreatebvar(Var vi) {
     Var v;
     Lexp newbv;

     assert(vi < 0);
     v = -vi;

     if (v > g.blevel) {
	  /* XXX: not bound */
	  newbv = Lnewvar(v);
	  return newbv;
     }

     /* okay, it's a bound variable */
     /*
      * This is the fixed point
      */
     v = g.blevel - v + 1;
     newbv = Lnewvar(v);
     return newbv;
}

/* EOF */
