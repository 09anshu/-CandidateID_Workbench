#pragma once

#include <string>
#include <vector>

namespace workbench {

/// Parameters of the neural network
struct NetworkParams {
    double w = 0.5; // weight
    double b = 0.0; // hidden bias
    double v = 1.0; // output weight
    double c = 0.0; // output bias
};

/// Forward pass evaluation results
struct ForwardPassResult {
    double z = 0.0;       // hidden pre-activation: w*x + b
    double h = 0.0;       // hidden activation: tanh(z)
    double y_hat = 0.0;   // prediction: v*h + c
    double error = 0.0;   // y_hat - y
    double loss = 0.0;    // J = 0.5 * (y_hat - y)^2
};

/// Analytical and numerical gradients
struct ParameterGradients {
    double dJ_dw = 0.0;
    double dJ_db = 0.0;
    double dJ_dv = 0.0;
    double dJ_dc = 0.0;
};

/// Verification results comparing analytical gradients to central finite differences
struct GradientVerification {
    ParameterGradients analytical;
    ParameterGradients numerical;
    ParameterGradients absolute_error;
    double epsilon = 1e-6;
    double tolerance = 1e-5;
    bool all_passed = false;
};

/// Single step of gradient descent training record
struct TrainingStepRecord {
    int step = 0;
    NetworkParams params;
    ParameterGradients gradients;
    ForwardPassResult forward;
};

/// Complete backpropagation and training result
struct BackpropResult {
    NetworkParams initial_params;
    ForwardPassResult initial_forward;
    ParameterGradients initial_gradients;
    NetworkParams updated_params_step1;
    GradientVerification verification;
    std::vector<TrainingStepRecord> training_history; // 100 steps
    double final_loss = 0.0;
    std::string gradient_direction_explanation;
};

/// Compute forward pass for sample (x, y)
ForwardPassResult forward_pass(const NetworkParams& p, double x, double y);

/// Compute analytical gradients via chain rule
ParameterGradients compute_gradients(const NetworkParams& p, double x, double y);

/// Perform central finite difference verification for each parameter with epsilon = 1e-6
GradientVerification verify_gradients_finite_difference(const NetworkParams& p, double x, double y, double epsilon = 1e-6, double tolerance = 1e-5);

/// Run complete backpropagation analysis including 1-step simultaneous update and 100-step training loop
BackpropResult run_backpropagation_analysis(double x = 1.0, double y = 2.0, double learning_rate = 0.1, int steps = 100);

/// Export training curve to CSV
std::string export_training_csv(const BackpropResult& result);

} // namespace workbench
