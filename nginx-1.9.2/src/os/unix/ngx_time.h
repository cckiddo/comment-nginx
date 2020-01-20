
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_TIME_H_INCLUDED_
#define _NGX_TIME_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef ngx_rbtree_key_t      ngx_msec_t;
typedef ngx_rbtree_key_int_t  ngx_msec_int_t;

/*
��9-4 Nginx����ʱ��Ĳ�������
�����������������������������������ש����������������������������������ש�����������������������������������������������
��    ʱ�䷽����                  ��    ��������                      ��    ִ������                                  ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��void ngx_time_init(void);       ��    ��                            ��    ��ʼ����ǰ�����л����ʱ�������ͬ        ��
��                                ��                                  ��ʱ���һ�θ���gettimeofday����ˢ�»�          ��
��                                ��                                  ����ʱ��                                        ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��void ngx_time_update(void)      ��    ��                            ��    ʹ��gettimeofday������ϵͳʱ��            ��
��                                ��                                  �����»����ʱ�䣬������ngx_current_            ��
��                                ��                                  ��msec. ngx_cached time. ngx_cached err         ��
��                                ��                                  �� log_time. ngx_cached_http_time. ngx_         ��
��                                ��                                  ��cached_http_log_time. ngx_cached_http         ��
��                                ��                                  �� log_is08601��6��ȫ�ֱ�������õ�����         ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��                                ��    t����Ҫת����ʱ�䣬���Ǹ�     ��                                              ��
��u_char *ngx_http_time           ��������ʱ��1970��1��1���賿        ��    ��ʱ��tת���ɡ�Mon, 28 Sep 1970 06:00:00  ��
��                                ��0��O��O�뵽ĳһʱ���������       �� GMT����ʽ��ʱ�䣬����ֵ��buf����ͬ           ��
��(u_char *buf, time_t t)         ��                                  ���ģ�����ָ����ʱ����ַ���                  ��
��                                ��buf��tʱ��ת�����ַ�����ʽ��      ��                                              ��
��                                ��r-rrIPʱ�����������ַ������ڴ�  ��                                              ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��                                ��    t����Ҫת����ʱ�䣬���Ǹ�     ��                                              ��
��                                ��������ʱ��1970��1��1���賿        ��    ��ʱ��tת���ɡ�Mon. 28-Sep-70 06:00:00    ��
��u_char *ngx_http_cookie_time    ��0��0��0�뵽ĳһʱ���������       �� GMT����ʽ������cookie��ʱ�䣬����ֵ          ��
��(u_char *buf, time_t t)         ��buf��tʱ��ת�����ַ�����ʽ��      ����buf����ͬ�ģ�����ָ����ʱ�����           ��
��                                ������cookie��ʱ������������      ������                                          ��
��                                ���������ڴ�                        ��                                              ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��                                ��    t����Ҫת����ʱ�䣬���Ǹ���   ��                                              ��
��void ngx_gmtime                 ������ʱ��1970��1��1���賿O         ��    ��ʱ��tת����ngx_tm_t���͵�ʱ�䡣         ��
��                                ����0��0�뵽ĳһʱ���������tp      ��                                              ��
��(time_t t, ngx_tm_t *tp)        ��                                  �������˵��ngx_tm_t����                        ��
��                                ����ngx_tm_t���͵�ʱ�䣬ʵ����      ��                                              ��
��                                �����Ǳ�׼��tm����ʱ��              ��                                              ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��                                ��                                  ��    ����һ1��ʾʧ�ܣ�����᷵�أ�����         ��
��                                ��                                  ����when��ʾ����ʱ�������������ϲ���            ��
��                                ��                                  ��ʵ��ʱ����Ѿ�������ǰʱ�䣬��ô��          ��
��                                ��                                  ������when�ϲ���ʵ��ʱ������������            ��
��time_t ngx_next_time            ��    when���ڴ����ڵ�ʱ�䣬��    �����ڸ�������ʱ��1970��1��1���賿O             ��
��(time_t when)    :              ������ʾһ���ڵ�����                ����O��O�뵽ĳһʱ�����������                  ��
��                                ��                                  ��  �ڷ�֮������ϲ����ʱ�����ڵ�ǰ            ��
��                                ��                                  ��ʱ�䣬�򷵻���һ���ͬһʱ�̣�����ʱ          ��
��                                ��                                  ���̣���ʱ�䡣��Ŀǰ��������expires����         ��
��                                ��                                  ������صĻ�����ڹ���                          ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��#define ngx_time                ��    ��                            ��    ��ȡ����������ʱ��1970��1��1��            ��
��ngx_cached_time->sec            ��                                  ���賿0��0��0�뵽��ǰʱ�������                 ��
�ǩ��������������������������������贈���������������������������������贈����������������������������������������������
��#define ngx_timeofday           ��    ��                            ��    ��ȡ�����ngx_time_t����ʱ��              ��
��(ngx_time_t *) ngxLcached_time  ��                                  ��                                              ��
�����������������������������������ߩ����������������������������������ߩ�����������������������������������������������
*/
/*
struct   tm{
    ��һȡֵ����Ϊ[0��59]
    int tm_sec;
    ��һȡֵ����Ϊ[0��59]
    int tm__ min;
    ʱһȡֵ����Ϊ[o��23]
    int tm hour;
    һ�����е�����һȡֵ����Ϊ[1��31]
    iAt tm_mday;��
    �·ݣ���һ�¿�ʼ��0����һ�£�һȡֵ����Ϊ[0��II]
    int tm- mon,
    ��ݣ���ֵ����ʵ����ݼ�ȥ1900
    int tm_year,
    ����һȡֵ����Ϊ[0��6]������0���������죬1��������һ����������
    int tm_wday;
    ��ÿ���1��1�տ�ʼ������һȡֵ����Ϊ[0��365��������0����1��1�գ�1����1��2����������
    int tm_yday;
    ����ʱ��ʶ������ʵ������ʱ��ʱ��tm_isdstΪ������ʵ������ʱ��ʱ��tm_isdstΪo�ڲ��˽����ʱ��tm_ isdstΪ��
    int tm isdst,
    )��
    ngx_tmj��tm�÷�����ȫһ�µģ�������ʾ��
typedef struct tm    ngx_tm_t;
#define ngx_tm_sec
#define ngx_tm_min
#define ngx_tm_hour
#define ngx_tm_mday
#define ngx_tm_mon
#define ngx_tm_year
#define ngx_tm_wday
#define ngx_tm_isdst
*/
typedef struct tm             ngx_tm_t;

#define ngx_tm_sec            tm_sec
#define ngx_tm_min            tm_min
#define ngx_tm_hour           tm_hour
#define ngx_tm_mday           tm_mday
#define ngx_tm_mon            tm_mon
#define ngx_tm_year           tm_year
#define ngx_tm_wday           tm_wday
#define ngx_tm_isdst          tm_isdst

#define ngx_tm_sec_t          int
#define ngx_tm_min_t          int
#define ngx_tm_hour_t         int
#define ngx_tm_mday_t         int
#define ngx_tm_mon_t          int
#define ngx_tm_year_t         int
#define ngx_tm_wday_t         int


#if (NGX_HAVE_GMTOFF)
#define ngx_tm_gmtoff         tm_gmtoff
#define ngx_tm_zone           tm_zone
#endif


#if (NGX_SOLARIS)

#define ngx_timezone(isdst) (- (isdst ? altzone : timezone) / 60)

#else

#define ngx_timezone(isdst) (- (isdst ? timezone + 3600 : timezone) / 60)

#endif


void ngx_timezone_update(void);
void ngx_localtime(time_t s, ngx_tm_t *tm);
void ngx_libc_localtime(time_t s, struct tm *tm);
void ngx_libc_gmtime(time_t s, struct tm *tm);

#define ngx_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define ngx_msleep(ms)        (void) usleep(ms * 1000)
#define ngx_sleep(s)          (void) sleep(s)


#endif /* _NGX_TIME_H_INCLUDED_ */
