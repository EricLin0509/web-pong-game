#ifndef ANSI_COLOR_H
#define ANSI_COLOR_H

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

/* Browser console doesn't support ANSI escape codes */
#ifdef __EMSCRIPTEN__
#define ERROR_TEXT "[ERROR]"
#define WARNING_TEXT "[WARNING]"
#define INFO_TEXT "[INFO]"
#else
#define ERROR_TEXT ANSI_COLOR_RED "[ERROR]" ANSI_COLOR_RESET
#define WARNING_TEXT ANSI_COLOR_YELLOW "[WARNING]" ANSI_COLOR_RESET
#define INFO_TEXT ANSI_COLOR_BLUE "[INFO]" ANSI_COLOR_RESET
#endif

#endif /* ANSI_COLOR_H */