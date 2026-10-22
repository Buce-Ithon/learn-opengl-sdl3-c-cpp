// https://www.shadertoy.com/view/Ndc3zl
// up in the cloud sea
// by mdb 
#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform sampler2D iChannel0;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution.xy;
    vec3 col = texture(iChannel0, uv).rgb;
    col *= 0.5 + 0.5 * pow(
        16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y),
        0.2
    );
    FragColor = vec4(col, 1.0);
}
