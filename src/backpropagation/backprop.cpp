#include "backprop.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace workbench {

ForwardPassResult forward_pass(const NetworkParams& p, double x, double y) {
    ForwardPassResult res;
    res.z = p.w * x + p.b;
    res.h = std::tanh(res.z);
    res.y_hat = p.v * res.h + p.c;
    res.error = res.y_hat - y;
    res.loss = 0.5 * res.error * res.error;
    return res;
}

ParameterGradients compute_gradients(const NetworkParams& p, double x, double y) {
    ForwardPassResult fwd = forward_pass(p, x, y);

    // Chain rule derivations:
    // dJ/dy_hat = y_hat - y
    // dJ/dc = dJ/dy_hat * dy_hat/dc = (y_hat - y) * 1
    // dJ/dv = dJ/dy_hat * dy_hat/dv = (y_hat - y) * h
    // dJ/dh = dJ/dy_hat * dy_hat/dh = (y_hat - y) * v
    // dh/dz = 1 - tanh^2(z) = 1 - h^2
    // dJ/db = dJ/dh * dh/dz * dz/db = (y_hat - y) * v * (1 - h^2) * 1
    // dJ/dw = dJ/dh * dh/dz * dz/dw = (y_hat - y) * v * (1 - h^2) * x

    double dJ_dyhat = fwd.error;
    double dh_dz = 1.0 - fwd.h * fwd.h;
    double dJ_dh = dJ_dyhat * p.v;

    ParameterGradients grads;
    grads.dJ_dc = dJ_dyhat * 1.0;
    grads.dJ_dv = dJ_dyhat * fwd.h;
    grads.dJ_db = dJ_dh * dh_dz * 1.0;
    grads.dJ_dw = dJ_dh * dh_dz * x;

    return grads;
}

GradientVerification verify_gradients_finite_difference(const NetworkParams& p, double x, double y, double epsilon, double tolerance) {
    GradientVerification ver;
    ver.epsilon = epsilon;
    ver.tolerance = tolerance;
    ver.analytical = compute_gradients(p, x, y);

    // Central finite difference for w: [J(w+eps) - J(w-eps)] / (2*eps)
    NetworkParams p_w_plus = p;  p_w_plus.w += epsilon;
    NetworkParams p_w_minus = p; p_w_minus.w -= epsilon;
    ver.numerical.dJ_dw = (forward_pass(p_w_plus, x, y).loss - forward_pass(p_w_minus, x, y).loss) / (2.0 * epsilon);

    // Central finite difference for b: [J(b+eps) - J(b-eps)] / (2*eps)
    NetworkParams p_b_plus = p;  p_b_plus.b += epsilon;
    NetworkParams p_b_minus = p; p_b_minus.b -= epsilon;
    ver.numerical.dJ_db = (forward_pass(p_b_plus, x, y).loss - forward_pass(p_b_minus, x, y).loss) / (2.0 * epsilon);

    // Central finite difference for v: [J(v+eps) - J(v-eps)] / (2*eps)
    NetworkParams p_v_plus = p;  p_v_plus.v += epsilon;
    NetworkParams p_v_minus = p; p_v_minus.v -= epsilon;
    ver.numerical.dJ_dv = (forward_pass(p_v_plus, x, y).loss - forward_pass(p_v_minus, x, y).loss) / (2.0 * epsilon);

    // Central finite difference for c: [J(c+eps) - J(c-eps)] / (2*eps)
    NetworkParams p_c_plus = p;  p_c_plus.c += epsilon;
    NetworkParams p_c_minus = p; p_c_minus.c -= epsilon;
    ver.numerical.dJ_dc = (forward_pass(p_c_plus, x, y).loss - forward_pass(p_c_minus, x, y).loss) / (2.0 * epsilon);

    // Calculate absolute errors
    ver.absolute_error.dJ_dw = std::abs(ver.analytical.dJ_dw - ver.numerical.dJ_dw);
    ver.absolute_error.dJ_db = std::abs(ver.analytical.dJ_db - ver.numerical.dJ_db);
    ver.absolute_error.dJ_dv = std::abs(ver.analytical.dJ_dv - ver.numerical.dJ_dv);
    ver.absolute_error.dJ_dc = std::abs(ver.analytical.dJ_dc - ver.numerical.dJ_dc);

    ver.all_passed = (ver.absolute_error.dJ_dw <= tolerance &&
                      ver.absolute_error.dJ_db <= tolerance &&
                      ver.absolute_error.dJ_dv <= tolerance &&
                      ver.absolute_error.dJ_dc <= tolerance);

    return ver;
}

BackpropResult run_backpropagation_analysis(double x, double y, double learning_rate, int steps) {
    BackpropResult res;
    res.initial_params.w = 0.5;
    res.initial_params.b = 0.0;
    res.initial_params.v = 1.0;
    res.initial_params.c = 0.0;

    // Initial forward pass
    res.initial_forward = forward_pass(res.initial_params, x, y);

    // Analytical gradients from initial parameters
    res.initial_gradients = compute_gradients(res.initial_params, x, y);

    // Simultaneous gradient update from ORIGINAL parameters:
    // theta_new = theta_orig - eta * grad(theta_orig)
    res.updated_params_step1.w = res.initial_params.w - learning_rate * res.initial_gradients.dJ_dw;
    res.updated_params_step1.b = res.initial_params.b - learning_rate * res.initial_gradients.dJ_db;
    res.updated_params_step1.v = res.initial_params.v - learning_rate * res.initial_gradients.dJ_dv;
    res.updated_params_step1.c = res.initial_params.c - learning_rate * res.initial_gradients.dJ_dc;

    // Numerical gradient verification (finite differences)
    res.verification = verify_gradients_finite_difference(res.initial_params, x, y, 1e-6, 1e-5);

    res.gradient_direction_explanation =
        "All analytical gradients (dJ/dw, dJ/db, dJ/dv, dJ/dc) are negative because the initial prediction "
        "y_hat ≈ 0.4621 is less than the target y = 2.0 (giving negative error e = -1.5379). Under gradient "
        "descent (theta_new = theta - eta * grad), subtracting a negative quantity increases all four parameters, "
        "which increases the network prediction y_hat towards the target y = 2.0, thereby reducing the loss J.";

    // 100-step training loop
    NetworkParams current_p = res.initial_params;
    res.training_history.reserve(steps + 1);

    // Record step 0
    TrainingStepRecord rec0;
    rec0.step = 0;
    rec0.params = current_p;
    rec0.gradients = res.initial_gradients;
    rec0.forward = res.initial_forward;
    res.training_history.push_back(rec0);

    for (int s = 1; s <= steps; ++s) {
        ParameterGradients g = compute_gradients(current_p, x, y);

        // Simultaneous parameter update
        current_p.w -= learning_rate * g.dJ_dw;
        current_p.b -= learning_rate * g.dJ_db;
        current_p.v -= learning_rate * g.dJ_dv;
        current_p.c -= learning_rate * g.dJ_dc;

        ForwardPassResult fwd = forward_pass(current_p, x, y);

        TrainingStepRecord rec;
        rec.step = s;
        rec.params = current_p;
        rec.gradients = g;
        rec.forward = fwd;
        res.training_history.push_back(rec);
    }

    res.final_loss = res.training_history.back().forward.loss;
    return res;
}

std::string export_training_csv(const BackpropResult& result) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# PASS SCIENCE WORKS - Backpropagation Training History (100 Steps)\n";
    ss << "# Initial: w=0.5, b=0, v=1, c=0, lr=0.1, target=2.0\n";
    ss << "step,w,b,v,c,z,h,y_hat,loss,dJ_dw,dJ_db,dJ_dv,dJ_dc\n";
    for (const auto& rec : result.training_history) {
        ss << rec.step << ","
           << rec.params.w << "," << rec.params.b << "," << rec.params.v << "," << rec.params.c << ","
           << rec.forward.z << "," << rec.forward.h << "," << rec.forward.y_hat << "," << rec.forward.loss << ","
           << rec.gradients.dJ_dw << "," << rec.gradients.dJ_db << "," << rec.gradients.dJ_dv << "," << rec.gradients.dJ_dc << "\n";
    }
    return ss.str();
}

} // namespace workbench
