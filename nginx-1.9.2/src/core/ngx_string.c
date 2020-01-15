
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,
    u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width);
static void ngx_encode_base64_internal(ngx_str_t *dst, ngx_str_t *src,
    const u_char *basis, ngx_uint_t padding);
static ngx_int_t ngx_decode_base64_internal(ngx_str_t *dst, ngx_str_t *src,
    const u_char *basis);

//��д��ĸת��ΪСд��ĸ
void
ngx_strlow(u_char *dst, u_char *src, size_t n)
{
    while (n) {
        *dst = ngx_tolower(*src);
        dst++;
        src++;
        n--;
    }
}


u_char *
ngx_cpystrn(u_char *dst, u_char *src, size_t n)
{
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0';

    return dst;
}


u_char *
ngx_pstrdup(ngx_pool_t *pool, ngx_str_t *src)
{
    u_char  *dst;

    dst = ngx_pnalloc(pool, src->len);
    if (dst == NULL) {
        return NULL;
    }

    ngx_memcpy(dst, src->data, src->len);

    return dst;
}


/*
 * supported formats:
 *    %[0][width][x][X]O        off_t
 *    %[0][width]T              time_t
 *    %[0][width][u][x|X]z      ssize_t/size_t
 *    %[0][width][u][x|X]d      int/u_int
 *    %[0][width][u][x|X]l      long
 *    %[0][width|m][u][x|X]i    ngx_int_t/ngx_uint_t
 *    %[0][width][u][x|X]D      int32_t/uint32_t
 *    %[0][width][u][x|X]L      int64_t/uint64_t
 *    %[0][width|m][u][x|X]A    ngx_atomic_int_t/ngx_atomic_uint_t
 *    %[0][width][.width]f      double, max valid number fits to %18.15f
 *    %P                        ngx_pid_t
 *    %M                        ngx_msec_t
 *    %r                        rlim_t
 *    %p                        void *
 *    %V                        ngx_str_t *
 *    %v                        ngx_variable_value_t *
 *    %s                        null-terminated string
 *    %*s                       length and string
 *    %Z                        '\0'
 *    %N                        '\n'
 *    %c                        char
 *    %%                        %
 *
 *  reserved:
 *    %t                        ptrdiff_t
 *    %S                        null-terminated wchar string
 *    %C                        wchar
 */


u_char * ngx_cdecl
ngx_sprintf(u_char *buf, const char *fmt, ...)
{
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, (void *) -1, fmt, args);
    va_end(args);

    return p;
}


u_char * ngx_cdecl
ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...)
{
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, buf + max, fmt, args);
    va_end(args);

    return p;
}


u_char * ngx_cdecl
ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...)
{
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);

    return p;
}

/*
��4-8��ӡ��־����ʹ��ngx_sprintfϵ�з���ת���ַ���ʱ֧�ֵ�27��ת����ʽ
�������������ש�����������������������������������������������������������������������������������������
��ת����ʽ  ��    �÷�                                                                                ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ��ʾ�޷��ţ���󻹿��Ը�����ת�����ţ���%ui��ʾҪת����������ngx_uint_t��������û   ��
��%U        ��                                                                                        ��
��          ���и�ת�����ţ����ʾҪת�����������޷���ʮ��������                                      ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%m        ��  ��ʾ����󳤶���ת����������(��int)                                                   ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ��ʮ����������ʽ��ת��������ݡ�ע�⣬Nginx�е�%X��printf��ת����ʽ��ȫ��ͬ����ֻ     ��
��          ��������ת�����������ʮ�����Ƹ�ʽ����ʾ��������������Ӧ���������͡����磬%Xd�����int    ��
��%X        ��                                                                                        ��
��          �����ͣ���ʾ��ʮ�����Ƹ�ʽ����ʾint��������%Xp��ʾ��ʮ�����Ƹ�ʽ����ʾָ���ַ�������    ��
��          ����%X����ô��û���κ������                                                              ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  %X��%X���÷���ȫ��ͬ��ֻ��%X��A��B��C��D��E��F��ʾʮ�����е�10��11��12��13��          ��
��%x        ��                                                                                        ��
��          ��14��15����%x����Сд��a��b��c��d��e��f����ʾ                                            ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ������������֡���ǰʵ�ְ汾�±�����%f���ʹ�ã���ʾת��������ʱС�����ֵ�λ������  ��
��%��       ��                                                                                        ��
��          ���磬%��lOf��ʾת��double����ʱ��С�����ת���ұ���ת��Ϊ10λ������IOλ��O�           ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ת��double�������ݡ�ע�⣬����printf�ȱ�׼C�����е�%f��ȫ��ͬ�������ת��С�����֣�   ��
��%f        ��                                                                                        ��
��          ����������%��(number)f���μ�������%��������                                             ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%+        ��  ��ʾҪת�����ַ����ȡ�Ŀǰ����%s���ʹ��                                              ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ת��1��char~����u- char~���ַ�������%+���ʹ��ʱ��%ts��ʾ���ָ�����ȵ��ַ�������     ��
��%S        ���������������������ʾ����ַ������ȵ�size_t���ַ�����ַchar~���͡��������%+���ʹ�ã� ��
��          ������printf�ȱ�׼��ʽ��ͬ����ô�ַ��������ԡ���0����β                                   ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%V        ��                                                                                        ��
��          ��  ת��ngx_str_t���ͣ�%V��Ӧ�Ĳ���������ngx_str_t�����ĵ�ַ�������ᰴ��ngx_str_t���͵�   ��
��          ��len���������data�ַ���                                                                 ��
�ǩ����������贈����������������������������������������������������������������������������������������
��          ��  ת��ngx_variable_value_t���ͣ�%V��Ӧ�Ĳ���������ngx_variable_valuej�����ĵ�ַ�������� ��
��%V        ��                                                                                        ��
��          ������ngx_variable_value-t���͵�len���������data�ַ���                                   ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%O        ��  ת��1��offt����                                                                       ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%P        ��  ת��1��ngx_pid_t����                                                                  ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%T        ��  ת��1��time��t����                                                                    ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%M        ��  ת��1��ngx_msec_t����                                                                 ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%Z        ��  ת��ssize_t�������ݣ������%UZ����ת��������������size-t                              ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%i        ��  ת��ngx_int_t�����ݣ������%ui����ת��������������ngx_uint_t                          ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%d        ��  ת��int�����ݣ������%ud����ת��������������u_int                                     ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%1        ��  ת��long�����ݣ������%ul����ת��������������u_long                                   ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%D        ��  ת��int32-t�����ݣ������%uD����ת��������������uint32-t                              ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%L        ��  ת��int64j�����ݣ������%uL����ת��������������uint64 t                               ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%A        ��  ת��ngx_atomic_int_t�����ݣ������%uA����ת��������������ngx_atomic_uint_t            ��
�������������ߩ�����������������������������������������������������������������������������������������
�������������ש�����������������������������������������������������������������������������������������
��ת����ʽ  ��    �÷�                                                                                ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%r        ��                                                                                        ��
��          ��  ת��1��rlimj���͡�ϵͳ����getrlimit����setrlimitʱ����ʹ��rlimj���Ͳ�������ʵ������   ��
��          ��һ�������������ͣ���ͬ������int��size t����offt                                         ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%p        ��  ת��1��ָ�루��ַ��                                                                   ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%C        ��  ת��1���ַ�����                                                                       ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%Z        ��  ��ʾ����O��                                                                           ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%N        ��  ��ʾ����n�����з�������\xOa������windows����ϵͳ�����ʾ��\r\n����Ҳ���ǡ�\xOd\xOa��  ��
�ǩ����������贈����������������������������������������������������������������������������������������
��%%        ��  ��ӡ1���ٷֺ�(%)                                                                      ��
�������������ߩ�����������������������������������������������������������������������������������������
���磬��4.2.4���Զ����ngx_c onf_set_myc onfig�����У��������������־��
long tl = 4900000000;
u_long tul = 5000000000;
int32_t ti32 = 110;
ngx_str_t   tstr  =  ngx_string ( " teststr" ) ;
double   tdoub   =  3.1415 926535897932 ;
int x = 15;
ngx_log_error (NGX_LOG_ALERT , cf->log ,  0 ,
              " l= %l , ul=%ul , D=%D, p=%p, f=%.lOf , str=%V, x=%xd, X=%Xd "
                tl, tul, ti32 , &ti3 2 , tdoub , &tstr, x, x)  ;
������δ��뽫�������
      nginx :   [alert]   1=4900000000 , ul=5000000000 , D=110, p=00007FFFF26836DC, f=3 . 1415926536
, str=teststr, x=f , X=F
*/
u_char *
ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args)
{
    u_char                *p, zero;
    int                    d;
    double                 f;
    size_t                 len, slen;
    int64_t                i64;
    uint64_t               ui64, frac;
    ngx_msec_t             ms;
    ngx_uint_t             width, sign, hex, max_width, frac_width, scale, n;
    ngx_str_t             *v;
    ngx_variable_value_t  *vv;

    while (*fmt && buf < last) {

        /*
         * "buf < last" means that we could copy at least one character:
         * the plain character, "%%", "%c", and minus without the checking
         */

        if (*fmt == '%') {

            i64 = 0;
            ui64 = 0;

            zero = (u_char) ((*++fmt == '0') ? '0' : ' ');
            width = 0;
            sign = 1;
            hex = 0;
            max_width = 0;
            frac_width = 0;
            slen = (size_t) -1;

            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt++ - '0';
            }


            for ( ;; ) {
                switch (*fmt) {

                case 'u':
                    sign = 0;
                    fmt++;
                    continue;

                case 'm':
                    max_width = 1;
                    fmt++;
                    continue;

                case 'X':
                    hex = 2;
                    sign = 0;
                    fmt++;
                    continue;

                case 'x':
                    hex = 1;
                    sign = 0;
                    fmt++;
                    continue;

                case '.':
                    fmt++;

                    while (*fmt >= '0' && *fmt <= '9') {
                        frac_width = frac_width * 10 + *fmt++ - '0';
                    }

                    break;

                case '*':
                    slen = va_arg(args, size_t);
                    fmt++;
                    continue;

                default:
                    break;
                }

                break;
            }


            switch (*fmt) {

            case 'V': //���ngx_str_t��������
                v = va_arg(args, ngx_str_t *);
                if(v == NULL)
                    continue;
                    
                len = ngx_min(((size_t) (last - buf)), v->len);
                buf = ngx_cpymem(buf, v->data, len);
                fmt++;

                continue;

            case 'v':
                vv = va_arg(args, ngx_variable_value_t *);

                len = ngx_min(((size_t) (last - buf)), vv->len);
                buf = ngx_cpymem(buf, vv->data, len);
                fmt++;

                continue;

            case 's':
                p = va_arg(args, u_char *);

                if (slen == (size_t) -1) {
                    while (*p && buf < last) {
                        *buf++ = *p++;
                    }

                } else {
                    len = ngx_min(((size_t) (last - buf)), slen);
                    buf = ngx_cpymem(buf, p, len);
                }

                fmt++;

                continue;

            case 'O':
                i64 = (int64_t) va_arg(args, off_t);
                sign = 1;
                break;

            case 'P':
                i64 = (int64_t) va_arg(args, ngx_pid_t);
                sign = 1;
                break;

            case 'T':
                i64 = (int64_t) va_arg(args, time_t);
                sign = 1;
                break;

            case 'M': //��ӡʱ�䣬������������Ϊngx_msec_t
                ms = (ngx_msec_t) va_arg(args, ngx_msec_t);
                if ((ngx_msec_int_t) ms == -1) {
                    sign = 1;
                    i64 = -1;
                } else {
                    sign = 0;
                    ui64 = (uint64_t) ms;
                }
                break;

            case 'z':
                if (sign) {
                    i64 = (int64_t) va_arg(args, ssize_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, size_t);
                }
                break;

            case 'i':
                if (sign) {
                    i64 = (int64_t) va_arg(args, ngx_int_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, ngx_uint_t);
                }

                if (max_width) {
                    width = NGX_INT_T_LEN;
                }

                break;

            case 'd':
                if (sign) {
                    i64 = (int64_t) va_arg(args, int);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_int);
                }
                break;

            case 'l':
                if (sign) {
                    i64 = (int64_t) va_arg(args, long);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_long);
                }
                break;

            case 'D':
                if (sign) {
                    i64 = (int64_t) va_arg(args, int32_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, uint32_t);
                }
                break;

            case 'L':
                if (sign) {
                    i64 = va_arg(args, int64_t);
                } else {
                    ui64 = va_arg(args, uint64_t);
                }
                break;

            case 'A':
                if (sign) {
                    i64 = (int64_t) va_arg(args, ngx_atomic_int_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, ngx_atomic_uint_t);
                }

                if (max_width) {
                    width = NGX_ATOMIC_T_LEN;
                }

                break;

            case 'f':
                f = va_arg(args, double);

                if (f < 0) {
                    *buf++ = '-';
                    f = -f;
                }

                ui64 = (int64_t) f;
                frac = 0;

                if (frac_width) {

                    scale = 1;
                    for (n = frac_width; n; n--) {
                        scale *= 10;
                    }

                    frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);

                    if (frac == scale) {
                        ui64++;
                        frac = 0;
                    }
                }

                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

                if (frac_width) {
                    if (buf < last) {
                        *buf++ = '.';
                    }

                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width);
                }

                fmt++;

                continue;

#if !(NGX_WIN32)
            case 'r':
                i64 = (int64_t) va_arg(args, rlim_t);
                sign = 1;
                break;
#endif

            case 'p':
                ui64 = (uintptr_t) va_arg(args, void *);
                hex = 2;
                sign = 0;
                zero = '0';
                width = NGX_PTR_SIZE * 2;
                break;

            case 'c':
                d = va_arg(args, int);
                *buf++ = (u_char) (d & 0xff);
                fmt++;

                continue;

            case 'Z':
                *buf++ = '\0';
                fmt++;

                continue;

            case 'N':
#if (NGX_WIN32)
                *buf++ = CR;
                if (buf < last) {
                    *buf++ = LF;
                }
#else
                *buf++ = LF;
#endif
                fmt++;

                continue;

            case '%':
                *buf++ = '%';
                fmt++;

                continue;

            default:
                *buf++ = *fmt++;

                continue;
            }

            if (sign) {
                if (i64 < 0) {
                    *buf++ = '-';
                    ui64 = (uint64_t) -i64;

                } else {
                    ui64 = (uint64_t) i64;
                }
            }

            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);

            fmt++;

        } else {
            *buf++ = *fmt++;
        }
    }

    return buf;
}


static u_char *
ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero,
    ngx_uint_t hexadecimal, ngx_uint_t width)
{
    u_char         *p, temp[NGX_INT64_LEN + 1];
                       /*
                        * we need temp[NGX_INT64_LEN] only,
                        * but icc issues the warning
                        */
    size_t          len;
    uint32_t        ui32;
    static u_char   hex[] = "0123456789abcdef";
    static u_char   HEX[] = "0123456789ABCDEF";

    p = temp + NGX_INT64_LEN;

    if (hexadecimal == 0) {

        if (ui64 <= (uint64_t) NGX_MAX_UINT32_VALUE) {

            /*
             * To divide 64-bit numbers and to find remainders
             * on the x86 platform gcc and icc call the libc functions
             * [u]divdi3() and [u]moddi3(), they call another function
             * in its turn.  On FreeBSD it is the qdivrem() function,
             * its source code is about 170 lines of the code.
             * The glibc counterpart is about 150 lines of the code.
             *
             * For 32-bit numbers and some divisors gcc and icc use
             * a inlined multiplication and shifts.  For example,
             * unsigned "i32 / 10" is compiled to
             *
             *     (i32 * 0xCCCCCCCD) >> 35
             */

            ui32 = (uint32_t) ui64;

            do {
                *--p = (u_char) (ui32 % 10 + '0');
            } while (ui32 /= 10);

        } else {
            do {
                *--p = (u_char) (ui64 % 10 + '0');
            } while (ui64 /= 10);
        }

    } else if (hexadecimal == 1) {

        do {

            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = hex[(uint32_t) (ui64 & 0xf)];

        } while (ui64 >>= 4);

    } else { /* hexadecimal == 2 */

        do {

            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = HEX[(uint32_t) (ui64 & 0xf)];

        } while (ui64 >>= 4);
    }

    /* zero or space padding */

    len = (temp + NGX_INT64_LEN) - p;

    while (len++ < width && buf < last) {
        *buf++ = zero;
    }

    /* number safe copy */

    len = (temp + NGX_INT64_LEN) - p;

    if (buf + len > last) {
        len = last - buf;
    }

    return ngx_cpymem(buf, p, len);
}


/*
 * We use ngx_strcasecmp()/ngx_strncasecmp() for 7-bit ASCII strings only,
 * and implement our own ngx_strcasecmp()/ngx_strncasecmp()
 * to avoid libc locale overhead.  Besides, we use the ngx_uint_t's
 * instead of the u_char's, because they are slightly faster.
 */

ngx_int_t
ngx_strcasecmp(u_char *s1, u_char *s2)
{
    ngx_uint_t  c1, c2;

    for ( ;; ) {
        c1 = (ngx_uint_t) *s1++;
        c2 = (ngx_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {

            if (c1) {
                continue;
            }

            return 0;
        }

        return c1 - c2;
    }
}


ngx_int_t
ngx_strncasecmp(u_char *s1, u_char *s2, size_t n)
{
    ngx_uint_t  c1, c2;

    while (n) {
        c1 = (ngx_uint_t) *s1++;
        c2 = (ngx_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {

            if (c1) {
                n--;
                continue;
            }

            return 0;
        }

        return c1 - c2;
    }

    return 0;
}


u_char *
ngx_strnstr(u_char *s1, char *s2, size_t len)
{
    u_char  c1, c2;
    size_t  n;

    c2 = *(u_char *) s2++;

    n = ngx_strlen(s2);

    do {
        do {
            if (len-- == 0) {
                return NULL;
            }

            c1 = *s1++;

            if (c1 == 0) {
                return NULL;
            }

        } while (c1 != c2);

        if (n > len) {
            return NULL;
        }

    } while (ngx_strncmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


/*
 * ngx_strstrn() and ngx_strcasestrn() are intended to search for static
 * substring with known length in null-terminated string. The argument n
 * must be length of the second substring - 1.
 */

u_char *
ngx_strstrn(u_char *s1, char *s2, size_t n)
{
    u_char  c1, c2;

    c2 = *(u_char *) s2++;

    do {
        do {
            c1 = *s1++;

            if (c1 == 0) {
                return NULL;
            }

        } while (c1 != c2);

    } while (ngx_strncmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


u_char *
ngx_strcasestrn(u_char *s1, char *s2, size_t n)
{
    ngx_uint_t  c1, c2;

    c2 = (ngx_uint_t) *s2++;
    c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

    do {
        do {
            c1 = (ngx_uint_t) *s1++;

            if (c1 == 0) {
                return NULL;
            }

            c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;

        } while (c1 != c2);

    } while (ngx_strncasecmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


/*
 * ngx_strlcasestrn() is intended to search for static substring
 * with known length in string until the argument last. The argument n
 * must be length of the second substring - 1.
 */

u_char *
ngx_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n)
{
    ngx_uint_t  c1, c2;

    c2 = (ngx_uint_t) *s2++;
    c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
    last -= n;

    do {
        do {
            if (s1 >= last) {
                return NULL;
            }

            c1 = (ngx_uint_t) *s1++;

            c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;

        } while (c1 != c2);

    } while (ngx_strncasecmp(s1, s2, n) != 0);

    return --s1;
}


ngx_int_t
ngx_rstrncmp(u_char *s1, u_char *s2, size_t n)
{
    if (n == 0) {
        return 0;
    }

    n--;

    for ( ;; ) {
        if (s1[n] != s2[n]) {
            return s1[n] - s2[n];
        }

        if (n == 0) {
            return 0;
        }

        n--;
    }
}


ngx_int_t
ngx_rstrncasecmp(u_char *s1, u_char *s2, size_t n)
{
    u_char  c1, c2;

    if (n == 0) {
        return 0;
    }

    n--;

    for ( ;; ) {
        c1 = s1[n];
        if (c1 >= 'a' && c1 <= 'z') {
            c1 -= 'a' - 'A';
        }

        c2 = s2[n];
        if (c2 >= 'a' && c2 <= 'z') {
            c2 -= 'a' - 'A';
        }

        if (c1 != c2) {
            return c1 - c2;
        }

        if (n == 0) {
            return 0;
        }

        n--;
    }
}


ngx_int_t
ngx_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2)
{
    size_t     n;
    ngx_int_t  m, z;

    if (n1 <= n2) {
        n = n1;
        z = -1;

    } else {
        n = n2;
        z = 1;
    }

    m = ngx_memcmp(s1, s2, n);

    if (m || n1 == n2) {
        return m;
    }

    return z;
}


ngx_int_t
ngx_dns_strcmp(u_char *s1, u_char *s2)
{
    ngx_uint_t  c1, c2;

    for ( ;; ) {
        c1 = (ngx_uint_t) *s1++;
        c2 = (ngx_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {

            if (c1) {
                continue;
            }

            return 0;
        }

        /* in ASCII '.' > '-', but we need '.' to be the lowest character */

        c1 = (c1 == '.') ? ' ' : c1;
        c2 = (c2 == '.') ? ' ' : c2;

        return c1 - c2;
    }
}


ngx_int_t
ngx_filename_cmp(u_char *s1, u_char *s2, size_t n)
{
    ngx_uint_t  c1, c2;

    while (n) {
        c1 = (ngx_uint_t) *s1++;
        c2 = (ngx_uint_t) *s2++;

#if (NGX_HAVE_CASELESS_FILESYSTEM)
        c1 = tolower(c1);
        c2 = tolower(c2);
#endif

        if (c1 == c2) {

            if (c1) {
                n--;
                continue;
            }

            return 0;
        }

        /* we need '/' to be the lowest character */

        if (c1 == 0 || c2 == 0) {
            return c1 - c2;
        }

        c1 = (c1 == '/') ? 0 : c1;
        c2 = (c2 == '/') ? 0 : c2;

        return c1 - c2;
    }

    return 0;
}


ngx_int_t
ngx_atoi(u_char *line, size_t n)
{
    ngx_int_t  value, cutoff, cutlim;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_INT_T_VALUE / 10;
    cutlim = NGX_MAX_INT_T_VALUE % 10;

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NGX_ERROR;
        }

        if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
            return NGX_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    return value;
}


/* parse a fixed point number, e.g., ngx_atofp("10.5", 4, 2) returns 1050 */

ngx_int_t
ngx_atofp(u_char *line, size_t n, size_t point)
{
    ngx_int_t   value, cutoff, cutlim;
    ngx_uint_t  dot;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_INT_T_VALUE / 10;
    cutlim = NGX_MAX_INT_T_VALUE % 10;

    dot = 0;

    for (value = 0; n--; line++) {

        if (point == 0) {
            return NGX_ERROR;
        }

        if (*line == '.') {
            if (dot) {
                return NGX_ERROR;
            }

            dot = 1;
            continue;
        }

        if (*line < '0' || *line > '9') {
            return NGX_ERROR;
        }

        if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
            return NGX_ERROR;
        }

        value = value * 10 + (*line - '0');
        point -= dot;
    }

    while (point--) {
        if (value > cutoff) {
            return NGX_ERROR;
        }

        value = value * 10;
    }

    return value;
}


ssize_t
ngx_atosz(u_char *line, size_t n)
{
    ssize_t  value, cutoff, cutlim;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_SIZE_T_VALUE / 10;
    cutlim = NGX_MAX_SIZE_T_VALUE % 10;

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NGX_ERROR;
        }

        if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
            return NGX_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    return value;
}


off_t
ngx_atoof(u_char *line, size_t n)
{
    off_t  value, cutoff, cutlim;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_OFF_T_VALUE / 10;
    cutlim = NGX_MAX_OFF_T_VALUE % 10;

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NGX_ERROR;
        }

        if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
            return NGX_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    return value;
}


time_t
ngx_atotm(u_char *line, size_t n)
{
    time_t  value, cutoff, cutlim;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_TIME_T_VALUE / 10;
    cutlim = NGX_MAX_TIME_T_VALUE % 10;

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NGX_ERROR;
        }

        if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
            return NGX_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    return value;
}


ngx_int_t
ngx_hextoi(u_char *line, size_t n)
{
    u_char     c, ch;
    ngx_int_t  value, cutoff;

    if (n == 0) {
        return NGX_ERROR;
    }

    cutoff = NGX_MAX_INT_T_VALUE / 16;

    for (value = 0; n--; line++) {
        if (value > cutoff) {
            return NGX_ERROR;
        }

        ch = *line;

        if (ch >= '0' && ch <= '9') {
            value = value * 16 + (ch - '0');
            continue;
        }

        c = (u_char) (ch | 0x20);

        if (c >= 'a' && c <= 'f') {
            value = value * 16 + (c - 'a' + 10);
            continue;
        }

        return NGX_ERROR;
    }

    return value;
}

//�ַ���ת��Ϊ16���Ƶ�ַ������4���ֽ��ַ���"5566"����ת��Ϊ16���Ƶ�ַ0X5566�����ֽ�
u_char *
ngx_hex_dump(u_char *dst, u_char *src, size_t len)
{
    static u_char  hex[] = "0123456789abcdef";

    while (len--) {
        *dst++ = hex[*src >> 4];
        *dst++ = hex[*src++ & 0xf];
    }

    return dst;
}

/*
�������������ڶ�str����base64��������룬����ǰ����Ҫ��֤dst�����㹻�Ŀռ�����Ž���������֪�������С�����ȵ���
ngx_base64_encoded_length��ngx_base64_decoded_length��Ԥ�����ռ�ÿռ䡣
*/ //ngx_encode_base64  ngx_decode_base64��Ӧ���ܽ���
void
ngx_encode_base64(ngx_str_t *dst, ngx_str_t *src)
{
    static u_char   basis64[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    ngx_encode_base64_internal(dst, src, basis64, 1);
}

//ngx_decode_base64url  ngx_encode_base64url��Ӧ���ܽ���
//ngx_encode_base64url  ngx_decode_base64��Ӧ���ܽ���
void
ngx_encode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
    static u_char   basis64[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    ngx_encode_base64_internal(dst, src, basis64, 0);
}


static void
ngx_encode_base64_internal(ngx_str_t *dst, ngx_str_t *src, const u_char *basis,
    ngx_uint_t padding)
{
    u_char         *d, *s;
    size_t          len;

    len = src->len;
    s = src->data;
    d = dst->data;

    while (len > 2) {
        *d++ = basis[(s[0] >> 2) & 0x3f];
        *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len) {
        *d++ = basis[(s[0] >> 2) & 0x3f];

        if (len == 1) {
            *d++ = basis[(s[0] & 3) << 4];
            if (padding) {
                *d++ = '=';
            }

        } else {
            *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis[(s[1] & 0x0f) << 2];
        }

        if (padding) {
            *d++ = '=';
        }
    }

    dst->len = d - dst->data;
}

//��src������hashȻ��浽dst�У�����Ϊʲô���Ա�֤dst����Խ�磬һ�㶼���ڴ������ֽڱ�֤src�ĳ�����ʵ�ֵģ����Բο�ngx_http_secure_link_variable
//ngx_encode_base64  ngx_decode_base64��Ӧ���ܽ���
ngx_int_t
ngx_decode_base64(ngx_str_t *dst, ngx_str_t *src)
{
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return ngx_decode_base64_internal(dst, src, basis64);
}

//��src������hashȻ��浽dst�У�����Ϊʲô���Ա�֤dst����Խ�磬һ�㶼���ڴ������ֽڱ�֤src�ĳ�����ʵ�ֵģ����Բο�ngx_http_secure_link_variable
//ngx_decode_base64url  ngx_encode_base64url��Ӧ���ܽ���
ngx_int_t
ngx_decode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 63,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return ngx_decode_base64_internal(dst, src, basis64);
}

//��src������hashȻ��浽dst�У�����Ϊʲô���Ա�֤dst����Խ�磬һ�㶼���ڴ������ֽڱ�֤src�ĳ�����ʵ�ֵģ����Բο�ngx_http_secure_link_variable
static ngx_int_t
ngx_decode_base64_internal(ngx_str_t *dst, ngx_str_t *src, const u_char *basis)
{
    size_t          len;
    u_char         *d, *s;

    for (len = 0; len < src->len; len++) {
        if (src->data[len] == '=') {
            break;
        }

        if (basis[src->data[len]] == 77) {
            return NGX_ERROR;
        }
    }

    if (len % 4 == 1) {
        return NGX_ERROR;
    }

    s = src->data;
    d = dst->data;

    while (len > 3) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (u_char) (basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) { //s[0]��s[1]������Ҳ���ã��������d��ʵ�ʵ�s/2Ҫ��1�ֽڣ��ڼ��Ϻ����1�ֽڣ��ܹ���2�ֽ�
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {//s[0]��s[1]������Ҳ���ã��������d��ʵ�ʵ�s/2Ҫ��1�ֽڣ��ڼ��Ϻ����1�ֽڣ��ܹ���2�ֽ�
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    dst->len = d - dst->data;

    return NGX_OK;
}


/*
 * ngx_utf8_decode() decodes two and more bytes UTF sequences only
 * the return values:
 *    0x80 - 0x10ffff         valid character
 *    0x110000 - 0xfffffffd   invalid sequence
 *    0xfffffffe              incomplete sequence
 *    0xffffffff              error
 */

uint32_t
ngx_utf8_decode(u_char **p, size_t n)
{
    size_t    len;
    uint32_t  u, i, valid;

    u = **p;

    if (u >= 0xf0) {

        u &= 0x07;
        valid = 0xffff;
        len = 3;

    } else if (u >= 0xe0) {

        u &= 0x0f;
        valid = 0x7ff;
        len = 2;

    } else if (u >= 0xc2) {

        u &= 0x1f;
        valid = 0x7f;
        len = 1;

    } else {
        (*p)++;
        return 0xffffffff;
    }

    if (n - 1 < len) {
        return 0xfffffffe;
    }

    (*p)++;

    while (len) {
        i = *(*p)++;

        if (i < 0x80) {
            return 0xffffffff;
        }

        u = (u << 6) | (i & 0x3f);

        len--;
    }

    if (u > valid) {
        return u;
    }

    return 0xffffffff;
}


size_t
ngx_utf8_length(u_char *p, size_t n)
{
    u_char  c, *last;
    size_t  len;

    last = p + n;

    for (len = 0; p < last; len++) {

        c = *p;

        if (c < 0x80) {
            p++;
            continue;
        }

        if (ngx_utf8_decode(&p, n) > 0x10ffff) {
            /* invalid UTF-8 */
            return n;
        }
    }

    return len;
}


u_char *
ngx_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len)
{
    u_char  c, *next;

    if (n == 0) {
        return dst;
    }

    while (--n) {

        c = *src;
        *dst = c;

        if (c < 0x80) {

            if (c != '\0') {
                dst++;
                src++;
                len--;

                continue;
            }

            return dst;
        }

        next = src;

        if (ngx_utf8_decode(&next, len) > 0x10ffff) {
            /* invalid UTF-8 */
            break;
        }

        while (src < next) {
            *dst++ = *src++;
            len--;
        }
    }

    *dst = '\0';

    return dst;
}


uintptr_t
ngx_escape_uri(u_char *dst, u_char *src, size_t size, ngx_uint_t type)
{//���dstΪ�գ��򷵻���Ҫת����ַ��ж��ٸ��������ַ���ת���ˣ������dst���档
    ngx_uint_t      n;
    uint32_t       *escape;
    static u_char   hex[] = "0123456789ABCDEF";

                    /* " ", "#", "%", "?", %00-%1F, %7F-%FF */

    static uint32_t   uri[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x80000029, /* 1000 0000 0000 0000  0000 0000 0010 1001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "#", "%", "&", "+", "?", %00-%1F, %7F-%FF */

    static uint32_t   args[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x88000869, /* 1000 1000 0000 0000  0000 1000 0110 1001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* not ALPHA, DIGIT, "-", ".", "_", "~" */

    static uint32_t   uri_component[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc009fff, /* 1111 1100 0000 0000  1001 1111 1111 1111 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "#", """, "%", "'", %00-%1F, %7F-%FF */

    static uint32_t   html[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x000000ad, /* 0000 0000 0000 0000  0000 0000 1010 1101 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", """, "%", "'", %00-%1F, %7F-%FF */

    static uint32_t   refresh[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000085, /* 0000 0000 0000 0000  0000 0000 1000 0101 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "%", %00-%1F */

    static uint32_t   memcached[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000021, /* 0000 0000 0000 0000  0000 0000 0010 0001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

                    /* mail_auth is the same as memcached */

    static uint32_t  *map[] =
        { uri, args, uri_component, html, refresh, memcached, memcached };


    escape = map[type];

    if (dst == NULL) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }
            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while (size) {
        if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
            *dst++ = '%';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;

        } else {
            *dst++ = *src++;
        }
        size--;
    }

    return (uintptr_t) dst;
}

/*
��ַ���е��ʺ���ʲô����
�������������ӣ�
http://www.xxx.com/Show.asp?id=77&nameid=2905210001&page=1
�������������У��ʺŵĺ��岻���������������ᵽ�İ汾�����⣬���Ǵ��ݲ��������á�����ʺŽ�show.asp�ļ��ͺ����id��nameid��page������������


��src���з����룬type������0��NGX_UNESCAPE_URI��NGX_UNESCAPE_REDIRECT������ֵ�������0�����ʾsrc�е������ַ���Ҫ����ת�롣���
��NGX_UNESCAPE_URI��NGX_UNESCAPE_REDIRECT����������?����ͽ����ˣ�������ַ��Ͳ����ˡ���NGX_UNESCAPE_URI��NGX_UNESCAPE_REDIRECT֮��
��������NGX_UNESCAPE_URI������������Ҫת����ַ�������ת�룬��NGX_UNESCAPE_REDIRECT��ֻ��Էǿɼ��ַ�����ת�롣
*/
void
ngx_unescape_uri(u_char **dst, u_char **src, size_t size, ngx_uint_t type)
{
    u_char  *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    d = *dst;
    s = *src;

    state = 0;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
        case sw_usual:
            if (ch == '?'
                && (type & (NGX_UNESCAPE_URI|NGX_UNESCAPE_REDIRECT)))
            {
                *d++ = ch;
                goto done;
            }

            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            *d++ = ch;
            break;

        case sw_quoted:

            if (ch >= '0' && ch <= '9') {
                decoded = (u_char) (ch - '0');
                state = sw_quoted_second;
                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (u_char) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }

            /* the invalid quoted character */

            state = sw_usual;

            *d++ = ch;

            break;

        case sw_quoted_second:

            state = sw_usual;

            if (ch >= '0' && ch <= '9') {
                ch = (u_char) ((decoded << 4) + ch - '0');

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                    break;
                }

                *d++ = ch;

                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (u_char) ((decoded << 4) + c - 'a' + 10);

                if (type & NGX_UNESCAPE_URI) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    *d++ = ch;
                    break;
                }

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                    break;
                }

                *d++ = ch;

                break;
            }

            /* the invalid quoted character */

            break;
        }
    }

done:

    *dst = d;
    *src = s;
}


uintptr_t
ngx_escape_html(u_char *dst, u_char *src, size_t size)
{
    u_char      ch;
    ngx_uint_t  len;

    if (dst == NULL) {

        len = 0;

        while (size) {
            switch (*src++) {

            case '<':
                len += sizeof("&lt;") - 2;
                break;

            case '>':
                len += sizeof("&gt;") - 2;
                break;

            case '&':
                len += sizeof("&amp;") - 2;
                break;

            case '"':
                len += sizeof("&quot;") - 2;
                break;

            default:
                break;
            }
            size--;
        }

        return (uintptr_t) len;
    }

    while (size) {
        ch = *src++;

        switch (ch) {

        case '<':
            *dst++ = '&'; *dst++ = 'l'; *dst++ = 't'; *dst++ = ';';
            break;

        case '>':
            *dst++ = '&'; *dst++ = 'g'; *dst++ = 't'; *dst++ = ';';
            break;

        case '&':
            *dst++ = '&'; *dst++ = 'a'; *dst++ = 'm'; *dst++ = 'p';
            *dst++ = ';';
            break;

        case '"':
            *dst++ = '&'; *dst++ = 'q'; *dst++ = 'u'; *dst++ = 'o';
            *dst++ = 't'; *dst++ = ';';
            break;

        default:
            *dst++ = ch;
            break;
        }
        size--;
    }

    return (uintptr_t) dst;
}


uintptr_t
ngx_escape_json(u_char *dst, u_char *src, size_t size)
{
    u_char      ch;
    ngx_uint_t  len;

    if (dst == NULL) {
        len = 0;

        while (size) {
            ch = *src++;

            if (ch == '\\' || ch == '"') {
                len++;

            } else if (ch <= 0x1f) {
                len += sizeof("\\u001F") - 2;
            }

            size--;
        }

        return (uintptr_t) len;
    }

    while (size) {
        ch = *src++;

        if (ch > 0x1f) {

            if (ch == '\\' || ch == '"') {
                *dst++ = '\\';
            }

            *dst++ = ch;

        } else {
            *dst++ = '\\'; *dst++ = 'u'; *dst++ = '0'; *dst++ = '0';
            *dst++ = '0' + (ch >> 4);

            ch &= 0xf;

            *dst++ = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
        }

        size--;
    }

    return (uintptr_t) dst;
}

/*
��7-4 NginxΪ������Ѿ�ʵ�ֺõ�3��������ӷ��� (ngx_rbtree_insert_ptָ���������ַ���)
���������������������������������������ש��������������������������������������ש�������������������������������
��    ������                          ��    ��������                          ��    ִ������                  ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_value        ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     �����ݽڵ�Ĺؼ��ֶ���Ψһ�ģ�  ��
��ngx_rbtree_node_t *node,            ����ָ�룻sentinel����ú������ʼ��    ��������ͬһ���ؼ����ж���ڵ�  ��
��ngx_rbtree_node_t *sentinel)        ��ʱ�ڱ��ڵ��ָ��                      ��������                        ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_timer_value  ��  root�Ǻ����������ָ�룻node��      ��                              ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     ��  ������������ݽڵ㣬ÿ��  ��
��ngx_rbtree_node_t *node,            ����ָ�룬����Ӧ�Ĺؼ�����ʱ�����      �����ݽڵ�Ĺؼ��ֱ�ʾʱ������  ��
��                                    ��ʱ�������Ǹ�����sentinel�����    ��ʱ���                        ��
��ngx_rbtree_node_t *sentinel)        ��                                      ��                              ��
��                                    ���������ʼ��ʱ���ڱ��ڵ�              ��                              ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_str_rbtree_insert_value    ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *temp,           �������Ԫ�ص�ngx_str_node_t��Ա��      �����ݽڵ�Ĺؼ��ֿ��Բ���Ψһ  ��
��ngx_rbtree_node_t *node,            ��ָ�루ngx- rbtree_node_t���ͻ�ǿ��ת  ���ģ������������ַ�����ΪΨһ  ��
��                                    ����Ϊngx_str_node_t���ͣ���sentinel��  ���ı�ʶ�������ngx_str_node_t  ��
��ngx_rbtree_node t *sentinel)        ��                                      ��                              ��
��                                    ����ú������ʼ��ʱ�ڱ��ڵ��ָ��      ���ṹ���str��Ա��             ��
���������������������������������������ߩ��������������������������������������ߩ�������������������������������
    ͬʱ������ngx_str_node_t�ڵ㣬Nginx���ṩ��ngx_str_rbtree_lookup�������ڼ���
������ڵ㣬��������һ�����Ķ��壬�������¡�
    ngx_str_node_t  *ngx_str_rbtree_lookup(ngx_rbtree t  *rbtree,  ngx_str_t *name, uint32_t hash)��
    ���У�hash������Ҫ��ѯ�ڵ��key�ؼ��֣���name��Ҫ��ѯ���ַ����������ͬ��
������Ӧ��ͬkey�ؼ��ֵ����⣩�����ص��ǲ�ѯ���ĺ�����ڵ�ṹ�塣
    ���ں���������ķ�������7-5��
��7-5  ����������ṩ�ķ���
���������������������������������������ש��������������������������������ש�������������������������������������
��    ������                          ��    ��������                    ��    ִ������                        ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��                                    ��  tree�Ǻ����������ָ�룻s��   ��  ��ʼ���������������ʼ������      ��
��                                    ���ڱ��ڵ��ָ�룻i��ngx_rbtree_  ��                                    ��
��ngx_rbtree_init(tree, s, i)         ��                                ���㡢�ڱ��ڵ㡢ngx_rbtree_insert_pt  ��
��                                    ��insert_pt���͵Ľڵ���ӷ������� ���ڵ���ӷ���                        ��
��                                    �������7-4                       ��                                    ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��void ngx_rbtree_insert(ngx_rbtree_t ��  tree�Ǻ����������ָ�룻node  ��  ����������ӽڵ㣬�÷�����      ��
��*tree, ngx_rbtree node_t *node)     ������Ҫ��ӵ�������Ľڵ�ָ��    ��ͨ����ת�������������ƽ��          ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��void ngx_rbtree_delete(ngx_rbtree_t ��  tree�Ǻ����������ָ�룻node  ��  �Ӻ������ɾ���ڵ㣬�÷�����      ��
��*tree, ngx_rbtree node_t *node)     ���Ǻ��������Ҫɾ���Ľڵ�ָ��    ��ͨ����ת�������������ƽ��          ��
���������������������������������������ߩ��������������������������������ߩ�������������������������������������
    �ڳ�ʼ�������ʱ����Ҫ�ȷ���ñ���������ngx_rbtree_t�ṹ�壬�Լ�ngx_rbtree_
node_t���͵��ڱ��ڵ㣬��ѡ������Զ���ngx_rbtree_insert_pt���͵Ľڵ���Ӻ�����
    ���ں������ÿ���ڵ���˵�����Ƕ��߱���7-6���е�7�����������ֻ�����˽����
ʹ�ú��������ôֻ��Ҫ�˽�ngx_rbtree_min������


��7��Nginx�ṩ�ĸ߼����ݽṹר233
��7-6������ڵ��ṩ�ķ���
���������������������������������������ש����������������������������������ש���������������������������������������
��    ������                          ��    ��������                      ��    ִ������                          ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbt_red(node)                   ��                                  ��  ����node�ڵ����ɫΪ��ɫ            ��
��                                    �� t���͵Ľڵ�ָ��                  ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbt_black(node)                 ��                                  ��  ����node�ڵ����ɫΪ��ɫ            ��
��                                    ��t���͵Ľڵ�ָ��                   ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��  ��node�ڵ����ɫΪ��ɫ���򷵻ط�O   ��
��ngx_rbt_is_red(node)                ��                                  ��                                      ��
��                                    ��t���͵Ľڵ�ָ��                   ����ֵ�����򷵻�O                       ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��ngx_rbt_is_black(node)              ��  node�Ǻ������ngx_rbtree_node_  ��  ��node�ڵ����ɫΪ��ɫ���򷵻ط�0   ��
��                        I           ��t���͵Ľڵ�ָ��                   ����ֵ�����򷵻�O                       ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��               I                    ��  nl��n2���Ǻ������ngx_rbtree_   ��                                      ��
��ngx_rbt_copy_color(nl, n2)          ��                                  ��  ��n2�ڵ����ɫ���Ƶ�nl�ڵ�          ��
��                                 I  ��nodej���͵Ľڵ�ָ��               ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��ngx_rbtree_node_t *                 ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbtree_min                      ��t���͵Ľڵ�ָ�룻sentinel����ú� ��  �ҵ���ǰ�ڵ㼰�������е���С�ڵ�    ��
��(ngx_rbtree_node_tľnode,           ���������ڱ��ڵ�                    ��������key�ؼ��֣�                     ��
��ngx_rbtree_node_t *sentinel)        ��                                  ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��  ��ʼ���ڱ��ڵ㣬ʵ���Ͼ��ǽ��ýڵ�  ��
��ngx_rbtree_sentinel_init(node)      ��                                  ��                                      ��
��                                    ��t���͵Ľڵ�ָ��                   ����ɫ��Ϊ��ɫ                          ��
���������������������������������������ߩ����������������������������������ߩ���������������������������������������
  
ʹ�ú�����ļ�����
    ������һ���򵥵�������˵�����ʹ�ú����������������ջ�з���rbtree���������
�ṹ���Լ��ڱ��ڵ�sentinel����Ȼ��Ҳ����ʹ���ڴ�ػ��ߴӽ��̶��з��䣩�������еĽ�
����ȫ��key�ؼ�����Ϊÿ���ڵ��Ψһ��ʶ�������Ϳ��Բ���Ԥ���ngx_rbtree insert
value�����ˡ����ɵ���ngx_rbtree_init������ʼ�������������������ʾ��
    ngx_rbtree_node_t  sentinel ;
    ngx_rbtree_init ( &rbtree, &sentinel,ngx_str_rbtree_insert_value)
    ���������ڵ�Ľṹ�彫ʹ��7.5.3���н��ܵ�TestRBTreeNode�ṹ�壬ÿ��Ԫ�ص�key�ؼ��ְ���1��6��8��11��13��15��17��22��25��27��˳
��һһ����������ӣ�����������ʾ��
    rbTreeNode [0] .num=17;
    rbTreeNode [1] .num=22;
    rbTreeNode [2] .num=25;
    rbTreeNode [3] .num=27;
    rbTreeNode [4] .num=17;
    rbTreeNode [7] .num=22;
    rbTreeNode [8] .num=25;
    rbTreeNode [9] .num=27;
    for(i=0j i<10; i++)
    {
        rbTreeNode [i].node. key=rbTreeNode[i]. num;
        ngx_rbtree_insert(&rbtree,&rbTreeNode[i].node);
    )

ngx_rbtree_node_t *tmpnode   =   ngx_rbtree_min ( rbtree . root ,    &sentinel )  ;
    ��Ȼ�������������ʹ�ø��ڵ����ʹ����һ���ڵ�Ҳ�ǿ��Եġ���������һ�����
����1���ڵ㣬��ȻNginx�Դ˲�û���ṩԤ��ķ����������ַ��������ṩ��ngx_str_
rbtree_lookup��������������ʵ���ϼ����Ƿǳ��򵥵ġ�������Ѱ��key�ؼ���Ϊ13�Ľڵ�
Ϊ��������˵����
    ngx_uint_t lookupkey=13;
    tmpnode=rbtree.root;
    TestRBTreeNode *lookupNode;
    while (tmpnode  !=&sentinel)  {
        if (lookupkey!-tmpnode->key)  (
        ��������key�ؼ����뵱ǰ�ڵ�Ĵ�С�Ƚϣ������Ǽ�������������������
        tmpnode=  (lookupkey<tmpnode->key)  ?tmpnode->left:tmpnode->right;
        continue��
        )
        �����ҵ���ֵΪ13�����ڵ�
        lookupNode=  (TestRBTreeNode*)  tmpnode;
        break;
    )
    �Ӻ������ɾ��1���ڵ�Ҳ�Ƿǳ��򵥵ģ���Ѹո��ҵ���ֵΪ13�Ľڵ��rbtree��
ɾ����ֻ�����ngx_rbtree_delete������
ngx_rbtree_delete ( &rbtree , &lookupNode->node);

    ���ڽڵ��key�ؼ��ֱ��������ͣ��⵼�ºܶ�����²�ͬ�Ľڵ�������ͬ��key��
���֡������ϣ�����־�����ͬkey�ؼ��ֵĲ�ͬ�ڵ������������ʱ���ָ���ԭ�ڵ��
���������Ҫʵ�����е�ngx_rbtree_insert_ptܵ����
*/
/*
ngx_str_rbtree_insert_value������Ӧ�ó���Ϊ���ڵ�ı�ʶ�����ַ�����������ĵ�һ����������Ȼ�ǽڵ��key�ؼ��֣��ڶ������������ǽڵ���ַ���
*/

/*
���������������������������������������ש��������������������������������������ש�������������������������������
��    ������                          ��    ��������                          ��    ִ������                  ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_value        ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     �����ݽڵ�Ĺؼ��ֶ���Ψһ�ģ�  ��
��ngx_rbtree_node_t *node,            ����ָ�룻sentinel����ú������ʼ��    ��������ͬһ���ؼ����ж���ڵ�  ��
��ngx_rbtree_node_t *sentinel)        ��ʱ�ڱ��ڵ��ָ��                      ��������                        ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
*/
void
ngx_str_rbtree_insert_value(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_str_node_t      *n, *t;
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        n = (ngx_str_node_t *) node;
        t = (ngx_str_node_t *) temp;

        //���ȱȽ�key�ؼ��֣����������key��Ϊ��һ�����ؼ���
        if (node->key != temp->key) {
            //�������ڵ�Ĺؼ���С��������
            p = (node->key < temp->key) ? &temp->left : &temp->right;

        } else if (n->str.len != t->str.len) {//��key�ؼ�����ͬʱ�����ַ�������Ϊ�ڶ������ؼ���
            //�������ڵ��ַ����ĳ���С��������
            p = (n->str.len < t->str.len) ? &temp->left : &temp->right;

        } else {//key�ؼ�����ͬ���ַ���������ͬʱ���ټ����Ƚ��ַ�������
            p = (ngx_memcmp(n->str.data, t->str.data, n->str.len) < 0)
                 ? &temp->left : &temp->right;
        }

        if (*p == sentinel) {//�����ǰ�ڵ�p���ڱ��ڵ㣬��ô����ѭ��׼������ڵ�
            break;
        }

        temp = *p;
    }

    *p = node;///p�ڵ���Ҫ����Ľڵ������ͬ�ı�ʶ��ʱ�����븲������
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}

ngx_str_node_t *
ngx_str_rbtree_lookup(ngx_rbtree_t *rbtree, ngx_str_t *val, uint32_t hash)
{
    ngx_int_t           rc;
    ngx_str_node_t     *n;
    ngx_rbtree_node_t  *node, *sentinel;

    node = rbtree->root;
    sentinel = rbtree->sentinel;

    while (node != sentinel) {

        n = (ngx_str_node_t *) node;

        if (hash != node->key) {
            node = (hash < node->key) ? node->left : node->right;
            continue;
        }

        if (val->len != n->str.len) {
            node = (val->len < n->str.len) ? node->left : node->right;
            continue;
        }

        rc = ngx_memcmp(val->data, n->str.data, val->len);

        if (rc < 0) {
            node = node->left;
            continue;
        }

        if (rc > 0) {
            node = node->right;
            continue;
        }

        return n;
    }

    return NULL;
}


/* ngx_sort() is implemented as insertion sort because we need stable sort */

//�ַ�����������
void
ngx_sort(void *base, size_t n, size_t size,
    ngx_int_t (*cmp)(const void *, const void *))
{
    u_char  *p1, *p2, *p;

    p = ngx_alloc(size, ngx_cycle->log);
    if (p == NULL) {
        return;
    }

    for (p1 = (u_char *) base + size;
         p1 < (u_char *) base + n * size;
         p1 += size)
    {
        ngx_memcpy(p, p1, size);

        for (p2 = p1;
             p2 > (u_char *) base && cmp(p2 - size, p) > 0;
             p2 -= size)
        {
            ngx_memcpy(p2, p2 - size, size);
        }

        ngx_memcpy(p2, p, size);
    }

    ngx_free(p);
}


#if (NGX_MEMCPY_LIMIT)

void *
ngx_memcpy(void *dst, const void *src, size_t n)
{
    if (n > NGX_MEMCPY_LIMIT) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "memcpy %uz bytes", n);
        ngx_debug_point();
    }

    return memcpy(dst, src, n);
}

#endif
