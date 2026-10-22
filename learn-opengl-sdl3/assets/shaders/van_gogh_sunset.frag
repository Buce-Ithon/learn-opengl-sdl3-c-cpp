// https://www.shadertoy.com/view/fXlXWN
// Van Gogh Sunset
// by Noztol
// Inspired by  bitless Coastal Landscape https://www.shadertoy.com/view/fstyD4
#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;
uniform float uTime;

#define MountainLayerThreecol vec3(26., 65., 74.) / 255.
#define MountainLayerFourCol vec3(14., 49., 55.) / 255.
#define FieldMid vec3(94., 121., 62.) / 255.

vec3 palette(float t)
{
    return vec3(.26, .76, .77)
         + vec3(1., .3, 1.) * cos(6.28318 * (vec3(.8, .4, .7) * t + vec3(0., .12, .54)));
}

vec4 hue(float v)
{
    return .6 + .76 * cos(6.3 * v + vec4(0., 23., 21., 0.));
}

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3. - 2. * f);

    return mix(
        mix(dot(hash22(i), f), dot(hash22(i + vec2(1., 0.)), f - vec2(1., 0.)), u.x),
        mix(dot(hash22(i + vec2(0., 1.)), f - vec2(0., 1.)),
            dot(hash22(i + vec2(1., 1.)), f - vec2(1., 1.)), u.x),
        u.y);
}

vec2 rotate2D(vec2 st, float angle)
{
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * st;
}

float st(float a, float b, float s)
{
    return smoothstep(a - s, a + s, b);
}

vec3 getPaintedSkyAndMountains(vec2 pixel, vec2 resolution, float time)
{
    // Equivalent to ShaderToy's (fragCoord + fragCoord - iResolution) / iResolution.y.
    vec2 uv = (pixel + pixel - resolution) / resolution.y;
    vec2 sunPos = vec2(resolution.x / resolution.y * .35, -.15);
    vec2 sh = rotate2D(sunPos, noise(uv + time * .25) * .3);

    vec3 color = vec3(0.);
    float pixelSmooth = 3. / resolution.y;
    vec2 u, id, local, transformed;
    float xCount, yCount, height;

    u = uv + sh;
    yCount = 60.;
    id = vec2((length(u) + .01) * yCount, 0.);
    xCount = floor(id.x) * .09;
    height = (hash12(floor(id.xx)) * .5 + .25) * (time + 10.) * .25;
    transformed = rotate2D(u, height);
    id.y = atan(transformed.y, transformed.x) * xCount;
    local = fract(id);
    id -= local;

    transformed = vec2(
        cos((id.y + .5) / xCount) * (id.x + .5) / yCount,
        sin((id.y + .5) / xCount) * (id.x + .5) / yCount);
    transformed = rotate2D(transformed, -height) - sh;

    height = noise(transformed * vec2(.5, 1.) - vec2(time * .2, 0.))
           * step(-.25, transformed.y);
    height = smoothstep(.052, .055, height);
    local += noise(local * vec2(1., 4.) + id) * vec2(.7, .2);

    color = mix(
        palette(sin(length(u) - .1)) * .35,
        mix(palette(sin(length(u) - .1) + (hash12(id) - .5) * .15),
            vec3(1.), height),
        st(abs(local.x - .5), .4, pixelSmooth * yCount)
        * st(abs(local.y - .5), .48, pixelSmooth * xCount));

    float cloud = noise(-sh * vec2(.5, 1.) - vec2(time * .2, 0.));
    cloud = 1. - smoothstep(0., .15, cloud) * .5;

    u = (uv - vec2(0., .25)) * vec2(1., 15.);
    id = floor(u);

    for (float layer = 1.; layer > -1.; layer--)
    {
        if (id.y + layer < 0.)
        {
            local = fract(u) - .5;
            local.y = (local.y + (sin(uv.x * 8. - time * .8 + id.y + layer) * .3 - layer)) * 4.;
            height = hash12(vec2(id.y + layer, floor(local.y)));
            xCount = 6. + height * 4.;
            yCount = 30.;
            local.x = uv.x * xCount + sh.x * 9.;
            local.x += sin(time * (.2 + height * 1.5)) * .5;
            height = .8 * smoothstep(5., 0., abs(floor(local.x))) * cloud + .1;

            color = mix(color, mix(MountainLayerFourCol, MountainLayerThreecol, height),
                        st(local.y, 0., pixelSmooth * yCount));
            local += noise(local * vec2(3., .5)) * vec2(.1, .6);

            vec3 strokeColor = hue(hash12(floor(local)) * .1 + .35).rgb
                             * (1.2 + floor(local.y) * .17);
            color = mix(color, mix(strokeColor, FieldMid, height),
                        st(local.y, 0., pixelSmooth * xCount)
                        * st(abs(fract(local.x) - .5), .48, pixelSmooth * xCount)
                        * st(abs(fract(local.y) - .5), .3, pixelSmooth * yCount));
        }
    }

    return color;
}

vec3 getPaintedWaterAndSand(vec2 pixel, vec2 resolution, float time, vec3 background)
{
    vec2 uv = (pixel + pixel - resolution) / resolution.y;
    vec2 sunPos = vec2(resolution.x / resolution.y * .35, -.15);
    vec2 sh = rotate2D(sunPos, noise(uv + time * .25) * .3);
    float pixelSmooth = 3. / resolution.y;
    vec3 color = background;

    // Water begins below the horizon.
    float waterZone = smoothstep(.16, .14, uv.y);
    color = mix(color, vec3(.05, .15, .3), waterZone);

    float cloud = noise(-sh * vec2(.5, 1.) - vec2(time * .2, 0.));
    cloud = 1. - smoothstep(0., .15, cloud) * .5;

    vec2 u = (uv - vec2(0., .15)) * vec2(1., 15.);
    vec2 id = floor(u);

    // Horizontal blue/gold water strokes.
    for (float layer = 1.; layer > -1.; layer--)
    {
        if (id.y + layer < 0.)
        {
            vec2 local = fract(u) - .5;
            local.y = (local.y
                     + sin(uv.x * 12. - time * 3. + id.y + layer) * .25
                     - layer) * 4.;

            float randomHeight = hash12(vec2(id.y + layer, floor(local.y)));
            float xCount = 6. + randomHeight * 4.;
            float yCount = 30.;
            local.x = uv.x * xCount + sh.x * 9.;
            local.x += sin(time * (.5 + randomHeight * 2.)) * .5;

            float reflection = .8 * smoothstep(5., 0., abs(floor(local.x))) * cloud + .1;
            vec3 waterBase = mix(vec3(0., .15, .4), vec3(.6, .4, .1), reflection);
            color = mix(color, waterBase, st(local.y, 0., pixelSmooth * yCount));

            local += noise(local * vec2(3., .5)) * vec2(.1, .6);
            vec3 strokeColor = mix(
                hue(hash12(floor(local)) * .1 + .56).rgb
                * (1.2 + floor(local.y) * .17),
                vec3(1., .8, .2),
                reflection);

            float strokeMask = st(local.y, 0., pixelSmooth * xCount)
                             * st(abs(fract(local.x) - .5), .48, pixelSmooth * xCount)
                             * st(abs(fract(local.y) - .5), .3, pixelSmooth * yCount);
            color = mix(color, strokeColor, strokeMask);
        }
    }

    // Irregular sand shoreline in the foreground.
    vec2 sand = uv + noise(uv * 2.) * .1
              + vec2(0., sin(uv.x * 1.2 + 3.) * .2 + .7);
    vec3 sandColor = vec3(.85, .75, .50);
    vec3 sandDark = vec3(.60, .45, .25);
    color = mix(color, sandDark * .7, step(sand.y, 0.));

    float sandScale = 60.;
    sand *= vec2(sandScale, sandScale / 3.5);

    if (sand.y < 1.2)
    {
        for (float y = 0.; y > -3.; y--)
        {
            for (float x = -2.; x < 3.; x++)
            {
                vec2 sandId = floor(sand) + vec2(x, y);
                vec2 local = (fract(sand) + vec2(1. - x, -y)) / vec2(5., 3.);
                float randomHeight = (hash12(sandId) - .5) * .25 + .5;

                local -= vec2(.3, .5 - randomHeight * .4);
                local.x += sin(((time * .5 + randomHeight * 2.
                              - sandId.x * .05 - sandId.y * .05) * 1.1
                              + sandId.y * .5) * 2.)
                          * (local.y + .5) * .2;

                vec2 edge = abs(local) - vec2(.03, .5 - randomHeight * .5);
                float distance = length(max(edge, 0.)) + min(max(edge.x, edge.y), 0.);
                distance -= noise(local * 7. + sandId) * .1;

                float darkStroke = st(distance, .1, pixelSmooth * sandScale * .09);
                float lightStroke = st(distance, .04, pixelSmooth * sandScale * .09);
                vec3 stroke = mix(sandDark * .5,
                                  sandColor * (1.1 + local.y * 1.2)
                                  * (1.2 - randomHeight * 1.2),
                                  lightStroke);
                float alpha = darkStroke * step(sandId.y, -1.);
                color = mix(color, stroke, alpha);
            }
        }
    }

    return color;
}

float sdTri(vec2 p, float base, float tip, float width)
{
    float dY = max(base - p.y, p.y - tip);
    float fraction = clamp((p.y - base) / (tip - base), 0., 1.);
    float currentWidth = width * (1. - fraction);
    float dX = abs(p.x) - currentWidth;
    return max(dX, dY);
}

float sdTree(vec2 p, vec2 position, float scale, float time)
{
    vec2 q = (p - position) / scale;
    q.x -= sin(time * 1.5 + position.x * 10.) * .02 * max(0., q.y);

    float distance = max(abs(q.x) - .03, max(-q.y, q.y - .2));
    distance = min(distance, sdTri(q, .1, .45, .25));
    distance = min(distance, sdTri(q, .25, .65, .2));
    distance = min(distance, sdTri(q, .45, .85, .15));
    distance = min(distance, sdTri(q, .65, 1., .1));
    return distance * scale;
}

float getLand(vec2 uv, float aspect)
{
    float distance = 1.;

    float width1 = .05 * exp(-12. * pow(uv.x - aspect * .1, 2.));
    float center1 = .36 - .01 * uv.x;
    float cutoff1 = 1. - smoothstep(0., .008, width1);
    float land1 = abs(uv.y - center1) - width1 + cutoff1;
    distance = min(distance, land1);

    float width2 = .03 * exp(-30. * pow(uv.x - aspect * .52, 2.));
    float center2 = .48;
    float cutoff2 = 1. - smoothstep(0., .008, width2);
    float land2 = abs(uv.y - center2) - width2 + cutoff2;
    distance = min(distance, land2);

    return distance;
}

float getTrees(vec2 uv, float aspect, float time)
{
    float distance = 1.;
    distance = min(distance, sdTree(uv, vec2(aspect * .05, .41), .18, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .11, .40), .22, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .17, .38), .14, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .23, .36), .09, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .48, .508), .07, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .51, .51), .10, time));
    distance = min(distance, sdTree(uv, vec2(aspect * .54, .50), .06, time));
    return distance;
}

vec4 getPaintedTrees(vec2 uv, vec2 resolution, float time, float aspect)
{
    float pixelSmooth = 3. / resolution.y;
    float treeDistance = getTrees(uv, aspect, time);
    float treeMask = smoothstep(.02, -.01, treeDistance);
    float coreMask = smoothstep(.003, 0., treeDistance);

    if (treeMask <= 0.) return vec4(0.);

    vec4 color = vec4(vec3(.05, .1, .08), coreMask);
    float xCount = 160.;
    float yCount = 45.;

    for (float layer = 0.; layer < 3.; layer++)
    {
        vec2 u = uv;
        u.x -= sin(time * 1.5 + uv.x * 10.) * .02 * max(0., uv.y - .4);
        u += noise(u * 12. + vec2(time * .3, layer * 5.)) * .025;

        vec2 localGrid = u * vec2(xCount, yCount) + vec2(layer * 11.3, layer * 17.7);
        vec2 id = floor(localGrid);
        vec2 local = fract(localGrid);
        float randomValue = hash12(id);

        if (randomValue < .4) continue;

        float taper = mix(1.2, .2, local.y);
        float outline = st(abs(local.x - .5), .35 * taper, pixelSmooth * xCount)
                      * st(abs(local.y - .5), .48, pixelSmooth * yCount);
        float body = st(abs(local.x - .5), .22 * taper, pixelSmooth * xCount)
                   * st(abs(local.y - .5), .4, pixelSmooth * yCount);

        vec3 strokeColor = mix(vec3(.05, .25, .1), vec3(.15, .4, .15), randomValue);
        if (randomValue > .8)
            strokeColor = mix(strokeColor, vec3(.3, .5, .15), .6);
        if (randomValue < .55)
            strokeColor = mix(strokeColor, vec3(0., .15, .2), .7);

        float strokeAlpha = outline * treeMask;
        color.rgb = mix(color.rgb, vec3(.02, .04, .03), strokeAlpha);
        color.rgb = mix(color.rgb, strokeColor, body * treeMask);
        color.a = max(color.a, strokeAlpha);
    }

    return color;
}

vec4 getIslandGrass(vec2 uv, vec2 resolution, float time, float aspect)
{
    float pixelSmooth = 3. / resolution.y;
    vec4 outputColor = vec4(0.);
    vec2 u = uv + noise(uv * 2.) * .05;
    float xCount = 160.;
    u *= vec2(xCount, xCount / 2.);

    vec3 grassColor = mix(vec3(.3, .65, .2), vec3(.5, .8, .2),
                          sin(time * .5) * .5 + .5);
    vec3 rockColor = vec3(.08, .1, .12);

    for (float y = 0.; y > -4.; y--)
    {
        for (float x = -2.; x < 3.; x++)
        {
            vec2 id = floor(u) + vec2(x, y);
            vec2 rootUv = id / vec2(xCount, xCount / 2.);

            float width1 = .05 * exp(-12. * pow(rootUv.x - aspect * .1, 2.));
            float center1 = .36 - .01 * rootUv.x;
            float cutoff1 = 1. - smoothstep(0., .008, width1);
            float land1 = abs(rootUv.y - center1) - width1 + cutoff1;

            float width2 = .03 * exp(-30. * pow(rootUv.x - aspect * .52, 2.));
            float land2 = abs(rootUv.y - .48) - width2
                        + 1. - smoothstep(0., .008, width2);
            float rootAlpha = smoothstep(.01, -.01, min(land1, land2));

            if (rootAlpha <= 0.) continue;

            vec2 local = (fract(u) + vec2(1. - x, -y)) / vec2(5., 3.);
            float randomHeight = (hash12(id) - .5) * .25 + .5;
            local -= vec2(.3, .5 - randomHeight * .4);
            local.x += sin(((time * 1.5 + randomHeight * 2.
                          - id.x * .05 - id.y * .05) * 1.1
                          + id.y * .5) * 2.)
                      * (local.y + .5) * .25;

            vec2 edge = abs(local) - vec2(.02, .5 - randomHeight * .5);
            float distance = length(max(edge, 0.)) + min(max(edge.x, edge.y), 0.);
            distance -= noise(local * 7. + id) * .1;

            float rockMask = st(distance, .1, pixelSmooth * xCount * .09);
            float grassMask = st(distance, .04, pixelSmooth * xCount * .09);
            vec3 stroke = mix(rockColor, grassColor * (1.2 + local.y * 2.)
                              * (1.8 - randomHeight * 2.5), grassMask);

            float alpha = rockMask * rootAlpha;
            outputColor = mix(outputColor, vec4(stroke, 1.), alpha);
        }
    }

    return outputColor;
}

void main()
{
    vec3 color = getPaintedSkyAndMountains(gl_FragCoord.xy, uResolution, uTime);
    color = getPaintedWaterAndSand(gl_FragCoord.xy, uResolution, uTime, color);

    // Land, trees and grass follow the original ShaderToy coordinate system:
    // normalized screen coordinates with the bottom-left corner at (0, 0).
    // The sky and water functions above intentionally use centered coordinates.
    vec2 uv = gl_FragCoord.xy / uResolution;
    float aspect = uResolution.x / uResolution.y;
    uv.x *= aspect;

    // Island base.
    float landDistance = getLand(uv, aspect);
    float landMask = smoothstep(.003, 0., landDistance);
    color = mix(color, vec3(.08, .1, .12), landMask);

    // Trees are drawn before the grass, so the grass visually plants them into the island.
    vec4 trees = getPaintedTrees(uv, uResolution, uTime, aspect);
    color = mix(color, trees.rgb, trees.a);

    vec4 grass = getIslandGrass(uv, uResolution, uTime, aspect);
    color = mix(color, grass.rgb, grass.a);

    FragColor = vec4(color, 1.);
}
