# 3D OpenGL Engine



## Getting Started

Copy `vendor/src/bistro/PatchedMaterials/XXX.mtl` into `vendor/src/bistro/Exterior/XXX.mtl` and `vendor/src/bistro/Interior/XXX.mtl`. Overwrite if necessary (to fix the materials)

## Points of Interest

- `#!/usr/bin/python3` is a shebang line. For unix based OS, this line defines where the interpreter is located such that we can run the script using `./script.py`. Otherwise, we need to use `python3 script.py`. On the other hand, Windows has its own rules for figuring out how to run the script (using filename extensions). So the shebang line has no effects in this case. https://stackoverflow.com/q/7670303/13795171

- `${CMAKE_SOURCE_DIR}` vs `${CMAKE_CURRENT_SOURCE_DIR}`: `CMAKE_SOURCE_DIR` is where cmake was originally invoked, and `CMAKE_CURRENT_SOURCE_DIR` is where cmake is currently working.
