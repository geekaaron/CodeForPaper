
#include <poly.h>

// Fit: y = a + b*x + c*x^2
void polyfit_quadratic(const double x[], const Queue *q, double coef[3]) {
    double sum_x = 0, sum_x2 = 0, sum_x3 = 0, sum_x4 = 0;
    double sum_y = 0, sum_xy = 0, sum_x2y = 0;

    for (int i = 0, j = q->front; i < q->capacity-1; i++, j = (j + 1) % q->capacity) {
        sum_x += x[i];
        sum_x2 += x[i] * x[i];
        sum_x3 += x[i] * x[i] * x[i];
        sum_x4 += x[i] * x[i] * x[i] * x[i];
        sum_y += q->data[j];
        sum_xy += x[i] * q->data[j];
        sum_x2y += x[i] * x[i] * q->data[j];
    }

    // [n   sum_x   sum_x2 ] [a]   [sum_y  ]
    // [sum_x sum_x2 sum_x3 ] [b] = [sum_xy ]
    // [sum_x2 sum_x3 sum_x4] [c]   [sum_x2y]

    // Cramer rule
    double A[3][3] = {
        {q->capacity-1, sum_x, sum_x2},
        {sum_x, sum_x2, sum_x3},
        {sum_x2, sum_x3, sum_x4}
    };
    double B[3] = {sum_y, sum_xy, sum_x2y};

    // calculate determinant
    double det = A[0][0]*(A[1][1]*A[2][2] - A[1][2]*A[2][1]) -
                 A[0][1]*(A[1][0]*A[2][2] - A[1][2]*A[2][0]) +
                 A[0][2]*(A[1][0]*A[2][1] - A[1][1]*A[2][0]);

    double det_a = B[0]*(A[1][1]*A[2][2] - A[1][2]*A[2][1]) -
                  A[0][1]*(B[1]*A[2][2] - A[1][2]*B[2]) +
                  A[0][2]*(B[1]*A[2][1] - A[1][1]*B[2]);

    double det_b = A[0][0]*(B[1]*A[2][2] - A[1][2]*B[2]) -
                  B[0]*(A[1][0]*A[2][2] - A[1][2]*A[2][0]) +
                  A[0][2]*(A[1][0]*B[2] - B[1]*A[2][0]);

    double det_c = A[0][0]*(A[1][1]*B[2] - B[1]*A[2][1]) -
                  A[0][1]*(A[1][0]*B[2] - B[1]*A[2][0]) +
                  B[0]*(A[1][0]*A[2][1] - A[1][1]*A[2][0]);

    coef[0] = det_a / det;  // a
    coef[1] = det_b / det;  // b
    coef[2] = det_c / det;  // c
}

// Evaluation: y = a + b*x + c*x^2
double polyval_quadratic(const double coef[3], double x) {
    return coef[0] + coef[1]*x + coef[2]*x*x;
}
