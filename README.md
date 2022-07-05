# 3D OpenGL Engine



## Getting Started

Copy `vendor/src/bistro/PatchedMaterials/XXX.mtl` into `vendor/src/bistro/Exterior/XXX.mtl` and `vendor/src/bistro/Interior/XXX.mtl`. Overwrite if necessary (to fix the materials)



`cd build`

`cmake .. -G "Visual Studio 17 2022" -A x64`

## Points of Interest

- `#!/usr/bin/python3` is a shebang line. For unix based OS, this line defines where the interpreter is located such that we can run the script using `./script.py`. Otherwise, we need to use `python3 script.py`. On the other hand, Windows has its own rules for figuring out how to run the script (using filename extensions). So the shebang line has no effects in this case. https://stackoverflow.com/q/7670303/13795171

- `${CMAKE_SOURCE_DIR}` vs `${CMAKE_CURRENT_SOURCE_DIR}`: `CMAKE_SOURCE_DIR` is where cmake was originally invoked, and `CMAKE_CURRENT_SOURCE_DIR` is where cmake is currently working.

---

```glsl
layout (std140, binding = 0) uniform PerFrameData
{
	uniform mat4 mvp;
	uniform int isWireFrame;
}  // Notice here: missing semicolon! nothing shows up on screen
```

---



## Features

- Direct-State-Access (DSA)



## Samples

| Name                                                         | Screenshot                                                   | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 01HelloWorld                                                 |                                                              | "Hello World"                                                |
| 02HelloTriangle                                              |                                                              |                                                              |
| SSAO                                                         |                                                              |                                                              |
| [Shadow Mapping](https://github.com/zixin96/d3d12book/blob/master/Chapter%2020%20Shadow%20Mapping/Shadows) | ![](https://github.com/zixin96/d3d12book/blob/master/Chapter%2020%20Shadow%20Mapping/Shadows/images/demo.gif) | This demo shows a basic implementation of the shadow mapping algorithm. |
