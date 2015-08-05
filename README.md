# lambda GP for lil-gp

This package is to be put in lil-gp's app/ directory.
I used lil-gp 1.1 version.
The includes a lambda-calculus beta-reducing module.

The GP setup tries to find *f(x)=2x* function.
The code for it is embedded in the ```app_eval_fitness``` C function
in [app.c](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/app.c)
as ```correct = Cchurch_num(testcases[i]*2)```, along with test cases for x.
Other GP parameters are defined
in [template.in](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/template.in).

The [try](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/try) executable script
drives multiple GP runs.
If you want to run a session just once, create input.file (of lil-gp)
from [template.in](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/template.in)
by replacing ```$``` placeholders with desired number and file name.

To run this package, the PGPLOT library and some other libraries must be linked
(see [GNUmakefile](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/GNUmakefile))
to render the progress of run visually to a PostScript file.
If you don't need this feature, remove ```cpg*``` stuff
from [app.c](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/app.c)
and adjust [GNUmakefile](https://github.com/kazutomi/lambda-gp-for-lilgp/blob/master/GNUmakefile).

## Reference

Kazuto Tominaga, et al.:
"An encoding scheme for generating lambda-expressions in genetic programming".
In *Proceedings of GECCO 2003*, 2003.
