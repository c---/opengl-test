//
// To compile:
// g++ -Wall -O2 -g main.cpp -o main -lGL -lSDL2
//

#define GL_GLEXT_PROTOTYPES
#define GL3_PROTOTYPES 1

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

unsigned char green;
SDL_Window* window_;
SDL_GLContext glctx_;
GLuint program_;
GLint clglcolor_;
GLint projection_;
GLint modelview_;
GLuint vbo_[2];
double projection[16], modelview[16];

int processevents()
{
  SDL_Event event;

   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
      case SDL_QUIT:
         return 0;
      default:
         break;
      }
   } 

   return 1;
}

void sdlinit()
{
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

   if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDL_GL_LoadLibrary(0) < 0)
   {
      fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
      exit(255);
   }

   atexit(SDL_Quit);

   window_ = SDL_CreateWindow("The Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
   if (!window_)
   {
      fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
      exit(255);
   }

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   glctx_ = SDL_GL_CreateContext(window_);
   if (!glctx_)
   {
      fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
      exit(255);
   }

   SDL_GL_MakeCurrent(window_, glctx_);
   SDL_GL_SetSwapInterval(0);
}

void mat_identity(double* m_)
{
   m_[ 0] = 1; m_[ 1] = 0; m_[ 2] = 0; m_[ 3] = 0;
   m_[ 4] = 0; m_[ 5] = 1; m_[ 6] = 0; m_[ 7] = 0;
   m_[ 8] = 0; m_[ 9] = 0; m_[10] = 1; m_[11] = 0;
   m_[12] = 0; m_[13] = 0; m_[14] = 0; m_[15] = 1;
}

void mat_ortho(double* m_, double left, double right, double bottom, double top, double nearVal, double farVal)
{
   m_[0] = 2.0 / (right - left); m_[1] = 0; m_[2] = 0; m_[3] = 0;
   m_[4] = 0; m_[5] = 2 / (top - bottom); m_[6] = 0; m_[7] = 0;
   m_[8] = 0; m_[9] = 0; m_[10] = -2 / (farVal - nearVal); m_[11] = 0;
   m_[12] = -(right + left) / (right - left); m_[13] = -(top + bottom) / (top - bottom); m_[14] = -(farVal + nearVal) / (farVal - nearVal); m_[15] = 1.0;
}

void ortho2d()
{
   glViewport(0, 0, 1024, 768);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 1024, 0, 768, 0, 1);
   mat_ortho(projection, 0, 1024, 0, 768, 0, 1);
   mat_identity(modelview);

   glDisable(GL_DEPTH_TEST);
   glMatrixMode(GL_MODELVIEW);
}

void compileshaders()
{
      const char* src0 =
         "#version 420\n"
         "#extension GL_ARB_vertex_attrib_64bit : enable\n"
         "uniform dmat4 projection_matrix;\n"
         "uniform dmat4 modelview_matrix;\n"
         "uniform vec4 glcolor;\n"
         "in dvec3 vertex;\n"
         "in vec4 color;\n"
         "out vec4 frag_color;\n"
         "void main() {\n"
         "   if (glcolor[0] < 0)\n"
         "      frag_color = color;\n"
         "   else\n"
         "      frag_color = glcolor;\n"

         "   gl_Position = vec4(projection_matrix * modelview_matrix * dvec4(vertex, 1.0));\n"
         "}\n";

      GLuint shader = glCreateShader(GL_VERTEX_SHADER);
      GLint len = strlen(src0);
      glShaderSource(shader, 1, &src0, &len);
      glCompileShader(shader);

      GLint shaderok = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderok);
      if (shaderok == 0)
      {
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shaderok);
         char log[shaderok];
         GLint result;
         glGetShaderInfoLog(shader, shaderok, &result, log);
         printf("%s\n", log);
      }

      const char* src1 =
         "#version 420\n"
         "in vec4 frag_color;\n"
         "out vec4 out_color;\n"
         "void main() {\n"
         "   out_color = frag_color;\n"
         "}\n";

      GLuint fragshader = glCreateShader(GL_FRAGMENT_SHADER);
      len = strlen(src1);
      glShaderSource(fragshader, 1, &src1, &len);
      glCompileShader(fragshader);

      glGetShaderiv(fragshader, GL_COMPILE_STATUS, &shaderok);
      if (shaderok == 0)
      {
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shaderok);
         char log[shaderok];
         GLint result;
         glGetShaderInfoLog(shader, shaderok, &result, log);
         printf("%s\n", log);
      }

      program_ = glCreateProgram();
      glAttachShader(program_, shader);
      glAttachShader(program_, fragshader);

      glBindAttribLocation(program_, 0, "vertex");
      glBindAttribLocation(program_, 1, "color");

      glLinkProgram(program_);

      clglcolor_ = glGetUniformLocation(program_, "glcolor");
      projection_ = glGetUniformLocation(program_, "projection_matrix");
      modelview_ = glGetUniformLocation(program_, "modelview_matrix");
}

void bufferdata()
{
   double vertex[] = {
         200, 300,   200, 200,
         300, 200,   300, 300
      };
   unsigned char color[] = {
         255, green, 0, 255,   255, green, 0, 255,
         255, green, 0, 255,   255, green, 0, 255,
      };

   glGenBuffers(2, vbo_);
   glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_DYNAMIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_DYNAMIC_DRAW);
}

int main(int argc, char* argv[])
{
   if (argc > 1)
      green = atoi(argv[1]);
   else
      green = 55;

   sdlinit();
   compileshaders();
   bufferdata();

   while (processevents())
   {
      ortho2d();

      // Immediate mode green square
      glColor4d(0,1,0,1);
      glBegin(GL_QUADS);
      glVertex2d(100, 200);
      glVertex2d(100, 100);
      glVertex2d(200, 100);
      glVertex2d(200, 200);
      glEnd();

      // VBO square
      glUseProgram(program_);
      glUniformMatrix4dv(projection_, 1, 0, projection);
      glUniformMatrix4dv(modelview_, 1, 0, modelview);
      glUniform4f(clglcolor_, -1, -1, -1, 0);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
      glVertexAttribLPointer(0, 2, GL_DOUBLE, 0, 0);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
      glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);

      glDrawArrays(GL_QUADS, 0, 4);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glUseProgram(0);
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

      SDL_GL_SwapWindow(window_);
   }
}
