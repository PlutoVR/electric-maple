#include "gst/gst_common.h"
#include <GLES3/gl3.h>

struct em_state;

void
initializeEGL(struct em_state &state);
void
setupRender();
void
draw(GLuint framebuffer, GLuint texture, GLenum texture_target);
