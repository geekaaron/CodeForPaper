
#include "include/mymath.h"

double myfabs(double x)
{
    if (x < 0) return -x;
    return x;
}

double mylog(double x) {
    union {
        double d;
        long long i;
    } u;
    u.d = x;
    return (u.i - 4606921280493453312LL) * 1.4219997742072553e-16;
}