/*
 * liblambda.c - lambda calculus engine in C
 *
 * $Id: lambda.c,v 1.5 2003/07/09 11:21:24 tominaga Exp $
 */

/*
 * standard headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * constant definitions (was in const.h)
 */

enum {
  OK = 0,
  FATAL = 1,
  MISCSTRBUFLEN = 4096,
};

/*
 * debug facilities (was in debug.h)
 */

enum level {
  L_MESG = 0,	/* always print */
  /* the latter, the more verbose; cf. syslog(3) */
  L_CRIT,
  L_ERR,
  L_WARNING,
  L_NOTICE,
  L_INFO,
  L_CURDEBUG,
  L_DEBUG,
  /* end of levels */
  L_PRODUCT = L_ERR,	/* product level */
};

enum facility {
  F_MAIN = 0,
  F_POOL,
  F_PARSER,
  F_LAMBOPS,
  F_STRLEXP,
  F_INTERFACE,
  F_MISC,
  F_NUMFACIL,	/* number of debug'ged facilities */
};

/*
 * for convenience for calling msg_generic()
 *
 * XXX: C99 required
 */
#define msg_crit(debugfacility, ...)     msg_generic(stderr, debugfacility, L_CRIT,     __VA_ARGS__)
#define msg_err(debugfacility, ...)      msg_generic(stderr, debugfacility, L_ERR,      __VA_ARGS__)
#define msg_warning(debugfacility, ...)  msg_generic(stderr, debugfacility, L_WARNING,  __VA_ARGS__)
#define msg_notice(debugfacility, ...)   msg_generic(stderr, debugfacility, L_NOTICE,   __VA_ARGS__)
#define msg_info(debugfacility, ...)     msg_generic(stderr, debugfacility, L_INFO,     __VA_ARGS__)
#define msg_curdebug(debugfacility, ...) msg_generic(stderr, debugfacility, L_CURDEBUG, __VA_ARGS__)
#define msg_debug(debugfacility, ...)    msg_generic(stderr, debugfacility, L_DEBUG,    __VA_ARGS__)
#define message(debugfacility, ...)      msg_generic(stderr, debugfacility, L_MESG,     __VA_ARGS__)

/* debug level condition */
#define deblev(l, debugfacility) (dlevel[debugfacility]>=(l))

/*
 * definitions for lambda expressions (was in lexp.h)
 */

enum {
  /* for struct lcell.type */
  /* must be zero (free) to NOTYPE;
     this range is used for checking of valid type */
  FREE = 0,
  VAR = 1,
  ABST = 2,
  APPL = 3,
  NOTYPE = 4,
  /* size constants */
  INITPOOLSIZE = 1,
  /* beta reduction strategies */
  CANONICAL = 1,
  INNERMOST = 2,
  /*
   * maximum abstraction depth for Lexp canonicalization (canonbvars());
   * requires storage of (MAXABSTDEPTH * sizeof(Var)) bytes
   */
  MAXABSTDEPTH = 1024,
  /*
   * maximum tree height.  used for diff'ing
   */
  MAXTREEHEIGHT = 2048,
};

typedef long int Var;		/* variable */
typedef long int Cellidx;	/* cell pool index */
typedef long int Lexp;		/* top of lambda expression; actually Cellidx */

enum {
  CELLIDX_MAX = LONG_MAX,
  VAR_MAX = LONG_MAX,
};

struct lcell {
  int type;	/* FREE, VAR, ABST or APPL */
  union {
    /* VAR */
    Var var;
    /* ABST */
    struct {
      Var bv;
      Cellidx body;
    } ab;
    /* APPL */
    struct {
      Cellidx left;
      Cellidx right;
    } ap;
  } d;
  Cellidx nextfree;
  Var bdist;	/* binding distance; used to calculate differences */
};

typedef struct lcell Lcell;

struct velem {
  Var v;
  struct velem *next;
};

typedef struct velem VarList;

/* aliases for simplicity */
#define Ctype(idx)	(pool[idx].type)
#define Cvar(idx)	(pool[idx].d.var)
#define Cbv(idx)	(pool[idx].d.ab.bv)
#define Cbody(idx)	(pool[idx].d.ab.body)
#define Cleft(idx)	(pool[idx].d.ap.left)
#define Cright(idx)	(pool[idx].d.ap.right)
#define Cbdist(idx)	(pool[idx].bdist)

/*
 * parser definitions (was in parser.h)
 */

/* terminal / nonterminal symbols */
enum token {
  LP_ERROR = 0,
  LP_NULL,	/* '\0' */
  LP_VAR,	/* digits */
  LP_LAMBDA,	/* L */
  LP_LPAREN,	/* ( */
  LP_RPAREN,	/* ) */
  LP_DOT,	/* . */
  LP_NUMTOK,	/* number of kinds of tokens */
};

/*
 * misc definitions (was in misc.h)
 */
#define max(x, y) (((x)>(y))?(x):(y))
#define min(x, y) (((x)<(y))?(x):(y))
#define ABS(x) ((x)>=0?(x):-(x))
#define DIST(x, y) (((x)-(y))>0?((x)-(y)):((y)-(x)))

/*
 * prototype declarations of functions (was in proto.h)
 */

/*
 * functions that was in ilambda.c
 */
Lexp Lnewvar(Var);
Lexp Labst(Var, Lexp);
Lexp Lappl(Lexp, Lexp);
Lexp Lcopy(Lexp);
void Lfree(Lexp);
Lexp Lstr2Lexp(char *);
int LLexp2str(Lexp, char *, int);
void Lcanon(Lexp);
int Leq(Lexp, Lexp);
int Lbeta(Lexp, int, int, int);
void Linit(void);
void LdfsLexp(Lexp, int (*)(Cellidx, int));
int Ltype(Cellidx);
int Lcountcells(Lexp);
void Lepoolinfo(void);

/*
 * functions that was in strlexp.c
 */
static void fprintlexp_n(FILE *, Lexp);
static void fprintlexp(FILE *, Lexp);
static void printlexp(Lexp);
static void eprintlexp(Lexp);
static int lexp2str(Cellidx, char *, int);
static Lexp str2lexp(char *);

/*
 * functions that was in pool.c
 */
void *emalloc(size_t);
static void fprintfreelist(FILE *);
static void printfreelist(void);
static void eprintfreelist(void);
static void fprintcell(FILE *, Cellidx);
static void printcell(Cellidx);
static void eprintcell(Cellidx);
static void initpool(void);
static Cellidx newcell(int);
static void enlargepool(void);
static void fprintpool(FILE *);
static void printpool(void);
static void eprintpool(void);
static void fpoolinfo(FILE *);
static void poolinfo(void);
static void epoolinfo(void);
static void freecell(Cellidx);
static void prunecell(Cellidx);
static Cellidx deepcopy(Cellidx);
static void copycell(Cellidx, Cellidx);
static void canonbvars(Lexp);
static int isequalLexp_r(Cellidx, VarList *, Cellidx, VarList *);
static int isequalLexp(Lexp, Lexp);
static void dfsLexp_rec(Cellidx, int (*)(Cellidx, int));
static void dfsLexp(Lexp, int (*)(Cellidx, int));

/*
 * functions that was in message.c
 */
static void fatal(char *, ...);
static void abortwithcore(char *, ...);
static void msg_generic(FILE *, enum facility, enum level, char *, ...);

/*
 * functions that was in parser.c
 */
static Cellidx create_lexp(char *);
static enum token peeknext(void);
static long int getlong(void);
static void getnext(void);
static Cellidx do_lexp(void);
static Cellidx do_abst(void);
static Cellidx do_appl(void);

/*
 * functions that was in lambops.c
 */
static Lexp alpha(Lexp, Var, Lexp);
static Var findmaxvar(Lexp);
static Lexp subst(Lexp, Var, Lexp, Var);
static int betaat(Cellidx);
static Cellidx canon_findredex(Cellidx);
static Cellidx findredex(Lexp, int);
static int betastep(Lexp, int);
static int nbeta(Lexp, int, int, int);

/*
 * newly added
 */
static int countcells_handler(Cellidx, int);
static int countcells(Lexp);

/*
 * functions that was in diff.c
 */
static void calcbdist_r(Cellidx, int);
static void calcbdist(Cellidx);
static int diff_r(Lexp, Lexp);
static int diff(Lexp, Lexp);
static int arraynodes_r(Cellidx, int, int [], int);
static int arraynodes(Cellidx, int [], int);

/*
 * global variables (was in global.c)
 */

static enum level dlevel[F_NUMFACIL] = {
  /* must be in order of enum facility; see debug.h */
  L_PRODUCT,	/* F_MAIN */
  L_PRODUCT,	/* F_POOL */
  L_PRODUCT,	/* F_PARSER */
  L_PRODUCT,	/* F_LAMBOPS */
  L_PRODUCT,	/* F_STRLEXP */
  L_PRODUCT,	/* F_INTERFACE */
  L_PRODUCT,	/* F_MISC */
};

static Lcell *pool;	/* cell pool */
static Cellidx poolsize = 0;	/* current #cells in the pool */

static char *typename[NOTYPE+1] = {
  "FREE",
  "VAR",
  "ABST",
  "APPL",
  "NOTYPE",
};

static char typechar[NOTYPE+2] = {	/* last entry means 'unknown' */
  '-',
  'V',
  'B',
  'P',
  'N',
  'X',
};

static char *tokenname[LP_NUMTOK+1] = {
  "LP_ERROR",
  "LP_NULL",
  "LP_VAR",
  "LP_LAMBDA",
  "LP_LPAREN",
  "LP_RPAREN",
  "LP_DOT",
  "LP_NUMTOK",
};

/*
 * variables private to specific facility (have been already "static")
 */

/* pool */
static unsigned long int count_allocated = 0, count_freed = 0;
static Cellidx freehead;	/* head of the free list */

/* parser */
static char *parser_cur;	/* where lex looks at */
static enum token parser_next;	/* token got */
static Var parser_tokdata;	/* token itself; only Var needs this data */
static int parser_error;

/* diff calculation */
static Var boundvars[MAXABSTDEPTH];

/*
 * user (library) interface (was in ilambda.c)
 */

Lexp
Lnewvar(Var v) {
  Cellidx c = newcell(VAR);
  Cvar(c) = v;
  return c;
}

Lexp
Labst(Var bv, Lexp body) {
  Cellidx c = newcell(ABST);
  Cbv(c) = bv;
  Cbody(c) = body;
  return c;
}

Lexp
Lappl(Lexp left, Lexp right) {
  Cellidx c = newcell(APPL);
  Cleft(c) = left;
  Cright(c) = right;
  return c;
}

Lexp
Lcopy(Lexp orig) {
  return deepcopy(orig);
}

void
Lfree(Lexp l) {
  prunecell(l);
}

Lexp
Lstr2Lexp(char *s) {
  return str2lexp(s);
}

int
LLexp2str(Lexp l, char *buf, int len) {
  Cellidx ci;

  ci = l;
  return lexp2str(ci, buf, len);
}

void
Lcanon(Lexp l) {
  /* XXX */
  fatal("Lcanon not implemented yet\n");

  canonbvars(l);
}

int
Leq(Lexp l1, Lexp l2) {
  return isequalLexp(l1, l2);
}

int
Lbeta(Lexp l, int strategy, int times, int maxcells) {
  return nbeta(l, strategy, times, maxcells);
}

void
Linit() {
  initpool();
}

void
LdfsLexp(Lexp l, int (*func)(Cellidx, int)) {
  dfsLexp(l, func);
}

int
Ltype(Cellidx ci) {
  return Ctype(ci);
}

int
Lcountcells(Lexp l) {
  return countcells(l);
}

void
Lepoolinfo() {
  epoolinfo();
}

int
Ldiff(Lexp l1, Lexp l2) {
  return diff(l1, l2);
}

/*
 * manages the cell pool (was in pool.c)
 */

void *
emalloc(size_t size) {
  void *p;

  if ((p = malloc(size)) == NULL)
    fatal("emalloc: cannot allocate %d bytes\n", size);
  return p;
}

static void
fprintfreelist(FILE *fp) {
  Cellidx i;

  fprintf(fp, "Free list: ");
  for (i = freehead; i >= 0; i = pool[i].nextfree)
    fprintf(fp, "%ld ", i);
  fprintf(fp, "\n");
}

static void
printfreelist() {
  fprintfreelist(stdout);
}

static void
eprintfreelist() {
  fprintfreelist(stderr);
}

static void
fprintcell(FILE *fp, Cellidx c) {
  fprintf(fp, "[cell#%ld]", c);
  if (poolsize <= c) {
    fprintf(fp, " out of range (poolsize %ld)\n", poolsize);
  }
  if (Ctype(c) <= NOTYPE) {
    /* nb. NOTYPE is curious but it has typename entry */
    fprintf(fp, " type = %d (%s)\n", Ctype(c), typename[Ctype(c)]);
  } else {
    fprintf(fp, " unknown type %d\n", Ctype(c));
  } 
  switch (Ctype(c)) {
    case FREE:
      break;
    case VAR:
      fprintf(fp, " var = %ld\n", Cvar(c));
      break;
    case ABST:
      fprintf(fp, " bv = %ld, body = %ld\n", Cbv(c), Cbody(c));
      break;
    case APPL:
      fprintf(fp, " left = %ld, right = %ld\n", Cleft(c), Cright(c));
      break;
    case NOTYPE:
      break;
  }
}

static void
printcell(Cellidx c) {
  fprintcell(stdout, c);
}

static void
eprintcell(Cellidx c) {
  fprintcell(stderr, c);
}

static void
initpool() {
  Cellidx i;

  /* canonbvars() requires this, and should be done once for efficiency */
  assert(MAXABSTDEPTH <= ULONG_MAX);

  /* allocate the pool */
  /* nb. The area returned by calloc is initialized by zero,
     and Lcell.type = 0 means FREE cell */
  if ((pool = calloc(INITPOOLSIZE, sizeof(Lcell))) == NULL) {
    fatal("cannot allocate the pool; size=%ld\n", INITPOOLSIZE);
  }

  /* make free list */
  pool[0].nextfree = -1;	/* means tail */
  for (i = 1; i < INITPOOLSIZE; i++)
    pool[i].nextfree = i - 1;
  freehead = INITPOOLSIZE - 1;

  poolsize = INITPOOLSIZE;
  msg_info(F_POOL, "initpool: allocated %ld cells\n", poolsize);
  return;
}

static Cellidx
newcell(int type) {
  Cellidx i;

  msg_debug(F_POOL, "newcell: requested one cell of type %d\n", type);
  /* valid type? */
  if (type <= FREE || NOTYPE <= type)
    fatal("newcell: unknown type %d requested\n", type);

  if (freehead >= 0) {
    /* get one cell from the free list */
    i = freehead;
    /* link next */
    freehead = pool[i].nextfree;
    /* set up the cell */
    msg_debug(F_POOL, "newcell: get one cell from free list [index %ld]\n", i);
    Ctype(i) = type;
    count_allocated++;
    return i;
  }

  /* free list empty; allocate new space */
  msg_debug(F_POOL, "newcell: space ran out; enlarging\n");
  enlargepool();

  /* Enlargepool assures that it allocates at least one new cell space,
     and the free list must be set up */

  /* integrity check */
  if (freehead < 0)
    abortwithcore("newcell: internal error: poolsize = %ld, freehead = %ld\n", poolsize, freehead);

  i = freehead;
  /* link next */
  freehead = pool[i].nextfree;
  /* set up the cell */
  msg_debug(F_POOL, "newcell: returns a cell from new space; [index %ld]\n", i);
  Ctype(i) = type;
  count_allocated++;
  return i;
}

/*
 * enlargepool - enlarge the pool
 */
static void
enlargepool() {
  Lcell *newp;
  Cellidx oldsize, newsize;
  Cellidx i;

  oldsize = poolsize;

  if (poolsize >= CELLIDX_MAX)	/* actually == */
    fatal("enlargepool: already reached limit (%ld cells)\n", poolsize);
  
  if (poolsize >= CELLIDX_MAX / 2)
    newsize = CELLIDX_MAX;
  else
    newsize = poolsize * 2;

  msg_debug(F_POOL, "enlargepool: enlarging the pool: %ld -> %ld\n", poolsize, newsize);
  msg_debug(F_POOL, " current pool: ");
  if (deblev(L_DEBUG, F_POOL)) eprintpool();
  newp = realloc(pool, sizeof(Lcell) * newsize);
  if (newp == NULL)
    /*
     * XXX: should show some information?
     */
    fatal("enlargepool: cannot enlarge pool (%ld cells) to have %ld cells (size = %ld)\n",
	poolsize, newsize, sizeof(Lcell) * newsize);

  /* clear newly allocated area */
  /*
   * XXX: for speeding: clear only .type field
   */
  memset(newp + poolsize, 0, (newsize - poolsize) * sizeof(Lcell));

  /* set new pool pointer */
  pool = newp;
  poolsize = newsize;
  msg_debug(F_POOL, "     new pool: ");
  if (deblev(L_DEBUG, F_POOL)) eprintpool();

  /* link new cells to the free list */
  pool[oldsize].nextfree = freehead;
  for (i = oldsize + 1; i < newsize; i++) {
    pool[i].nextfree = i - 1;
  }
  freehead = newsize - 1;
}

/*
 * fprintpool - print map of current pool
 */
static void
fprintpool(FILE *fp) {
  Cellidx i;

  fprintf(fp, "*** pool (%ld slots, %ld bytes) ***\n", poolsize, poolsize*sizeof(pool[0]));
  for (i = 0; i < poolsize; i++)
    if (pool[i].type <= NOTYPE)
      fprintf(fp, "%c ", typechar[pool[i].type]);
    else
      fprintf(fp, "%c ", typechar[NOTYPE+1]);	/* holds a char for 'unknown' */
  fprintf(fp, "\n");
}

static void
printpool() {
  fprintpool(stdout);
}

static void
eprintpool() {
  fprintpool(stderr);
}

/*
 * fpoolinfo - show summary info of the pool
 */
static void
fpoolinfo(FILE *fp) {
  Cellidx i;
  Cellidx nfree = 0;

  for (i = 0; i < poolsize; i++)
    if (pool[i].type == FREE) nfree++;
  fprintf(fp, "pool info: %ld slots (%ld bytes), %ld used, %ld free: "
	      "cumulative %lu allocated, %lu freed, %lu leaked\n",
      poolsize, poolsize*sizeof(pool[0]), poolsize-nfree, nfree,
      count_allocated, count_freed, (poolsize-nfree) - (count_allocated-count_freed));
}

static void
poolinfo() {
  fpoolinfo(stdout);
}

static void
epoolinfo() {
  fpoolinfo(stderr);
}


/*
 * freecell - free a cell
 * nb. to be in harmony with free(3), freeing an unallocated cell is allowed
 */
static void
freecell(Cellidx ci) {
  msg_debug(F_POOL, "freecell: requested freeing cell #%ld of type %d\n", ci, Ctype(ci));

  /* check cell index specified */
  if (poolsize <= ci) {
    msg_warning(F_POOL, "freecell: tried to free nonexisting cell, Cellidx = %ld, poolsize = %ld (but did nothing)\n", ci, poolsize);
  }

  /* link to the free list */
  pool[ci].nextfree = freehead;
  freehead = ci;

  Ctype(ci) = FREE;
  count_freed++;
  return; 
}

/*
 * prunecell - free all cells linked from the specified cell
 */
static void
prunecell(Cellidx ci) {
  msg_debug(F_POOL, "prunecell: requested freeing cells from #%ld of type %d\n", ci, Ctype(ci));

  /* check cell index specified */
  if (poolsize <= ci) {
    msg_warning(F_POOL, "prunecell: tried to free from nonexisting cell, Cellidx = %ld, poolsize = %ld (but did nothing)\n", ci, poolsize);
  }

  switch (Ctype(ci)) {
    case FREE:
      msg_notice(F_POOL, "prunecell: free an already free cell? %ld\n", ci);
      return;
    case VAR:
      freecell(ci);
      return;
    case ABST:
      prunecell(Cbody(ci));
      freecell(ci);
      return;
    case APPL:
      prunecell(Cleft(ci));
      prunecell(Cright(ci));
      freecell(ci);
      return;
    default:
      msg_warning(F_POOL, "prunecell: strange type cell encountered; "); if (deblev(L_WARNING, F_POOL)) eprintcell(ci);
      return;
  }
}

/*
 * deepcopy - create entirely new cell structure that denotes specified lexp
 */
static Cellidx
deepcopy(Cellidx ci) {
  Cellidx newci;
  Cellidx t;

  switch (Ctype(ci)) {
    case VAR:
      newci = newcell(VAR);
      Cvar(newci) = Cvar(ci);
      return newci;
    case ABST:
      newci = newcell(ABST);
      Cbv(newci) = Cbv(ci);
      /*
       * deepcopy has side effect on pool, which is in Cbody; we need the temporary variable here
       */
      t = deepcopy(Cbody(ci));
      Cbody(newci) = t;
      return newci;
    case APPL:
      newci = newcell(APPL);
      /*
       * deepcopy has side effect on pool, which is in Cleft and Cright; we need the temporary variable here
       */
      t = deepcopy(Cleft(ci));
      Cleft(newci) = t;
      t = deepcopy(Cright(ci));
      Cright(newci) = t;
      return newci;
    default:
      abortwithcore("deepcopy: unknown cell type %s, Cellidx = %ld\n", Ctype(ci), ci);
      /* NOTREACHED */
      return ci;
  }
}

/*
 * copycell - copy a cell by bcopy
 */
static void
copycell(Cellidx src, Cellidx dst) {
  bcopy(&pool[src], &pool[dst], sizeof(pool[dst]));
}

/*
 * canonbvars - canonicalize binding variables' ids
 */

static int
canonbvarhandler(Cellidx ci, int descending) {
  static Var bvstack[MAXABSTDEPTH];
  static unsigned long int sp = 0;	/* stack pointer */
  int i;

  /* XXX: doesn't work yet; must handle free variables correctly */
  fatal("XXX: canonbvarhandler not implemented correctly (so far)\n");

  msg_curdebug(F_POOL, "canonbvarhander: ci=%ld, %s\n", ci, descending?"descending":"ascending");
  switch (Ctype(ci)) {
    case APPL:
      return 1;		/* simply continue dfs */
    case ABST:
      if (descending) {
	if (sp >= MAXABSTDEPTH) {
	  /*
	   * XXX: might be saved but we might have rewritten some vars...
	   */
	  abortwithcore("canonbvarhandler: stack overflow."
	      " stacksize (MAXABSTDEPTH) = %lu."
	      " increase MAXABSTDEPTH in lexp.h\n", MAXABSTDEPTH);
	}
	/* push bv */
	bvstack[sp++] = Cbv(ci);
	return 1;		/* go deeper */
      } else {
	/* ascending */
	sp--;
	Cbv(ci) = sp + 1;	/* canonicalized on 1-origin */
	return 1;
      }
    case VAR:	/* always bottom so called once; no need to check descending */
      /* search the frame */
      for (i = sp - 1; i >= 0; i--)
	if (Cvar(ci) == bvstack[i])
	  break;
      if (i < 0) {
	/* not found = free variable.  leave it as it is */
      } else {
	/* bound var */
	Cvar(ci) = i + 1;	/* canonicalize on 1-origin */
      }
      return 1;
    default:
      abortwithcore("canonbvarhandler: specified non-Lexp or incomplete Lexp; met type %d\n", Ctype(ci));
      /*NOTREACHED*/
      return 0;
  }
}

static void
canonbvars(Lexp l) {
  /* XXX */
  fatal("canonbvars not implemented correctly\n");

  dfsLexp(l, canonbvarhandler);
  return;
}

static int
isequalLexp_r(Cellidx c1, VarList *bv1, Cellidx c2, VarList *bv2) {
  VarList *p1, *p2;
  Var v1, v2;
  int res;

  if (Ctype(c1) != Ctype(c2))
    return 0;
  if (Ctype(c1) == APPL)
    return isequalLexp_r(Cleft(c1), bv1, Cleft(c2), bv2) &&
           isequalLexp_r(Cright(c1), bv1, Cright(c2), bv2);
  if (Ctype(c1) == ABST) {
    p1 = emalloc(sizeof(VarList));
    p1->v = Cbv(c1);
    p1->next = bv1;
    p2 = emalloc(sizeof(VarList));
    p2->v = Cbv(c2);
    p2->next = bv2;
    res = isequalLexp_r(Cbody(c1), p1, Cbody(c2), p2);
    free(p1);
    free(p2);
    return res;
  }
  if (Ctype(c1) != VAR) {
    msg_warning(F_POOL, "isequalLexp_r: comparing bad type (%d)\n", Ctype(c1));
    return 0;	/* XXX: should abort? */
  }
  /* comparing variables */
  v1 = Cvar(c1);
  v2 = Cvar(c2);
  /* are those bound variables? */
  for (p1 = bv1, p2 = bv2; p1 != NULL && p2 != NULL; p1 = p1->next, p2 = p2->next) {
    if (p1->v == v1) {
      if (p2->v == v2) {
	/* same bound variable */
	return 1;
      } else {
	/* v1 is bound but v2 is not the same bound var */
	return 0;
      }
    } else if (p2->v == v2) {
      /* v2 is bound here but v1 not */
      return 0;
    } else {
      /* both not bound; continue */
      ;
    }
  }
  /* both are free variables */
  if (v1 == v2)
    return 1;
  else
    return 0;
}

int
isequalLexp(Lexp l1, Lexp l2) {	
  return isequalLexp_r(l1, NULL, l2, NULL);
}

/*
 * dfsLexp - generic dfs routine for lexp
 *
 * does depth-first search on specified Lexp and call func on the node of Cellidx
 * second parameter for func: 1 = descending, 0 = ascending.
 *
 * go deeper when func returns 1, terminates (and returns) when func returns 0.
 */
static void
dfsLexp_rec(Cellidx ci, int (*func)(Cellidx, int)) {
  int g;	/* go deeper? */

  switch (Ctype(ci)) {
    case VAR:
      (void)((*func)(ci, 1));
      return;
    case ABST:
      g = (*func)(ci, 1);
      if (g) {
	dfsLexp_rec(Cbody(ci), func);
	(void)((*func)(ci, 0));
      }
      return;
    case APPL:
      g = (*func)(ci, 1);
      if (g) {
	dfsLexp_rec(Cleft(ci), func);
	dfsLexp_rec(Cright(ci), func);
	(void)((*func)(ci, 0));
      }
      return;
    default:
      abortwithcore("dfsLexp_rec: specified non-Lexp or incomplete Lexp: type %d\n", Ctype(ci));
      /*NOTREACHED*/
      return;
  }
}

static void
dfsLexp(Lexp l, int (*func)(Cellidx, int)) {
  Cellidx rootci = l;

  dfsLexp_rec(rootci, func);
}

/*
 * lambda expression operations (interface) (was in lambops.c)
 */

/*
 * alpha - substitution M[x:=N]
 *
 * substitute all occurrences of M's free variable x with N
 */
static Lexp
alpha(Lexp m, Var x, Lexp n) {
  Cellidx ci;

  ci = m;	/* top cell */
  switch (Ctype(ci)) {
    case VAR: {
      if (Cvar(ci) == x) {
	freecell(ci);
	return deepcopy(n);
      } else
	return ci;
    }
    case APPL: {
      Cellidx cleft, cright;

      cleft = alpha(Cleft(ci), x, n);
      cright = alpha(Cright(ci), x, n);
      Cleft(ci) = cleft;
      Cright(ci) = cright;
      return ci;
    }
    case ABST: {
      Cellidx body;

      if (Cbv(ci) == x)
	return ci;
      else {
	body = alpha(Cbody(ci), x, n);
	Cbody(ci) = body;
	return ci;
      }
    }
    default:
      msg_warning(F_LAMBOPS, "alpha: unknown cell type %s\n", typename[Ctype(ci)]);
      return ci;
  }
}

/* findmaxvar - finds max var in Lexp */
static Var
findmaxvar(Lexp l) {
  switch (Ctype(l)) {
    case VAR:
      return Cvar(l);
    case APPL: {
      Var maxleft, maxright;

      maxleft = findmaxvar(Cleft(l));
      maxright = findmaxvar(Cright(l));
      return max(maxleft, maxright);
    }
    case ABST: {
      Var maxbody;

      maxbody = findmaxvar(Cbody(l));
      return max(Cbv(l), maxbody);
    }
    default:
      fatal("findmaxvar: unknown cell type %d\n", Ctype(l));
      /*NOTREACHED*/
      return -1;	/* dummy for cc -Wall */
  }
}

/*
 * subst - substitute x in m with n, avoiding binding free variables
 *
 * maxvar is maximum Var used, i.e., (maxvar + 1) is a new variable
 *
 * call with maxvar = -1 at the top level
 */
static Lexp
subst(Lexp m, Var x, Lexp n, Var maxvar) {
  Lexp lret;
  Var maxinm, maxinn;

  if (maxvar < 0) {
    /* toplevel */
    maxinm = findmaxvar(m);
    maxinn = findmaxvar(n);
    maxvar = max(max(maxinm, maxinn), x);
  }

  switch (Ctype(m)) {
    case VAR: {
	if (Cvar(m) == x) {
	  freecell(m);
	  lret = deepcopy(n);
	} else
	  lret = m;
      }
      break;
    case APPL: {
	Cellidx cleft, cright;

	cleft = subst(Cleft(m), x, n, maxvar);
	cright = subst(Cright(m), x, n, maxvar);
	Cleft(m) = cleft;
	Cright(m) = cright;
	lret = m;
      }
      break;
    case ABST: {
	Var bv, newbv;
	Cellidx body, newbody, tmpv;

	/* create new binding variable */
	bv = Cbv(m);
	maxvar = max(maxvar, bv);
	newbv = maxvar + 1;
	maxvar = newbv;

	/* replace old bv with newbv to avoid collision */
	tmpv = newcell(VAR);
	Cvar(tmpv) = newbv;

	body = alpha(Cbody(m), bv, tmpv);
	freecell(tmpv);

	/* go further */
	newbody = subst(body, x, n, maxvar);
	Cbv(m) = newbv;
	Cbody(m) = newbody;
	lret = m;
      }
      break;
    default:
      msg_warning(F_LAMBOPS, "subst: unknown cell type %s\n", typename[Ctype(m)]);
      lret = m;
      break;
  }
  return lret;
}

/*
 * betaat - beta reduction at specified redex
 * Returns 1 if reduced, 0 if not.
 */
static int
betaat(Cellidx redex) {
  Cellidx left, right, newci;
  Var bv;

  /* reducible? */
  if (Ctype(redex) != APPL) {
    return 0;
  }
  left = Cleft(redex);
  right = Cright(redex);
  if (Ctype(left) != ABST) {
    return 0;
  }

  /* reduction */
  bv = Cbv(left);
  newci = subst(Cbody(left), bv, right, -1);

  /*
   * Link the result to parent, and free cells which are no longer necessary.
   *
   * XXX: Because we do not know which is the parent,
   *      we overwrite redex with the result.
   */
  copycell(newci, redex);
  freecell(newci);
  freecell(left);
  prunecell(right);
  
  return 1;
}

/*
 * canon_findredex - find a redex for canonical reduction
 *
 * returns Cellidx when found, -1 if not found
 */
static Cellidx
canon_findredex(Cellidx c) {
  assert(Ctype(c)==VAR||Ctype(c)==ABST||Ctype(c)==APPL);

  switch (Ctype(c)) {
    case VAR:
      return -1;
    case ABST:
      return canon_findredex(Cbody(c));
    case APPL: {
	Cellidx left, right, redex;

	left = Cleft(c);
	right = Cright(c);

	if (Ctype(left) == ABST) {
	  /* found */
	  return c;
	}

	/* search in leftmost manner (and outermost = topdown recursive) */
	if ((redex = canon_findredex(left)) >= 0) {
	  /* found in left */
	  return redex;
	}
	/* not found; try right */
	return canon_findredex(right);
      }
    default:
      /* assert prevents control coming here */
      /*NOTREACHED*/
      return -1;	/* dummy for cc -Wall */
  }
}

/*
 * findredex - find a redex according to the specified strategy
 *
 * returns Cellidx when found, -1 if not found (i.e., l is already canonical)
 */
static Cellidx
findredex(Lexp l, int strategy) {
  static int warned = 0;	/* to warn once */

  switch (strategy) {
    case INNERMOST:
      if (!warned) {
	msg_warning(F_LAMBOPS, "findredex: strategy INNERMOST not yet implemented; using CANONICAL hereafter\n");
	warned = 1;
      }
      /* FALLTHROUGH */
    case CANONICAL:
      return canon_findredex(l);
    default:
      fatal("findredex: unknown strategy %d\n", strategy);
      /*NOTREACHED*/
      return -1;	/* dummy for cc -Wall */
  }
}

/*
 * betastep - one step beta reduction
 *
 * reduce Lexp l with strategy, and overwrite l with the result.
 * return value: 1 = reduced (but possibly same as l), 0 = no redex
 */
static int
betastep(Lexp l, int strategy) {
  Cellidx redex;
  int reduced;

  if (deblev(L_DEBUG, F_LAMBOPS)) {
    msg_debug(F_LAMBOPS, "betastep: on ");
    eprintlexp(l);
  }
  if ((redex = findredex(l, strategy)) < 0) {
    /* no redex */
    return 0;
  }
  reduced = betaat(redex);

  assert(reduced);	/* must be reduced because findredex must have found a redex */

  /* now 'redex' points reduced expression */
  /* l points 'redex' so nothing has to be done to link them */
  return 1;
}

/*
 * nbeta - do beta reductions
 *
 * times specifies max #times of beta reductions, 0 = as many times as possible.
 * returns #times done.
 *
 * added for lilgp: max #cells can now be specified.  0 = no limit
 */
static int
nbeta(Lexp l, int strategy, int times, int maxcells) {
  int i;

  if (times > 0) {
    for (i = 0; i < times; i++) {
      if (maxcells > 0 && countcells(l) > maxcells)
	break;
      if (!betastep(l, strategy))
	break;
    }
  } else {
    for (i = 0; ; i++) {
      if (maxcells > 0 && countcells(l) > maxcells)
	break;
      if (!betastep(l, strategy))
	break;
    }
  }
  if (deblev(L_DEBUG, F_LAMBOPS)) {
    msg_debug(F_LAMBOPS, "nbeta: %d steps, returns ", i);
    eprintlexp(l);
  }
  return i;
}

/*
 * countcells - count #cells that the lexp possesses
 */

static int ncells;

static int
countcells_handler(Cellidx ci, int descending) {
  if (!descending)
    return 1;	/* ignore; carry on */
  ncells++;
  return 1;	/* go deeper */
}

static int
countcells(Lexp l) {
  ncells = 0;
  dfsLexp(l, countcells_handler);
  return ncells;
}

/*
 * LL(1) recursive descent parser for my lambda expressions (was in parser.c)
 */

/*
 * error message.  sets error flag for convenience.
 */
static void
syntax_error(char *expected) {
  msg_warning(F_PARSER, "parser.c: syntax error: next %s (value %d), expected %s, tokdata %ld, followed by \"%s\"\n",
      (0<=parser_next&&parser_next<=LP_NUMTOK)?tokenname[parser_next]:"---",
      parser_next, expected, parser_tokdata, parser_cur);
  parser_error = 1;
}

/*
 * create lambda expression list from lexp buf
 */
static Cellidx
create_lexp(char *buf) {
  Cellidx c;

  parser_error = 0;
  msg_debug(F_PARSER, "create_lexp: called \"%s\"\n", buf);
  parser_cur = buf;
  c = do_lexp();
  getnext();
  if (parser_error) {
    msg_warning(F_PARSER, "create_lexp: failed to create list for bad lexp \"%s\"\n", buf);
    return -1;
  }
  if (parser_next != LP_NULL) {
    syntax_error("end of line");
    return -1;
  }
  return c;
}

/*
 * peek next token
 */
static enum token
peeknext() {
  char *p;

  for (p = parser_cur; isspace(*p); p++) ;
  switch (*p) {
    case '\0':
      return LP_NULL;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return LP_VAR;
    case 'L':
      return LP_LAMBDA;
    case '(':
      return LP_LPAREN;
    case ')':
      return LP_RPAREN;
    case '.':
      return LP_DOT;
    default:
      msg_warning(F_PARSER, "peeknext: unknown token met \"%s\"\n", p);
      return LP_ERROR;
  }
  /* NOTREACHED */
}

/*
 * get next long int string
 */
static long int
getlong() {
  long int val = 0L;

  assert('0'+1=='1'&&'1'+1=='2'&&'2'+1=='3'&&'3'+1=='4'&&
      '4'+1=='5'&&'5'+1=='6'&&'6'+1=='7'&&'7'+1=='8'&&'8'+1=='9');
  /* or use strtol(3) for each digit, but take efficiency */

  while (isdigit(*parser_cur)) {
    if (val >= LONG_MAX / 10) {
      msg_warning(F_PARSER, "getlong: value would exceed the limit (%ld -> ...)", val);
    }
    val *= 10;
    val += *parser_cur - '0';
    parser_cur++;
  }
  return val;
}

/*
 * get next token to next and tokdata
 */
static void
getnext() {
  parser_tokdata = -1;	/* for sanity */
  for (; isspace(*parser_cur); parser_cur++) ;
  switch (*parser_cur) {
    case '\0':
      parser_next = LP_NULL;
      break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      parser_tokdata = getlong();
      parser_next = LP_VAR;
      break;
    case 'L':
      parser_cur++;
      parser_next = LP_LAMBDA;
      break;
    case '(':
      parser_cur++;
      parser_next = LP_LPAREN;
      break;
    case ')':
      parser_cur++;
      parser_next = LP_RPAREN;
      break;
    case '.':
      parser_cur++;
      parser_next = LP_DOT;
      break;
    default:
      msg_warning(F_PARSER, "getnext: unknown token met \"%s\"\n", parser_cur);
      parser_next = LP_ERROR;
      parser_error = 1;
      break;
  }
  msg_debug(F_PARSER, "getnext: got %s, tokdata %ld\n", tokenname[parser_next], parser_tokdata);
}

static Cellidx
do_lexp() {

  msg_debug(F_PARSER, "do_lexp invoked\n");
  
  if (parser_error) {
    msg_warning(F_PARSER, "do_lexp: error has occurred; bailing out\n");
    return -1;
  }

  /* get next token */
  getnext();
  switch (parser_next) {
    case LP_ERROR:
      syntax_error("nothing (fatal)");
      return -1;
    case LP_NULL:
      msg_warning(F_PARSER, "do_lexp: nothing to parse\n");
      parser_error = 1;
      return -1;
    case LP_VAR: {
      Var var;
      Cellidx c;

      var = parser_tokdata;
      c = newcell(VAR);
      Cvar(c) = var;
      msg_debug(F_PARSER, "allocated var %ld\n", var);
      return c;
    }
    case LP_LPAREN: {	/* ABST or APPL */
      Cellidx c;
      enum token peek;

      peek = peeknext();
      switch (peek) {
	case LP_LAMBDA:	/* ABST */
	  c = do_abst();
	  return c;
	case LP_VAR:
	case LP_LPAREN:	/* APPL */
	  c = do_appl();
	  return c;
	default:
	  getnext();	/* set next for error message */
	  syntax_error("L, var or lparen");
	  return -1;
      }
    }
    default:
      syntax_error("start of lexp");
      return -1;
  }
  /* NOTREACHED */
}

static Cellidx
do_abst() {
  Var bv;
  Cellidx c, body;

  msg_debug(F_PARSER, "do_abst invoked\n");

  getnext();	/* next token must be L */
  if (parser_next != LP_LAMBDA) {
    syntax_error("L");
    return -1;
  }

  getnext();	/* must be variable */
  if (parser_next != LP_VAR) {
    syntax_error("variable");
    return -1;
  }
  bv = parser_tokdata;

  getnext();	/* must be a dot */
  if (parser_next != LP_DOT) {
    syntax_error("dot");
    return -1;
  }
  body = do_lexp();
  
  c = newcell(ABST);
  Cbv(c) = bv;
  Cbody(c) = body;

  getnext();	/* skip rparen */
  if (parser_next != LP_RPAREN) {
    syntax_error("rparen");
    return -1;
  }
  return c;
}

static Cellidx
do_appl() {
  Cellidx c, left, right;

  msg_debug(F_PARSER, "do_appl invoked\n");

  left = do_lexp();
  right = do_lexp();

  c = newcell(APPL);
  Cleft(c) = left;
  Cright(c) = right;

  getnext();	/* skip rparen */
  if (parser_next != LP_RPAREN) {
    syntax_error("rparen");
    return -1;
  }
  return c;
}

/*
 * string and internal data conversion routines (was in strlexp.c)
 */

/*
 * just to print
 *
 * XXX: might be better to use lexp2str, but don't want to be bothered with buffer
 */
static void
fprintlexp_n(FILE *fp, Lexp ci) {
  switch (Ctype(ci)) {
    case FREE:
      fprintf(fp, "-");
      break;
    case VAR:
      fprintf(fp, "%ld", Cvar(ci));
      break;
    case ABST:
      fprintf(fp, "(L %ld.", Cbv(ci));
      fprintlexp_n(fp, Cbody(ci));
      fprintf(fp, ")");
      break;
    case APPL:
      fprintf(fp, "(");
      fprintlexp_n(fp, Cleft(ci));
      fprintf(fp, " ");
      fprintlexp_n(fp, Cright(ci));
      fprintf(fp, ")");
      break;
    default:
      fprintf(fp, "X");
      break;
  }
  return;
}

static void
fprintlexp(FILE *fp, Lexp ci) {
  fprintlexp_n(fp, ci);
  fputc('\n', fp);
}

static void
printlexp(Lexp ci) {
  fprintlexp(stdout, ci);
}

static void
eprintlexp(Lexp ci) {
  fprintlexp(stderr, ci);
}

/*
 * proceedwith - proceed in buf with str.  always null terminate buf.
 *
 * return value: > 0 proceed, -1 overrun
 */
static int
proceedwith(char *buf, char *str, int bytesleft) {
  int proceed, n, overrun = 0;

  msg_debug(F_STRLEXP, "proceedwith: \"%s\", left %d bytes\n", str, bytesleft);

  assert(bytesleft > 0);

  n = strlen(str);
  if (n == 0)
    msg_warning(F_STRLEXP, "proceedwith: called with null string; bad logic in lexp2str?\n");

  /* if strncpy(3) works with n=0, the following if statement is not necessary */
  if (bytesleft == 1) {
    *buf = '\0';
    if (n > 0)
      return -1;	/* overrun */
    else
      return 1;		/* proceeded normally */
  }

  if (n > bytesleft - 1) {
    overrun = 1;
    n = bytesleft - 1;
  }
  strncpy(buf, str, n);
  proceed = n;

  buf[n] = '\0';

  msg_debug(F_STRLEXP, "proceedwith: returning with buf \"%s\", proceed %d bytes\n", buf, proceed);

  if (overrun)
    return -1;
  else
    return proceed;
}

/*
 * lexp2str - convert Lexp to string
 *
 * return value: >= 0 result length, -1 = failure
 */

static int
lexp2str(Cellidx ci, char *buf, int len) {
  int proceed, n;
  static char tmp[MISCSTRBUFLEN];
  int shouldbe;
  
  msg_debug(F_STRLEXP, "lexp2str: called for type %s with len = %d\n", typename[Ctype(ci)], len);
  assert(len > 0);

  switch (Ctype(ci)) {
    case FREE:
      proceed = proceedwith(buf, "-", len);
      return proceed;
    case VAR:
      shouldbe = snprintf(tmp, sizeof(tmp), "%ld", Cvar(ci));
      if (shouldbe > strlen(tmp))
	fatal("lexp2str: too short buffer len %d to get var\n", sizeof(tmp));
      proceed = proceedwith(buf, tmp, len);
      return proceed;
    case ABST:
      proceed = 0;

      shouldbe = snprintf(tmp, sizeof(tmp), "(L %ld.", Cbv(ci));
      if (shouldbe > strlen(tmp))
	fatal("lexp2str: too short buffer len %d to get bv\n", sizeof(tmp));
      n = proceedwith(buf+proceed, tmp, len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = lexp2str(Cbody(ci), buf+proceed, len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = proceedwith(buf+proceed, ")", len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      return proceed;
    case APPL:
      proceed = 0;
      
      n = proceedwith(buf+proceed, "(", len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = lexp2str(Cleft(ci), buf+proceed, len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = proceedwith(buf+proceed, " ", len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = lexp2str(Cright(ci), buf+proceed, len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      n = proceedwith(buf+proceed, ")", len);
      if (n < 0)
	return -1;
      proceed += n;
      len -= n;

      return proceed;
    default:
      proceed = proceedwith(buf, "X", len);
      return proceed;
  }
}

/*
 * parse lambda expression string, make internal data and return the cell index
 */
static Lexp
str2lexp(char *str) {
  return create_lexp(str);
}

/*
 * message printing routines (was in message.c)
 */

static void
fatal(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(FATAL);
}

static void
abortwithcore(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  abort();
}

/*
 * message printing
 *
 * (each facility might have macros for convenience)
 */
static void
msg_generic(FILE *fp, enum facility f, enum level l, char *fmt, ...) {
  va_list ap;

  if (dlevel[f] >= l) {
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
  }
}

/*
 * calculate differences (was in diff.c)
 */

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
static void
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
	msg_warning(F_MISC, "calcbdist: MAXABSTDEPTH reached; ignoring the subtree\n");
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

static void
calcbdist(Lexp l) {
  calcbdist_r(l, 0);
}

/*
 * diff - returns difference between two lexps
 */

static int
diff_r(Cellidx c1, Cellidx c2) {
  int i;
  int w1, w2, dif;
  int a1[MAXTREEHEIGHT], a2[MAXTREEHEIGHT];
  int lev1, lev2, bot;

  if (Ctype(c1) == VAR && Ctype(c2) == VAR) {
    w1 = Cbdist(c1);
    w2 = Cbdist(c2);
    if (w1 > 0 && w2 > 0) {
      /* both bound; return difference */
      return DIST(w1, w2);
    } else if (w1 > 0) {
      /* c1 is bound, c2 is free */
      return w1 - w2;
    } else if (w2 > 0) {
      /* c2 is bound, c1 is free */
      return w2 - w1;
    } else {
      /* both free */
      return DIST(Cvar(c1), Cvar(c2));
    }
  } else if (Ctype(c1) == VAR && Ctype(c2) == ABST) {
    return numnodes(c2);
  } else if (Ctype(c1) == ABST && Ctype(c2) == VAR) {
    return numnodes(c1);
  } else if (Ctype(c1) == VAR && Ctype(c2) == APPL) {
    return numnodes(c2);
  } else if (Ctype(c1) == APPL && Ctype(c2) == VAR) {
    return numnodes(c1);
  } else if (Ctype(c1) == ABST && Ctype(c2) == ABST) {
    return diff_r(Cbody(c1), Cbody(c2));
  } else if ((Ctype(c1) == ABST && Ctype(c2) == APPL) ||
             (Ctype(c1) == APPL && Ctype(c2) == ABST)) {
    lev1 = arraynodes(c1, a1, MAXTREEHEIGHT);
    lev2 = arraynodes(c2, a2, MAXTREEHEIGHT);
    bot = max(lev1, lev2);
    dif = 0;
    for (i = 0; i < bot; i++)
      dif += 2 + DIST(a1[i], a2[i]);
    return dif;
  } else if (Ctype(c1) == APPL && Ctype(c2) == APPL) {
    return diff_r(Cleft(c1), Cleft(c2)) + diff_r(Cright(c1), Cright(c2));
  } else {
    fatal("diff_r: unexpected cell type %d and %d\n", Ctype(c1), Ctype(c2));
  }
  fatal("diff_r: cannot come here\n");
  /*NOTREACHED*/
  return 0;
}

static int
diff(Lexp l1, Lexp l2) {
  int d;

  char s1[4096], s2[4096];
  calcbdist(l1);
  calcbdist(l2);
  lexp2str(l1, s1, sizeof(s1));
  lexp2str(l2, s2, sizeof(s2));
  d = diff_r(l1, l2);
  /* printf("|%s - %s| = %d\n", s1, s2, d); */
  return d;
}

/*
 * arraynodes - make array of the number of nodes at each level.
 *              returns deepest level reached.
 */

static int
arraynodes_r(Cellidx ci, int curlev, int a[], int asize) {
  int lev1, lev2;

  if (curlev >= asize) {
    msg_warning(F_MISC, "arraynodes_r: array size (%d) reached; ignoring the subtree\n", asize);
    return asize;
  }
  a[curlev]++;
  curlev++;
  switch (Ctype(ci)) {
    case VAR:
      return curlev;
    case ABST:
      return arraynodes_r(Cbody(ci), curlev, a, asize);
    case APPL:
      lev1 = arraynodes_r(Cleft(ci), curlev, a, asize);
      lev2 = arraynodes_r(Cright(ci), curlev, a, asize);
      return max(lev1, lev2);
    default:
      fatal("arraynodes_r: unexpected cell type %d\n", Ctype(ci));
  }
  /*NOTREACHED*/
  return curlev;
}

static int
arraynodes(Cellidx ci, int a[], int asize) {
  int i;

  for (i = 0; i < asize; i++)
    a[i] = 0;

  return arraynodes_r(ci, 0, a, asize);
}

/* [EOF] */
