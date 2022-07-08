#version 460 core

#include <data/shaders/13Tessellation/GridParameters.glsl>
#include <data/shaders/13Tessellation/GridCalculation.glsl>

layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;

void main()
{
	out_FragColor = gridColor(uv);
};
