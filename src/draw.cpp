/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "draw.hpp"
#include "util.hpp"

#define UTF_INVALID 0xFFFD
#define UTF_SIZ 4

static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {0, 0, 0x80, 0x800, 0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

dmDrawable::dmDrawable(Display *dpy, int _screen, Window _root, int w, int h)
{
	display = dpy;
	screen = _screen;
	root = _root;
	width = w;
	height = h;
	drawable = XCreatePixmap(display, root, width, height, DefaultDepth(display, screen));
	gc = XCreateGC(display, root, 0, nullptr);
	XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
}
dmDrawable::~dmDrawable()
{
	XFreePixmap(display, drawable);
	XFreeGC(display, gc);
	drw_fontset_free(fonts);
}

void dmDrawable::dm_resize(int w, int h)
{
	width = w;
	height = h;
	if (drawable)
		XFreePixmap(display, drawable);
	drawable = XCreatePixmap(display, root, width, height, DefaultDepth(display, screen));
}

static long
utf8decodebyte(const char c, size_t *i)
{
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

static size_t
utf8validate(long *u, size_t i)
{
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

static size_t
utf8decode(const char *c, long *u, size_t clen)
{
	size_t i, j, len, type;
	long udecoded;

	*u = UTF_INVALID;
	if (!clen)
		return 0;
	udecoded = utf8decodebyte(c[0], &len);
	if (!BETWEEN(len, 1, UTF_SIZ))
		return 1;
	for (i = 1, j = 1; i < clen && j < len; ++i, ++j)
	{
		udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
		if (type)
			return j;
	}
	if (j < len)
		return 0;
	*u = udecoded;
	utf8validate(u, len);

	return len;
}
/*
dmDrawable *drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	dmDrawable *drw = ecalloc(1, sizeof(dmDrawable));

	drw->display = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w;
	drw->h = h;
	drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	drw->gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

	return drw;
}
*/
void drw_resize(dmDrawable *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	drw->width = w;
	drw->height = h;
	if (drw->drawable)
		XFreePixmap(drw->display, drw->drawable);
	drw->drawable = XCreatePixmap(drw->display, drw->root, w, h, DefaultDepth(drw->display, drw->screen));
}

void drw_free(dmDrawable *drw)
{
	XFreePixmap(drw->display, drw->drawable);
	XFreeGC(drw->display, drw->gc);
	drw_fontset_free(drw->fonts);
	free(drw);
}

/* This function is an implementation detail. Library users should use
 * drw_fontset_create instead.
 */
static Fnt *
xfont_create(dmDrawable *drw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	if (fontname)
	{
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(drw->display, drw->screen, fontname)))
		{
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *)fontname)))
		{
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(drw->display, xfont);
			return NULL;
		}
	}
	else if (fontpattern)
	{
		if (!(xfont = XftFontOpenPattern(drw->display, fontpattern)))
		{
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	}
	else
	{
		die("no font specified.");
	}

	/* Do not allow using color fonts. This is a workaround for a BadLength
	 * error from Xft with color glyphs. Modelled on the Xterm workaround. See
	 * https://bugzilla.redhat.com/show_bug.cgi?id=1498269
	 * https://lists.suckless.org/dev/1701/30932.html
	 * https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=916349
	 * and lots more all over the internet.
	 */
	FcBool iscol;
	if (FcPatternGetBool(xfont->pattern, FC_COLOR, 0, &iscol) == FcResultMatch && iscol)
	{
		XftFontClose(drw->display, xfont);
		return NULL;
	}

	font = new Fnt; //ecalloc<Fnt>(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;
	font->h = xfont->ascent + xfont->descent;
	font->dpy = drw->display;

	return font;
}

static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	XftFontClose(font->dpy, font->xfont);
	free(font);
}

Fnt *drw_fontset_create(dmDrawable *drw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	if (!drw || !fonts)
		return NULL;

	for (i = 1; i <= fontcount; i++)
	{
		if ((cur = xfont_create(drw, fonts[fontcount - i], NULL)))
		{
			cur->next = ret;
			ret = cur;
		}
	}
	return (drw->fonts = ret);
}

void drw_fontset_free(Fnt *font)
{
	if (font)
	{
		drw_fontset_free(font->next);
		xfont_free(font);
	}
}

void drw_clr_create(dmDrawable *drw, Clr *dest, const char *clrname)
{
	if (!drw || !dest || !clrname)
		return;

	if (!XftColorAllocName(drw->display, DefaultVisual(drw->display, drw->screen),
						   DefaultColormap(drw->display, drw->screen),
						   clrname, dest))
		die("error, cannot allocate color '%s'", clrname);
}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it. */
Clr *drw_scm_create(dmDrawable *drw, const char *clrnames[], size_t clrcount)
{
	size_t i;
	Clr *ret;

	/* need at least two colors for a scheme */
	if (!drw || !clrnames || clrcount < 2 || !(ret = new Clr /*ecalloc(clrcount, sizeof(XftColor))*/))
		return NULL;

	for (i = 0; i < clrcount; i++)
		drw_clr_create(drw, &ret[i], clrnames[i]);
	return ret;
}

void drw_setfontset(dmDrawable *drw, Fnt *set)
{
	if (drw)
		drw->fonts = set;
}

void drw_setscheme(dmDrawable *drw, Clr *scm)
{
	if (drw)
		drw->scheme = scm;
}

void drw_rect(dmDrawable *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	if (!drw || !drw->scheme)
		return;
	XSetForeground(drw->display, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	if (filled)
		XFillRectangle(drw->display, drw->drawable, drw->gc, x, y, w, h);
	else
		XDrawRectangle(drw->display, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

int drw_text(dmDrawable *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
{
	char buf[1024];
	int ty;
	unsigned int ew;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	size_t i, len;
	int utf8strlen, utf8charlen, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0;

	if (!drw || (render && !drw->scheme) || !text || !drw->fonts)
		return 0;

	if (!render)
	{
		w = ~w;
	}
	else
	{
		XSetForeground(drw->display, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(drw->display, drw->drawable, drw->gc, x, y, w, h);
		d = XftDrawCreate(drw->display, drw->drawable,
						  DefaultVisual(drw->display, drw->screen),
						  DefaultColormap(drw->display, drw->screen));
		x += lpad;
		w -= lpad;
	}

	usedfont = drw->fonts;
	while (1)
	{
		utf8strlen = 0;
		utf8str = text;
		nextfont = NULL;
		while (*text)
		{
			utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
			for (curfont = drw->fonts; curfont; curfont = curfont->next)
			{
				charexists = charexists || XftCharExists(drw->display, curfont->xfont, utf8codepoint);
				if (charexists)
				{
					if (curfont == usedfont)
					{
						utf8strlen += utf8charlen;
						text += utf8charlen;
					}
					else
					{
						nextfont = curfont;
					}
					break;
				}
			}

			if (!charexists || nextfont)
				break;
			else
				charexists = 0;
		}

		if (utf8strlen)
		{
			drw_font_getexts(usedfont, utf8str, utf8strlen, &ew, NULL);
			/* shorten text if necessary */
			for (len = MIN(utf8strlen, sizeof(buf) - 1); len && ew > w; len--)
				drw_font_getexts(usedfont, utf8str, len, &ew, NULL);

			if (len)
			{
				memcpy(buf, utf8str, len);
				buf[len] = '\0';
				if (len < utf8strlen)
					for (i = len; i && i > len - 3; buf[--i] = '.')
						; /* NOP */

				if (render)
				{
					ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
					XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
									  usedfont->xfont, x, ty, (XftChar8 *)buf, len);
				}
				x += ew;
				w -= ew;
			}
		}

		if (!*text)
		{
			break;
		}
		else if (nextfont)
		{
			charexists = 0;
			usedfont = nextfont;
		}
		else
		{
			/* Regardless of whether or not a fallback font is found, the
			 * character must be drawn. */
			charexists = 1;

			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (!drw->fonts->pattern)
			{
				/* Refer to the comment in xfont_create for more information. */
				die("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(drw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);
			FcPatternAddBool(fcpattern, FC_COLOR, FcFalse);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);
			match = XftFontMatch(drw->display, drw->screen, fcpattern, &result);

			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			if (match)
			{
				usedfont = xfont_create(drw, NULL, match);
				if (usedfont && XftCharExists(drw->display, usedfont->xfont, utf8codepoint))
				{
					for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				}
				else
				{
					xfont_free(usedfont);
					usedfont = drw->fonts;
				}
			}
		}
	}
	if (d)
		XftDrawDestroy(d);

	return x + (render ? w : 0);
}

void drw_map(dmDrawable *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	XCopyArea(drw->display, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	XSync(drw->display, False);
}

unsigned int
drw_fontset_getwidth(dmDrawable *drw, const char *text)
{
	if (!drw || !drw->fonts || !text)
		return 0;
	return drw_text(drw, 0, 0, 0, 0, 0, text, 0);
}

void drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	if (!font || !text)
		return;

	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	if (w)
		*w = ext.xOff;
	if (h)
		*h = font->h;
}

Cur *drw_cur_create(dmDrawable *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = new Cur /*ecalloc(1, sizeof(Cur))*/))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->display, shape);

	return cur;
}

void drw_cur_free(dmDrawable *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->display, cursor->cursor);
	free(cursor);
}