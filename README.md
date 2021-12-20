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

- 关于法线贴图，使用世界空间下的三角形顶点坐标计算切线方向，和法线方向计算副切线方向，然后在vert阶段构造切线空间到世界空间的转换矩阵，再光栅化插值（插值过程中会造成矩阵的模不为1，但从减少计算量和结果差异不明显的角度来说，可以接受在vert阶段构造矩阵）。

  [怎样计算模型的顶点切线？_porry20009_新浪博客 (sina.com.cn)](http://blog.sina.com.cn/s/blog_15ff6002b0102y8b9.html)

- 

![](https://github.com/HeyCat-svg/my-soft-raster/blob/92d415880f759543f294c9a8ca4416ac91f5b8c9/img/Snipaste_2021-12-16_11-55-35.png)

<p align="center" style="color:grey">blin phong no normal map</p>

![](https://github.com/HeyCat-svg/my-soft-raster/blob/07c3a312ee512141681e8278b758dfdd4f44bbe6/img/Snipaste_2021-12-19_15-05-47.png)

<p align="center" style="color:grey">with normal map</p>

![](https://github.com/HeyCat-svg/my-soft-raster/blob/971ccfbfc4da6237e2afab23f377dd890c24f78a/img/shadow_map.png)

<p align="center" style="color:grey">shadow map</p>

![](https://github.com/HeyCat-svg/my-soft-raster/blob/971ccfbfc4da6237e2afab23f377dd890c24f78a/img/no_shadow.png)

<p align="center" style="color:grey">no shadow</p>

![](https://github.com/HeyCat-svg/my-soft-raster/blob/971ccfbfc4da6237e2afab23f377dd890c24f78a/img/with_shadow.png)

<p align="center" style="color:grey">with shadow</p>

