# Handwritten Backpropagation Derivation Appendix

> **PASS SCIENCE WORKS — R&D Internship 2026-2027**  
> Task F: Neural Network Backpropagation Derivation Reference (Maximum 2 Pages)

---

## PAGE 1: Problem Definition & Forward Pass

### 1. Network Architecture & Problem Formulation
Single-input, single-hidden-neuron, single-output feedforward regression model:
- **Input**: $x = 1.0$
- **Target Value**: $y = 2.0$
- **Hidden Neuron Pre-activation**: $z = w x + b$
- **Hidden Activation Function**: $h = \tanh(z) = \frac{e^z - e^{-z}}{e^z + e^{-z}}$
- **Output Linear Layer**: $\hat{y} = v h + c$
- **Objective / Loss Function**: $J = \frac{1}{2} (\hat{y} - y)^2$
- **Learning Rate**: $\eta = 0.1$

### Initial Parameter Values ($\theta_0$):
$$w_0 = 0.50000000, \quad b_0 = 0.00000000, \quad v_0 = 1.00000000, \quad c_0 = 0.00000000$$

```
  x (1.0)
     │
     ▼  * w (0.5)
    (+) ◄─── + b (0.0)
     │
     ▼  z = 0.5
  [tanh]
     │
     ▼  h = 0.462117
     │
     ▼  * v (1.0)
    (+) ◄─── + c (0.0)
     │
     ▼  ŷ = 0.462117
  [Loss] ◄─── Target y = 2.0
     │
     ▼  J = 0.5 * (ŷ - y)² = 1.182542
```

---

### 2. Forward Pass Step-by-Step Numerical Evaluation

1. **Pre-activation ($z$)**:
   $$z = w x + b = (0.5)(1.0) + 0.0 = \mathbf{0.50000000}$$

2. **Hidden Activation ($h$)**:
   $$h = \tanh(0.5) = \frac{e^{0.5} - e^{-0.5}}{e^{0.5} + e^{-0.5}} = \frac{1.64872127 - 0.60653066}{1.64872127 + 0.60653066} = \frac{1.04219061}{2.25525193} \approx \mathbf{0.46211716}$$

3. **Model Prediction ($\hat{y}$)**:
   $$\hat{y} = v h + c = (1.0)(0.46211716) + 0.0 = \mathbf{0.46211716}$$

4. **Residual Error ($e$)**:
   $$e = \hat{y} - y = 0.46211716 - 2.00000000 = \mathbf{-1.53788284}$$

5. **Initial Loss ($J$)**:
   $$J = \frac{1}{2} (\hat{y} - y)^2 = \frac{1}{2} (-1.53788284)^2 = \frac{1}{2} (2.36508364) = \mathbf{1.18254182}$$

---
\newpage
---

## PAGE 2: Analytical Chain-Rule Gradients & Simultaneous Update

### 3. Intermediate Derivative Calculations via Chain Rule

1. **Loss derivative w.r.t. prediction ($\hat{y}$)**:
   $$\frac{\partial J}{\partial \hat{y}} = \frac{\partial}{\partial \hat{y}}\left[\frac{1}{2}(\hat{y} - y)^2\right] = \hat{y} - y = e = \mathbf{-1.53788284}$$

2. **Hyperbolic tangent derivative identity**:
   $$\frac{dh}{dz} = \frac{d}{dz}[\tanh(z)] = 1 - \tanh^2(z) = 1 - h^2$$
   $$\frac{dh}{dz} = 1 - (0.46211716)^2 = 1 - 0.21355227 = \mathbf{0.78644773}$$

3. **Loss derivative w.r.t. output bias ($c$)**:
   $$\frac{\partial J}{\partial c} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial c} = (\hat{y} - y) \cdot 1 = \mathbf{-1.53788284}$$

4. **Loss derivative w.r.t. output weight ($v$)**:
   $$\frac{\partial J}{\partial v} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial v} = (\hat{y} - y) \cdot h = (-1.53788284) \cdot (0.46211716) = \mathbf{-0.71068202}$$

5. **Loss derivative w.r.t. hidden bias ($b$)**:
   $$\frac{\partial J}{\partial b} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial h} \cdot \frac{\partial h}{\partial z} \cdot \frac{\partial z}{\partial b} = (\hat{y} - y) \cdot v \cdot (1 - h^2) \cdot 1$$
   $$\frac{\partial J}{\partial b} = (-1.53788284) \cdot (1.0) \cdot (0.78644773) \cdot 1 = \mathbf{-1.20946427}$$

6. **Loss derivative w.r.t. input weight ($w$)**:
   $$\frac{\partial J}{\partial w} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial h} \cdot \frac{\partial h}{\partial z} \cdot \frac{\partial z}{\partial w} = (\hat{y} - y) \cdot v \cdot (1 - h^2) \cdot x$$
   $$\frac{\partial J}{\partial w} = (-1.53788284) \cdot (1.0) \cdot (0.78644773) \cdot (1.0) = \mathbf{-1.20946427}$$

---

### 4. Simultaneous Gradient Descent Parameter Update
> **CRITICAL RULE**: All four parameters must be updated simultaneously using the gradients evaluated at the **original** parameters $\theta_0$. Parameters are not updated sequentially.

Update formula: $\theta_{\text{new}} = \theta_{\text{original}} - \eta \frac{\partial J}{\partial \theta}$, with $\eta = 0.1$:

$$\begin{aligned}
w_1 &= w_0 - \eta \frac{\partial J}{\partial w} = 0.50000000 - (0.1)(-1.20946427) = 0.50000000 + 0.12094643 = \mathbf{0.62094643} \\
b_1 &= b_0 - \eta \frac{\partial J}{\partial b} = 0.00000000 - (0.1)(-1.20946427) = 0.00000000 + 0.12094643 = \mathbf{0.12094643} \\
v_1 &= v_0 - \eta \frac{\partial J}{\partial v} = 1.00000000 - (0.1)(-0.71068202) = 1.00000000 + 0.07106820 = \mathbf{1.07106820} \\
c_1 &= c_0 - \eta \frac{\partial J}{\partial c} = 0.00000000 - (0.1)(-1.53788284) = 0.00000000 + 0.15378828 = \mathbf{0.15378828}
\end{aligned}$$

---

### 5. Physical Meaning & Explanation of Gradient Direction
- **Why are all gradients negative?**  
  Because the initial prediction $\hat{y}_0 \approx 0.462$ is substantially below the target $y = 2.0$, producing a negative error $(\hat{y} - y = -1.538 < 0)$.
- **Direction of Descent**:  
  Gradient descent applies $-\eta \nabla J$. Subtracting a negative gradient **increases** every parameter:
  $$\Delta w > 0, \quad \Delta b > 0, \quad \Delta v > 0, \quad \Delta c > 0$$
- **Effect on Prediction and Loss**:  
  Increasing $w$ and $b$ drives $z$ higher, which increases hidden activation $h = \tanh(z) \in (0, 1)$. Increasing $v$ and $c$ further scales and offsets the output $\hat{y} = vh + c$ upward toward $2.0$. Evaluating the updated parameters at Step 1 yields $\hat{y}_1 \approx 0.826$ and $J_1 \approx 0.689$ (a $41.7\%$ reduction in error loss in a single step).
