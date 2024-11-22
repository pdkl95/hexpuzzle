#version 100

#ifdef GL_ES
precision mediump float;
#define in
#endif

// Input vertex attributes (from vertex shader)
varying in vec2 fragTexCoord;
varying in vec4 fragColor;

// Input uniform values
uniform vec2 resolution;
uniform float time;
uniform vec4 fade;

// Output fragment color
//out vec4 finalColor;

#define TAU 6.283185307179586

#define saturate(V) clamp(V, 0.0, 1.0)

vec3 hue2rgb(const in float hue) {
    float R = abs(hue * 6.0 - 3.0) - 1.0;
    float G = 2.0 - abs(hue * 6.0 - 2.0);
    float B = 2.0 - abs(hue * 6.0 - 4.0);
    return saturate(vec3(R,G,B));
}

vec3 hsv2rgb(const in vec3 hsv) {
    return ((hue2rgb(hsv.x) - 1.0) * hsv.y + 1.0) * hsv.z;
}

void main()
{
    float px = 1.0/resolution.y;
    float aspect = resolution.y/resolution.x;

    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 position = (uv * 2.0) - 1.0;

    if (aspect < 1.0) {
        aspect = 1.0 / aspect;
        position.x *= aspect;
    } else {
        position.y *= aspect;
    }

    float dist_center = distance(vec2(0.0), position);
    float radial_wave = (sin((3.333 * time) - (10.0 * dist_center)) + 1.0) / 2.0;

    vec3 color = vec3(0);

    float tile_radius    = fragColor.g;
    float perlin_noise   = fragColor.b;
    float path_highlight = fragColor.r;
    float extra_mag      = fragColor.a;

    float spin_fade = 1.0;
    float saturate = 0.0;
    if (path_highlight > 0.5) {
        float theta = atan(position.y/ position.x);
        float spin = (theta * TAU) + (time * 3.57) - (dist_center * TAU * 2.0);
        spin_fade = (sin(spin) + 1.0) * 0.5 + 0.3;
        if (spin_fade > 1.0) {
            saturate = fract(spin_fade);
            spin_fade = 1.0;
        }
    }

    float hue = fade.y;
    float fade_in_override = fade.z;
    saturate *= fade_in_override;

    float wave_mix = (sin(1.0 * time) + 1.0) / 2.0;

    float wave = (radial_wave * 0.5) + (perlin_noise * 0.5);
    wave = mix(wave, radial_wave, wave_mix);

    float base_wave = wave;
    wave *= spin_fade;

    vec3 hsv = vec3(hue, 0.7, clamp(wave, 0.0, 1.0));
    color = hsv2rgb(hsv);

    vec3 blank_color = vec3(0.19607843137254902);
    float spin_extra = (1.0 - spin_fade) * fade_in_override;
    color = mix(blank_color, color, fade_in_override + spin_extra);

    vec3 saturate_color = vec3(1.0);
    color = mix(color, saturate_color, min(saturate, (1.0-base_wave)));

    color = mix(color, vec3(1.0), extra_mag * 0.333);

    gl_FragColor = clamp(vec4(color, 1.0), 0.0, 1.0);
}
