#version 330 core

layout (location = 0) in vec3 aPos;      // cube vertex position
layout (location = 1) in vec3 aNormal;   // cube normal (object space)
layout (location = 2) in vec2 aTexCoords;

// per-instance packed vec4: x = offsetX, y = offsetZ, z = phase, w = distFromCenter
layout (location = 3) in vec4 aInst;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
flat out vec3 InstOffset; // pass to fragment shader for color mapping

uniform mat4 view;
uniform mat4 projection;

// animation uniforms
uniform float time;       // scaled time
uniform float amplitude;  // global amplitude
uniform float freq;       // primary freq
uniform float freq2;      // secondary freq
uniform float rippleFreq; // radial ripple freq
uniform float heightPow;  // exponent to smooth peaks
uniform float baseScale;  // uniform scale of cubes

void main()
{
    float ox = aInst.x;
    float oz = aInst.y;
    float phase = aInst.z;
    float dist = aInst.w;

    // layered mathy motion:
    float y1 = sin(time * 1.0 + phase * freq) * 0.95;
    float y2 = sin(time * 0.6 + (ox * 0.9 + oz * 1.1) * freq2 + phase * 0.8) * 0.6;
    float ripple = sin(time * 1.3 - dist * rippleFreq) * 0.55;
    float lissa = 0.15 * sin(1.2 * time + 0.9 * ox + 1.7 * oz);

    float raw = (y1 + y2 + ripple) * 0.6 + lissa;
    float h = raw * amplitude;

    // soften peaks:
    float signh = sign(h);
    float ah = abs(h);
    ah = pow(ah, heightPow);
    h = signh * ah;

    // Build translation matrix (T) and uniform scale matrix (S) manually:
    mat4 T = mat4(1.0);
    // column-major: 4th column is translation
    T[3] = vec4(ox, h, oz, 1.0);

    mat4 S = mat4(1.0);
    S[0][0] = baseScale;
    S[1][1] = baseScale;
    S[2][2] = baseScale;

    // Model = T * S  (translate after scale)
    mat4 model = T * S;

    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    InstOffset = vec3(ox, h, oz);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
