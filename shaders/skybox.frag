#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(vec3(sin(fract(gl_FragCoord.x/30.0))),0.8);
}
