#include "pspgl_internal.h"
#include "pspgl_buffers.h"

GLboolean glUnmapBufferARB(GLenum target)
{
	struct pspgl_bufferobj *buf, **bufp;

	bufp = __pspgl_bufferobj_for_target(target);

	if (bufp == NULL)
		return GL_FALSE;

	buf = *bufp;

	if (buf == NULL || buf->data == NULL || !buf->mapped) {
		GLERROR(GL_INVALID_OPERATION);
		return GL_FALSE;
	}

	buf->mapped = GL_FALSE;

	__pspgl_buffer_unmap(buf->data, buf->access);

	return GL_TRUE;
}

GLboolean glUnmapBuffer(GLenum target)
	__attribute__((alias("glUnmapBufferARB")));
