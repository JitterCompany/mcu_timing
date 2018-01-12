#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static struct
{
    int error_count;
    int warning_count;
    int info_count;
} g_log_mock_state;

void log_mock_state_reset()
{
    memset(&g_log_mock_state, 0, sizeof(g_log_mock_state));
}

void log_error(const char format[], ...)
{
    g_log_mock_state.error_count++;

    va_list va;
    va_start(va, format);

    printf("Error: ");
    vprintf(format,va);
    printf("\n");

    va_end(va);
}

void log_warning(const char format[], ...)
{
    g_log_mock_state.warning_count++;

    va_list va;
    va_start(va, format);

    printf("Warning: ");
    vprintf(format,va);
    printf("\n");

    va_end(va);
}

void log_info(const char format[], ...)
{
    g_log_mock_state.info_count++;

    va_list va;
    va_start(va, format);

    printf("Info: ");
    vprintf(format,va);
    printf("\n");

    va_end(va);
}

void log_debug(const char format[], ...)
{
    g_log_mock_state.info_count++;

    va_list va;
    va_start(va, format);

    printf("Debug: ");
    vprintf(format,va);
    printf("\n");

    va_end(va);
}

