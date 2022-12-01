#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse0;

void main()
{    
    FragColor = texture(texture_diffuse0, TexCoords);
    // FragColor = vec4(1., 1., 1., 1.);
}