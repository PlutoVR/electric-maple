#include "gst/gst_common.h"

void
initializeEGL(struct state_t &state);
void
setupRender();
void
draw(GLuint framebuffer, GLuint texture);