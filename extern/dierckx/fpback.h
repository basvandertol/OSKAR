#ifndef DIERCKX_FPBACK_H_
#define DIERCKX_FPBACK_H_

/**
 * @file fpback.h
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @details
 * Subroutine fpback calculates the solution of the system of
 * equations a*c = z with a a n x n upper triangular matrix
 * of bandwidth k.
 */
void fpback_f(const float *a, const float *z, int n, int k, float *c,
        int nest);

/**
 * @details
 * Subroutine fpback calculates the solution of the system of
 * equations a*c = z with a a n x n upper triangular matrix
 * of bandwidth k.
 */
void fpback_d(const double *a, const double *z, int n, int k, double *c,
        int nest);

#ifdef __cplusplus
}
#endif

#endif /* DIERCKX_FPBACK_H_ */
