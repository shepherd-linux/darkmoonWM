/* cc transient.c -o transient -lX11 */

//#include <stdlib.h>
//#include <unistd.h>
#include <chrono>
#include <thread>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "config.hpp"

int main()
{
	std::copy(init_c1.begin(), init_c1.end(), colors[0]);
	std::copy(init_c2.begin(), init_c2.end(), colors[1]);
	Display *display;
	Window root_w, float_w, transient = 0;
	XSizeHints hint;
	XEvent e;

	display = XOpenDisplay(nullptr); //NULL
	if (!display)
		return 1 ; //exit(1);
	root_w = DefaultRootWindow(display);

	float_w = XCreateSimpleWindow(display, root_w, 100, 100, 400, 400, 0, 0, 0);
	hint.min_width = hint.max_width = hint.min_height = hint.max_height = 400;
	hint.flags = PMinSize | PMaxSize;
	XSetWMNormalHints(display, float_w, &hint);
	XStoreName(display, float_w, "floating");
	XMapWindow(display, float_w);

	XSelectInput(display, float_w, ExposureMask);
	while (true)
	{
		XNextEvent(display, &e);

		if (transient == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			transient = XCreateSimpleWindow(display, root_w, 50, 50, 100, 100, 0, 0, 0);
			XSetTransientForHint(display, transient, float_w);
			XStoreName(display, transient, "transient");
			XMapWindow(display, transient);
			XSelectInput(display, transient, ExposureMask);
		}
	}

	XCloseDisplay(display);
	return 0; //exit(0);
}
