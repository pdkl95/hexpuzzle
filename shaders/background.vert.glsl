#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;
uniform vec2 resolution;
uniform float time;
uniform float warp;

#define SCALE_FACTOR_HIGH 0.075
#define SCALE_FACTOR_LOW  0.125

#define nsin(x) ((sin(x) + 1.0) / 2.0)

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    float aspect = resolution.y/resolution.x;
    vec2 halfres = resolution/2.0;
    vec3 pos = vertexPosition;
    if (aspect < 1.0) {
        aspect = 1.0 / aspect;
        pos.x *= aspect;
    } else {
        pos.y *= aspect;
    }

    float scale_factor_rate = nsin(time * 0.753);
    float scale_factor_mult = mix(SCALE_FACTOR_HIGH, SCALE_FACTOR_LOW, scale_factor_rate);
    float scale = scale_factor_mult * resolution.x;

    vec2 cent_pos = pos.xy - halfres;
    float cent_dist = length(cent_pos);

    cent_dist /= scale;

    pos.z -= pow(cent_dist, 2.0) * warp;

    gl_Position = mvp*vec4(pos, 1.0);
}
