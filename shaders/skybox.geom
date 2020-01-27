#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in int index[];

void main(void) {
	gl_Position = vec4(-1, 1, 0, 1);
	gl_Layer = index[0];
	EmitVertex();
	gl_Position = vec4(-1, -1, 0, 1);
	gl_Layer = index[0];
	EmitVertex();
	gl_Position = vec4(1, 1, 0, 1);
	gl_Layer = index[0];
	EmitVertex();
	gl_Position = vec4(1, -1, 0, 1);
	gl_Layer = index[0];
	EmitVertex();
	
	EndPrimitive();
}
