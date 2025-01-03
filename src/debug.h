#ifndef DNAFX_DEBUG_H
#define DNAFX_DEBUG_H

#include <inttypes.h>

#include <glib.h>
#include <glib/gprintf.h>

extern int dnafx_log_level;
extern gboolean dnafx_log_timestamps;
extern gboolean dnafx_log_colors;

/* Log colors */
#define DNAFX_ANSI_COLOR_RED     "\x1b[31m"
#define DNAFX_ANSI_COLOR_GREEN   "\x1b[32m"
#define DNAFX_ANSI_COLOR_YELLOW  "\x1b[33m"
#define DNAFX_ANSI_COLOR_BLUE    "\x1b[34m"
#define DNAFX_ANSI_COLOR_MAGENTA "\x1b[35m"
#define DNAFX_ANSI_COLOR_CYAN    "\x1b[36m"
#define DNAFX_ANSI_COLOR_RESET   "\x1b[0m"

/* Log levels */
#define DNAFX_LOG_NONE     (0)
#define DNAFX_LOG_FATAL    (1)
#define DNAFX_LOG_ERR      (2)
#define DNAFX_LOG_WARN     (3)
#define DNAFX_LOG_INFO     (4)
#define DNAFX_LOG_VERB     (5)
#define DNAFX_LOG_HUGE     (6)
#define DNAFX_LOG_DBG      (7)
#define DNAFX_LOG_MAX DNAFX_LOG_DBG

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
/*  Coloured prefixes for errors and warnings logging. */
static const char *dnafx_log_prefix[] = {
/* no colors */
	"",
	"[FATAL] ",
	"[ERR] ",
	"[WARN] ",
	"",
	"",
	"",
	"",
/* with colors */
	"",
	DNAFX_ANSI_COLOR_MAGENTA"[FATAL]"DNAFX_ANSI_COLOR_RESET" ",
	DNAFX_ANSI_COLOR_RED"[ERR]"DNAFX_ANSI_COLOR_RESET" ",
	DNAFX_ANSI_COLOR_YELLOW"[WARN]"DNAFX_ANSI_COLOR_RESET" ",
	"",
	"",
	"",
	""
};
#pragma GCC diagnostic pop

#define DNAFX_COLOR_BOLD	"\e[1m"
#define DNAFX_COLOR_OFF		"\e[m"

/* Log wrappers */
#define DNAFX_PRINT g_print
#define DNAFX_LOG(level, format, ...) \
do { \
	if (level > DNAFX_LOG_NONE && level <= DNAFX_LOG_MAX && level <= dnafx_log_level) { \
		char dnafx_log_ts[64] = ""; \
		char dnafx_log_src[128] = ""; \
		if (dnafx_log_timestamps) { \
			struct tm dnafxtmresult; \
			time_t dnafxltime = time(NULL); \
			localtime_r(&dnafxltime, &dnafxtmresult); \
			strftime(dnafx_log_ts, sizeof(dnafx_log_ts), \
			         "[%a %b %e %T %Y] ", &dnafxtmresult); \
		} \
		if (level == DNAFX_LOG_FATAL || level == DNAFX_LOG_ERR || level == DNAFX_LOG_DBG) { \
			snprintf(dnafx_log_src, sizeof(dnafx_log_src), \
			         "[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); \
		} \
		g_print("%s%s%s" format, \
		        dnafx_log_ts, \
		        dnafx_log_prefix[level | ((int)dnafx_log_colors << 3)], \
		        dnafx_log_src, \
		        ##__VA_ARGS__); \
	} \
} while (0)

#endif
