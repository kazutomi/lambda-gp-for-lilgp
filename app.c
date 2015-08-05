/*  lil-gp Genetic Programming System, version 1.0, 11 July 1995
 *  Copyright (C) 1995  Michigan State University
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *  
 *  Douglas Zongker       (zongker@isl.cps.msu.edu)
 *  Dr. Bill Punch        (punch@isl.cps.msu.edu)
 *
 *  Computer Science Department
 *  A-714 Wells Hall
 *  Michigan State University
 *  East Lansing, Michigan  48824
 *  USA
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <lilgp.h>

#include <cpgplot.h>

#include "lambda.h"

#define NELEMS(a)	(sizeof(a)/(sizeof((a)[0])))

/* declare the (globaldata) struct if you use it.  the name "g" is suggested
 * for this instance.
 */

globaldata g;

/* comparison function for qsort */
int orderofsize(struct iinfo *x, struct iinfo *y) {
     return x->ncells - y->ncells;
}

/* app_build_function_sets()
 *
 * this function should build data structures describing the function
 * set(s) and the trees of the individual.  it should then pass these
 * structures to function_sets_init(), destroy its data structures,
 * and return the (integer) return value from function_sets_init().
 *
 * see the manual for more information on constructing this function.
 */

int app_build_function_sets ( void )
{
     function_set *fset;
     int *tree_map;
     char **tree_name;
     int ret;
     function sets[3] = {
       { f_abstraction, NULL, NULL, 1, "L", FUNC_EXPR, -1, 0 },
       { f_application, NULL, NULL, 2, "P", FUNC_DATA, -1, 0 },
       { NULL, f_variable_gen, f_variable_print, 0, "V", TERM_ERC, -1, 0 },
     };

     int num_function_sets = 1;
     int num_trees = 1;

     fset = (function_set *)MALLOC ( num_function_sets *
                                    sizeof ( function_set ) );
     fset[0].size = 3;		/* # of functions and terminals */
     fset[0].cset = sets;	/* definition of the function set */

     tree_map = (int *)MALLOC ( num_trees * sizeof ( int ) );
     /* specify, for each tree, what function set to use. */
     tree_map[0] = 0;

     tree_name = (char **)MALLOC ( num_trees * sizeof ( char * ) );
     /* give each tree a name. */
     tree_name[0] = "TREE";

     ret = function_sets_init ( fset, num_function_sets, tree_map,
                               tree_name, num_trees );

     FREE ( fset );
     FREE ( tree_map );
     FREE ( tree_name );

     return ret;
}

/* app_eval_fitness()
 *
 * this function should evaluate the fitness of the individual.  typically
 * this function will loop over all the fitness cases.  the following
 * fields in the (individual *) should be filled in:
 *    r_fitness    (raw fitness)
 *    s_fitness    (standardized fitness)
 *    a_fitness    (adjusted fitness)
 *    hits         (hits)
 *    evald        (always set to EVAL_CACHE_VALID)
 */

void app_eval_fitness ( individual *ind )
{
     int testcases[] = { 10, 20, 50, 100, 200 };
     int maxstep = 5000;	/* max #step of beta reduction; XXX: should be defined as constant */
     int maxcells = 5000;	/* max #cells during beta reductions; ditto */
     int i;
     DATATYPE indiv0;
     char s[65536];
     Lexp indiv, sample, correct, applied, sample0;
     int steps, beta_finished;
     int ncells;
     int dist, dist0;

     set_current_individual ( ind );
     
     ind->r_fitness = 0.0;
     ind->hits = 0;

     /*
      * convert GP internal expression to lambda-lib expression
      */
     g.blevel = 0;

     if (g.debug) {
	  printf("tree: ");
	  print_tree(ind->tr[0].data, stdout);
     }

     indiv0 = evaluate_tree ( ind->tr[0].data, 0 );
     if (indiv0 < 0) {
	  /* singleton tree */
	  indiv0 = Icreatebvar(indiv0);
     }

     ncells = Lcountcells(indiv0);
     g.idata[g.npop].ncells = ncells;

     LLexp2str(indiv0, s, sizeof(s));
     if (g.debug)
	  printf("indiv0: %s\n", s);

     /*
      * loop over all the fitness cases.
      */
     beta_finished = 1;
     for ( i = 0 ; i < NELEMS(testcases) ; i++ )
     {
	  indiv = Lcopy(indiv0);		/* to preserve original */

	  LLexp2str(indiv, s, sizeof(s));
	  if (g.debug)
	       printf("indiv: %s\n", s);

	  sample = Cchurch_num(testcases[i]);
	  sample0 = Lcopy(sample);			/* sample for avoiding identity function */
	  correct = Cchurch_num(testcases[i]*2);	/* try to find *2 function */

	  applied = Lappl(indiv, sample);

	  LLexp2str(applied, s, sizeof(s));
	  if (g.debug)
	       printf("applied: %s\n", s);

	  steps = Lbeta(applied, CANONICAL, maxstep, maxcells);

	  /* beta reduction may not finish within maxstep, */
	  /* but let us regard the result as the answer */

	  LLexp2str(applied, s, sizeof(s));
	  if (g.debug)
	       printf("applied rewritten to: %s\n", s);

	  /* applied overwritten with the result */

	  if (steps == maxstep) {
	       beta_finished = 0;
	       if (g.debug)
		    printf("maxstep reached\n");
	  }
	  dist0 = Ldiff(applied, sample0);
	  if (dist0 == 0)			/* got identity function */
	       dist = IPENALTY;
	  else
	       dist = Ldiff(applied, correct);
	  if (g.debug)
	       printf("case %d, r_fitness += %d\n", i, dist);

	  ind->r_fitness += (double)dist;

	  Lfree(applied);
	  Lfree(correct);
	  Lfree(sample0);

          /* here you would score the value returned by the individual
           * and update the raw fitness and/or hits. */
        
     }
     g.idata[g.npop].reduction_finished = beta_finished;

     Lfree(indiv0);

     g.idata[g.npop].fitness = ind->r_fitness;

     /* compute the standardized and raw fitness. */

     ind->r_fitness /= NELEMS(testcases);
     ind->s_fitness = ind->r_fitness;
     ind->a_fitness = 1/(1+ind->s_fitness);

     if (g.debug)
       printf("raw %lf, std %lf, adj %lf\n", ind->r_fitness, ind->s_fitness, ind->a_fitness);

     /* always leave this line in. */
     /* 
      * XXX: tominaga
      *
      * This was EVAL_CACHE__VALID, but it omits
      * duplicate evaluation for same individuals.
      * We need to collect array g.napp[]
      * so turned of the cache.
      */
     ind->evald = EVAL_CACHE_INVALID;

     g.npop++;

     if (g.debug)
	  Lepoolinfo();
}

/* app_end_of_evaluation()
 *
 * this is called every generation after the entire popualation has
 * been evaluated.  "mpop" is a pointer to basically everything --
 * every individual in every subpopulation, exchange topology for
 * multipop runs, etc.  see the documentation for useful things to
 * find inside it.
 *
 * run_stats[0].best[0]->ind is always a pointer to the best-of-run
 * individual.  if the "newbest" flag is 1, it indicates that this
 * is a NEW best-of-run individual.
 *
 * similarly, gen_stats[0].best[0]->ind points to the best-of-generation
 * individual.
 *
 * DO NOT MODIFY ANY OF THE STRUCTURES YOU ARE PASSED POINTERS TO.
 *
 * should return 1 to indicate that the termination criterion has been
 * met and the run should terminate, 0 otherwise.  you do not need to
 * test the generation number against the maximum -- this is done by
 * the kernel.
 *
 * you can dynamically modify breeding parameters and/or subpop exchange
 * topology by making the appropriate changes to the parameter database.
 * see the manual for more information.  after making changes, you need
 * to call:
 *    rebuild_breeding_table ( mpop );
 * or
 *    rebuild_exchange_topology ( mpop );
 * for your changes to take effect.
 */
 
int app_end_of_evaluation ( int gen, multipop *mpop, int newbest,
                           popstats *gen_stats, popstats *run_stats )
{
     int i, sizerank, fitnessrank, colorindex;
     individual temp;
     DATATYPE indiv;
     char buf[4096];
     int maxstep, maxcell, steps;
     int bestrawfit;

     /*
      * visualize
      */

     /* sort */
     qsort(g.idata, g.npop, sizeof(g.idata[0]), orderofsize);

     oprintf(OUT_SYS, 50, "npop=%d\n", g.npop);
     for (i = 0; i < g.npop; i++) {
	  oprintf(OUT_SYS, 50, "[%d]%c ncells=%d, fitness=%f. ",
	    i, (g.idata[i].reduction_finished?' ':'!'), g.idata[i].ncells, g.idata[i].fitness);

	  /* 1 <= sizerank <= 9 */
	  sizerank = log10((double)g.idata[i].ncells+10.0) / g.ncelldenom * 9 + 1;
	  if (sizerank > 9)
	       sizerank = 9;

	  /* 0 <= fitnessrank <= 9; the larger the fitter */
	  fitnessrank = 9 - (int)(log10((double)g.idata[i].fitness+10.0) / g.distdenom * 10.0);
	  if (fitnessrank < 0)
	       fitnessrank = 0;

	  if (g.idata[i].reduction_finished) {
	       colorindex = sizerank * 10 + fitnessrank;
	  } else {
	       colorindex = 0;
	  }
	  oprintf(OUT_SYS, 50, "color %d\n", colorindex);

	  cpgsci(colorindex);
	  cpgrect((float)g.gen, (float)(g.gen+1), (float)i, (float)(i+1));
     }
     putchar('\n');

     g.npop = 0;

     g.gen++;

     /*
      * print best individual in readable form
      */

     temp.tr = run_stats[0].best[0]->ind->tr;
     set_current_individual (&temp);

     indiv = evaluate_tree ( temp.tr[0].data, 0 );
     if (indiv < 0) {
	  /* singleton tree */
	  indiv = Icreatebvar(indiv);
     }
     maxstep = 1000;
     maxcell = 1000;
     steps = Lbeta(indiv, CANONICAL, maxstep, maxcell);
     LLexp2str(indiv, buf, sizeof(buf));

     oprintf ( OUT_HIS, 50, "%s\n", buf );
     Lfree(indiv);

     /*
      * early termination
      */
     bestrawfit = run_stats[0].best[0]->ind->r_fitness;
     if (bestrawfit == 0)
	  return 1;	/* perfect individual found. finish! */
     else
	  return 0;	/* go on to the next generation */
}

/* app_end_of_breeding()
 *
 * this is called every generation after the next population is created
 * (but before it is evaluated).
 *
 * DO NOT MODIFY ANY OF THE STRUCTURES YOU ARE PASSED POINTERS TO.
 */

void app_end_of_breeding ( int gen, multipop *mpop )
{
     return;
}

/* app_create_output_streams()
 *
 * if you are going to create any custom output streams, do it here.
 * this is not documented yet, look at the source for the symbolic
 * regression problem for a guide.
 *
 * return 0 if init went OK and the run can proceed, or 1 to abort.
 */

int app_create_output_streams()
{
     return 0;
}
 

/* app_initialize()
 *
 * this should perform any application-specific initialization the user
 * needs to do.
 *
 * return 0 if init went OK and the run can proceed, or 1 to abort.
 */

int app_initialize ( int startfromcheckpoint )
{
     int i, j;

     g.blevel = 0;
     g.poilam = 1.0;
     g.debug = 0;
     Linit();

     /*
      * pgplot initialize
      */
     cpgopen("?");
     cpgenv(0.0, (float)MAXGEN, 0.0, (float)MAXPOP, 0, 2);
     cpglab("generation", "population", "size (green) and fitness (blue) of individuals");

     /* set up color index */
     for (j = 0; j < 10; j++)
	  cpgscr(j, 1.0, 0.0, 0.0);

     for (i = 1; i < 10; i++)
	  for (j = 0; j < 10; j++)
	       cpgscr(i*10+j, 0.0, i/10.0, j/10.0);

     g.npop = 0;
     g.gen = 0;

     /* params for visualize; note: log a(X)/log a(D) = log b(X)/log b(D) */
     g.ncelldenom = log10((double)CELLSCEIL);
     g.distdenom = log10((double)DISTCEIL);

     return 0;
}

/* app_uninitialize()
 *
 * perform application cleanup (free memory, etc.)
 */

void app_uninitialize ( void )
{
     cpgclos();
     return;
}

/* app_read_checkpoint()
 *
 * read state information from a checkpoint file.  it is passed a handle
 * for a file that has been opened for read in text mode.  it should
 * leave the file pointer at the end of the user information.
 */

void app_read_checkpoint ( FILE *f )
{
     return;
}

/* app_write_checkpoint()
 *
 * write state information to a checkpoint file.  it is passed a handle
 * for a file that has been opened for write in text mode.  it should
 * leave the file pointer at the end of the user information.
 */

void app_write_checkpoint ( FILE *f )
{
     return;
}

/* EOF */
