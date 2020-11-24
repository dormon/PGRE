#include <iostream>
#include <vector>

#include<SDL.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<glm/glm.hpp>
#include<glm/mat3x3.hpp>
#include<glm/gtx/transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include<stb_image.h>

#include<loadTxtFile.hpp>

using namespace ge::gl;

GLuint createTexture(std::string const&fileName){
  int texx;
  int texy;
  int channels;
  std::cerr << "loading: " << fileName << std::endl;
  auto pixels = stbi_load(fileName.c_str(),&texx,&texy,&channels,0);
  std::cerr << "channels: " << channels << std::endl;
  std::cerr << "texx: " << texx << std::endl;
  std::cerr << "texy: " << texy << std::endl;

  for(int y=0;y<texy/2;++y){
    for(int x=0;x<texx;++x){
      for(int c=0;c<channels;++c){
        auto z = pixels[(y*texx+x)*channels + c];
        pixels[(y*texx+x)*channels + c] = pixels[((texy-y-1)*texx+x)*channels + c];
        pixels[((texy-y-1)*texx+x)*channels + c] = z;
      }
    }
  }

  GLuint tex;
  glCreateTextures(GL_TEXTURE_2D,1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);

  glPixelStorei(GL_UNPACK_ROW_LENGTH,texx);
  glPixelStorei(GL_UNPACK_ALIGNMENT ,1   );

  if(channels == 3)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,texx,texy,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);

  if(channels == 4)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texx,texy,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);

  stbi_image_free(pixels);

  glTextureParameteri(tex,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTextureParameteri(tex,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTextureParameteri(tex,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTextureParameteri(tex,GL_TEXTURE_WRAP_T,GL_REPEAT);

  return tex;
}

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

void printError(GLenum source,GLenum type, GLuint id,GLenum severity, GLsizei length, const GLchar *message,const GLvoid*userParam){
  std::cerr << "My Error: " << message << std::endl;
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
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG);
  auto context  = SDL_GL_CreateContext(window);

  ge::gl::init();

  glDebugMessageCallback(printError,nullptr);

  //ge::gl::setDefaultDebugMessage();
 
  //Empty Vertex Array (empty Vertex Puller setting)

  
  GLuint vbo;
  glCreateBuffers(1,&vbo);


  struct Vertex{
    glm::vec3 position;
    glm::vec3 normal  ;
    glm::vec2 coord   ;
  };

  std::vector<Vertex>vertices;

  uint32_t nx = 20;
  uint32_t ny = 20;

  for(uint32_t iy=0;iy<ny;++iy){
    for(uint32_t ix=0;ix<nx;++ix){
      Vertex vertex;

      float yangleNorm = (float)iy / (float)(ny-1);
      float xangleNorm = (float)ix / (float)(nx-1);

      float xangle = xangleNorm * glm::pi<float>() * 2.f;
      float yangle = yangleNorm * glm::pi<float>()      ;

      float x = glm::cos(xangle)*glm::sin(yangle);
      float y =                 -glm::cos(yangle);
      float z = glm::sin(xangle)*glm::sin(yangle);

      vertex.position = glm::vec3(x,y,z);
      vertex.normal   = glm::normalize(vertex.position);
      vertex.coord    = glm::vec2(1-xangleNorm,yangleNorm);

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
    for(uint32_t ix=0;ix<nx-1;++ix){
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

  //tex coord
  glEnableVertexArrayAttrib (vao,2);
  glVertexArrayAttribFormat (vao,2,2,GL_FLOAT,GL_FALSE,0);
  glVertexArrayVertexBuffer (vao,2,vbo,sizeof(float)*6,sizeof(Vertex));
  glVertexArrayAttribBinding(vao,2,2);

  glVertexArrayElementBuffer(vao,ebo);

  GLuint prg;
  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,"#version 460\n" + loadTxtFile("../shaders/earth.vp")),
      createShader(GL_FRAGMENT_SHADER,"#version 460\n" + loadTxtFile("../shaders/lightingFunctions.vp") + loadTxtFile("../shaders/earth.fp"))
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

  auto tex = createTexture("../images/earth.png");

  glBindTextureUnit(0,tex);

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
          camYAngle = glm::clamp(camYAngle,-glm::half_pi<float>()*0.99f,+glm::half_pi<float>()*0.99f);
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
    glClearColor(0.1,0.1,0.1,1);
    //glClearColor(0,0,0,1);
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
