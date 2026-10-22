#version 330 core

out vec4 FragColor;

uniform vec3 iResolution;
uniform float iTime;
uniform sampler2D iChannel0; // 16 x 16 character atlas; red = glyph coverage.
uniform sampler2D iChannel1; // Repeatable two-channel noise, using as a random-number-sheet to choose character.
uniform vec3 iChannelResolution[2]; // Resolution of iChannel0 and iChannel1 (include width and height)

/**
 * @details Divide the screen into a grid of 16×16-pixel cells, randomly select a glyph from a character set for each cell, and return the coverage of that character at the current pixel location.
 * @param fragCoord Screen pixel coordinates of the current fragment.
 * reutrn float The coverage of that character at the current pixel location.
*/
float text(vec2 fragCoord)
{
    // `mod(fragCoord, 16.0)` retrieves the relative position of the current pixel within its 16x16-pixel grid cell. For example: (36, 49) -> (4, 1).
    // Multiplying it by 0.0625 (=1/16) normalizes the local coordinates to a range of approximately [0,1)
    vec2 uv = mod(fragCoord.xy, 16.0) * 0.0625;
    // Determine which 16x16-pixel grid cell the current pixel belongs to.
    // block = floor(fragCoord / 16). For example: (36, 49) -> (2, 3)
    vec2 block = fragCoord * 0.0625 - uv;
    // The sampling area within each glyph is scaled from [0, 1] to [0.1, 0.9].
    // This serves to provide a margin of approximately 10% around each character, preventing sampling issues such as hitting the glyph edges, encroaching on adjacent characters, or encountering color bleeding caused by texture filtering. In other words, the character does not fill the entire 16×16 grid.
    uv = uv * 0.8 + 0.1;
    // Each character cell receives a random 2D index derived from dynamic noise `iChannel1`.
    // `texture(iChannel1, uv)`: Reads a `vec4(R, G, B, A)` from the texture `iChannel1` at the specified `uv` position (`uv` must be normalized).
    // `block / iChannelResolution[1].xy + iTime * 0.002` it's not conflict with "`uv` parameter in texture() must be normalized in 0~1", because iChannel1 is a repeatable two-channel noise which is similiar with `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);`. So as it's over 0~1, the GPU treats this uv coordinates that fall outside the range as fractional parts. 
    vec2 randomValue = texture(iChannel1, block / iChannelResolution[1].xy + iTime * 0.002).xy;
    // Normalized 8-bit textures (`randomValue`) can return exactly 1.0: so we need keep tile IDs in 0..15.
    uv += min(floor(randomValue * 16.0), vec2(15.0));
    uv *= 0.0625; // normalize for following texture()
    //uv.x = -uv.x;
    return texture(iChannel0, uv).r;
}

/**
 * @brief Create a brightness effect where characters fall like raindrops.
 * @param fragCoord Screen pixel coordinates of the current fragment.
 * @return vec3 A RGB value.
*/
vec3 rain(vec2 fragCoord)
{
    // Align the x-coordinate to the left boundary of a 16-pixel column, so that all pixels within the same column share the same column-specific parameters—meaning they all exhibit the same falling speed, phase, and brightness variations.
    fragCoord.x -= mod(fragCoord.x, 16.0);
    // Based on the horizontal coordinate of each character column, a unique starting point (phase offset) for the animation cycle is generated for that column.
    // The sine function outputs values ​​in the range [-1, 1]. Consequently, even though the raindrops in every column follow the same cyclic pattern, they do not begin their descent from the top of the screen at the same time.
    float offset = sin(fragCoord.x * 15.0);
    // Generate different speeds for each column.
    float speed = cos(fragCoord.x * 3.0) * 0.3 + 0.7; // always positive speed
    /*
     *This is the core of the raindrop movement.
     *- `fragCoord.y / iResolution.y`: The normalized vertical position of the current pixel on the screen, ranging from near 0 at the bottom to near 1 at the top.
     *- `iTime * speed`: Advances the animation over time, with each column moving at a different speed.
     *- `offset`: Assigns a unique starting position to each column.
     *- `fract(...)`: Retains only the fractional part, causing the value to cycle continuously within the range [0, 1).
     *Thus, `y` can be interpreted as the cyclic distance from the current character to the "head" of the raindrop in its respective column. Once a raindrop moves beyond the screen boundary, the `fract` function causes it to reappear from the opposite side.
    */
    float y = fract(fragCoord.y / iResolution.y + iTime * speed + offset);
    // Avoid division by zero (and 0 * infinity on black glyph pixels).
    return vec3(0.1, 0.35, 1.0) / max(y * 20.0, 0.001);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    fragColor = vec4(text(fragCoord) * rain(fragCoord), 1.0);
}

void main()
{
    mainImage(FragColor, gl_FragCoord.xy);
}