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

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include <lilgp.h>

#include "lambda.h"

/* here you need one C function for every ordinary (non-ERC) function
 * and terminal.  each should take an int and array of type (farg) and
 * return a type (DATATYPE).
 *
 * functions declared as type DATA can get the value of child i by
 * reading "args[i].d".
 *
 * functions declared as type EXPR can evaluate the child and gets its
 * value by passing "args[i].t" (and the int argument) to evaluate_tree().
 *
 * terminals should not need to use either argument passed to them.
 *
 */

DATATYPE f_abstraction ( int tree, farg *args )
{
     DATATYPE body;
     Lexp nabst;

     g.blevel++;

     body = evaluate_tree(args[0].t, tree);
     /*
      * if direct child is a variable, create Var cell for it.
      * otherwise it must be lexp
      */
     if (body < 0) {
	  body = Icreatebvar(body);
     }
     nabst = Labst(g.blevel, body);

     g.blevel--;

     return nabst;
}

DATATYPE f_application ( int tree, farg *args )
{
     DATATYPE left, right;
     Lexp napp;

     left = args[0].d;
     right = args[1].d;

     /*
      * if direct child(ren) is (are) a variable(s), create Var(s) for it (them).
      * otherwise they must be Lexps
      */
     if (left < 0) {
	  left = Icreatebvar(left);
     }
     if (right < 0) {
	  right = Icreatebvar(right);
     }
     napp = Lappl(left, right);

     return napp;
}

/*
 * Generate variable by Poisson process with lambda = g.poilam
 * n.b.: internal expression of variables is a negative integer
 */
void f_variable_gen ( DATATYPE *v )
{
     int k;
     double pi;
     double thresh;

     pi = 1.0;
     thresh = exp(-g.poilam);
     for (k = 0; pi >= thresh; k++)
	  pi *= random_double();
     
     assert(k != 0);
     if (g.debug)
	  printf("[%d]", k);
     *v = (DATATYPE)(-k);
}

char *f_variable_print ( DATATYPE v )
{
     static char buffer[20];

     sprintf ( buffer, "%d", v );
     return buffer;
}

/* EOF */
