/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include "fluid_sys.h"


#if READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef DBUS_SUPPORT
#include "fluid_rtkit.h"
#endif

#if HAVE_PTHREAD_H && !defined(_WIN32)
// Do not include pthread on windows. It includes winsock.h, which collides with ws2tcpip.h from fluid_sys.h
// It isn't need on Windows anyway.
#include <pthread.h>
#endif

/* WIN32 HACK - Flag used to differentiate between a file descriptor and a socket.
 * Should work, so long as no SOCKET or file descriptor ends up with this bit set. - JG */
#ifdef _WIN32
#define FLUID_SOCKET_FLAG      0x40000000
#else
#define FLUID_SOCKET_FLAG      0x00000000
#define SOCKET_ERROR           -1
#define INVALID_SOCKET         -1
#endif

/* SCHED_FIFO priority for high priority timer threads */
#define FLUID_SYS_TIMER_HIGH_PRIO_LEVEL         10


static fluid_log_function_t fluid_log_function[LAST_LOG_LEVEL] =
{
    fluid_default_log_function,
    fluid_default_log_function,
    fluid_default_log_function,
    fluid_default_log_function,
#ifdef DEBUG
    fluid_default_log_function
#else
    NULL
#endif
};
static void *fluid_log_user_data[LAST_LOG_LEVEL] = { NULL };

static const char fluid_libname[] = "fluidsynth";

/**
 * Installs a new log function for a specified log level.
 * @param level Log level to install handler for.
 * @param fun Callback function handler to call for logged messages
 * @param data User supplied data pointer to pass to log function
 * @return The previously installed function.
 */
fluid_log_function_t
fluid_set_log_function(int level, fluid_log_function_t fun, void *data)
{
    fluid_log_function_t old = NULL;

    if((level >= 0) && (level < LAST_LOG_LEVEL))
    {
        old = fluid_log_function[level];
        fluid_log_function[level] = fun;
        fluid_log_user_data[level] = data;
    }

    return old;
}

/**
 * Default log function which prints to the stderr.
 * @param level Log level
 * @param message Log message
 * @param data User supplied data (not used)
 */
void
fluid_default_log_function(int level, const char *message, void *data)
{
    FILE *out;

#if defined(_WIN32)
    out = stdout;
#else
    out = stderr;
#endif

    switch(level)
    {
    case FLUID_PANIC:
        FLUID_FPRINTF(out, "%s: panic: %s\n", fluid_libname, message);
        break;

    case FLUID_ERR:
        FLUID_FPRINTF(out, "%s: error: %s\n", fluid_libname, message);
        break;

    case FLUID_WARN:
        FLUID_FPRINTF(out, "%s: warning: %s\n", fluid_libname, message);
        break;

    case FLUID_INFO:
        FLUID_FPRINTF(out, "%s: %s\n", fluid_libname, message);
        break;

    case FLUID_DBG:
        FLUID_FPRINTF(out, "%s: debug: %s\n", fluid_libname, message);
        break;

    default:
        FLUID_FPRINTF(out, "%s: %s\n", fluid_libname, message);
        break;
    }

    fflush(out);
}

/**
 * Print a message to the log.
 * @param level Log level (#fluid_log_level).
 * @param fmt Printf style format string for log message
 * @param ... Arguments for printf 'fmt' message string
 * @return Always returns #FLUID_FAILED
 */
int
fluid_log(int level, const char *fmt, ...)
{
    if((level >= 0) && (level < LAST_LOG_LEVEL))
    {
        fluid_log_function_t fun = fluid_log_function[level];

        if(fun != NULL)
        {
            char errbuf[1024];
            
            va_list args;
            va_start(args, fmt);
            FLUID_VSNPRINTF(errbuf, sizeof(errbuf), fmt, args);
            va_end(args);
        
            (*fun)(level, errbuf, fluid_log_user_data[level]);
        }
    }

    return FLUID_FAILED;
}

/**
 * An improved strtok, still trashes the input string, but is portable and
 * thread safe.  Also skips token chars at beginning of token string and never
 * returns an empty token (will return NULL if source ends in token chars though).
 * NOTE: NOT part of public API
 * @internal
 * @param str Pointer to a string pointer of source to tokenize.  Pointer gets
 *   updated on each invocation to point to beginning of next token.  Note that
 *   token char gets overwritten with a 0 byte.  String pointer is set to NULL
 *   when final token is returned.
 * @param delim String of delimiter chars.
 * @return Pointer to the next token or NULL if no more tokens.
 */
char *fluid_strtok(char **str, char *delim)
{
    char *s, *d, *token;
    char c;

    if(str == NULL || delim == NULL || !*delim)
    {
        FLUID_LOG(FLUID_ERR, "Null pointer");
        return NULL;
    }

    s = *str;

    if(!s)
    {
        return NULL;    /* str points to a NULL pointer? (tokenize already ended) */
    }

    /* skip delimiter chars at beginning of token */
    do
    {
        c = *s;

        if(!c)	/* end of source string? */
        {
            *str = NULL;
            return NULL;
        }

        for(d = delim; *d; d++)	/* is source char a token char? */
        {
            if(c == *d)	/* token char match? */
            {
                s++;		/* advance to next source char */
                break;
            }
        }
    }
    while(*d);		/* while token char match */

    token = s;		/* start of token found */

    /* search for next token char or end of source string */
    for(s = s + 1; *s; s++)
    {
        c = *s;

        for(d = delim; *d; d++)	/* is source char a token char? */
        {
            if(c == *d)	/* token char match? */
            {
                *s = '\0';	/* overwrite token char with zero byte to terminate token */
                *str = s + 1;	/* update str to point to beginning of next token */
                return token;
            }
        }
    }

    /* we get here only if source string ended */
    *str = NULL;
    return token;
}

/**
 * Get time in milliseconds to be used in relative timing operations.
 * @return Monotonic time in milliseconds.
 */
unsigned int fluid_curtime(void)
{
    double now;
    static double initial_time = 0;

    if(initial_time == 0)
    {
        initial_time = fluid_utime();
    }

    now = fluid_utime();

    return (unsigned int)((now - initial_time) / 1000.0);
}
