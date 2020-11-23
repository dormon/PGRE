#include <iostream>
#include <vector>

#include<SDL.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<glm/glm.hpp>
#include<glm/mat3x3.hpp>
#include<glm/gtx/transform.hpp>

using namespace ge::gl;

GLuint createShader(GLenum type,std::string const&src){
  GLuint id = glCreateShader(type);
  char const*const srcs [] = {
    src.data()
  };
  glShaderSource(id,1,srcs,nullptr);
  glCompileShader(id);


  GLint status;
  glGetShaderiv(id,GL_COMPILE_STATUS,&status);

  if(status != GL_TRUE){
    GLint len;
    glGetShaderiv(id,GL_INFO_LOG_LENGTH,&len);

    char*buffer = new char[len];

    glGetShaderInfoLog(id,len,nullptr,buffer);

    auto msg = "shader compilation failed: "+std::string(buffer);
    delete[]buffer;

    throw std::runtime_error(msg);

  }

  return id;
}

GLuint createProgram(std::vector<GLuint> const&shaders){
  GLuint prg = glCreateProgram();

  for(auto const&shader:shaders)
    glAttachShader(prg,shader);

  glLinkProgram(prg);

  GLint status;
  glGetProgramiv(prg,GL_LINK_STATUS,&status);

  if(status != GL_TRUE){
    GLint len;
    glGetProgramiv(prg,GL_INFO_LOG_LENGTH,&len);

    char*buffer = new char[len];

    glGetProgramInfoLog(prg,len,nullptr,buffer);

    auto msg = "program linking failed: "+std::string(buffer);
    delete[]buffer;

    throw std::runtime_error(msg);
  }

  return prg;
}

int main(int argc,char*argv[]){
  //SDL2 glfw glaux QT ...
  SDL_Init(SDL_INIT_EVERYTHING);

  //create window
  
  int windowWidth = 1024;
  int windowHeight = 768;
  auto window = SDL_CreateWindow("PGRe",0,0,windowWidth,windowHeight,SDL_WINDOW_OPENGL);

  //create opengl context
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  auto context  = SDL_GL_CreateContext(window);

  ge::gl::init();
 
  //Empty Vertex Array (empty Vertex Puller setting)

  
  GLuint vbo;
  glCreateBuffers(1,&vbo);


  struct Vertex{
    glm::vec3 position;
    glm::vec3 normal  ;
  };

  std::vector<Vertex>vertices;

  uint32_t nx = 20;
  uint32_t ny = 20;

  for(uint32_t iy=0;iy<ny;++iy){
    for(uint32_t ix=0;ix<nx;++ix){
      Vertex vertex;

      float yangleNorm = (float)iy / (float)(ny-1);
      float xangleNorm = (float)ix / (float)(nx  );

      float xangle = xangleNorm * glm::pi<float>() * 2.f;
      float yangle = yangleNorm * glm::pi<float>()      ;

      float x = glm::cos(xangle)*glm::sin(yangle);
      float y =                 -glm::cos(yangle);
      float z = glm::sin(xangle)*glm::sin(yangle);

      vertex.position = glm::vec3(x,y,z);
      vertex.normal   = glm::normalize(vertex.position);

      vertices.push_back(vertex);
    }
  }

  glNamedBufferData(vbo,sizeof(Vertex)*vertices.size(),vertices.data(),GL_DYNAMIC_COPY);


  struct Face{
    uint32_t triangleA[3];
    uint32_t triangleB[3];

    void print(){
      std::cerr << triangleA[0] << " " << triangleA[1] << " "<< triangleA[2];
      std::cerr << " - ";
      std::cerr << triangleB[0] << " " << triangleB[1] << " "<< triangleB[2];
      std::cerr << std::endl;
    }
  };

  std::vector<Face>faces;

  for(uint32_t iy=0;iy<ny-1;++iy){
    for(uint32_t ix=0;ix<nx;++ix){
      Face face;

      face.triangleA[0] = (iy  )*nx + (ix       );
      face.triangleA[1] = (iy  )*nx + ((ix+1)%nx);
      face.triangleA[2] = (iy+1)*nx + (ix       );

      face.triangleB[0] = (iy+1)*nx + (ix       );
      face.triangleB[1] = (iy  )*nx + ((ix+1)%nx);
      face.triangleB[2] = (iy+1)*nx + ((ix+1)%nx);

      faces.push_back(face);
    }
  }

  //for(auto &f:faces)
  //  f.print();


  GLuint ebo;
  glCreateBuffers(1,&ebo);
  glNamedBufferData(ebo,sizeof(Face)*faces.size(),faces.data(),GL_DYNAMIC_COPY);

  GLuint vao;
  glCreateVertexArrays(1,&vao);


  //position
  glEnableVertexArrayAttrib (vao,0);
  glVertexArrayAttribFormat (vao,0,3,GL_FLOAT,GL_FALSE,0);
  glVertexArrayVertexBuffer (vao,0,vbo,sizeof(float)*0,sizeof(Vertex));
  glVertexArrayAttribBinding(vao,0,0);

  //normal
  glEnableVertexArrayAttrib (vao,1);
  glVertexArrayAttribFormat (vao,1,3,GL_FLOAT,GL_FALSE,0);
  glVertexArrayVertexBuffer (vao,1,vbo,sizeof(float)*3,sizeof(Vertex));
  glVertexArrayAttribBinding(vao,1,1);

  glVertexArrayElementBuffer(vao,ebo);



  //Shader Program
  //glCreateShader glShaderSource glCompileShader
  //glCreateProgram glAttachShader glLinkShader


  //GLSL 
  std::string const vsSrc = R".(
  #line 201

  layout(location=0)in vec3 position;
  layout(location=1)in vec3 normal  ;

  out vec3 vNormal;
  out vec3 vPosition;
  out vec3 vLighting;

  uniform mat4 modelMatrix      = mat4(1);
  uniform mat4 viewMatrix       = mat4(1);
  uniform mat4 projectionMatrix = mat4(1);

  uniform int lightingType = 0;

  void main(){
    vNormal = normal;
    vPosition = position;


    vec3 ambientLight  = vec3(0.2,0.2,0.2);
    vec3 diffuseLight  = vec3(1,1,1);
    vec3 lightPosition = vec3(10,10,10);
    vec3 materialColor = vec3(1,0,0);
    vec3 specularMaterialColor = vec3(1,1,1);
    vec3 specularLight = diffuseLight;
    float shininess = 100.f;

    vec3 L = normalize(lightPosition - position);

    vec3 cameraPosition = vec3(inverse(viewMatrix) * vec4(0,0,0,1));
    vec3 V = normalize(cameraPosition - position);

    vec3 lighting;

    if(lightingType == 0)
      lighting = phongLighting(normal,L,V,materialColor,specularMaterialColor,ambientLight,diffuseLight,specularLight,shininess);

    if(lightingType == 1)
      lighting = lambertLighting(normal,L,materialColor,ambientLight,diffuseLight);

    vLighting = lighting;

    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position,1);
  }

  ).";

  std::string const lightingFunctionsSrc = R".(

  vec3 ambientLighting(vec3 materialColor,vec3 ambientLightColor){
    return materialColor * ambientLightColor;
  }

  vec3 diffuseLighting(vec3 N,vec3 L,vec3 materialColor,vec3 diffuseLightColor){
    float diffuseFactor = max(dot(N,L),0);
    return materialColor * diffuseLightColor * diffuseFactor;
  }

  vec3 specularLighting(vec3 N,vec3 L,vec3 V,vec3 specularMaterialColor,vec3 specularLightColor,float shininess){
    vec3 R = -reflect(L,N);
    float specularFactor = pow(max(dot(R,V),0),shininess);
    return specularMaterialColor * specularLightColor * specularFactor;
  }

  vec3 lambertLighting(vec3 N,vec3 L, vec3 materialColor,vec3 ambientLightColor,vec3 diffuseLightColor){
    return ambientLighting(materialColor,ambientLightColor) + diffuseLighting(N,L,materialColor,diffuseLightColor);
  }

  vec3 phongLighting(vec3 N,vec3 L,vec3 V,
                     vec3 materialColor,vec3 specularMaterialColor,vec3 ambientLightColor,
                     vec3 diffuseLightColor,vec3 specularLightColor,float shininess){
    return lambertLighting (N,L,materialColor,ambientLightColor,diffuseLightColor   )+
           specularLighting(N,L,V,specularMaterialColor,specularLightColor,shininess);
  }

  ).";

  std::string const fsSrc = R".(
  
  layout(location=0)out vec4 fColor;

  in vec3 gNormal;
  in vec3 gPosition;
  in vec3 gLighting;

  uniform int shadingType = 0;
  uniform int lightingType = 0;

  uniform mat4 viewMatrix = mat4(1);

  void main(){

    vec3 ambientLight  = vec3(0.2,0.2,0.2);
    vec3 diffuseLight  = vec3(1,1,1);
    vec3 lightPosition = vec3(10,10,10);
    vec3 materialColor = vec3(1,0,0);

    vec3 specularMaterialColor = vec3(1,1,1);
    vec3 specularLight = diffuseLight;
    float shininess = 200.f;

    vec3 N = normalize(gNormal);
    vec3 L = normalize(lightPosition - gPosition);
  
    vec3 cameraPosition = vec3(inverse(viewMatrix) * vec4(0,0,0,1));

    vec3 V = normalize(cameraPosition - gPosition);


    vec3 lighting;

    if(shadingType == 0){

      if(lightingType == 0)
        lighting = phongLighting(N,L,V,materialColor,specularMaterialColor,ambientLight,diffuseLight,specularLight,shininess);

      if(lightingType == 1)
        lighting = lambertLighting(N,L,materialColor,ambientLight,diffuseLight);
    }

    if(shadingType == 1)
      lighting = gLighting;

    if(shadingType == 2){
      if(lightingType == 0)
        lighting = phongLighting(N,L,V,materialColor,specularMaterialColor,ambientLight,diffuseLight,specularLight,shininess);

      if(lightingType == 1)
        lighting = lambertLighting(N,L,materialColor,ambientLight,diffuseLight);
    }



    fColor = vec4(lighting,1);
  }

  ).";

  
  std::string const gsSrc = R".(

  layout(triangles)in;
  layout(triangle_strip,max_vertices=3)out;

  in vec3 vNormal[];
  in vec3 vPosition[];
  in vec3 vLighting[];

  out vec3 gNormal;
  out vec3 gPosition;
  out vec3 gLighting;

  uniform int shadingType = 0;

  void main(){
    vec3 normal = normalize(vNormal[0] + vNormal[1] + vNormal[2]);

    for(int i=0;i<3;++i){
      gl_Position = gl_in[i].gl_Position;

      if(shadingType == 2)
        gNormal     = normal;
      else
        gNormal     = vNormal[i];

      gPosition   = vPosition[i];
      gLighting   = vLighting[i];
      EmitVertex();
    }
    EndPrimitive();

  }

  ).";

  GLuint prg;
  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,"#version 460\n" + lightingFunctionsSrc + vsSrc),
      createShader(GL_GEOMETRY_SHADER,"#version 460\n" +                        gsSrc),
      createShader(GL_FRAGMENT_SHADER,"#version 460\n" + lightingFunctionsSrc + fsSrc)
      });

  //locations
  GLuint saturationLocation = glGetUniformLocation(prg,"saturation");

  GLuint modelMatrixLocation      = glGetUniformLocation(prg,"modelMatrix"     );
  GLuint viewMatrixLocation       = glGetUniformLocation(prg,"viewMatrix"      );
  GLuint projectionMatrixLocation = glGetUniformLocation(prg,"projectionMatrix");
  GLuint shadingTypeLocation      = glGetUniformLocation(prg,"shadingType"     );
  GLuint lightingTypeLocation     = glGetUniformLocation(prg,"lightingType"    );

  float saturation = 0.5f;
  glm::vec3 position = glm::vec3(0.f);
  float scale[2]    = {1.f,1.f};
  float alpha       = 0.f;

  float camXAngle = 0.f;
  float camYAngle = 0.f;
  float camDistance = 3.f;

  bool wireframe = false;

  bool running = true;

  int  shadingType = 0;
  bool lambertLighting = false;

  while(running){//main loop

    //event handling
    SDL_Event event;
    //event loop
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT)
        running = false;
      if(event.type == SDL_KEYDOWN){
        if(event.key.keysym.sym == SDLK_o)saturation += 0.01f;
        if(event.key.keysym.sym == SDLK_p)saturation -= 0.01f;
        glProgramUniform1f(prg,saturationLocation,saturation);

        if(event.key.keysym.sym == SDLK_w)position[2] += 0.01;
        if(event.key.keysym.sym == SDLK_s)position[2] -= 0.01;
        if(event.key.keysym.sym == SDLK_a)position[0] -= 0.01;
        if(event.key.keysym.sym == SDLK_d)position[0] += 0.01;
        if(event.key.keysym.sym == SDLK_SPACE)position[1] -= 0.01;
        if(event.key.keysym.sym == SDLK_LSHIFT)position[1] += 0.01;

        if(event.key.keysym.sym == SDLK_t)scale[1] += 0.01;
        if(event.key.keysym.sym == SDLK_g)scale[1] -= 0.01;
        if(event.key.keysym.sym == SDLK_f)scale[0] -= 0.01;
        if(event.key.keysym.sym == SDLK_h)scale[0] += 0.01;

        if(event.key.keysym.sym == SDLK_q)alpha += 0.003;
        if(event.key.keysym.sym == SDLK_e)alpha -= 0.003;

        if(event.key.keysym.sym == SDLK_k){shadingType--;if(shadingType<0)shadingType=0;}
        if(event.key.keysym.sym == SDLK_l){shadingType++;if(shadingType>2)shadingType=2;}
        if(event.key.keysym.sym == SDLK_n)lambertLighting = !lambertLighting;

        if(event.key.keysym.sym == SDLK_m)wireframe = !wireframe;

        auto T = glm::translate(glm::mat4(1.f),position                              );
        auto S = glm::scale    (glm::mat4(1.f),glm::vec3(scale   [0],scale   [1],1.f));
        auto R = glm::rotate   (glm::mat4(1.f),alpha,glm::vec3(0.f,0.f,1.f));

        auto modelMatrix = T*R*S;

        glProgramUniformMatrix4fv(prg,modelMatrixLocation,1,GL_FALSE,(float*)&modelMatrix);

       
      }
      if(event.type == SDL_MOUSEMOTION){
        if(event.motion.state & SDL_BUTTON_LMASK){
          camXAngle += event.motion.xrel * 0.01f;
          camYAngle += event.motion.yrel * 0.01f;
        }
        if(event.motion.state & SDL_BUTTON_RMASK){
          camDistance += event.motion.yrel * 0.1f;
        }
      }
    }

    glProgramUniform1i(prg,shadingTypeLocation,shadingType);
    glProgramUniform1i(prg,lightingTypeLocation,(int)lambertLighting);

    auto projectionMatrix = glm::perspective(glm::half_pi<float>(),(float)windowWidth / (float)windowHeight,0.1f,1000.f);
    glProgramUniformMatrix4fv(prg,projectionMatrixLocation,1,GL_FALSE,(float*)&projectionMatrix);


    glm::vec3 camPosition;
    camPosition.x = camDistance*glm::cos(camXAngle)*glm::cos(camYAngle);
    camPosition.y = camDistance*glm::sin(camYAngle);
    camPosition.z = camDistance*glm::sin(camXAngle)*glm::cos(camYAngle);

    auto viewMatrix = glm::lookAt(camPosition,glm::vec3(0,0,0),glm::vec3(0,1,0));
    glProgramUniformMatrix4fv(prg,viewMatrixLocation,1,GL_FALSE,(float*)&viewMatrix);


    glEnable(GL_DEPTH_TEST);
    //glClearColor(0.1,0.1,0.1,1);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(vao);

    glPointSize(10);

    if(wireframe)
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    glUseProgram(prg);
    glDrawElements(GL_TRIANGLES,nx*ny*2*3,GL_UNSIGNED_INT,nullptr);
    //glDrawArrays(GL_POINTS,0,nx*ny);

    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);

  }

  return 0;
}
