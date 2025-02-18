/* Generated by import-lpsolve -- do not edit! */
#ifndef LP_SOLVE_H
#define LP_SOLVE_H
#include <glib.h>
#include <numbers.h>
typedef struct _lprec     lprec;
#define ROWTYPE_LE               1
#define ROWTYPE_GE               2
#define ROWTYPE_EQ               3
/* Solver status values */
#define UNKNOWNERROR            -5
#define DATAIGNORED             -4
#define NOBFP                   -3
#define NOMEMORY                -2
#define NOTRUN                  -1
#define OPTIMAL                  0
#define SUBOPTIMAL               1
#define INFEASIBLE               2
#define UNBOUNDED                3
#define DEGENERATE               4
#define NUMFAILURE               5
#define USERABORT                6
#define TIMEOUT                  7
#define RUNNING                  8
#define FUTURESTATUS             9

void lp_solve_version(int *majorversion, int *minorversion, int *release, int *build);
lprec  * lp_solve_make_lp(int rows, int columns);
void lp_solve_delete_lp(lprec *lp);
void lp_solve_set_maxim(lprec *lp);
void lp_solve_set_minim(lprec *lp);
gboolean lp_solve_set_constr_type(lprec *lp, int rownr, int con_type);
gboolean lp_solve_set_rh(lprec *lp, int rownr, gnm_float value);
gboolean lp_solve_set_mat(lprec *lp, int rownr, int colnr, gnm_float value);
gboolean lp_solve_set_upbo(lprec *lp, int colnr, gnm_float value);
gboolean lp_solve_set_lowbo(lprec *lp, int colnr, gnm_float value);
gboolean lp_solve_set_int(lprec *lp, int colnr, gboolean must_be_int);
int lp_solve_solve(lprec *lp);
void lp_solve_print_lp(lprec *lp);
void lp_solve_set_timeout(lprec *lp, long sectimeout);
void lp_solve_set_scalelimit(lprec *lp, gnm_float scalelimit);
gint64 lp_solve_get_total_iter(lprec *lp);
gnm_float lp_solve_get_primal(lprec *lp, int index);
gnm_float lp_solve_get_dual(lprec *lp, int index);
int lp_solve_get_nrows(lprec *lp);
#endif
