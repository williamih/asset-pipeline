#ifndef OS_SOCKETFUNCTIONS_H
#define OS_SOCKETFUNCTIONS_H

// N.B. This header references OS-specific files, and so should only be
// #included by .cpp files, and >not< other header files.

// Provides access to the select() function.

#ifdef _WIN32
#  error Windows not supported yet
#else
#  include <sys/select.h>
#endif

#endif // OS_SOCKETFUNCTIONS_H
