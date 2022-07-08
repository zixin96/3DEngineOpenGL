#version 460 core

struct GSOut
{
    vec3 barycoords;
};

layout (location = 0) in GSOut gsOut;

layout (location=0) out vec4 out_FragColor;

layout (binding = 0) uniform sampler2D texture0;

float edgeFactor(float thickness)
{
	vec3 a3 = smoothstep( vec3( 0.0 ), fwidth(gsOut.barycoords) * thickness, gsOut.barycoords);
	return min( min( a3.x, a3.y ), a3.z );
}

void main()
{
	vec4 color = vec4(1.0);
	out_FragColor = mix( color * vec4(0.8), color, edgeFactor(1.0) );
};
