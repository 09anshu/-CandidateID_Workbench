#include "linear_algebra.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workbench {

// -----------------------------------------------------------------------
// Thomas algorithm for tridiagonal systems
// -----------------------------------------------------------------------
Result<std::vector<double>> solve_tridiagonal(
    std::vector<double> a,
    std::vector<double> b,
    std::vector<double> c,
    std::vector<double> d) {

    const int n = static_cast<int>(b.size());

    // --- input validation ------------------------------------------------
    if (n == 0) {
        return Result<std::vector<double>>::fail(
            "solve_tridiagonal: empty system (n == 0)");
    }
    if (static_cast<int>(a.size()) != n - 1 ||
        static_cast<int>(c.size()) != n - 1 ||
        static_cast<int>(d.size()) != n) {
        return Result<std::vector<double>>::fail(
            "solve_tridiagonal: dimension mismatch — "
            "expected a(n-1), b(n), c(n-1), d(n)");
    }

    // --- forward sweep ---------------------------------------------------
    for (int i = 1; i < n; ++i) {
        if (std::abs(b[i - 1]) < TOLERANCE) {
            return Result<std::vector<double>>::fail(
                "solve_tridiagonal: zero pivot at row " +
                std::to_string(i - 1));
        }
        double w = a[i - 1] / b[i - 1];
        b[i] = b[i] - w * c[i - 1];
        d[i] = d[i] - w * d[i - 1];
    }

    // --- back substitution -----------------------------------------------
    if (std::abs(b[n - 1]) < TOLERANCE) {
        return Result<std::vector<double>>::fail(
            "solve_tridiagonal: zero pivot at last row");
    }

    std::vector<double> x(n);
    x[n - 1] = d[n - 1] / b[n - 1];
    for (int i = n - 2; i >= 0; --i) {
        if (std::abs(b[i]) < TOLERANCE) {
            return Result<std::vector<double>>::fail(
                "solve_tridiagonal: zero pivot at row " +
                std::to_string(i));
        }
        x[i] = (d[i] - c[i] * x[i + 1]) / b[i];
    }

    return Result<std::vector<double>>::ok(std::move(x));
}

// -----------------------------------------------------------------------
// Gaussian elimination with partial pivoting
// -----------------------------------------------------------------------
Result<std::vector<double>> solve_linear_system(
    Matrix A,
    std::vector<double> b) {

    const int n = A.rows;

    // --- input validation ------------------------------------------------
    if (n == 0) {
        return Result<std::vector<double>>::fail(
            "solve_linear_system: empty system (n == 0)");
    }
    if (A.cols != n) {
        return Result<std::vector<double>>::fail(
            "solve_linear_system: matrix is not square (" +
            std::to_string(A.rows) + "x" +
            std::to_string(A.cols) + ")");
    }
    if (static_cast<int>(b.size()) != n) {
        return Result<std::vector<double>>::fail(
            "solve_linear_system: RHS size (" +
            std::to_string(b.size()) +
            ") does not match matrix dimension (" +
            std::to_string(n) + ")");
    }

    // --- forward elimination with partial pivoting -----------------------
    for (int k = 0; k < n; ++k) {
        // Find the row with the largest absolute value in column k
        int max_row = k;
        double max_val = std::abs(A[k][k]);
        for (int i = k + 1; i < n; ++i) {
            double v = std::abs(A[i][k]);
            if (v > max_val) {
                max_val = v;
                max_row = i;
            }
        }

        if (max_val < TOLERANCE) {
            return Result<std::vector<double>>::fail(
                "solve_linear_system: singular or near-singular "
                "matrix at column " + std::to_string(k));
        }

        // Swap rows k and max_row in A and b
        if (max_row != k) {
            std::swap(A[k], A[max_row]);
            std::swap(b[k], b[max_row]);
        }

        // Eliminate entries below pivot
        for (int i = k + 1; i < n; ++i) {
            double factor = A[i][k] / A[k][k];
            for (int j = k + 1; j < n; ++j) {
                A[i][j] -= factor * A[k][j];
            }
            A[i][k] = 0.0;  // exact zero
            b[i] -= factor * b[k];
        }
    }

    // --- back substitution -----------------------------------------------
    std::vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = b[i];
        for (int j = i + 1; j < n; ++j) {
            sum -= A[i][j] * x[j];
        }
        x[i] = sum / A[i][i];
    }

    return Result<std::vector<double>>::ok(std::move(x));
}

// -----------------------------------------------------------------------
// Matrix-vector multiply
// -----------------------------------------------------------------------
std::vector<double> mat_vec_multiply(
    const Matrix& A,
    const std::vector<double>& x) {

    std::vector<double> y(A.rows, 0.0);
    for (int i = 0; i < A.rows; ++i) {
        double sum = 0.0;
        int cols = std::min(A.cols, static_cast<int>(x.size()));
        for (int j = 0; j < cols; ++j) {
            sum += A[i][j] * x[j];
        }
        y[i] = sum;
    }
    return y;
}

// -----------------------------------------------------------------------
// Matrix addition
// -----------------------------------------------------------------------
Matrix mat_add(const Matrix& A, const Matrix& B) {
    int r = std::max(A.rows, B.rows);
    int c_dim = std::max(A.cols, B.cols);
    Matrix C(r, c_dim, 0.0);

    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c_dim; ++j) {
            double va = (i < A.rows && j < A.cols) ? A[i][j] : 0.0;
            double vb = (i < B.rows && j < B.cols) ? B[i][j] : 0.0;
            C[i][j] = va + vb;
        }
    }
    return C;
}

// -----------------------------------------------------------------------
// Dot product
// -----------------------------------------------------------------------
double dot_product(
    const std::vector<double>& a,
    const std::vector<double>& b) {

    int n = static_cast<int>(std::min(a.size(), b.size()));
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

// -----------------------------------------------------------------------
// L2 norm
// -----------------------------------------------------------------------
double vec_norm(const std::vector<double>& v) {
    return std::sqrt(dot_product(v, v));
}

} // namespace workbench
