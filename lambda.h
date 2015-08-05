/*
 * lambda.h - user header file
 *
 * Copyright (c) 2002 Kazuto Tominaga.  All rights reserved.
 *
 */

#ifndef __LAMBDA_H
#define __LAMBDA_H

/* types */
typedef long int Var;		/* variable */
typedef long int Lexp;		/* lambda expression */
typedef long int Cellidx;	/* is Lexp; necessary for dfs */

/* constants */
enum {
  /* for Ltype */
  FREE = 0,
  VAR = 1,
  ABST = 2,
  APPL = 3,
  NOTYPE = 4,
  /* beta reduction strategies */
  CANONICAL = 1,
  INNERMOST = 2,
};

/* interface */
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

/* lambint.c - interface with lilgp */
Lexp Icreatebvar(Var);

/* church.c - handling church numerals */
Lexp Cchurch_num(int);
int Ccount_app(Lexp);

#endif /* __LAMBDA_H */

/* [EOF] */
