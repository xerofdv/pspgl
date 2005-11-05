#include <string.h>
#include <psputils.h>

#include "pspgl_internal.h"
#include "pspgl_texobj.h"

static void copy_pixels(const void *srcbuf, int srcstride, int srcx, int srcy,
			void *dstbuf, int dststride, int dstx, int dsty,
			int width, int height, unsigned pixfmt)
{
	sendCommandiUncached(CMD_COPY_SRC, (unsigned)srcbuf);
	sendCommandiUncached(CMD_COPY_SRC_STRIDE, (((unsigned)srcbuf & 0xff000000) >> 8) | (srcstride & 0xffff));
	sendCommandiUncached(CMD_COPY_SRC_XY, (srcy << 10) | srcx);

	sendCommandiUncached(CMD_COPY_DST, (unsigned)dstbuf);
	sendCommandiUncached(CMD_COPY_DST_STRIDE, (((unsigned)dstbuf & 0xff000000) >> 8) | (dststride & 0xffff));
	sendCommandiUncached(CMD_COPY_DST_XY, (dsty << 10) | dstx);

	sendCommandiUncached(CMD_COPY_SIZE, ((height-1) << 10) | (width-1));

	sendCommandiUncached(CMD_COPY_START, (pixfmt == GE_RGBA_8888));
}

static const struct {
	GLenum format, type;
} formats[] = {
	[GE_RGB_565]	= { GL_RGB,  GL_UNSIGNED_SHORT_5_6_5_REV },
	[GE_RGBA_5551]	= { GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV },
	[GE_RGBA_4444]	= { GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV },
	[GE_RGBA_8888]	= { GL_RGBA, GL_UNSIGNED_BYTE },
};

void glCopyTexImage2D(GLenum target,
		      GLint level,
		      GLenum internalformat,
		      GLint x, GLint y,
		      GLsizei width, GLsizei height,
		      GLint border)
{
	GLenum fb_fmt, fb_type;
	struct pspgl_texobj *tobj;
	struct pspgl_teximg *timg;
	struct pspgl_surface *read;

	read = pspgl_curctx->read;
	fb_fmt = formats[read->pixfmt].format;
	fb_type = formats[read->pixfmt].type;

	/* glTexImage2D does all the hard work... */
	glTexImage2D(target, level, fb_fmt, width, height, border, fb_fmt, fb_type, NULL);
	if (pspgl_curctx->glerror != GL_NO_ERROR)
		return;

	tobj = pspgl_curctx->texture.bound;
	timg = tobj->images[level];

	assert(timg->texfmt->hwformat == read->pixfmt);
	assert(width == timg->width);
	assert(height == timg->height);

	/* The framebuffer and the texture are upside down with
	   respect to each other, so we need to flip the image (in the
	   framebuffer, lower addresses are in the upper-left, but for
	   textures, lower addresses are lower-left).

	   We can't do the flip in the copy (a negative stride doesn't
	   seem to work), so we set the per-texture object vflip flag
	   to reverse it.  This will do the wrong thing if some images
	   in the texture are from CopyTexImage, and some are created
	   normally.  It will also upset texsubimage and the use of
	   pixel buffer objects.

	   Not ideal, overall.
	*/
	y = read->height - y - height;
	tobj->vflip = 1;
	sendCommandf(CMD_TEXTURE_SV, -1.);
	sendCommandf(CMD_TEXTURE_TV, 1.);

	copy_pixels(read->color_buffer[!read->current_front], read->pixelperline, x, y,
		    timg->image.base, timg->width, 0, 0,
		    width, height, read->pixfmt);
	__pspgl_dlist_pin_buffer(&timg->image);

	sendCommandi(CMD_TEXCACHE_SYNC, getReg(CMD_TEXCACHE_SYNC)+1);
	sendCommandi(CMD_TEXCACHE_FLUSH, getReg(CMD_TEXCACHE_FLUSH)+1);

	return;
}
