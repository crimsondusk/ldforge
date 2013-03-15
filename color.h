#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

#ifdef QT_VERSION
 #include <QColor>
#endif // QT_VERSION

class color {
public:
	unsigned char r, g, b;
	
	color () {}
	color (const char* other) {
		parseFromString (str (other));
	}
	
	bool parseFromString (str in) {
#ifdef QT_VERSION
		// Use Qt's method for a quick color check. Handles
		// named colors, too.
		QColor col (in.chars ());
		if (col.isValid()) {
			r = col.red ();
			g = col.green ();
			b = col.blue ();
			return true;
		}
#else
		if (in[0] == '#') {
			// Hex code
			
			if (~in != 4 && ~in != 7)
				return false; // bad length
			
			printf ("%s\n", in.chars ());
			
			if (~in == 4) {
				in.format ("#%c%c%c%c%c%c",
					in[1], in[1],
					in[2], in[2],
					in[3], in[3]);
			}
			
			printf ("%s\n", in.chars ());
			
			if (sscanf (in.chars(), "#%2hhx%2hhx%2hhx", &r, &g, &b))
				return true;
		}
#endif // QT_VERSION
		
		return false;
	}
	
	str toString () {
		str val;
		val.format ("#%.2X%.2X%.2X", r, g, b);
		return val;
	}
	
	char* chars() {
		return toString ().chars ();
	}
	
	operator str () {
		return toString ();
	}
	
	void operator= (const str other) {
		parseFromString (other);
	}
};

#endif // __COLOR_H__