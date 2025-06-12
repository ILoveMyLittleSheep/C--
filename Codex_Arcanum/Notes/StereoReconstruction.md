# 双目立体视觉三维重建
## 1. 坐标转换基础
### 1.1 **物理空间坐标系（世界坐标系，World Frame）**
   - **定义**：物体在真实三维空间中的坐标，记为 \( \mathbf{P}_w = [X_w, Y_w, Z_w, 1]^T \)（齐次坐标）。
   - **特点**：这是一个全局坐标系，用于描述物体和相机的位置关系。

---

### 1.2 **相机空间坐标系（Camera Frame）**
   - **定义**：以相机光心为原点、光轴为 \( Z_c \) 轴的坐标系，记为 \( \mathbf{P}_c = [X_c, Y_c, Z_c, 1]^T \)。
   - **转换方法**：通过 **刚体变换（Rigid Transformation）** 将世界坐标转换到相机坐标：
     \[
     \mathbf{P}_c = \mathbf{T}_{cw} \cdot \mathbf{P}_w
     \]
     其中：
     - \( \mathbf{T}_{cw} \) 是 \( 4 \times 4 \) 的变换矩阵，包含：
       - **旋转矩阵 \( \mathbf{R}_{3 \times 3} \)**：描述相机坐标系相对于世界坐标系的旋转。
       - **平移向量 \( \mathbf{t}_{3 \times 1} \)**：描述相机光心在世界坐标系中的位置。
     - 展开形式：
       \[
       \begin{bmatrix} X_c \\ Y_c \\ Z_c \\ 1 \end{bmatrix} = 
       \begin{bmatrix} \mathbf{R} & \mathbf{t} \\ \mathbf{0} & 1 \end{bmatrix}
       \begin{bmatrix} X_w \\ Y_w \\ Z_w \\ 1 \end{bmatrix}
       \]

   - **关键点**：
     - 旋转矩阵 \( \mathbf{R} \) 是正交矩阵（\( \mathbf{R}^T \mathbf{R} = \mathbf{I} \)）。
     - 平移向量 \( \mathbf{t} \) 是相机光心在世界坐标系中的坐标。

---

### 1.3 **像平面坐标系（Image Plane Frame）**
   - **定义**：相机光心投影到成像平面上的二维坐标，记为 \( \mathbf{p} = [x, y]^T \)。
   - **转换方法**：通过 **透视投影（Perspective Projection）** 将相机坐标投影到像平面：
     1. **归一化平面投影**：将 \( \mathbf{P}_c \) 投影到归一化平面（\( Z_c = 1 \)）：
        \[
        x' = \frac{X_c}{Z_c}, \quad y' = \frac{Y_c}{Z_c}
        \]
     2. **考虑内参**：通过相机内参矩阵 \( \mathbf{K} \) 转换到像素坐标：
        \[
        \mathbf{p} = \mathbf{K} \begin{bmatrix} x' \\ y' \\ 1 \end{bmatrix}, \quad 
        \mathbf{K} = \begin{bmatrix} f_x & 0 & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix}
        \]
        - \( f_x, f_y \)：焦距（像素单位）。
        - \( c_x, c_y \)：主点坐标（像平面中心）。

   - **关键点**：
     - 透视投影是非线性操作（除以 \( Z_c \)）。
     - 内参矩阵 \( \mathbf{K} \) 描述了相机的光学特性（如焦距、畸变等）。

---

### 1.4 **完整转换流程总结**
   1. **世界坐标 → 相机坐标**：  
      \[
      \mathbf{P}_c = \mathbf{T}_{cw} \cdot \mathbf{P}_w
      \]
   2. **相机坐标 → 归一化坐标**：  
      \[
      x' = X_c / Z_c, \quad y' = Y_c / Z_c
      \]
   3. **归一化坐标 → 像素坐标**：  
      \[
      \begin{bmatrix} u \\ v \\ 1 \end{bmatrix} = 
      \mathbf{K} \begin{bmatrix} x' \\ y' \\ 1 \end{bmatrix}
      \]

---

### 1.5 **示例（简化版）**
假设：
- 世界坐标 \( \mathbf{P}_w = [1, 0, 2, 1]^T \)。
- 相机外参：旋转 \( \mathbf{R} = \mathbf{I} \)，平移 \( \mathbf{t} = [0, 0, -1]^T \)（相机后退1单位）。
- 内参：\( f_x = f_y = 500 \)，\( c_x = c_y = 320 \)。

**步骤**：
1. 相机坐标：  
   \[
   \mathbf{P}_c = \mathbf{T}_{cw} \cdot \mathbf{P}_w = [1, 0, 1, 1]^T
   \]
2. 归一化坐标：  
   \[
   x' = 1/1 = 1, \quad y' = 0/1 = 0
   \]
3. 像素坐标：  
   \[
   u = 500 \cdot 1 + 320 = 820, \quad v = 500 \cdot 0 + 320 = 320
   \]

---

### 1.6 **常见问题**
   - **为什么需要齐次坐标？**  
     为了统一表示旋转和平移（线性变换）。
   - **内参矩阵的作用？**  
     将物理长度（毫米）转换为像素单位，并处理图像传感器的不对称性（如非方形像素）。
   - **畸变如何处理？**  
     在归一化坐标后，需额外应用径向/切向畸变模型校正。

---
### **1.6 真实世界因素考虑**
在从 **世界坐标系 → 相机坐标系 → 像平面坐标系** 的转换中，是否需要通过 **显式引入缩放系数消除 \( Z \) 轴影响**，本质上取决于是否要保留深度信息。

---

- **透视投影的本质**：在透视投影中，\( Z_c \)（相机坐标系下的深度）会导致物体在像平面上的尺寸随距离变化（近大远小）。若直接使用未归一化的坐标，会导致投影后的坐标受 \( Z_c \) 非线性影响。
- **是否需要消除 \( Z \) 轴影响**：
  - **情况1**：如果目标是获取 **像平面上的纯几何坐标**（如像素位置），则必须通过除以 \( Z_c \) 完成归一化（即透视除法）。
  - **情况2**：如果目标是保留深度信息（如三维重建），则需单独存储 \( Z_c \)，而非通过矩阵乘法消除它。

---

### **1.7 完整推导（含归一化步骤）**
#### **(1) 世界坐标 → 相机坐标**
\[
\begin{bmatrix} X_c \\ Y_c \\ Z_c \\ 1 \end{bmatrix} = 
\begin{bmatrix} \mathbf{R} & \mathbf{t} \\ \mathbf{0} & 1 \end{bmatrix}
\begin{bmatrix} X_w \\ Y_w \\ Z_w \\ 1 \end{bmatrix}
\]
- 此时 \( Z_c \) 表示物体到相机光心的距离。

#### **(2) 相机坐标 → 归一化坐标（关键步骤）**
为了消除 \( Z_c \) 对像平面坐标的影响，需进行 **归一化**：
\[
x = \frac{X_c}{Z_c}, \quad y = \frac{Y_c}{Z_c}, \quad z = \frac{Z_c}{Z_c} = 1
\]
- **数学解释**：这等价于将相机坐标投影到 **归一化平面**（\( Z_c = 1 \) 的平面）。
- **几何意义**：所有位于同一条射线上的点（即 \( (X_c, Y_c, Z_c) \) 和 \( (kX_c, kY_c, kZ_c) \)）会投影到同一个 \( (x, y) \) 点。

#### **(3) 归一化坐标 → 像素坐标**
通过内参矩阵 \( \mathbf{K} \) 转换：
\[
\begin{bmatrix} u \\ v \\ 1 \end{bmatrix} = 
\mathbf{K} \begin{bmatrix} x \\ y \\ 1 \end{bmatrix}, \quad 
\mathbf{K} = \begin{bmatrix} f_x & \gamma & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix}
\]

---

### **1.8 为什么不能直接用矩阵乘法消除 \( Z \)?**
#### **(1) 尝试构造“消除 \( Z \)”的矩阵**
假设我们强行构造一个 \( 4 \times 4 \) 矩阵 \( \mathbf{M} \)，试图通过一次乘法完成归一化：
\[
\mathbf{M} = \begin{bmatrix} 
1 & 0 & 0 & 0 \\ 
0 & 1 & 0 & 0 \\ 
0 & 0 & 1 & 0 \\ 
0 & 0 & 1 & 0 
\end{bmatrix}
\]
- 作用效果：
  \[
  \mathbf{M} \begin{bmatrix} X_c \\ Y_c \\ Z_c \\ 1 \end{bmatrix} = 
  \begin{bmatrix} X_c \\ Y_c \\ Z_c \\ Z_c \end{bmatrix}
  \]
- 齐次坐标归一化后：
  \[
  \begin{bmatrix} X_c/Z_c \\ Y_c/Z_c \\ Z_c/Z_c \\ 1 \end{bmatrix} = 
  \begin{bmatrix} x \\ y \\ 1 \\ 1 \end{bmatrix}
  \]
- **问题**：这种构造虽然形式上实现了归一化，但破坏了矩阵乘法的线性性质，且无法与后续内参矩阵 \( \mathbf{K} \) 直接相乘（因为 \( \mathbf{K} \) 是 \( 3 \times 3 \)）。

#### **(2) 根本原因**
- **非线性性**：透视除法（除以 \( Z_c \)）是一个非线性操作，无法通过单一的线性矩阵乘法实现。
- **投影矩阵的局限性**：在计算机图形学中，投影矩阵（如OpenGL的透视矩阵）仅能通过将 \( Z_c \) 映射到裁剪空间，但不能消除它对 \( (x, y) \) 的影响。

---

### **1.9 综合公式（完整流程）**
将世界坐标 \( \mathbf{P}_w \) 转换到像素坐标 \( \mathbf{p} \) 的完整步骤：
\[
\mathbf{p} = \mathbf{K} \cdot 
\underbrace{
\begin{bmatrix} 
\frac{X_c}{Z_c} \\ 
\frac{Y_c}{Z_c} \\ 
1 
\end{bmatrix}
}_{\text{归一化坐标}}
= \mathbf{K} \cdot 
\left( 
\underbrace{
\begin{bmatrix} 
\mathbf{R} & \mathbf{t} \\ 
\mathbf{0} & 1 
\end{bmatrix}
\mathbf{P}_w
}_{\text{相机坐标}}
\right)_{1:3} \Big/ Z_c
\]
- 其中 \( (\cdot)_{1:3} \) 表示取前三个分量（\( X_c, Y_c, Z_c \)）。

---

### **1.10 特殊情况讨论**
#### **(1) 正交投影（无透视效应）**
- 当相机为 **正交投影模型** 时，\( Z_c \) 不影响 \( (x, y) \) 坐标。此时投影矩阵直接丢弃 \( Z_c \)：
  \[
  \begin{bmatrix} x \\ y \end{bmatrix} = 
  \begin{bmatrix} f_x & 0 & c_x \\ 0 & f_y & c_y \end{bmatrix}
  \begin{bmatrix} X_c \\ Y_c \\ 1 \end{bmatrix}
  \]
- 但正交投影不适用于大多数真实相机（仅用于CAD等场景）。

#### **(2) 深度信息的保留**
- 若需要保留 \( Z_c \)（如深度图生成），可在归一化后单独存储：
  \[
  \text{深度} = Z_c \quad \text{或} \quad \text{归一化深度} = \frac{Z_c - Z_{\text{min}}}{Z_{\text{max}} - Z_{\text{min}}}
  \]

---

### **1.11 代码验证（Python示例）**
```python
import numpy as np

# 定义外参和内参
R = np.eye(3)  # 无旋转
t = np.array([0, 0, 1])  # 相机向后退1单位
K = np.array([[500, 0, 320], [0, 500, 240], [0, 0, 1]])  # 内参

# 世界坐标 (X_w, Y_w, Z_w)
P_w = np.array([1, 0, 2, 1])  # 齐次坐标

# 世界 → 相机坐标
T_cw = np.vstack((np.hstack((R, t.reshape(3,1))), [0, 0, 0, 1]))
P_c = T_cw @ P_w  # X_c=1, Y_c=0, Z_c=3

# 归一化并转换到像素坐标
x, y = P_c[0]/P_c[2], P_c[1]/P_c[2]
u = K[0,0] * x + K[0,2]  # 500*(1/3) + 320 ≈ 486.67
v = K[1,1] * y + K[1,2]  # 500*(0/3) + 240 = 240
```

---

### **总结**
- **必须显式归一化**：无法通过单一的矩阵乘法消除 \( Z_c \) 的影响，必须通过除以 \( Z_c \) 完成透视除法。
- **数学本质**：透视投影是非线性的，而矩阵乘法是线性的，二者无法完全等价。
- **实际应用**：在SLAM、三维重建中，归一化后的坐标 \( (x, y) \) 和深度 \( Z_c \) 通常分开处理。