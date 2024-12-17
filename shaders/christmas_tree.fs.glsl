#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform bool isLightOn;
uniform bool useTex;
uniform vec4 Ka;    
uniform vec4 Kd;
uniform vec4 Ks;
uniform float Ns;

uniform vec3 lightPos;
uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;
uniform vec3 viewPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_opacity1;

void main()
{    
   vec4 texColor = texture(texture_diffuse1, TexCoords);
   vec3 color = texColor.rgb;

   vec3 norm = normalize(Normal);
   vec3 lightDir = normalize(lightPos - FragPos);
   float diff = max(dot(norm, lightDir), 0.0);
 
   vec3 viewDir = normalize(viewPos - FragPos);
   vec3 reflectDir = reflect(-lightDir, norm);  
   float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
   float tex_spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
   
    vec3 ambient = vec3(1.0);
    vec3 diffuse = vec3(1.0); 
    vec3 specular = vec3(1.0);
    vec3 result = vec3(1.0);

   if(useTex)
   {
        ambient = lightAmbient * color;
        diffuse = diff * color;
        specular = tex_spec * Ks.rgb;
        result = ambient + diffuse + specular;

        float alpha = texture2D(texture_diffuse1, TexCoords).a;

        if(alpha < 0.2f)
            discard;
   }
   else
   {
        ambient = lightAmbient * Kd.rgb;
        diffuse = diff * Kd.rgb;
        specular = spec * Ks.rgb;
        result = ambient + diffuse + specular;
   }

   if (isLightOn) {
       if (Kd.rgb == vec3(0.517331, 0.007343, 0.000000)) {
           result *= 2;
           result += vec3(0.517331, 0.007343, 0.000000);
       }
       if (Kd.rgb == vec3(0.496920, 0.471582, 0.114475)) {
           result *= 2;
           result += vec3(0.496920, 0.471582, 0.114475);
       }
   }

   gl_FragColor = vec4(result, 1.0);
}