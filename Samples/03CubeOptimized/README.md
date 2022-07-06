# 03 Cube Optimized

This demo improves upon 02 Cube by uploading the entire per-frame data once and bind the correct instance before each draw call. In more complicated real-world use cases, this approach is more desirable. 

## Points of Interest

- Uniform buffer offset has an alignment requirement of 256 bytes (for my machine). Thus, we pad the `PerFrameData` struct to 256 bytes. 