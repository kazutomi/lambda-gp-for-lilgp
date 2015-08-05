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

/*
 *  Modified to try finding the f(x) = 2x function
 *  Kazuto Tominaga <tomi@acm.org>
 *  July 10, 2003
 */

#ifndef _APP_H
#define _APP_H

#include <lilgp.h>

/* here you should create a typedef struct called "globaldata" to pass
 * information between the evaluation C function and the functions and
 * terminals (if that is needed by your application).
 */

enum {
  MAXPOP = 1000,
  MAXGEN = 1000,
  CELLSCEIL = 10000,	/* for visualize; should be in accordance with in.xxx */
  DISTCEIL = 20000,	/* for visualize; typical value = sum of worst raw fitness + alpha */
  IPENALTY = 10000,	/* penalty distance for identity function */
};

typedef struct
{
     Var blevel;	/* level of binding; root=0 */
     double poilam;	/* lambda parameter for Poisson random value generation */
     double ncelldenom;	/* for visualize; denominator for log10(ncells) to fit within 0-1 */
     double distdenom;	/* for visualize; coefficient for log10(sum of dist) to fit within 0-1 */
     /* misc */
     int debug;
     /* data */
     int gen;
     int npop;
     struct iinfo {
	  int	ncells;
	  float	fitness;
	  int	reduction_finished;
     } idata[MAXPOP];
} globaldata;

/* leave this definition in if you pass information via globaldata. */
extern globaldata g;

#endif
