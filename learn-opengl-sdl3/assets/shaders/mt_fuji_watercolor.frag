// https://www.shadertoy.com/view/f3lSRH
// Mt. Fuji Watercolor
// by noztol

#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform float uTime;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return value;
}

// Organic Calligraphy Brush Stroke (Guarantees A -> B connection)
float brushStroke(vec2 p, vec2 a, vec2 b, float rStart, float rEnd, float droop) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    

    vec2 pBended = p;
    pBended.y += sin(h * 3.14159) * droop;
    
    // Recalculate distance from the newly bended space
    pa = pBended - a;
    float d = length(pa - ba * h);
    float taper = smoothstep(1.0, 0.0, h);
    float r = mix(rEnd, rStart, taper);
    
    return d - r - (fbm(p * 60.0) * 0.005);
}


void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 iResolution = uResolution;
    float iTime = uTime;
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // animation times
    float t = iTime * 0.8; 
    float t_sun     = clamp((t - 0.5) / 1.5, 0.0, 1.0); 
    float t_mnt     = clamp((t - 2.0) / 2.0, 0.0, 1.0); 
    float t_branch  = clamp((t - 4.0) / 2.0, 0.0, 1.0); 
    float t_flowers = clamp((t - 6.0) / 2.5, 0.0, 1.0); 
    float swayStrength = smoothstep(9.0, 11.0, t) * 0.02;         
    
    // 1. PAPER BACKGROUND
    vec3 col = vec3(0.97, 0.95, 0.92);
    col -= fbm(uv * 15.0) * 0.04; 

    float wetEdgeJitter = fbm(uv * 40.0) * 0.08;

    // 2. THE RED SUN 
    vec2 sunUV = uv - vec2(0.6, 0.3); 
    float sunRadius = 0.15;
    float sunDist = length(sunUV) - sunRadius + fbm(uv * 20.0) * 0.03;
    float sunShape = smoothstep(0.01, -0.01, sunDist);
    float sunReveal = smoothstep(0.02, -0.02, (sunUV.x + sunUV.y) - (t_sun * 0.6 - 0.3) + wetEdgeJitter);
    
    sunShape *= (fbm(uv * 50.0) * 0.5 + 0.5) * sunReveal;
    col = mix(col, vec3(0.8, 0.15, 0.15), clamp(sunShape * 0.9, 0.0, 1.0));

    // 3. MOUNTAIN
    vec2 mntUV = uv;
    mntUV.x += fbm(uv * 10.0) * 0.05;
    mntUV.y += fbm(uv * 15.0) * 0.03;
    
    float fujiX = mntUV.x + 0.05; 
    float mountainShape = -0.1 - pow(abs(fujiX), 0.8) * 0.6; 
    float mountainMask = smoothstep(0.0, 0.02, mountainShape - mntUV.y);
    
    float outlineDist = abs(mountainShape - mntUV.y);
    float mntOutline = smoothstep(0.01, 0.002, outlineDist) * smoothstep(0.0, 0.05, mntUV.y + 0.15);
    float ridgeTexture = fbm(vec2(mntUV.x * 25.0, mntUV.y * 5.0)); 
    float lightSide = smoothstep(-0.15, 0.25, fujiX + ridgeTexture * 0.1);
    
    vec3 deepBlackInk = vec3(0.04, 0.04, 0.05);
    vec3 grayWaterWash = vec3(0.6, 0.6, 0.65);
    vec3 baseMntColor = mix(deepBlackInk, grayWaterWash, lightSide);
    baseMntColor *= (0.6 + 0.4 * fbm(uv * 40.0)); 

    float snowLine = -0.22 + abs(fujiX) * 0.5 + (fbm(uv * 30.0) * 0.04);
    float snowCap = smoothstep(snowLine - 0.01, snowLine + 0.01, mntUV.y);
    vec3 mountainColor = mix(baseMntColor, vec3(0.98, 0.99, 1.0), snowCap);
    mountainColor = mix(mountainColor, vec3(0.02), mntOutline); 

    float mntReveal = smoothstep(0.05, -0.05, mntUV.x - (t_mnt * 2.0 - 1.0) + wetEdgeJitter);
    col = mix(col, mountainColor, mountainMask * mntReveal);

    // 4. BRANCHES
    vec2 branchUV = uv;
    branchUV.x += sin(iTime * 1.5 + uv.y * 3.0) * swayStrength;
    branchUV.y += cos(iTime * 1.2 + uv.x * 2.0) * swayStrength;

    vec2 n0 = vec2(-1.0, -0.2);  // Root
    vec2 n1 = vec2(-0.6,  0.0);  // Joint 1
    vec2 n2 = vec2(-0.2,  0.1);  // Joint 2
    

    vec2 t1 = vec2( 0.2,  0.15); // Tip 1 (Main sweeping end)
    vec2 t2 = vec2(-0.2, -0.15); // Tip 2 (Lower drop)
    vec2 t3 = vec2(-0.0,  0.3);  // Tip 3 (Upper shoot)
    vec2 t4 = vec2(-0.6, -0.25); // Tip 4 (Inner drop)
    vec2 t5 = vec2(-0.1, -0.0);  // Tip 5 (Forward drop)

    float bd = 10.0;
    
    // Main Trunk (droops downward slightly using the 0.03 bend parameter)
    bd = min(bd, brushStroke(branchUV, n0, n1, 0.020, 0.015, 0.03));
    bd = min(bd, brushStroke(branchUV, n1, n2, 0.015, 0.010, 0.03));
    bd = min(bd, brushStroke(branchUV, n2, t1, 0.010, 0.003, 0.02)); // Connects perfectly to t1
    
    // Lower Sub Branch 1
    vec2 s1_mid = vec2(-0.4, -0.05);
    bd = min(bd, brushStroke(branchUV, n1, s1_mid, 0.00, 0.006, -0.02)); 
    bd = min(bd, brushStroke(branchUV, s1_mid, t2, 0.006, 0.002, 0.01)); // Connects perfectly to t2
    
    // Upper Sub Branch 2
    bd = min(bd, brushStroke(branchUV, n2, t3, 0.01, 0.009, -0.02));    // Connects perfectly to t3
    
    // Inner Sub Branch 3
    vec2 s3_start = mix(n0, n1, 0.2);
    bd = min(bd, brushStroke(branchUV, s3_start, t4, 0.008, 0.002, 0.01)); // Connects perfectly to t4
    
    // Forward Sub Branch 4
    vec2 s4_start = mix(n1, n2, 0.9);
    bd = min(bd, brushStroke(branchUV, s4_start, t5, 0.006, 0.002, 0.01)); // Connects perfectly to t5

    float branchMask = smoothstep(0.004, -0.002, bd);
    float branchReveal = smoothstep(0.05, -0.05, branchUV.x - (t_branch * 1.5 - 1.0) + wetEdgeJitter);
    col = mix(col, vec3(0.04, 0.03, 0.03), branchMask * branchReveal);

    // 5. FLOWERS 
    float flowerMask = 0.0;
    vec2 targets[5];
    targets[0] = t1; targets[1] = t2; targets[2] = t3; 
    targets[3] = t4; targets[4] = t5; 
    
    for(int i = 0; i < 5; i++) {
        float individualTime = clamp((t_flowers - float(i)*0.2) * 5.0, 0.0, 1.0);
        
        if (individualTime > 0.0) {
            vec2 center = targets[i];
            
            // Draw a cluster of 3 textured splotches exactly on the tip
            for(float j = 0.0; j < 3.0; j++) {
                vec2 offset = vec2(noise(vec2(float(i), j*10.0)) - 0.5, noise(vec2(float(i), j*20.0)) - 0.5) * 0.03;
                vec2 fpos = center + offset;
                
                float currentRadius = mix(0.0, 0.012, smoothstep(0.0, 1.0, individualTime));
                float fdist = length(branchUV - fpos) - currentRadius - fbm(branchUV * 70.0 + float(i)*j) * 0.02;
                
                flowerMask = max(flowerMask, smoothstep(0.008, -0.005, fdist));
            }
        }
    }
    
    col = mix(col, vec3(0.85, 0.1, 0.2), flowerMask);

    FragColor = vec4(col, 1.0);
}