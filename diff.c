/*
 * diff.c - calculate difference between two lambda expressions
 *
 * $Id: diff.c,v 1.1.1.1 2002/12/07 08:59:42 tominaga Exp $
 */

#include "lambda.h"

static enum facility debugfacility = F_MISC;

static Var boundvars[MAXABSTDEPTH];

/*
 * numnodes - count number of nodes in specified subtree
 */
static int
numnodes(Cellidx ci) {
  return countcells(ci);
}

/*
 * calcbdist - calculate binding distance for each variable
 */
void
calcbdist_r(Cellidx ci, int depth) {
  int i;

  switch (Ctype(ci)) {
    case VAR:
      /* search the binding lambda */
      for (i = depth - 1; i >= 0; i--)
	if (Cvar(ci) == boundvars[i])
	  break;
      if (i < 0)
	/* free variable */
	Cbdist(ci) = -(depth + 1);	/* +1 to reserve 0 */
      else {
	/* bound variable */
	Cbdist(ci) = depth - i;
      }
      return;
    case ABST:
      if (depth < MAXABSTDEPTH) {
	boundvars[depth] = Cbv(ci);
	calcbdist_r(Cbody(ci), depth + 1);
      } else {
	msg_warning("calcbdist: MAXABSTDEPTH reached; ignoring the subtree\n");
      }
      return;
    case APPL:
      calcbdist_r(Cleft(ci), depth);
      calcbdist_r(Cright(ci), depth);
      return;
    default:
      fatal("calcbdist: invalid cell type %d\n", Ctype(ci));
      /*NOTREACHED*/
  }
}

void
calcbdist(Lexp l) {
  calcbdist_r(l, 0);
}

/*
 * diff - returns difference between two lexps
 */

int
diff_r(Cellidx c1, Cellidx c2) {
  if (Ctype(c1) == VAR && Ctype(c2) == VAR) {
    if (Cbdist(c1) > 0 && Cbdist(c2) > 0) {
      /* both bound; return difference */
      return DIST(Cbdist(c1), Cbdist(c2));
    } else if (Cbdist(c1) > 0) {
      /* c1 is bound, c2 is free */
      return Cbdist(c1) - Cbdist(c2);
    } else if (Cbdist(c2) > 0) {
      /* c2 is bound, c1 is free */
      return Cbdist(c2) - Cbdist(c1);
    } else {
      /* both free */
      return DIST(Cbdist(c1), Cbdist(c2));
    }
  } else if (Ctype(c1) == VAR && Ctype(c2) == ABST) {
    return ABS(Cbdist(c1)) + numnodes(c2);
  } else if (Ctype(c1) == ABST && Ctype(c2) == VAR) {
    return ABS(Cbdist(c2)) + numnodes(c1);
  } else if (Ctype(c1) == VAR && Ctype(c2) == APPL) {
    return ABS(Cbdist(c1)) + numnodes(c2);
  } else if (Ctype(c1) == APPL && Ctype(c2) == VAR) {
    return ABS(Cbdist(c2)) + numnodes(c1);
  } else if (Ctype(c1) == ABST && Ctype(c2) == ABST) {
    return diff_r(Cbody(c1), Cbody(c2));
  } else if (Ctype(c1) == ABST && Ctype(c2) == APPL) {
    return numnodes(c1) + numnodes(c2);
  } else if (Ctype(c1) == APPL && Ctype(c2) == ABST) {
    return numnodes(c1) + numnodes(c2);
  } else if (Ctype(c1) == APPL && Ctype(c2) == APPL) {
    return diff_r(Cleft(c1), Cleft(c2)) + diff_r(Cright(c1), Cright(c2));
  } else {
    fatal("diff_r: unexpected cell type %d and %d\n", Ctype(c1), Ctype(c2));
  }
  fatal("diff_r: cannot come here\n");
  /*NOTREACHED*/
}

int
diff(Lexp l1, Lexp l2) {
  calcbdist(l1);
  calcbdist(l2);
  return diff_r(l1, l2);
}

/* EOF */
