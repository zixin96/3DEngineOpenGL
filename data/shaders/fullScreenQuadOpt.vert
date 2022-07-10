#version 460 core

// rendering a fullscreen triangle covering the screen
// this must be used with glDrawArrays(GL_TRIANGLES, 0, 3)

//? Is & (bitwise and) supported? 
//? Is << (bitwise shift) supported? 

layout (location = 0) out vec2 uv;

vec4 fsTrianglePosition(int vtx)
{
	float x = -1.0 + float((vtx & 1) << 2);
	float y = -1.0 + float((vtx & 2) << 1);
	return vec4(x,y,0.0,1.0);
}

vec2 fsTriangleUV(int vtx)
{
	float u = (vtx == 1) ? 2.0 : 0.0; // 0, 2, 0
	float v = (vtx == 2) ? 2.0 : 0.0; // 0, 0, 2
	return vec2(u,v);
}

void main()
{
	gl_Position = fsTrianglePosition(gl_VertexID);
	uv = fsTriangleUV(gl_VertexID);
}
