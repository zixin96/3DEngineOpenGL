#version 460 core

// rendering a full screen quad
// this must be used with glDrawArrays(GL_TRIANGLES, 0, 6)

layout (location = 0) out vec2 uv;

void main()
{
    //? is % (modulus) supported? 
	float u = float( ((uint(gl_VertexID) + 2u) / 3u) % 2u );
	float v = float( ((uint(gl_VertexID) + 1u) / 3u) % 2u );

	gl_Position = vec4(-1.0 + u * 2.0, -1.0 + v * 2.0, 0.0, 1.0);
	uv = vec2(u, v);
}
