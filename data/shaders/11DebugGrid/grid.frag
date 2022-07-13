#version 460 core

// the fragment shader calculates a programmatic texture that looks like a grid

#include <data/shaders/11DebugGrid/GridParameters.glsl>
#include <data/shaders/11DebugGrid/GridCalculation.glsl>

layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;

void main()
{
	out_FragColor = gridColor(uv);
};
