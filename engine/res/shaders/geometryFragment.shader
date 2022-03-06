#version 440 core

out vec4 fragColor;

in vec3 fragPos;
in vec4 vertexColor;
in vec3 vertexNormal;
in vec4 fragPosLightSpace;

uniform vec3 cameraPosition;
uniform vec3 lightDirection;
//uniform vec3 lightColor;
uniform bool isLightingActive;
uniform sampler2D shadowMap;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;


float shadowCalculation(vec3 lightDir){
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closesDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(vertexNormal, lightDir)), 0.005);
    return currentDepth - bias > closesDepth ? 1.0f : 0.0f;
}

void main(){
    if (isLightingActive){
        vec4 _lightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

        float ambientStrength = 0.1f;
        vec4 ambient = ambientStrength * _lightColor * vec4(material.ambient, 1.0f);

        vec3 lightDir = normalize(lightDirection);
        float diff = max(dot(vertexNormal, lightDir), 0.0f);
        vec4 diffuse = diff * _lightColor * vec4(material.diffuse, 1.0f);

        float specularStrength = 0.2f;
        vec3 viewDir = normalize(cameraPosition - fragPos);
        vec3 reflectDir = reflect(-lightDir, vertexNormal);

        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec4 specular = specularStrength * spec * _lightColor * vec4(material.specular, 1.0f);

        float shadow = shadowCalculation(lightDir);
        fragColor = (ambient + (1.0 - shadow) * (diffuse + specular)) * vertexColor;
    } else {
        fragColor = vertexColor;
    }
}