#line 1
layout(location=0)out vec4 fColor;

in vec3 vNormal  ;
in vec2 vCoord   ;
in vec3 vPosition;

uniform mat4 viewMatrix = mat4(1);

layout(binding=0)uniform sampler2D image;

void main(){

  vec3 ambientLight  = vec3(0.2,0.2,0.2);
  vec3 diffuseLight  = vec3(1,1,1);
  vec3 lightPosition = vec3(10,10,10);

  vec3 materialColor = texture(image,vCoord).rgb;

  vec3 specularMaterialColor = vec3(1,1,1);
  vec3 specularLight = diffuseLight;
  float shininess = 200.f;

  vec3 N = normalize(vNormal);
  vec3 L = normalize(lightPosition - vPosition);

  vec3 cameraPosition = vec3(inverse(viewMatrix) * vec4(0,0,0,1));

  vec3 V = normalize(cameraPosition - vPosition);


  vec3 lighting;

  lighting = phongLighting(N,L,V,materialColor,specularMaterialColor,ambientLight,diffuseLight,specularLight,shininess);

  fColor = vec4(lighting,1);
}
