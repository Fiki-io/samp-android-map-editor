#version 300 es
precision mediump float;

in vec2 vTexCoord;
in vec4 vColor;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec4 uTint;
uniform vec3 uLightDir; // Arah matahari (normalized)

out vec4 FragColor;

void main() {
    vec4 texColor = vec4(1.0);
    if (uHasTexture) {
        texColor = texture(uTexture, vTexCoord);
        // Alpha testing: buang pixel transparan (dedaunan, pagar kawat GTA)
        if (texColor.a < 0.25) {
            discard;
        }
    }

    // GTA style basic lighting: Ambient + Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = vec3(0.55);
    vec3 diffuse = diff * vec3(0.65);
    vec3 lighting = ambient + diffuse;

    vec4 finalColor = texColor * vColor * uTint * vec4(lighting, 1.0);
    FragColor = vec4(finalColor.rgb, texColor.a * uTint.a);
}
