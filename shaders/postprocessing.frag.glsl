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
float warp_amount;

// NOTE: Add here your custom variables

const vec2 size = vec2(800, 450);   // Framebuffer size
const float samples = 5.0;          // Pixels per axis; higher = bigger glow, worse performance
const float quality = 2.5;          // Defines size factor: Lower = smaller glow, better quality

float lengthSq(in vec2 v) { return dot(v, v); }
float lengthSq(in vec3 v) { return dot(v, v); }
float lengthSq(in vec4 v) { return dot(v, v); }


vec2 hash( in vec2 x )
{
    vec2 k = vec2( 0.3183099, 0.3678794 );
    x = x*k + k.yx;
    return -1.0 + 2.0*fract( 16.0 * k*fract( x.x*x.y*(x.x+x.y)) );
}

vec3 noised( in vec2 p )
{
    vec2 i = floor( p );
    vec2 f = fract( p );

    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);

    vec2 ga = hash( i + vec2(0.0,0.0) );
    vec2 gb = hash( i + vec2(1.0,0.0) );
    vec2 gc = hash( i + vec2(0.0,1.0) );
    vec2 gd = hash( i + vec2(1.0,1.0) );

    float va = dot( ga, f - vec2(0.0,0.0) );
    float vb = dot( gb, f - vec2(1.0,0.0) );
    float vc = dot( gc, f - vec2(0.0,1.0) );
    float vd = dot( gd, f - vec2(1.0,1.0) );

    return vec3( va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),
                 ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +
                 du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));
}

vec2 barrel_coord(vec2 st, float amt, float dist) {
    vec2 dist_st = st + (st-.5) * dist * amt;
    return mix(st, dist_st, distort_amount * 0.7);
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
}

vec4 barrel(in vec2 st, in vec2 dist) {
    return barrel(st, dot(vec2(.5), dist));
}

vec4 barrel(in vec2 st) {
    return barrel(st, lengthSq(st-.5));
}

vec4 get_texel(vec2 coord)
{
    return mix(texture(texture0, coord), barrel(coord, 0.3), distort_amount * 0.3);
    //return texture(texture0, coord);
}

void main()
{
    bloom_amount = effect_amount.x;
    distort_amount = effect_amount.y;
    warp_amount = effect_amount.z;

    vec2 p = 1.1 * fragTexCoord.xy;

    vec3 nz1 = noised( 1.7 * p + vec2(time, 0.0));
    vec3 nz2 = noised( 1.3 * p + vec2(0.0,  time));
    vec3 nz = (nz1 + nz2) * 0.5;

    vec2 dist = (0.03 * (nz.yz+ nz.xx)) * distort_amount * warp_amount;

    //vec2 cPos = -1.0 + 2.0 * fragTexCoord;
    //float cLength = length(cPos);

    //float ripple = cos(cLength*1.85-time*0.7)*0.03;
    //ripple *= pow(distort_amount, 4.0);;

    //vec2 uv = fragTexCoord+(cPos/cLength)*ripple;
    vec2 uv = fragTexCoord + dist;

    vec4 sum = vec4(0);
    vec2 sizeFactor = vec2(1)/size*quality;

    //vec2 st = fragTexCoord;

    // Texel color fetching from texture sampler
    vec4 source = get_texel(uv);

    const int range = 3;            // should be = (samples - 1)/2;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec2 coord = uv + vec2(x, y)*sizeFactor;
            sum += get_texel(coord);
        }
    }

    vec4 blur = sum/(samples*samples);

    blur = pow(blur, vec4(2.0));

    blur *= bloom_amount;

    // Calculate final fragment color
    finalColor = blur + source; //*colDiffuse;
}
