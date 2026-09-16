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
    vec4 baseColor = vec4(1.0);
    if (uHasTexture) {
        vec4 texColor = texture(uTexture, vTexCoord);
        if (texColor.a < 0.25) {
            discard;
        }
        baseColor = texColor;
    }

    // Safe lighting calculation without NaN
    float nLen = length(vNormal);
    vec3 norm = (nLen > 0.001) ? (vNormal / nLen) : vec3(0.0, 0.0, 1.0);
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = vec3(0.55);
    vec3 diffuse = diff * vec3(0.65);
    vec3 lighting = ambient + diffuse;

    // Use vertex color if valid (alpha > 0.0), else default white
    vec4 vertCol = (vColor.a > 0.01) ? vColor : vec4(1.0);

    vec3 rgb = baseColor.rgb * vertCol.rgb * uTint.rgb * lighting;
    FragColor = vec4(rgb, baseColor.a * uTint.a);
}
