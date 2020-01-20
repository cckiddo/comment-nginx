
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The time may be updated by signal handler or by several threads.
 * The time update operations are rare and require to hold the ngx_time_lock.
 * The time read operations are frequent, so they are lock-free and get time
 * values and strings from the current slot.  Thus thread may get the corrupted
 * values only if it is preempted while copying and then it is not scheduled
 * to run more than NGX_TIME_SLOTS seconds.
 */

#define NGX_TIME_SLOTS   64

/*
nginx���������64��slotʱ�䣬Ҳ����ÿ�θ���ʱ���ʱ���Ǹ�����һ��
slot�����������ͬʱ���У������Ļ���֮ǰ��slot����û�б��ı䣬��Ȼ����ֻ���Ǿ���������ʱ����ҵļ��ʣ���Ϊslot�ĸ����������޵ģ�slot��ѭ���ģ�
д�������м��ʻ�д����������slot�ϡ�����nginx����ʵ���ϲ�û�в��ö��̵߳ķ�ʽ���������źŴ�����ֻ�Ǹ���cached_err_log_time�����Զ�����ʱ�����
�Ķ������ǲ��ᷢ�����ҵ� 
*/
static ngx_uint_t        slot;
static ngx_atomic_t      ngx_time_lock;

volatile ngx_msec_t      ngx_current_msec; //��������ʱ��1970��1��1���賿0��0��0�뵽��ǰʱ��ĺ�����
volatile ngx_time_t     *ngx_cached_time; //ngx_time_t�ṹ����ʽ�ĵ�ǰʱ��
volatile ngx_str_t       ngx_cached_err_log_time; //���ڼ�¼error_log�ĵ�ǰʱ���ַ��������ĸ�ʽ�����ڣ���1970/09/28  12��OO��OO��

//����HTTP��صĵ�ǰʱ���ַ��������ĸ�ʽ�����ڣ���Mon��28  Sep  1970  06��OO��OO  GMT��
volatile ngx_str_t       ngx_cached_http_time;
//���ڼ�¼HTTPԻ־�ĵ�ǰʱ���ַ��������ĸ�ʽ�����ڣ���28/Sep/1970��12��OO��00  +0600n"
volatile ngx_str_t       ngx_cached_http_log_time;
//��IS0 8601��׼��ʽ��¼�µ��ַ�����ʽ�ĵ�ǰʱ��
volatile ngx_str_t       ngx_cached_http_log_iso8601;
volatile ngx_str_t       ngx_cached_syslog_time;

#if !(NGX_WIN32)

/*
 * localtime() and localtime_r() are not Async-Signal-Safe functions, therefore,
 * they must not be called by a signal handler, so we use the cached
 * GMT offset value. Fortunately the value is changed only two times a year.
 */

static ngx_int_t         cached_gmtoff;
#endif

static ngx_time_t        cached_time[NGX_TIME_SLOTS]; //ϵͳ��ǰʱ�䣬��ngx_time_update
static u_char            cached_err_log_time[NGX_TIME_SLOTS]
                                    [sizeof("1970/09/28 12:00:00")];
static u_char            cached_http_time[NGX_TIME_SLOTS]
                                    [sizeof("Mon, 28 Sep 1970 06:00:00 GMT")]; //������ ʱ���� ���� ��ʽ
static u_char            cached_http_log_time[NGX_TIME_SLOTS]
                                    [sizeof("28/Sep/1970:12:00:00 +0600")];
static u_char            cached_http_log_iso8601[NGX_TIME_SLOTS]
                                    [sizeof("1970-09-28T12:00:00+06:00")];
static u_char            cached_syslog_time[NGX_TIME_SLOTS]
                                    [sizeof("Sep 28 12:00:00")];


static char  *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
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

void
ngx_time_init(void) //��ʼ��nginx�����ĵ�ǰʱ��
{
    ngx_cached_err_log_time.len = sizeof("1970/09/28 12:00:00") - 1;
    ngx_cached_http_time.len = sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1;
    ngx_cached_http_log_time.len = sizeof("28/Sep/1970:12:00:00 +0600") - 1;
    ngx_cached_http_log_iso8601.len = sizeof("1970-09-28T12:00:00+06:00") - 1;
    ngx_cached_syslog_time.len = sizeof("Sep 28 12:00:00") - 1;

    ngx_cached_time = &cached_time[0];

    ngx_time_update();
}
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

//ngx_time_update����������master�����е�ngx_master_process_cycle������ѭ���б����ã�����λ��Ϊsigsuspend��������֮��Ҳ����˵master���̲�׽����������һ���źŷ��ص�ʱ������ʱ�仺��
/*
����ʱ�仺��
Ϊ����ÿ�ζ�����OS��gettimeofday��nginx����ʱ�仺�棬ÿ��worker���̶�������ά����Ϊ���Ʋ������ʣ�ÿ�θ���ʱ�仺��ǰ��������������ʱ�仺�����������
Ϊ������Ѷ�����ĳworker���̶�ʱ�仺������н����ж������ڼ�ʱ�仺�汻����worker���£�����ǰ���ȡʱ�䲻һ�£�nginx����ʱ�仺������(��64����Ա)��ÿ�ζ����������е���һ��Ԫ�أ�
����ʱ��ͨ��ngx_time_update()ʵ��
ngx_time_update()������Ƶ��������worker���̴����¼�ʱ
ngx_worker_process_cycle -- ngx_process_events_and_timers -- ngx_process_events
#define ngx_process_events  ngx_event_actions.process_events
��epollΪ�������ӦAPIΪngx_epoll_process_events
ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags)
    events = epoll_wait(ep, event_list, (int) nevents, timer); 
    err = (events == -1) ? ngx_errno : 0;
    if (flags & NGX_UPDATE_TIME || ngx_event_timer_alarm) {  
        ngx_time_update();
    }

nginxʹ����ԭ�ӱ���ngx_time_lock����ʱ���������д����������nginx���ǵ���ʱ��Ĳ����Ƚ϶࣬�������ܵ�ԭ��û�жԶ����м��������ǲ���ά�����ʱ��
slot�ķ�ʽ���������ٶ����ʳ�ͻ������ԭ����ǣ�����������д����ͬʱ����ʱ��1�����߳�ʱ���ܷ�����2�����������ڶ�ʱ�仺��ʱ����һ�ź��ж�ȥִ��
�źŴ��������źŴ������л����ʱ�仺�棩��Ҳ���Ƕ��������ڽ���ʱ������տ�����ngx_cached_time->sec�����߿���ngx_cached_http_time.data����
��һ��ʱ�������д�����ı��˶�������ʱ�䣬���������յõ���ʱ��ͱ�����ˡ�nginx���������64��slotʱ�䣬Ҳ����ÿ�θ���ʱ���ʱ���Ǹ�����һ��
slot�����������ͬʱ���У������Ļ���֮ǰ��slot����û�б��ı䣬��Ȼ����ֻ���Ǿ���������ʱ����ҵļ��ʣ���Ϊslot�ĸ����������޵ģ�slot��ѭ���ģ�
д�������м��ʻ�д����������slot�ϡ�����nginx����ʵ���ϲ�û�в��ö��̵߳ķ�ʽ���������źŴ�����ֻ�Ǹ���cached_err_log_time�����Զ�����ʱ�����
�Ķ������ǲ��ᷢ�����ҵġ� ��һ���ط������������ж������� ngx_memory_barrier() ��ʵ�������Ҳ��һ���꣬���ľ��嶨��ͱ���������ϵ�ṹ�йأ�gcc
��x86�����£��������£�
#define ngx_memory_barrier()    __asm__ volatile ("" ::: "memory")
��������ʵ���ϻ��Ǻͷ�ֹ�����������йأ������߱�������Ҫ���������������Ż�����Ҫ������ִ��˳��
*/

/*
�������ʱ��ʲôʱ�������أ�����worker���̶��ԣ�����Nginx����ʱ����һ��ʱ���⣬�κθ���ʱ��Ĳ�����ֻ����ngx_epoll_process_events����
ִ�С��ع�һ��ngx_epoll_process_events�����Ĵ��룬��flags��������NGX_UPDATE_TIME��־λ������ngx_event_timer_alarm��־
λΪ1ʱ���ͻ����ngx_time_update�������»���ʱ�䡣
*/ //���û������timer_resolution��ʱ��������Զ����ʱ����Ϊepoll_wait�����أ��޷�����ʱ��
void
ngx_time_update(void)
{
    u_char          *p0, *p1, *p2, *p3, *p4;
    ngx_tm_t         tm, gmt;
    time_t           sec;
    ngx_uint_t       msec;
    ngx_time_t      *tp;
    struct timeval   tv;

    if (!ngx_trylock(&ngx_time_lock)) {
        return;
    }

    ngx_gettimeofday(&tv);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    ngx_current_msec = (ngx_msec_t) sec * 1000 + msec;

    tp = &cached_time[slot];//����ǰʱ�仺�� 

    if (tp->sec == sec) {//��������ʱ����=��ǰʱ���룬ֱ�Ӹ��µ�ǰslotԪ�ص�msec�����أ����������һ��slot����Ԫ�أ�
        tp->msec = msec;
        ngx_unlock(&ngx_time_lock);
        return;
    }

    if (slot == NGX_TIME_SLOTS - 1) {
        slot = 0;
    } else {
        slot++;
    }

    tp = &cached_time[slot];

    tp->sec = sec;
    tp->msec = msec;

    ngx_gmtime(sec, &gmt);


    p0 = &cached_http_time[slot][0];

    (void) ngx_sprintf(p0, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                       week[gmt.ngx_tm_wday], gmt.ngx_tm_mday,
                       months[gmt.ngx_tm_mon - 1], gmt.ngx_tm_year,
                       gmt.ngx_tm_hour, gmt.ngx_tm_min, gmt.ngx_tm_sec);

#if (NGX_HAVE_GETTIMEZONE)

    tp->gmtoff = ngx_gettimezone();
    ngx_gmtime(sec + tp->gmtoff * 60, &tm);

#elif (NGX_HAVE_GMTOFF)

    ngx_localtime(sec, &tm);
    cached_gmtoff = (ngx_int_t) (tm.ngx_tm_gmtoff / 60);
    tp->gmtoff = cached_gmtoff;

#else

    ngx_localtime(sec, &tm);
    cached_gmtoff = ngx_timezone(tm.ngx_tm_isdst);
    tp->gmtoff = cached_gmtoff;

#endif


    p1 = &cached_err_log_time[slot][0];

    (void) ngx_sprintf(p1, "%4d/%02d/%02d %02d:%02d:%02d",
                       tm.ngx_tm_year, tm.ngx_tm_mon,
                       tm.ngx_tm_mday, tm.ngx_tm_hour,
                       tm.ngx_tm_min, tm.ngx_tm_sec);


    p2 = &cached_http_log_time[slot][0];

    (void) ngx_sprintf(p2, "%02d/%s/%d:%02d:%02d:%02d %c%02d%02d",
                       tm.ngx_tm_mday, months[tm.ngx_tm_mon - 1],
                       tm.ngx_tm_year, tm.ngx_tm_hour,
                       tm.ngx_tm_min, tm.ngx_tm_sec,
                       tp->gmtoff < 0 ? '-' : '+',
                       ngx_abs(tp->gmtoff / 60), ngx_abs(tp->gmtoff % 60));

    p3 = &cached_http_log_iso8601[slot][0];

    (void) ngx_sprintf(p3, "%4d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
                       tm.ngx_tm_year, tm.ngx_tm_mon,
                       tm.ngx_tm_mday, tm.ngx_tm_hour,
                       tm.ngx_tm_min, tm.ngx_tm_sec,
                       tp->gmtoff < 0 ? '-' : '+',
                       ngx_abs(tp->gmtoff / 60), ngx_abs(tp->gmtoff % 60));

    p4 = &cached_syslog_time[slot][0];

    (void) ngx_sprintf(p4, "%s %2d %02d:%02d:%02d",
                       months[tm.ngx_tm_mon - 1], tm.ngx_tm_mday,
                       tm.ngx_tm_hour, tm.ngx_tm_min, tm.ngx_tm_sec);


      /*
    ���Կ���ngx_memory_barrier()֮����������ֵ��䣬���û�� ngx_memory_barrier()�����������ܻὫ ngx_cached_time = tp ��
    ngx_cached_http_time.data = p0��ngx_cached_err_log_time.data = p1�� ngx_cached_http_log_time.data = p2 �ֱ��֮ǰ�� 
    tp = &cached_time[slot] , p0 = &cached_http_time[slot][0] , p1 = &cached_err_log_time[slot][0] , p2 = &cached_http_log_time[slot][0] 
    �ϲ��Ż����������ĺ���� ngx_cached_time��ngx_cached_http_time��ngx_cached_err_log_time�� ngx_cached_http_log_time���ĸ�ʱ�仺���
    ��һ����ʱ�������ˣ���Ϊ�����һ��ngx_sprintfִ��������ĸ�ʱ�仺���һ�£�����֮ǰ����������ط����ڶ�ʱ�仺��Ϳ��ܵ��¶�����ʱ��
    ����ȷ���߲�һ�£�������ngx_memory_barrier() ��ʱ�仺����µ�һ�µ� ״ֻ̬��Ҫ����ʱ�����ڣ���Ϊֻ��������ֵָ���Ȼ����ô�̵�ʱ
    ���ڷ�����ʱ�仺��ĸ��ʻ�С�Ķ��ˡ���������Կ���Igor�����Ƿǳ�ϸ�µġ� 
    */
    ngx_memory_barrier();//��ֹ�������Ժ��������Ż������û��������ƣ����������ܽ�ǰ�������ִ���ϲ������ܵ�����6��ʱ����³��ּ�����ڼ�������ȡ�����ʱ�䲻һ�µ���� 

    ngx_cached_time = tp;
    ngx_cached_http_time.data = p0;
    ngx_cached_err_log_time.data = p1;
    ngx_cached_http_log_time.data = p2;
    ngx_cached_http_log_iso8601.data = p3;
    ngx_cached_syslog_time.data = p4;

    ngx_unlock(&ngx_time_lock);
}

/*
nginx�������ܿ��ǲ�������lib_event�ķ�ʽ���Լ���ʱ�������cache���������ٶ�gettimeofday�����ĵ��ã���Ϊһ����˵����
����ʱ��ľ���Ҫ�����ر�ĸߣ����������Ҫ�ȽϾ�ȷ��timer��nginx���ṩ��һ��timer_resolutionָ����������ʱ�侫�ȣ�
����Ļ����ٺ����������
*/
/*
ngx_time_update������ngx_time_sigsafe_update����������������ʵ�ֱȽϼ򵥣����ǻ����м���ֵ��ע��ĵط�����������ʱ��������źŴ����б����£�
������̵߳�ʱ��Ҳ����ͬʱ����ʱ�䣨nginx������Ȼû�п��Ŷ��̣߳����Ǵ������п��ǣ���nginxʹ����ԭ�ӱ���ngx_time_lock����ʱ���������д������
����nginx���ǵ���ʱ��Ĳ����Ƚ϶࣬�������ܵ�ԭ��û�жԶ����м��������ǲ���ά�����ʱ��slot�ķ�ʽ���������ٶ����ʳ�ͻ������ԭ����ǣ���������
��д����ͬʱ����ʱ��1�����߳�ʱ���ܷ�����2�����������ڶ�ʱ�仺��ʱ����һ�ź��ж�ȥִ���źŴ��������źŴ������л����ʱ�仺�棩��Ҳ���Ƕ�
�������ڽ���ʱ������տ�����ngx_cached_time->sec�����߿���ngx_cached_http_time.data���е�һ��ʱ�������д�����ı��˶�������ʱ�䣬���������յ�
����ʱ��ͱ�����ˡ�nginx���������64��slotʱ�䣬Ҳ����ÿ�θ���ʱ���ʱ���Ǹ�����һ��slot�����������ͬʱ���У������Ļ���֮ǰ��slot����û��
���ı䣬��Ȼ����ֻ���Ǿ���������ʱ����ҵļ��ʣ���Ϊslot�ĸ����������޵ģ�slot��ѭ���ģ�д�������м��ʻ�д����������slot�ϡ�����nginx����ʵ��
�ϲ�û�в��ö��̵߳ķ�ʽ���������źŴ�����ֻ�Ǹ���cached_err_log_time�����Զ�����ʱ������Ķ������ǲ��ᷢ�����ҵġ� ��һ���ط������������ж�
������ ngx_memory_barrier() ��ʵ�������Ҳ��һ���꣬���ľ��嶨��ͱ���������ϵ�ṹ�йأ�gcc��x86�����£��������£�
*/
#if !(NGX_WIN32)
//������ÿ��ִ���źŴ�������ʱ�򱻵��ã�Ҳ������ngx_signal_handler���������С�
void
ngx_time_sigsafe_update(void)
{
    u_char          *p, *p2;
    ngx_tm_t         tm;
    time_t           sec;
    ngx_time_t      *tp;
    struct timeval   tv;

    if (!ngx_trylock(&ngx_time_lock)) {
        return;
    }

    ngx_gettimeofday(&tv);

    sec = tv.tv_sec;

    tp = &cached_time[slot];

    if (tp->sec == sec) {
        ngx_unlock(&ngx_time_lock);
        return;
    }

    if (slot == NGX_TIME_SLOTS - 1) {
        slot = 0;
    } else {
        slot++;
    }

    tp = &cached_time[slot];

    tp->sec = 0;

    ngx_gmtime(sec + cached_gmtoff * 60, &tm);

    p = &cached_err_log_time[slot][0];

    (void) ngx_sprintf(p, "%4d/%02d/%02d %02d:%02d:%02d",
                       tm.ngx_tm_year, tm.ngx_tm_mon,
                       tm.ngx_tm_mday, tm.ngx_tm_hour,
                       tm.ngx_tm_min, tm.ngx_tm_sec);

    p2 = &cached_syslog_time[slot][0];

    (void) ngx_sprintf(p2, "%s %2d %02d:%02d:%02d",
                       months[tm.ngx_tm_mon - 1], tm.ngx_tm_mday,
                       tm.ngx_tm_hour, tm.ngx_tm_min, tm.ngx_tm_sec);


    /*
    ���Կ���ngx_memory_barrier()֮����������ֵ��䣬���û�� ngx_memory_barrier()�����������ܻὫ ngx_cached_time = tp ��
    ngx_cached_http_time.data = p0��ngx_cached_err_log_time.data = p1�� ngx_cached_http_log_time.data = p2 �ֱ��֮ǰ�� 
    tp = &cached_time[slot] , p0 = &cached_http_time[slot][0] , p1 = &cached_err_log_time[slot][0] , p2 = &cached_http_log_time[slot][0] 
    �ϲ��Ż����������ĺ���� ngx_cached_time��ngx_cached_http_time��ngx_cached_err_log_time�� ngx_cached_http_log_time���ĸ�ʱ�仺���
    ��һ����ʱ�������ˣ���Ϊ�����һ��ngx_sprintfִ��������ĸ�ʱ�仺���һ�£�����֮ǰ����������ط����ڶ�ʱ�仺��Ϳ��ܵ��¶�����ʱ��
    ����ȷ���߲�һ�£�������ngx_memory_barrier() ��ʱ�仺����µ�һ�µ� ״ֻ̬��Ҫ����ʱ�����ڣ���Ϊֻ��������ֵָ���Ȼ����ô�̵�ʱ
    ���ڷ�����ʱ�仺��ĸ��ʻ�С�Ķ��ˡ���������Կ���Igor�����Ƿǳ�ϸ�µġ� 
    */
    ngx_memory_barrier();

    //���źŴ�����ֻ�Ǹ���cached_err_log_time ngx_cached_syslog_time�����Զ�����ʱ������Ķ������ǲ��ᷢ�����ҵ�
    ngx_cached_err_log_time.data = p;
    ngx_cached_syslog_time.data = p2;

    ngx_unlock(&ngx_time_lock);
}

#endif

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

u_char *
ngx_http_time(u_char *buf, time_t t)
{
    ngx_tm_t  tm;

    ngx_gmtime(t, &tm);

    return ngx_sprintf(buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                       week[tm.ngx_tm_wday],
                       tm.ngx_tm_mday,
                       months[tm.ngx_tm_mon - 1],
                       tm.ngx_tm_year,
                       tm.ngx_tm_hour,
                       tm.ngx_tm_min,
                       tm.ngx_tm_sec);
}

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

u_char *
ngx_http_cookie_time(u_char *buf, time_t t)
{
    ngx_tm_t  tm;

    ngx_gmtime(t, &tm);

    /*
     * Netscape 3.x does not understand 4-digit years at all and
     * 2-digit years more than "37"
     */

    return ngx_sprintf(buf,
                       (tm.ngx_tm_year > 2037) ?
                                         "%s, %02d-%s-%d %02d:%02d:%02d GMT":
                                         "%s, %02d-%s-%02d %02d:%02d:%02d GMT",
                       week[tm.ngx_tm_wday],
                       tm.ngx_tm_mday,
                       months[tm.ngx_tm_mon - 1],
                       (tm.ngx_tm_year > 2037) ? tm.ngx_tm_year:
                                                 tm.ngx_tm_year % 100,
                       tm.ngx_tm_hour,
                       tm.ngx_tm_min,
                       tm.ngx_tm_sec);
}

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

//��time_tʱ��ת��Ϊtmʱ��
void
ngx_gmtime(time_t t, ngx_tm_t *tp)
{
    ngx_int_t   yday;
    ngx_uint_t  n, sec, min, hour, mday, mon, year, wday, days, leap;

    /* the calculation is valid for positive time_t only */

    n = (ngx_uint_t) t;

    days = n / 86400;

    /* January 1, 1970 was Thursday */

    wday = (4 + days) % 7;

    n %= 86400;
    hour = n / 3600;
    n %= 3600;
    min = n / 60;
    sec = n % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/ngx_http_parse_time.c
     */

    /* days since March 1, 1 BC */
    days = days - (31 + 28) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last February days to next year, but we catch the case when "yday"
     * becomes negative.
     */

    year = (days + 2) * 400 / (365 * 400 + 100 - 4 + 1);

    yday = days - (365 * year + year / 4 - year / 100 + year / 400);

    if (yday < 0) {
        leap = (year % 4 == 0) && (year % 100 || (year % 400 == 0));
        yday = 365 + leap + yday;
        year--;
    }

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    mon = (yday + 31) * 10 / 306;

    /* the Gauss' formula that evaluates days before the month */

    mday = yday - (367 * mon / 12 - 30) + 1;

    if (yday >= 306) {

        year++;
        mon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        mon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }

    tp->ngx_tm_sec = (ngx_tm_sec_t) sec;
    tp->ngx_tm_min = (ngx_tm_min_t) min;
    tp->ngx_tm_hour = (ngx_tm_hour_t) hour;
    tp->ngx_tm_mday = (ngx_tm_mday_t) mday;
    tp->ngx_tm_mon = (ngx_tm_mon_t) mon;
    tp->ngx_tm_year = (ngx_tm_year_t) year;
    tp->ngx_tm_wday = (ngx_tm_wday_t) wday;
}

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

time_t
ngx_next_time(time_t when)
{
    time_t     now, next;
    struct tm  tm;

    now = ngx_time();

    ngx_libc_localtime(now, &tm);

    tm.tm_hour = (int) (when / 3600);
    when %= 3600;
    tm.tm_min = (int) (when / 60);
    tm.tm_sec = (int) (when % 60);

    next = mktime(&tm);

    if (next == -1) {
        return -1;
    }

    if (next - now > 0) {
        return next;
    }

    tm.tm_mday++;

    /* mktime() should normalize a date (Jan 32, etc) */

    next = mktime(&tm);

    if (next != -1) {
        return next;
    }

    return -1;
}
