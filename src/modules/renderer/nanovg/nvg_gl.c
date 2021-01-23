#include <nanovg.h>

#if ! ( /* defined(_WIN32) && defined(__APPLE__) */ 0 )
#  if defined(__EMSCRIPTEN__)
#    include <GLES2/gl2.h>
#    define NANOVG_GLES2_IMPLEMENTATION
#  elif defined(ANDROID) || defined(__ANDROID__)
#    include <GLES3/gl3.h>
#    define NANOVG_GLES3_IMPLEMENTATION
#  else
#    ifdef _WIN32
#      include <GL/glew.h>
#    else
#      include <GL/gl.h>
#    endif
#    define NANOVG_GL3_IMPLEMENTATION
#  endif
#  include <nanovg_gl.h>
#  include <nanovg_gl_utils.h>
#endif
