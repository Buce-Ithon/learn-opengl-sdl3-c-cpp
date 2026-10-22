// https://www.shadertoy.com/view/td3czf
// Infinity loader
// by munrocket
#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform float uTime;

vec2 fragCoord = gl_FragCoord.xy;
vec2 iResolution = uResolution;
float iTime = uTime;


#define M_2_PI 6.28318530

#define SCALE 1.3
#define SPEED 0.5
#define POINTS 15.
#define LENGTH 0.6
#define RADIUS 0.08
#define FADING 0.28
#define GLOW 1.5

// optimized 2d version of https://www.shadertoy.com/view/ldj3Wh
vec2 sdBezier(vec2 pos, vec2 A, vec2 B, vec2 C)
{    
    vec2 a = B - A;
    vec2 b = A - 2.0*B + C;
    vec2 c = a * 2.0;
    vec2 d = A - pos;

    float kk = 1.0 / dot(b,b);
    float kx = kk * dot(a,b);
    float ky = kk * (2.0*dot(a,a)+dot(d,b)) / 3.0;
    float kz = kk * dot(d,a);      

    vec2 res;

    float p = ky - kx*kx;
    float p3 = p*p*p;
    float q = kx*(2.0*kx*kx - 3.0*ky) + kz;
    float h = q*q + 4.0*p3;

    h = sqrt(h);
    vec2 x = (vec2(h, -h) - q) / 2.0;
    vec2 uv = sign(x)*pow(abs(x), vec2(1.0/3.0));
    float t = clamp(uv.x+uv.y-kx, 0.0, 1.0);

    return vec2(length(d+(c+b*t)*t),t);
}

vec2 leminiscate(float t){
    float x = SCALE * cos(t) / (1.0 + sin(t) * sin(t));
    float y = SCALE * sin(t) * cos(t+0.01) / (1.0 + sin(t) * sin(t));
    return vec2(x, y);
}

// inspired by https://www.shadertoy.com/view/wdy3DD
float map(vec2 pos){
    float t = fract(-SPEED * iTime + 0.1);
    float dl = LENGTH / POINTS;
    vec2 p1 = leminiscate(t * M_2_PI);
    vec2 p2 = leminiscate((dl + t) * M_2_PI);
    vec2 c = (p1 + p2) / 2.0;
    float d = 1e9;
    
    for(float i = 2.0; i < POINTS; i++){
        p1 = p2;
        p2 = leminiscate((i * dl + t) * M_2_PI);
        vec2 c_prev = c;
        c = (p1 + p2) / 2.;
        vec2 f = sdBezier(pos, c_prev, p1, c);
        d = min(d, f.x + FADING * (f.y + i) / POINTS);
    }
    return d;
}

void main(){
    vec2 uv = (2.*fragCoord - iResolution.xy) / min(iResolution.x, iResolution.y);
	
    float dist = map(uv);
    
    vec3 col = vec3(0.2, 0.5, 1.0) * pow(RADIUS/dist, GLOW);
    
    FragColor = vec4(col, 1.0);
}