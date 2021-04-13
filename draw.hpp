/* See LICENSE file for copyright and license details. */

typedef struct {
	Cursor cursor;
} Cur;

typedef struct Fnt {
	Display *dpy;
	unsigned int h;
	XftFont *xfont;
	FcPattern *pattern;
	struct Fnt *next;
} Fnt;

enum { ColFg, ColBg, ColBorder }; /* Clr scheme index */
typedef XftColor Clr;

typedef struct {
	unsigned int w, h;
	Display *dpy;
	int screen;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme;
	Fnt *fonts;
} dmDrawable;

class dmDrawable
{
public:
	int width, height, screen;
	Display *display = nullptr;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme = nullptr;
	Fnt *fonts = nullptr;
	dmDrawable(Display *dpy, int _screen, Window _root, int w, int h);
	~dmDrawable();
	void dm_resize(int w, int h);
};

/* Drawable abstraction */
dmDrawable *drw_create(Display *dpy, int screen, Window win, unsigned int w, unsigned int h);
void drw_resize(dmDrawable *drw, unsigned int w, unsigned int h);
void drw_free(dmDrawable *drw);

/* Fnt abstraction */
Fnt *drw_fontset_create(dmDrawable* drw, const char *fonts[], size_t fontcount);
void drw_fontset_free(Fnt* set);
unsigned int drw_fontset_getwidth(dmDrawable *drw, const char *text);
void drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h);

/* Colorscheme abstraction */
void drw_clr_create(dmDrawable *drw, Clr *dest, const char *clrname);
Clr *drw_scm_create(dmDrawable *drw, const char *clrnames[], size_t clrcount);

/* Cursor abstraction */
Cur *drw_cur_create(dmDrawable *drw, int shape);
void drw_cur_free(dmDrawable *drw, Cur *cursor);

/* Drawing context manipulation */
void drw_setfontset(dmDrawable *drw, Fnt *set);
void drw_setscheme(dmDrawable *drw, Clr *scm);

/* Drawing functions */
void drw_rect(dmDrawable *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert);
int drw_text(dmDrawable *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert);

/* Map functions */
void drw_map(dmDrawable *drw, Window win, int x, int y, unsigned int w, unsigned int h);
