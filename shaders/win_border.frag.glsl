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

    vec3 color = fragColor.rgb;
    float alpha = 1.0;

    float wave_margin = 0.03;
    float wave_start = smoothstep(min(dist_center - wave_margin, 0.0),
                                  dist_center + wave_margin,
                                  fade.x);
    wave_margin *= 0.5;
    float wave_end = smoothstep(min(dist_center - wave_margin, 0.0),
                                dist_center + wave_margin,
                                1.0 - fade.y);

    float wave_total = clamp(wave_start /*- wave_end*/, 0.0, 1.0);
    color *= wave_total;

    finalColor = clamp(vec4(color, alpha), 0.0, 1.0);
}
