# 简易软光栅
### 软光栅v1.0

使用软光栅完成了blin-phong光照，其中涉及到的知识点有：

- 三角形光栅化（使用屏幕重心坐标扫描三角形的AABB包围盒，判断像素是否在三角形内）

- 透视矫正（使用屏幕重心坐标对3D场景中的属性插值）
  $$
  \frac{1}{z_t}=\alpha\cdot\frac{1}{z_1}+\beta\cdot\frac{1}{z_2}+\gamma\cdot\frac{1}{z_3} \\
  \frac{I_t}{z_t}=\alpha\cdot\frac{I_1}{z_1}+\beta\cdot\frac{I_2}{z_2}+\gamma\cdot\frac{I_3}{z_3} \\
  $$

- view空间坐标乘以proj后，clip空间坐标的w分量是view空间坐标z值的相反数（view空间右手系，z值为负，因此clip空间的w分量是实际z距离）。结合透视矫正公式，则有了经过矫正的3D空间重心坐标：

  ```c++
  vec3 screenBar = Barycentric(screenPts, vec2(x, y));    // 屏幕空间重心坐标
  vec3 clipBar = vec3(screenBar.x / clipPts[0].w, screenBar.y / clipPts[1].w, screenBar.z / clipPts[2].w);	// 分别除以重心坐标对应三角点的view空间z值
  clipBar = clipBar / (clipBar.x + clipBar.y + clipBar.z);	// 乘以zt
  ```

- 其他technique point想到再写

!()[https://github.com/HeyCat-svg/my-soft-raster/blob/92d415880f759543f294c9a8ca4416ac91f5b8c9/img/Snipaste_2021-12-16_11-55-35.png]

