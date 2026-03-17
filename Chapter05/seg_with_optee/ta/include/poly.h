
#ifndef POLY_H
#define POLY_H

#include <queue.h>

void polyfit_quadratic(const double x[], const Queue *q, double coef[3]);
double polyval_quadratic(const double coef[3], double x);

#endif
