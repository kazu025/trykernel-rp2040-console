#include <trykernel.h>
#include <stdarg.h>

#include "mini_printf.h"

/*
 * 出力先管理
 */
typedef struct {
    char *buf;  /* 出力バッファ */
    UW size;    /* バッファサイズ */
    UW pos;     /* 実際に格納した文字数*/
    UW total;   /* 本来出力される文字数 */
} MINI_OUTPUT;

/* 内部関数 */
static void mini_output_char(MINI_OUTPUT* out, char ch);
static void mini_output_string(MINI_OUTPUT* out, const char *text);
static void mini_output_unsigned(MINI_OUTPUT* out, UINT value, UINT base);
static void mini_output_signed(MINI_OUTPUT* out, INT value);

/*
 * １文字を出力バッファへ追加
 */
static void mini_output_char(MINI_OUTPUT* out, char ch){
    /*
     * 終端文字用の１バイトを残して格納する。
     * バッファが満杯の場合もtotalは増加させる
     */
    if((out->buf != NULL) && (out->size > 0U) && (out->pos < (out->size - 1U))) {
        out->buf[out->pos] = ch;
        out->pos++;
    }
    out->total++;
}
/*
 * 文字列を出力バッファへ追加
 */
static void mini_output_string(MINI_OUTPUT* out, const char *text){
    if (text == NULL){
        text = "(null)";
    }
    while(*text != '\0'){
        mini_output_char(out, *text);
        text++;
     }
}
/*
 * 符号なし整数を指定進数で出力
 */
static void mini_output_unsigned(MINI_OUTPUT* out, UINT value, UINT base){
    static const char digits[] = "0123456789abcdef";
    char work[32];
    UW count = 0U;
    /* 0は特別処理 */
    if(value == 0){
        mini_output_char(out, '0');
        return;
    }
    /* 下位桁からworkへ */
    while((value != 0) && (count < (UW)sizeof(work))){
        work[count] = digits[value % base];
        count ++;
        value /= base;
    }
    /* 逆順で出力 */
    while(count > 0U){
        count--;
        mini_output_char(out, work[count]);
    }
}
/*
 * 符号付き10進数
 */
static void mini_output_signed(MINI_OUTPUT* out, INT value){
    UINT magnitude;
    if(value < 0){
        mini_output_char(out,'-');
        /*
         * INT最小値でもオーバーフローしないように
         * -(value + 1)の後で1を加える
         */
        magnitude =(UINT)(-(value + 1));
        magnitude++;
    } else {
        magnitude = (UINT)value;
    }
    mini_output_unsigned(out, magnitude, 10U);
}
/*
 * 可変長引数リストから文字列を整形
 */
INT mini_vsnprintf(char *buf, UW size, const char *format, va_list args){
    MINI_OUTPUT out;
    if(format == NULL){
        return E_PAR;
    }
    if ((buf == NULL) && (size > 0U)){
        return E_PAR;
    }

    out.buf     = buf;
    out.size    = size;
    out.pos     = 0U;
    out.total   = 0U;

    while(*format != '\0'){
        /* 通常文字 */
        if(*format != '%'){
            mini_output_char(&out, *format);
            format++;
            continue;
        }
        /* '%'の次の文字を調べる */
        format++;
        if (*format == '\0'){
            mini_output_char(&out, '%');
            break;
        }

        switch(*format){
        case 's':
            mini_output_string(&out, va_arg(args, const char*));
            break;
        case 'c':
            /* char は可変長引数ではintへ整数拡張される */
            mini_output_char(&out, (char)va_arg(args, int));
            break;
        case 'd':
            mini_output_signed(&out, (INT)va_arg(args, int));
            break;
        case 'u':
            mini_output_unsigned(&out, (UINT)va_arg(args, unsigned int), 10U);
            break;
        case 'x':
            mini_output_unsigned(&out, (UINT)va_arg(args, unsigned int), 16U);
            break;
        case '%':
            mini_output_char(&out, '%');
            break;
        default:
            /* 未対応の指定はそのまま表示する */
            mini_output_char(&out, '%');
            mini_output_char(&out, *format);
            break;
        }
        format++;
    }
    /* 文字列終端を保証する */
    if((buf != NULL) && (size > 0 )){
        buf[out.pos] = '\0';
    }
    return (INT)out.total;
}

/*
 * 可変長引数を受け取って文字列を整形
 */
INT mini_snprintf(char* buf, UW size, const char* format, ...){
    va_list args;
    INT result;

    va_start(args, format);
    result = mini_vsnprintf(buf, size, format, args);
    va_end(args);
    return result;
}
