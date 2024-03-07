#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TextCoord;

uniform sampler2D atlas;

void main() {
    // light pos
    vec3 lightDir = normalize(-vec3(-0.3, -0.6, -0.7));

    // ambient
    vec3 lightColor = vec3(1, 1, 1);
    vec3 ambient = 0.3 * lightColor;

    // difuse
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse);
    FragColor = texture(atlas, TextCoord) * vec4(result, 1.0);
}
