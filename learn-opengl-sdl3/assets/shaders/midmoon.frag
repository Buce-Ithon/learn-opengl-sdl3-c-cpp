#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform float uTime;
uniform float uZoom;
uniform vec2 uViewCenter;
uniform vec3 uMoon; // Scene-space center and animated radius, shared with CPU hit testing.
uniform vec4 uRipples[8]; // xy center, z birth time, w: 0 inactive / 1 sky / 2 sea.

const float TAU = 6.28318530718;
const float HORIZON = -0.12;
const float MOON_RADIUS = 0.155;

float hash12(vec2 p);
vec2 hash22(vec2 p);
float noise(vec2 p);

// Small traveling swells keep the distant sea edge gently in motion.
float horizonHeight(float x, float time)
{
    return HORIZON
         + 0.009 * sin(x * 4.2 - time * 0.42)
         + 0.004 * sin(x * 10.7 + time * 0.63 + 1.4)
         + 0.002 * sin(x * 22.0 - time * 0.85 + 0.6);
}

// Distort the painted scene itself; a faint crest catches light as it travels.
vec3 clickRipples(vec2 uv, float time)
{
    vec2 displacement = vec2(0.0);
    float light = 0.0;
    float skyMask = smoothstep(-0.025, 0.025, uv.y - horizonHeight(uv.x, time));
    for (int i = 0; i < 8; ++i)
    {
        float age = time - uRipples[i].z;
        if (uRipples[i].w < 0.5 || age < 0.0 || age > 4.0) continue;
        bool sea = uRipples[i].w > 1.5;
        // Each click has a stable seed, including repeated clicks at the same spot.
        vec2 seed = hash22(uRipples[i].xy * 17.0 + uRipples[i].z * vec2(3.7, 8.1));
        vec2 phase = seed * TAU;
        vec2 metric = sea ? vec2(1.0, mix(1.9, 2.8, seed.x))
                          : vec2(1.0, mix(0.85, 1.15, seed.x));
        vec2 delta = (uv - uRipples[i].xy) * metric;
        float r = length(delta);
        // Integer angular harmonics close at the atan seam. Smoothly fade their
        // influence at the origin to avoid a pinwheel singularity at the click.
        float angle = atan(delta.y, delta.x + 0.000001);
        float lobes = 0.15 * sin(angle * 3.0 + phase.x + age * 0.18)
                    + 0.09 * sin(angle * 5.0 + phase.y - age * 0.13)
                    + 0.045 * sin(angle * 8.0 + phase.x - phase.y);
        float shape = 1.0 + lobes * smoothstep(0.0, 0.18, r);
        float irregularDistance = r / shape;
        irregularDistance += noise(delta * 7.0 + seed * 31.0)
                           * 0.055 * smoothstep(0.0, 0.16, r);
        float front = irregularDistance - age * mix(0.40, 0.56, seed.y);
        float envelope = exp(-front * front / mix(0.012, 0.025, seed.x)) * exp(-age * 0.8);
        envelope *= smoothstep(0.0, 0.10, age) * (1.0 - smoothstep(3.0, 4.0, age));
        envelope *= sea ? (1.0 - skyMask) : skyMask;
        envelope *= 0.72 + 0.28 * sin(angle * 4.0 + phase.y + r * 5.0);
        float wave = sin(front * mix(45.0, 62.0, seed.x));
        vec2 direction = delta / max(r, 0.001);
        vec2 tangent = vec2(-direction.y, direction.x);
        direction += tangent * sin(angle * 3.0 + phase.x) * 0.22;
        displacement += direction / metric * wave * envelope * 0.032;
        light += max(wave, 0.0) * envelope * 0.045;
    }
    return vec3(displacement, light);
}

// Hash/noise vocabulary inherited from fXlXWN.frag.
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(dot(hash22(i), f), dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
        mix(dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
            dot(hash22(i + vec2(1.0)), f - vec2(1.0)), u.x), u.y);
}

// Rounded, slightly ragged daubs with visible bristle ridges.
// Derivatives keep their edges soft when the framebuffer size changes.
float paintStroke(vec2 local, vec2 id, vec2 halfSize)
{
    local -= 0.5;
    local.y += 0.045 * sin(local.x * 7.0 + hash12(id) * TAU);
    vec2 edge = abs(local) - halfSize;
    float d = length(max(edge, 0.0)) + min(max(edge.x, edge.y), 0.0) - 0.045;
    d += noise(local * vec2(9.0, 17.0) + id) * 0.035;
    float aa = max(fwidth(d), 0.008);
    float body = 1.0 - smoothstep(-aa, aa, d);
    float bristles = 0.88 + 0.12 * sin(local.y * 95.0 + noise(id + local * 6.0) * 4.0);
    return body * bristles;
}

vec3 paintedSky(vec2 uv, vec2 moon, float time)
{
    float altitude = smoothstep(HORIZON, 1.0, uv.y);
    vec3 color = mix(vec3(0.075, 0.19, 0.29), vec3(0.022, 0.045, 0.15), altitude);

    // A broad winding current bends the concentric brushwork into a night sky.
    vec2 q = uv - moon;
    q += vec2(0.065 * sin(uv.y * 5.0 + uv.x * 1.8 + time * 0.055),
              0.10 * sin(uv.x * 2.8 - uv.y * 1.4 - time * 0.045));
    float radius = length(q);
    float angle = atan(q.y, q.x);
    float radial = radius * 43.0;
    float ring = floor(radial);
    // Integer angular counts close each ring cleanly at the atan seam.
    float count = max(8.0, floor((ring + 1.0) * 3.2));
    float phase = hash12(vec2(ring, 7.0));
    float along = (angle / TAU + 0.5) * count + phase * 7.0
                + time * 0.055 * (0.3 + phase);
    vec2 id = vec2(mod(floor(along), count), ring);
    float randomValue = hash12(id + 19.0);
    vec2 local = vec2(fract(along), fract(radial));
    float stroke = paintStroke(local, id, vec2(0.39, 0.29));

    float current = 0.5 + 0.5 * sin(radius * 12.0 - angle * 2.0
                                  + noise(uv * 2.0) * 4.0 - time * 0.07);
    vec3 ink = mix(vec3(0.055, 0.12, 0.32), vec3(0.13, 0.38, 0.59), current);
    ink = mix(ink, vec3(0.29, 0.32, 0.55), randomValue * 0.28);
    ink *= 0.65 + randomValue * 0.65;
    float halo = exp(-max(length(uv - moon) - uMoon.z, 0.0) * 6.5);
    ink = mix(ink, vec3(0.51, 0.61, 0.61), halo * 0.32);
    color = mix(color, ink, stroke * 0.83);

    // A second, long cloud curl gives the left side its own visual movement.
    float ribbon = uv.y - (0.49 + 0.12 * sin(uv.x * 3.0 + time * 0.025));
    float cloud = exp(-ribbon * ribbon * 100.0);
    cloud *= 0.5 + 0.5 * sin(uv.x * 4.0 + 1.0);
    color += vec3(0.07, 0.12, 0.16) * cloud * stroke;
    color += vec3(0.23, 0.22, 0.12) * halo * 0.55;
    return color;
}

vec3 paintedStars(vec2 uv, vec2 moon, float time, vec3 color)
{
    vec2 grid = uv * 6.5;
    vec2 cell = floor(grid);
    vec2 center = 0.25 + hash22(cell + 42.0) * 0.5;
    vec2 delta = (fract(grid) - center) / 6.5;
    float seed = hash12(cell + 13.0);
    // Occupy 28% instead of the original 14% of cells: about 2x as many stars,
    // retaining all existing star positions, sizes and twinkle phases.
    // Fade stars in just above the moving horizon, including the low sky.
    float altitude = uv.y - horizonHeight(uv.x, time);
    float visible = step(0.72, seed) * smoothstep(0.015, 0.09, altitude);
    visible *= smoothstep(0.25, 0.43, length(uv - moon));
    float r = length(delta);
    float size = mix(0.004, 0.010, hash12(cell + 8.0));
    float aa = max(fwidth(r), 0.0005);
    float core = 1.0 - smoothstep(size - aa, size + aa, r);
    float halo = exp(-r * r / (size * size * 14.0));
    float ring = exp(-pow((r - size * 2.4) / (size * 0.45), 2.0));
    float shimmer = 0.88 + 0.12 * sin(time * 0.65 + seed * TAU);
    color += vec3(0.28, 0.36, 0.42) * (halo * 0.35 + ring * 0.17) * visible;
    return mix(color, vec3(0.98, 0.87, 0.56) * shimmer, core * visible * 0.9);
}

vec3 paintedMoon(vec2 uv, vec2 moon, vec3 color)
{
    // Scale the disk and its painted texture together during the spring animation.
    vec2 q = (uv - moon) * (MOON_RADIUS / max(uMoon.z, 0.001));
    float r = length(q);
    float aa = max(fwidth(r), 0.0005);
    float mask = 1.0 - smoothstep(MOON_RADIUS - aa, MOON_RADIUS + aa, r);
    float halo = exp(-max(r - MOON_RADIUS, 0.0) * 28.0);
    color += vec3(0.24, 0.23, 0.12) * halo;

    vec2 grid = q * vec2(65.0, 100.0);
    grid.x += floor(grid.y) * 0.37;
    vec2 id = floor(grid);
    float daub = paintStroke(fract(grid), id, vec2(0.40, 0.32));
    float mottling = smoothstep(-0.18, 0.22, noise(q * 24.0));
    vec3 disk = mix(vec3(1.0, 0.85, 0.48), vec3(1.0, 0.96, 0.74), mottling);
    disk -= vec3(0.06, 0.07, 0.07) * (1.0 - daub);
    return mix(color, disk, mask);
}

vec3 paintedSea(vec2 uv, vec2 moon, float time)
{
    // Geometry must continue beyond the original frame (depth > 1). Clamping
    // this coordinate freezes the stroke rows and stretches them into stripes.
    float depth = max((HORIZON - uv.y) / (HORIZON + 1.0), 0.0);
    float toneDepth = min(depth, 1.0);
    vec3 color = mix(vec3(0.035, 0.16, 0.23), vec3(0.012, 0.04, 0.115), toneDepth);
    float moonScale = max(uMoon.z / MOON_RADIUS, 0.1);

    // Logarithmic row spacing makes distant strokes small and foreground waves broad.
    float rowCoord = log(1.0 + depth * 5.0) * 28.0;
    for (int layer = 0; layer < 2; ++layer)
    {
        float offset = float(layer);
        float wave = sin(uv.x * 9.0 - time * 0.65 + rowCoord * 0.8) * 0.15;
        float v = rowCoord + wave + offset * 0.5;
        float row = floor(v);
        float seed = hash12(vec2(row, offset + 31.0));
        // Keep the horizontal density positive while extending foreground waves.
        float count = mix(34.0, 6.5, sqrt(toneDepth))
                    / sqrt(1.0 + 0.4 * max(depth - 1.0, 0.0));
        float x = uv.x * count + seed * 15.0 + offset * 7.0;
        x += sin(time * 0.35 + row * 1.7) * 0.35;
        vec2 id = vec2(floor(x), row + offset * 100.0);
        vec2 local = vec2(fract(x), fract(v));
        float randomValue = hash12(id);
        float stroke = paintStroke(local, id, vec2(0.37, mix(0.12, 0.23, seed)));

        // Broken moonlight widens toward the viewer, swaying with the wave rows.
        float center = moon.x + sin(row * 1.7 + time * 0.45) * (0.018 + depth * 0.065)
                     + sin(row * 0.47 - time * 0.27) * depth * 0.08;
        // The same spring radius drives both the moon disk and its light path.
        float width = mix(0.065, 0.34, pow(depth, 0.8)) * moonScale;
        float reflection = exp(-pow(abs(uv.x - center) / width, 2.0));
        reflection *= smoothstep(0.18, 0.75, randomValue);
        vec3 ink = mix(vec3(0.035, 0.14, 0.30), vec3(0.12, 0.37, 0.46), randomValue);
        ink *= mix(1.0, 0.7, toneDepth);
        vec3 silverGold = mix(vec3(0.41, 0.57, 0.57), vec3(1.0, 0.87, 0.48), randomValue);
        ink = mix(ink, silverGold, reflection * 0.95);
        color = mix(color, ink, stroke * (0.85 - offset * 0.17));
    }

    // Thin, broken crests across the broad dark foreground swells.
    float swell = uv.y + 0.055 * sin(uv.x * 3.8 + time * 0.24)
                + 0.018 * sin(uv.x * 10.0 - time * 0.4);
    float crest = pow(0.5 + 0.5 * sin(swell * 37.0), 22.0);
    float broken = smoothstep(-0.05, 0.16, noise(uv * vec2(15.0, 45.0)));
    color += vec3(0.055, 0.13, 0.17) * crest * broken * smoothstep(0.2, 0.9, depth);
    return color;
}

void main()
{
    vec2 resolution = max(uResolution, vec2(1.0));
    vec2 screenUv = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;
    float zoom = max(uZoom, 0.01);
    vec2 originalUv = screenUv / zoom + uViewCenter;
    float time = max(uTime, 0.0);
    vec3 ripple = clickRipples(originalUv, time);
    vec2 uv = originalUv + ripple.xy;
    vec2 moon = uMoon.xy;
    vec3 sky = paintedSky(uv, moon, time);
    sky = paintedStars(uv, moon, time, sky);
    // Keep the moon disk anchored so its visible position matches the hit target.
    sky = paintedMoon(originalUv, moon, sky);
    float horizon = horizonHeight(uv.x, time);
    float belowHorizon = max(horizon - uv.y, 0.0);
    // Carry the distant brushwork with the edge; fade the displacement
    // into the sea so the broad foreground waves retain their own rhythm.
    float horizonInfluence = 1.0 - smoothstep(0.0, 0.24, belowHorizon);
    vec2 seaUv = uv;
    seaUv.y -= (horizon - HORIZON) * horizonInfluence;
    vec3 sea = paintedSea(seaUv, moon, time);
    float horizonDistance = uv.y - horizon;
    float aa = max(2.0 / (resolution.y * zoom), fwidth(horizonDistance));
    vec3 color = mix(sea, sky, smoothstep(-aa, aa, horizonDistance));
    float outsideMoon = smoothstep(uMoon.z, uMoon.z + 0.03, length(originalUv - moon));
    color += vec3(0.45, 0.65, 0.75) * ripple.z * outsideMoon;

    // Fixed canvas grain, independent of time, avoids animated pixel noise.
    vec2 canvas = uv * 420.0;
    float grain = hash12(floor(canvas)) - 0.5;
    float weave = sin(canvas.x * 2.8) * sin(canvas.y * 3.1);
    color *= 1.0 + grain * 0.045 + weave * 0.012;
    vec2 frame = gl_FragCoord.xy / resolution;
    float vignette = 16.0 * frame.x * frame.y * (1.0 - frame.x) * (1.0 - frame.y);
    color *= 0.82 + 0.18 * pow(max(vignette, 0.0), 0.25);
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
