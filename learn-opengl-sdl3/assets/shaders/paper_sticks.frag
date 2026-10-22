// https://www.shadertoy.com/view/sK3WV
/*
Original work
http://57.media.tumblr.com/5ca79532ad296fcdc07bb48209c16dea/tumblr_o32r02CWzb1tcuj64o1_400.gif
*/
#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform float uTime;
uniform vec2 uMouse;

#define PI 3.14159265359

float easeInOutExpo(float t, float b, float c, float d) {
	t /= d/2.0;
	if (t < 1.0) return c/2.0 * pow( 2.0, 10.0 * (t - 1.0) ) + b;
	t--;
	return c/2.0 * ( -pow( 2.0, -10.0 * t) + 2.0 ) + b;
}

/**
 * Draw a rectangle at vec2 `pos` with width `width`, height `height` and color `color`.
 * @author jonobr1 / http://jonobr1.com/
 */
vec4 rectangle(vec2 uv, vec2 pos, float width, float height, vec3 color) {
	float t = 0.0;
	if ((uv.x > pos.x) && (uv.x < pos.x + width)
		&& (uv.y > pos.y) && (uv.y < pos.y + height)) {
		t = 1.0;
	}
	return vec4(color, t);
}

vec3 getColor(float frame, float frequency) {
    vec3 color1 = vec3(0.86, 0.86, 0.82);
    vec3 color2 = vec3(0.15, 0.15, 0.15);
    
    if(cos(frame*2.0+frequency+PI/2.0)>0.0) return color1;
    return color2;
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 iResolution = uResolution;
    float iTime = uTime;
    vec2 iMouse = uMouse;
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
	uv = fragCoord.xy / iResolution.xy;
	vec2 center = vec2(0.5, 0.1);
    const int total = 8;//number of lines
    
    float frame = iTime;
    if(iMouse.x>0.0) frame *= 5.0 * (iMouse.x / iResolution.x);//velocity
    
    
    vec4 color = vec4(getColor(frame, PI), 1.0);//bg
    vec3 rectColor = getColor(frame, 0.0);
    
    float freqDecay = 2.0;
        
    float space = 0.05;
    float size = 0.7;
//*Vertical Lines
    for(int i=0; i<total; i++){
        float freq = (1.0-float(i)/float(total))/freqDecay;
        float x = center.x + space*float(i-total/2);
    	float y = center.y;
        float height = size * easeInOutExpo(((cos((frame+freq)*2.0)+1.0)/2.0), 0.0, 1.0, 1.0);
    	if(cos(frame+freq+PI/2.0)<0.0) y += size-height;;
        height += 0.035;
        float width = 0.02;
	    
        vec4 rect = rectangle(uv, vec2(x, y), width, height, rectColor);
	    color = mix(color, rect, rect.a);
    }
/**/
//*Horizontal Lines
    frame += PI/2.0;
    space = 0.08;
	center = vec2(0.25, 0.5);
    size = 0.45;
    for(int i=0; i<total; i++){
        float freq = (1.0-float(i)/float(total))/freqDecay;
        float x = center.x;
        float width = size * easeInOutExpo(((cos((frame+freq)*2.0)+1.0)/2.0), 0.0, 1.0, 1.0);
	    if(cos(frame+freq+PI/2.0)<0.0) x += size - width;
    	float y = center.y + space*float(i-total/2);
        float height = 0.035;
    	width += 0.021;
        
        vec4 rect = rectangle(uv, vec2(x, y), width, height, rectColor);
	    color = mix(color, rect, rect.a);
    }
/**/
	FragColor = color;
}