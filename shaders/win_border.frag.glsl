#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform vec2 resolution;
uniform float time;
uniform vec4 fade;

// Output fragment color
out vec4 finalColor;

#define TAU 6.283185307179586

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
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
    float alpha = 1.0;

    float tile_radius  = fragColor.r;
    float perlin_noise = fragColor.b;

    //float progress = fade.x;
    float hue = fade.y;

    //float wave = (sin((2.0 * time) + (tile_radius * TAU)) + 1.0) / 2.0;

    float wave_mix = (sin(1.0 * time) + 1.0) / 2.0;

    float wave = (radial_wave * 0.5) + (perlin_noise * 0.5);
    wave = mix(wave, radial_wave, wave_mix);

    vec3 hsv = vec3(hue, 0.7, clamp(wave, 0.0, 1.0));
    color = hsv2rgb(hsv);

    // float wave_margin = 0.03;
    // float wave_start = smoothstep(min(dist_center - wave_margin, 0.0),
    //                               dist_center + wave_margin,
    //                               fade.x);
    // wave_margin *= 0.5;
    // float wave_end = smoothstep(min(dist_center - wave_margin, 0.0),
    //                             dist_center + wave_margin,
    //                             1.0 - fade.y);

    // float wave_total = clamp(wave_start /*- wave_end*/, 0.0, 1.0);
    // color *= wave_total;

    finalColor = clamp(vec4(color, alpha), 0.0, 1.0);
}
