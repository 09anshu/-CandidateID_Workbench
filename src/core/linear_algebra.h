#pragma once

#include <string>
#include <vector>

#include "types.h"

namespace workbench {

/// Solve tridiagonal system Ax = d using the Thomas algorithm.
/// @param a  Sub-diagonal   (n-1 elements, indices 0..n-2)
/// @param b  Main diagonal  (n elements)
/// @param c  Super-diagonal (n-1 elements, indices 0..n-2)
/// @param d  Right-hand side (n elements)
/// @return   Solution vector x, or an error description on failure.
Result<std::vector<double>> solve_tridiagonal(
    std::vector<double> a,
    std::vector<double> b,
    std::vector<double> c,
    std::vector<double> d
);

/// Solve general dense system Ax = b via Gaussian elimination
/// with partial pivoting.
/// @param A  Coefficient matrix (n x n)
/// @param b  Right-hand side vector (n elements)
/// @return   Solution vector x, or an error description on failure.
Result<std::vector<double>> solve_linear_system(
    Matrix A,
    std::vector<double> b
);

/// Matrix-vector multiply: y = A * x
std::vector<double> mat_vec_multiply(
    const Matrix& A,
    const std::vector<double>& x
);

/// Matrix addition: C = A + B  (dimensions must match)
Matrix mat_add(const Matrix& A, const Matrix& B);

/// Dot product of two vectors (sizes must match)
double dot_product(
    const std::vector<double>& a,
    const std::vector<double>& b
);

/// L2 (Euclidean) norm of a vector
double vec_norm(const std::vector<double>& v);

} // namespace workbench
