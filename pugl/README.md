PUGL
====

Pugl is a minimal portable API for GUIs which supports embedding and is
suitable for use in plugins.  It works on X11, Mac OS X, and Windows.  GUIs can
be drawn with OpenGL or Cairo.

Pugl is vaguely similar to GLUT, but with some significant distinctions:

 * Minimal in scope, providing only what is necessary to draw and receive
   keyboard and mouse input.

 * No reliance on static data whatsoever, so the API can be used in plugins or
   multiple independent parts of a program.

 * Single implementation, which is small, liberally licensed Free / Open Source
   Software, and suitable for direct inclusion in programs if avoiding a
   library dependency is desired.

 * Support for embedding in other windows, so Pugl code can draw to a widget
   inside a larger GUI.

 * More complete support for keyboard input, including additional "special"
   keys, modifiers, and support for detecting individual modifier key presses.

For more information, see <http://drobilla.net/software/pugl>.

 -- David Robillard <d@drobilla.net>
