#ifndef MINI_PRINTF_H
#define MINI_PRINTF_H

#include <trykernel.h>
#include <stdarg.h>

/*
 * 可変長引数リストを使用して文字列を整形する
 */
INT mini_vsnprintf(
    char *buf,
    UW size,
    const char *format,
    va_list args
);

/*
 * 可変長引数を使用して文字列を整形する
 */
INT mini_snprintf(
    char *buf,
    UW size,
    const char *format,
    ...
);

#endif /* MINI_PRINTF_H */