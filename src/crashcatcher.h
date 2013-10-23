#ifndef LDFORGE_CRASHCATCHER_H
#define LDFORGE_CRASHCATCHER_H

#ifdef __unix__

void initCrashCatcher();

#else // ifdef __unix__
#define initCrashCatcher()
#endif // ifdef __unix__
#endif // ifndef LDFORGE_CRASHCATCHER_H