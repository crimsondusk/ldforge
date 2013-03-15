#include <stdio.h>
// #include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "str.h"
#include "config.h"

#ifdef CONFIG_WITH_QT
#include <QDir>
#endif // CONFIG_WITH_QT

// =============================================================================
// Define the configs
#define CFG(TYPE, SECT, NAME, DESCR, DEFAULT) \
	TYPE##config SECT##_##NAME (CFGSECTNAME (SECT), DESCR, \
		DEFAULT, #NAME, #SECT "_" #NAME, #TYPE, #DEFAULT);

#define SECT(...)
 #include "cfgdef.h"
#undef CFG
#undef SECT

// =============================================================================
config* config::pointers[] = {
#define CFG(TYPE, SECT, NAME, DESCR, DEFAULT) &SECT##_##NAME,
#define SECT(...)
 #include "cfgdef.h"
#undef CFG
#undef SECT
};

// =============================================================================
const char* config::sections[] = {
#define CFG(...)
#define SECT(A,B) #A,
 #include "cfgdef.h"
#undef CFG
#undef SECT
};

// =============================================================================
const char* config::sectionNames[] = {
#define CFG(...)
#define SECT(A,B) #B,
 #include "cfgdef.h"
#undef CFG
#undef SECT
};

// =============================================================================
const char* g_WeekdayNames[7] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
};

// =============================================================================
static const char* g_MonthNames[12] = {
	"Januray",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November"
	"December",
};

static const char* g_ConfigTypeNames[] = {
	"None",
	"Integer",
	"String",
	"Float",
	"Boolean",
	"Color",
};

// =============================================================================
// Load the configuration from file
bool config::load () {
	FILE* fp = fopen (filepath().chars(), "r");
	char linedata[MAX_INI_LINE];
	char* line;
	size_t ln = 0;
	configsection_e section = NO_CONFIG_SECTION;
	
	if (!fp)
		return false; // can't open for reading
	
	// Read the values.
	while (fgets (linedata, MAX_INI_LINE, fp)) {
		ln++;
		line = linedata;
		
		while (*line != 0 && (*line <= 32 || *line >= 127))
			line++; // Skip junk
		
		if (*line == '\0' || line[0] == '#')
			continue; // Empty line or comment.
		
		if (line[0] == '[') {
			// Section
			char* endbracket = strchr (line, ']');
			
			if (!endbracket) {
				fprintf (stderr, "badly formed section: %s", line);
				continue;
			}
			
			str sectionName = str (line).substr (1, endbracket - line);
			const configsection_e oldsection = section;
			section = NO_CONFIG_SECTION;
			
			// Find the section
			for (unsigned i = 0; i < NUM_ConfigSections && section == NO_CONFIG_SECTION; i++)
				if (sectionName.compare (sectionNames[i]) == 0)
					section = (configsection_e)i;
			
			if (section == NO_CONFIG_SECTION) {
				fprintf (stderr, "unknown config section `%s`\n", sectionName.chars());
				section = oldsection;
			}
			
			continue;
		}
		
		// Find the equals sign.
		char* equals = strchr (line, '=');
		if (!equals) {
			fprintf (stderr, "couldn't find `=` sign in entry `%s`\n", line);
			continue;
		}
		
		str entry = str (line).substr (0, equals - line);
		
		str configname;
		configname.format ("%s_%s", sections[section], entry.chars ());
		
		// Find the config entry for this.
		config* cfg = NULL;
		for (size_t i = 0; i < NUM_CONFIG && !cfg; i++)
			if (configname.compare (pointers[i]->fullname) == 0)
				cfg = pointers[i];
		
		if (!cfg) {
			fprintf (stderr, "unknown config `%s`\n", configname.chars());
			continue;
		}
		
		str valstring = str (line).substr (equals - line + 1, -1);
		
		// Trim the crap off the end
		while (~valstring) {
			char c = valstring[~valstring - 1];
			if (c <= 32 || c >= 127)
				valstring -= 1;
			else
				break;
		}
		
		switch (const_cast<config*> (cfg)->getType()) {
		case CONFIG_int:
			static_cast<intconfig*> (cfg)->value = atoi (valstring.chars());
			break;
		case CONFIG_str:
			static_cast<strconfig*> (cfg)->value = valstring;
			break;
		case CONFIG_float:
			static_cast<floatconfig*> (cfg)->value = atof (valstring.chars());
			break;
		case CONFIG_bool:
		{
			bool& val = static_cast<boolconfig*> (cfg)->value;
			
			if (+valstring == "TRUE" || valstring == "1")
				val = true;
			else if (+valstring == "FALSE" || valstring == "0")
				val = false;
			break;
		}
		case CONFIG_color:
			static_cast<colorconfig*> (cfg)->value.parseFromString (valstring);
			break;
		default:
			break;
		}
	}
	
	fclose (fp);
	return true;
}

// =============================================================================
// Write a given formatted string to the given file stream
static size_t writef (FILE* fp, const char* fmt, ...) {
	va_list va;
	
	va_start (va, fmt);
	char* buf = vdynformat (fmt, va, 256);
	va_end (va);
	
	size_t len = fwrite (buf, 1, strlen (buf), fp);
	delete[] buf;
	
	return len;
}

// =============================================================================
// Save the configuration to disk
bool config::save () {
#ifdef APPNAME
	#ifdef CONFIG_WITH_QT
		// If the directory doesn't exist, create it now.
		if (!QDir (dirpath().chars()).exists ()) {
			fprintf (stderr, "Creating config path %s...\n", dirpath().chars());
			if (!QDir ().mkpath (dirpath().chars())) {
				fprintf (stderr, "Failed to create the directory. Configuration cannot be saved!\n");
				return false; // Couldn't create directory
			}
		}
	#else
		#warning Need QT to check for directories. Config will not be able
		#warning to save properly if ~/.APPNAME/ does not exist.
	#endif // CONFIG_WITH_QT
#endif // CONFIG_DIRECTORY
	
	FILE* fp = fopen (filepath().chars(), "w");
	printf ("writing cfg to %s\n", filepath().chars());
	
	if (!fp) {
		printf ("Couldn't open %s for writing\n", filepath().chars());
		return false;
	}
	
	const time_t curtime = time (NULL);
	const struct tm* timeinfo = localtime (&curtime);
	const char* daysuffix =
		(timeinfo->tm_mday % 10 == 1) ? "st" :
		(timeinfo->tm_mday % 10 == 2) ? "nd" :
		(timeinfo->tm_mday % 10 == 3) ? "rd" : "th";
	
	writef (fp, "# Configuration file for " APPNAME "\n");
	writef (fp, "# Written on %s, %s %d%s %d %.2d:%.2d:%.2d\n",
		g_WeekdayNames[timeinfo->tm_wday], g_MonthNames[timeinfo->tm_mon],
		timeinfo->tm_mday, daysuffix, timeinfo->tm_year + 1900,
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	writef (fp, "\n");
	
	for (int i = 0; i < NUM_ConfigSections; i++) {
		if (i > 0)
			writef (fp, "\n");
		
		writef (fp, "[%s]\n", sectionNames[i]);
		bool first = true;
		
		for (size_t j = 0; j < NUM_CONFIG; j++) {
			config* cfg = pointers[j];
			
			if (cfg->sect != i)
				continue;
			
			if (!first)
				writef (fp, "\n");
			
			str valstring;
			switch (cfg->getType()) {
			case CONFIG_int:
				valstring.format ("%d", static_cast<intconfig*> (cfg)->value);
				break;
			case CONFIG_str:
				valstring = static_cast<strconfig*> (cfg)->value;
				break;
			case CONFIG_float:
				valstring.format ("%f", static_cast<floatconfig*> (cfg)->value);
				
				// Trim any trailing zeros
				if (valstring.first (".") != -1) {
					while (valstring[~valstring - 1] == '0')
						valstring -= 1;
					
					// But don't trim the only one out...
					if (valstring[~valstring - 1] == '.')
						valstring += '0';
				}
				
				break;
			case CONFIG_bool:
				valstring = (static_cast<boolconfig*> (cfg)->value) ? "true" : "false";
				break;
			case CONFIG_color:
				valstring = (str)(static_cast<colorconfig*> (cfg)->value);
				break;
			default:
				break;
			}
			
			// Write the entry now.
			writef (fp, "# %s, default: %s\n", g_ConfigTypeNames[cfg->getType()], cfg->defaultstring);
			writef (fp, "# %s: %s\n", cfg->fullname, cfg->description);
			writef (fp, "%s=%s\n", cfg->name, valstring.chars());
			first = false;
		}
	}
	
	fclose (fp);
	return true;
}

// =============================================================================
void config::reset () {
	for (size_t i = 0; i < NUM_CONFIG; i++)
		pointers[i]->resetValue ();
}

// =============================================================================
str config::filepath () {
#ifdef APPNAME
	str path;
	path.format ("%s" APPNAME ".ini", dirpath().chars());
	return path;
#else // APPNAME
	return "config.ini";
#endif // APPNAME
}

// =============================================================================
str config::dirpath () {
#ifdef APPNAME
	str path;
	
	#ifdef CONFIG_WITH_QT
		path = (QDir::homePath ().toStdString().c_str());
	#else // CONFIG_WITH_QT
		path = "~";
	#endif // CONFIG_WITH_QT
	
	path += "/." APPNAME "/";
#else
	path = "./";
#endif // APPNAME
	
	return path;
}