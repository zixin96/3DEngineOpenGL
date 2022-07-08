#version 460 core

layout (std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
    mat4 model;
	vec4 cameraPos;
};

layout (location = 0) in vec3 pos;

void main()
{
    mat4 MVP = proj * view * model;
    gl_Position = MVP * vec4(pos, 1.0);
}

