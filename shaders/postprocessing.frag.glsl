#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 effect_amount;
uniform float time;
uniform vec2 resolution;

// Output fragment color
out vec4 finalColor;

float bloom_amount;
float distort_amount;

// NOTE: Add here your custom variables

const vec2 size = vec2(800, 450);   // Framebuffer size
const float samples = 5.0;          // Pixels per axis; higher = bigger glow, worse performance
const float quality = 2.5;          // Defines size factor: Lower = smaller glow, better quality

float lengthSq(in vec2 v) { return dot(v, v); }
float lengthSq(in vec3 v) { return dot(v, v); }
float lengthSq(in vec4 v) { return dot(v, v); }

vec2 barrel_coord(vec2 st, float amt, float dist) {
    vec2 dist_st = st + (st-.5) * dist * amt;
    return mix(st, dist_st, sqrt(distort_amount));
}

vec2 barrel_coord(vec2 st, float amt) {
    return barrel_coord(st, amt, lengthSq(st-.5));
}

vec4 barrel(in vec2 st, float dist) {
    vec4 a1 = texture(texture0, barrel_coord(st, .0, dist));
    vec4 a2 = texture(texture0, barrel_coord(st, .2, dist));
    vec4 a3 = texture(texture0, barrel_coord(st, .4, dist));
    vec4 a4 = texture(texture0, barrel_coord(st, .6, dist));
    return (a1+a2+a3+a4)/4.;
/*
    vec4 a5 = texture(texture0, barrel_coord(st, .8, dist));
    vec4 a6 = texture(texture0, barrel_coord(st, 1.0, dist));
    vec4 a7 = texture(texture0, barrel_coord(st, 1.2, dist));
    vec4 a8 = texture(texture0, barrel_coord(st, 1.4, dist));
#ifdef BARREL_OCT_2
    return (a1+a2+a3+a4+a5+a6+a7+a8)/8.;
#endif
    vec4 a9 = texture(texture0, barrel_coord(st, 1.6, dist));
    vec4 a10 = texture(texture0, barrel_coord(st, 1.8, dist));
    vec4 a11 = texture(texture0, barrel_coord(st, 2.0, dist));
    vec4 a12 = texture(texture0, barrel_coord(st, 2.2, dist));
    return (a1+a2+a3+a4+a5+a6+a7+a8+a9+a10+a11+a12)/12.;*/
}

vec4 barrel(in vec2 st, in vec2 dist) {
    return barrel(st, dot(vec2(.5), dist));
}

vec4 barrel(in vec2 st) {
    return barrel(st, lengthSq(st-.5));
}

vec4 get_texel(vec2 coord)
{
    // if (distort_amount > 0.9) {
    //     return barrel(coord);
    // } else {
    //     return texture(texture0, coord);
    // }

    return mix(texture(texture0, coord), barrel(coord), distort_amount);
}

void main()
{
    bloom_amount = effect_amount.x;
    distort_amount = effect_amount.y;

    vec4 sum = vec4(0);
    vec2 sizeFactor = vec2(1)/size*quality;

    vec2 tex_coord = fragTexCoord;

    // Texel color fetching from texture sampler
    vec4 source = get_texel(tex_coord);

    const int range = 3;            // should be = (samples - 1)/2;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec2 coord = tex_coord + vec2(x, y)*sizeFactor;
            sum += get_texel(coord);
        }
    }

    vec4 blur = sum/(samples*samples);

    blur = pow(blur, vec4(2.0));

    blur *= bloom_amount;

    // Calculate final fragment color
    finalColor = blur + source; //*colDiffuse;
}
