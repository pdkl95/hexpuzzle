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
uniform vec4 effect_amount1;
uniform vec4 effect_amount2;

float bloom_amount;
float distort_amount;
float warp_amount;
float do_hue_override;
float extra_rotate_level;
float wave_amount;

// Output fragment color
//out vec4 finalColor;

#define TAU 6.283185307179586

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    bloom_amount = effect_amount1.x;
    distort_amount = effect_amount1.y;
    warp_amount = effect_amount1.z;
    do_hue_override = effect_amount1.w;
    extra_rotate_level = effect_amount2.x;
    wave_amount = effect_amount2.y;

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

#define RADIALWAVE(speed, radius_scale, offset)                    \
    ((sin(((speed) * time) - ((radius_scale) * dist_center) + (offset)) + 1.0) / 2.0)

    float radial_wave = RADIALWAVE(3.333, 10.0, 0.0);

    vec3 color = vec3(0);
    float alpha = 1.0;

    float path_highlight = fragColor.r;
    float hue_override   = fragColor.g;
    float perlin_noise   = fragColor.b;
    float hidden         = fragColor.a;

    wave_amount = max(wave_amount, hue_override);

    float spin_fade = 1.0;
    float saturate = 0.0;
    if (path_highlight > 0.5) {
        float theta = atan(position.y/ position.x);
        theta += extra_rotate_level;
        float spin = (theta * TAU) + (time * 3.57) - (dist_center * TAU * 2.0);
        spin_fade = (sin(spin) + 1.0) * 0.5 + 0.3;
        if (spin_fade > 1.0) {
            saturate = fract(spin_fade);
            spin_fade = 1.0;
        }
    }

    float hue = fade.y;
#ifdef EXTRA_RAINBOW
    hue += spin_fade + perlin_noise;
    hue = mod(hue, 1.0);
#endif

    float fade_in_override = fade.z;
    saturate *= fade_in_override;
#define WAVE_MIX_SPEED  1.0
    float wave_mix = (sin(WAVE_MIX_SPEED * time) + 1.0) / 2.0;

    float wave = (radial_wave * 0.5) + (perlin_noise * 0.5);
    wave = mix(wave, radial_wave, wave_mix);

    float base_wave = wave;
    wave *= spin_fade;
    wave *= wave_amount;

    vec3 hsv = vec3(hue, 0.7, clamp(wave, 0.0, 1.0));
    color = hsv2rgb(hsv);

    vec3 blank_color = vec3(0.19607843137254902);
    float spin_extra = (1.0 - spin_fade) * fade_in_override;

    float total_wave = fade_in_override + (spin_extra * wave_amount);

    if (hidden > 0.5) {
        float hidden_tmp;
        float hidden_mix = 0.0;
        float hidden_wave = 0.0;

#define HWAVE(speed, radius_scale, offset)                              \
    hidden_tmp = RADIALWAVE(speed, radius_scale, offset);               \
    hidden_mix += hidden_tmp;                                           \
    hidden_wave = max(hidden_wave, smoothstep(0.94, 0.97, hidden_tmp));

        HWAVE(3.333, 10.0, 0.0);
        hidden_mix *= 0.333333333;

        hidden_wave *= fade_in_override;

        hidden_mix  *= wave_amount;
        hidden_wave *= wave_amount;

        color = mix(color, vec3(0.7333333333333333, 1.0, 0.9568627450980393), hidden_mix);

        color = mix(vec3(0.0), color, hidden_wave);
        alpha = hidden_wave * (1.1 - dist_center) * bloom_amount;

    } else {
        color = mix(blank_color, color, total_wave);
    }

    vec3 saturate_color = vec3(1.0);
    float saturate_mix = clamp(min(saturate, (1.0 - base_wave)), 0.0, 1.0);
    color = mix(color, saturate_color, saturate_mix * wave_amount);

    if (do_hue_override > 0.6 && hue_override != 0.0) {
        vec3 override_color = hsv2rgb(vec3(hue_override, 1.0, 1.0));
        float override_mix = 1.0 - bloom_amount;
        override_mix = mix(override_mix, 1.0, wave_amount);
        color = mix(color, override_color, override_mix);
    }

    //color = vec3(path_highlight);
    //color = vec3(perlin_noise);
    //color = vec3(total_wave-1.0);
    //color = vec3(spin_fade-0.0);

    gl_FragColor = clamp(vec4(color, alpha), 0.0, 1.0);
}
