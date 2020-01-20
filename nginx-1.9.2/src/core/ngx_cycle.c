
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


static void ngx_destroy_cycle_pools(ngx_conf_t *conf);
static ngx_int_t ngx_init_zone_pool(ngx_cycle_t *cycle,
    ngx_shm_zone_t *shm_zone);
static ngx_int_t ngx_test_lockfile(u_char *file, ngx_log_t *log);
static void ngx_clean_old_cycles(ngx_event_t *ev);

//��ʼ���ο�ngx_init_cycle��������һ��ȫ�����͵�ngx_cycle_s����ngx_cycle
volatile ngx_cycle_t  *ngx_cycle; //ngx_cycle = cycle;  ��ֵ��main
ngx_array_t            ngx_old_cycles;

static ngx_pool_t     *ngx_temp_pool;
static ngx_event_t     ngx_cleaner_event;

ngx_uint_t             ngx_test_config;
ngx_uint_t             ngx_dump_config;
ngx_uint_t             ngx_quiet_mode;


/* STUB NAME */
static ngx_connection_t  dumb;
/* STUB */

/*
��8-2 ngx_cycle_t�ṹ��֧�ֵ���Ҫ����
�������������������������������������ש��������������������������������������ש���������������������������������������
��    ������                        ��    ��������                          ��    ִ������                          ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��                                  ��                                      ��  ���س�ʼ���ɹ���������ngx_cycle_    ��
��  ngx_cycle_t *ngx_init_cycle     ��  old_cycle��ʾ��ʱ��ngx_cycle_t      ��t�ṹ�壬�ú������Ḻ���ʼ��ngx_     ��
��                                  ��ָ�룬һ�����������ngx_cycle_t��     ��cycle_t�е����ݽṹ�����������ļ���   ��
��(ngx_cycle_t *old_cycle)          ��                                      ����������ģ�顢�򿪼����˿ڡ���ʼ��    ��
��                                  �������е������ļ�·���Ȳ���            ��                                      ��
��                                  ��                                      �����̼�ͨ�ŷ�ʽ�ȹ��������ʧ�ܣ���    ��
��                                  ��                                      ������NULL��ָ��                        ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��  ngx_int_t ngx_process_          ��  cycleͨ���Ǹոշ����ngx_cycle_t    ��  ������Nginxʱ����Я����Ŀ¼����     ��
��                                  ���ṹ��ָ�룬�����ڴ��������ļ�·      ������ʼ��cycle��������ʼ������Ŀ¼��   ��
��options (ngx_cycle_t *cycle)      ��                                      ������Ŀ¼��������������nginx��conf��   ��
��                                  ������Ϣ                                ��                                      ��
��                                  ��                                      �����ļ�·��                            ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��                                  ��                                      ��  �����в�������������Nginx�Ĳ�       ��
��                                  ��                                      ����ʱ���ϵ�Nginx���̻�ͨ����������     ��
��  ngx_int_t ngx_add_inherited     ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ����NGINX����������Ҫ�򿪵ļ�����       ��
��sockets(ngx_cycle_t��cycle)       ������ָ��                              ���ڣ��µ�Nginx���̻�ͨ��ngx_add_       ��
��                                  ��                                      ��inherited- sockets������ʹ���Ѿ���  ��
��                                  ��                                      ����TCP�����˿�                         ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��  ngxjnt_t ngx_open_listening_    ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��  ��������cycle��listening��̬��    ��
��sockets (ngx_cycle_t *cycle)      ������ָ��                              ����ָ������Ӧ�˿�                      ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��   void ngx_configure_listening_  ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��  ����nginx.conf�е������������Ѿ�    ��
��sockets(ngx_cycle_t��cycle)       ������ָ��                              �������ľ��                            ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��void ngx_close_listening_         ��    cycle�ǵ�ǰ���̵�ngx_cycle_t��    ��  �ر�cycle��listening��̬�����Ѿ�    ��
��sockets(ngx_cycle_t *cycle)       ������ָ��                              ���򿪵ľ��                            ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��void ngx_master_process_          ��    cycle�ǵ�ǰ���̵�ngx_cycle_t��    ��  ����master���̵Ĺ���ѭ��            ��
��cycle(ngx_cycle_t *cycle)         ������ָ��                              ��                                      ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��   void ngx_single_process_       ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��  ���뵥����ģʽ����master��worker    ��
��cycle (ngx_cycle_t *cycle)        ������ָ��                              �����̹���ģʽ���Ĺ���ѭ��              ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��                                  ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��                                      ��
��                                  ������ָ�룬n�������ӽ��̵ĸ�����       ��                                      ��
��                                  ��type��������ʽ������ȡֵ��Χ����      ��                                      ��
��                                  ����5����                               ��                                      ��
��   void ngx_start_worker_         ��    1)  NGX_PROCESS_RESPAWN;          ��  ����n��worker�ӽ��̣������ú�       ��
��processes (ngx_cycle_t *cycle,    ��    2) NGX__ PROCESS NORESPAWN;       ��ÿ���ӽ�����master������֮��ʹ��      ��
��                                  ��    3) NGX��PROCESS_JUST_SPAWN;       ��socketpairϵͳ���ý���������socket    ��
��ngx_int_t n, ngx_int_t type)      ��                                      ��                                      ��
��                                  ��    4) NGX.PROCESS JUST_RESPAWN;      �����ͨ�Ż���                          ��
��                                  ��    5) NGX_PROCESS��DETACHED.         ��                                      ��
��                                  ��type��ֵ��Ӱ��       ngn_process_t    ��                                      ��
��                                  ���ṹ���respawn. detached. just_spawn ��                                      ��
��                                  ����־λ��ֵ                            ��                                      ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��                                  ��                                      ��  �����Ƿ�ʹ���ļ�����ģ�飬Ҳ����    ��
��   void ngx_start_cache_          ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��cycle�д洢·���Ķ�̬�������Ƿ���     ��
��manager_processes(ngx_cycle_t     ������ָ�룬respawn�������ӽ��̵ķ�     ��·����manage��־�򿪣��������Ƿ�      ��
��                                  ��ʽ��  ����ngx_start_worker_processes  ������cache manage�ӽ��̣�ͬ������      ��
��*cycle, ngx_uint_t respawn)       ��                                      ��                                      ��
��                                  �������е�type����������ȫ��ͬ          ��loader��־�����Ƿ�����cache loader    ��
��                                  ��                                      ���ӽ���                                ��
�ǩ����������������������������������贈�������������������������������������贈��������������������������������������
��   void ngx_pass_open_channel     ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��      ��  �������Ѿ��򿪵�channel��ͨ��       ��
��(ngx_cycle_t *cycle, ngx_         ������ָ�룬ch�ǽ�Ҫ���ӽ��̷��͵�      ��socketpair���ɵľ������ͨ�ţ�����    ��
��channel_t *ch)                    ����Ϣ                                  ��ch��Ϣ                                ��
�������������������������������������ߩ��������������������������������������ߩ���������������������������������������
���������������������ש����������������ש����������������������������������ש�������������������������������������
��    ����          ����              ��    ��������                      ��    ִ������                        ��
�ǩ������������������ߩ����������������贈���������������������������������贈������������������������������������
��   void ngx_signal_worker_          ��                                  ��                                    ��
��processes (ngx_cycle_t *cycle,      ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��  ����worker���̽��յ����ź�        ��
��                                    ������ָ�룬signo���ź�             ��                                    ��
��int signo)                          ��                                  ��                                    ��
�ǩ������������������������������������贈���������������������������������贈������������������������������������
��                                    ��                                  ��  ���master���̵������ӽ��̣���    ��
��  ngx_uint_t ngx_reap_children      ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ����ÿ���ӽ��̵�״̬��ngx_process_t�� ��
��(ngx_cycle_t *cycle)                ������ָ��                          �������еı�־λ���ж��Ƿ�Ҫ�����ӽ�  ��
��                                    ��                                  ���̡�����pid�ļ���                   ��
�ǩ������������������������������������贈���������������������������������贈������������������������������������
��  voidngx_maste,r_process exit      ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��                                    ��
��(ngx_cycle_t *cyc, le)              ������ָ��                          ��  �˳�master���̹�����ѭ��          ��
�ǩ������������������ש����������������贈���������������������������������贈������������������������������������
��  void ngx_wo?    ��Lker_process_   ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��                                    ��
��cycle (ngx_cycle  ��-t *cycle, void ������ָ�룬���ﻹδ��ʼʹ��data��  ��  ����worker���̹�����ѭ��          ��
��*data)            ��                ����������data -��ΪNULL            ��                                    ��
�ǩ������������������ߩ����������������贈���������������������������������贈������������������������������������
��   void ngx_worker_process_         ��  cycle����ǰ���̵�ngx_cycle_t��  ��  ����worker���̹���ѭ��֮ǰ�ĳ�    ��
��init (ngx_cycle_t *cycle, ngx_      ������ָ�룬priority��worker���̵�  ��ʼ������                            ��
��uint_t priority)                    ��ϵͳ���ȼ�                        ��                                    ��
�ǩ������������������������������������贈���������������������������������贈������������������������������������
��  void ngx_woriker_process_         ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��                                    ��
��exit (ngx_cycle_t, *cycle)          ������ָ��                          ��  �˳�worker���̹�����ѭ��          ��
�ǩ������������������������������������贈���������������������������������贈������������������������������������
��  void ngx_cac:he_manager_          ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��  ִ�л����������ѭ������������  ��
��process_cycle(ngx_cycle_t           ������ָ�룬data�Ǵ��˵�ngx_cache_  ���ļ�����ģ��������أ��ڱ����в���  ��
��*cycle, void *data)                 ��manager_ctx_t�ṹ��ָ��           ����ϸ̽��                            ��
�ǩ������������������������������������贈���������������������������������贈������������������������������������
��   void ngx_process_events_         ��  cycle�ǵ�ǰ���̵�ngx_cycle_t��  ��  ʹ���¼�ģ�鴦���ֹ�������Ѿ���  ��
��                                    ��                                  ���������¼����ú������°�ģ��ʵ�֣�  ��
��and_timers (ngx_cycle_t *cycle)     ������ָ��                          ��                                    ��
��                                    ��                                  ��                                    ��
���������������������������������������ߩ����������������������������������ߩ�������������������������������������
*/

/*
Ϊʲô������Ҫ��old_cycle???
    �ɵ�ngx_cycle_t��������������һ��ngx_cycle_t�����еĳ�Ա������ngx_init_cycle���������������ڣ���Ҫ��
��һ����ʱ��ngx_cycle_t���󱣴�һЩ����(��ngx_process_options�еİ�װ·������cycle��������ǰ��ǰд��־��log),
�ٵ���ngx_init_cycle ����ʱ�Ϳ��԰Ѿɵ�ngx_cycle_t���󴫽�ȥ
*/
ngx_cycle_t *
ngx_init_cycle(ngx_cycle_t *old_cycle)
{
    void                *rv;
    char               **senv, **env;
    ngx_uint_t           i, n;
    ngx_log_t           *log;
    ngx_time_t          *tp;
    ngx_conf_t           conf;
    ngx_pool_t          *pool;
    ngx_cycle_t         *cycle, **old;
    ngx_shm_zone_t      *shm_zone, *oshm_zone;
    ngx_list_part_t     *part, *opart;
    ngx_open_file_t     *file;
    ngx_listening_t     *ls, *nls;
    ngx_core_conf_t     *ccf, *old_ccf;
    ngx_core_module_t   *module; //��Ӧ���Ǻ���ģ��NGX_CORE_MODULE
    char                 hostname[NGX_MAXHOSTNAMELEN];

    ngx_timezone_update();

    /* force localtime update with a new timezone */

    tp = ngx_timeofday();
    tp->sec = 0;

    ngx_time_update();

    //�ڽ��������ļ���error_logǰ��ʹ������ɵ�Ĭ��log,��������������ļ��󣬻����°���error_log�����ļ���д��־
    log = old_cycle->log;

    pool = ngx_create_pool(NGX_CYCLE_POOL_SIZE, log); /*16K */
    if (pool == NULL) {
        return NULL;
    }
    pool->log = log;

    cycle = ngx_pcalloc(pool, sizeof(ngx_cycle_t));
    if (cycle == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cycle->pool = pool;
    cycle->log = log;
    cycle->old_cycle = old_cycle;

    cycle->conf_prefix.len = old_cycle->conf_prefix.len;
    cycle->conf_prefix.data = ngx_pstrdup(pool, &old_cycle->conf_prefix);
    if (cycle->conf_prefix.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cycle->prefix.len = old_cycle->prefix.len;
    cycle->prefix.data = ngx_pstrdup(pool, &old_cycle->prefix);
    if (cycle->prefix.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cycle->conf_file.len = old_cycle->conf_file.len;
    cycle->conf_file.data = ngx_pnalloc(pool, old_cycle->conf_file.len + 1);
    if (cycle->conf_file.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }
    ngx_cpystrn(cycle->conf_file.data, old_cycle->conf_file.data,
                old_cycle->conf_file.len + 1);

    cycle->conf_param.len = old_cycle->conf_param.len;
    cycle->conf_param.data = ngx_pstrdup(pool, &old_cycle->conf_param);
    if (cycle->conf_param.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    n = old_cycle->paths.nelts ? old_cycle->paths.nelts : 10;

    cycle->paths.elts = ngx_pcalloc(pool, n * sizeof(ngx_path_t *));
    if (cycle->paths.elts == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cycle->paths.nelts = 0;
    cycle->paths.size = sizeof(ngx_path_t *);
    cycle->paths.nalloc = n;

    /*
    // ���ﶼ���ڴ�ظ���ֵ��������ڴ�����ԣ� ��Ϊ���ڴ����Դ�����棬nginxί���ڴ��ͳһ���� �������ͨ������Ķ����ҵ�
    ���õ��ڴ�ض��� ����ʵ���ڴ����Դ����Ĺ���  
    */
    cycle->paths.pool = pool;


    if (ngx_array_init(&cycle->config_dump, pool, 1, sizeof(ngx_conf_dump_t))
        != NGX_OK)
    {
        ngx_destroy_pool(pool);
        return NULL;
    }

    if (old_cycle->open_files.part.nelts) {
        n = old_cycle->open_files.part.nelts;
        for (part = old_cycle->open_files.part.next; part; part = part->next) {
            n += part->nelts;
        }

    } else {
        n = 20;
    }

    if (ngx_list_init(&cycle->open_files, pool, n, sizeof(ngx_open_file_t))
        != NGX_OK)
    {
        ngx_destroy_pool(pool);
        return NULL;
    }


    if (old_cycle->shared_memory.part.nelts) {
        n = old_cycle->shared_memory.part.nelts;
        for (part = old_cycle->shared_memory.part.next; part; part = part->next)
        {
            n += part->nelts;
        }

    } else {
        n = 1;
    }

    if (ngx_list_init(&cycle->shared_memory, pool, n, sizeof(ngx_shm_zone_t)) 
        != NGX_OK)
    {
        ngx_destroy_pool(pool);
        return NULL;
    }

    n = old_cycle->listening.nelts ? old_cycle->listening.nelts : 10;

    cycle->listening.elts = ngx_pcalloc(pool, n * sizeof(ngx_listening_t));
    if (cycle->listening.elts == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cycle->listening.nelts = 0;
    cycle->listening.size = sizeof(ngx_listening_t);
    cycle->listening.nalloc = n;
    cycle->listening.pool = pool;

    ngx_queue_init(&cycle->reusable_connections_queue);


    cycle->conf_ctx = ngx_pcalloc(pool, ngx_max_module * sizeof(void *));
    if (cycle->conf_ctx == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }


    if (gethostname(hostname, NGX_MAXHOSTNAMELEN) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "gethostname() failed");
        ngx_destroy_pool(pool);
        return NULL;
    }

    /* on Linux gethostname() silently truncates name that does not fit */
    hostname[NGX_MAXHOSTNAMELEN - 1] = '\0';
    cycle->hostname.len = ngx_strlen(hostname);

    cycle->hostname.data = ngx_pnalloc(pool, cycle->hostname.len);
    if (cycle->hostname.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    ngx_strlow(cycle->hostname.data, (u_char *) hostname, cycle->hostname.len);

/*
�ڳ�ʼ��ngx_cycle_t�е����������󣬻�Ϊ��ȡ�����������ļ���׼����������Ϊÿ��ģ�鶼��������Ӧ������
�ṹ���洢�����ļ��еĸ������������Щ���ݽṹ�Ĺ�������Ҫ����һ�����С�Nginx���ֻ����NGX_CORE_MODULE��
��ģ�飬��Ҳ��Ϊ�˽��Ϳ�ܵĸ��Ӷȡ����ｫ��������к���ģ���create conf������Ҳֻ�к���ģ����������������
����ζ����Ҫ���еĺ���ģ�鿪ʼ�������ڴ洢������Ľṹ�塣�����Ǻ���ģ����ô���أ���ʵ�ܼ򵥡���Щģ���
������һ������ģ�飬��ÿ��HTTPģ�鶼��ngx_http_module������ͼ8-2��ʾ��������ngx_http_module�ڽ����Լ�����
Ȥ�ġ�http��������ʱ�������������HTTPģ��Լ���ķ����������洢������Ľṹ�壨xxx_create_main_conf��xxx_create_srv_conf��xxx_create_loc_conf��������
*/
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_CORE_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_conf) {
            rv = module->create_conf(cycle);
            if (rv == NULL) {
                ngx_destroy_pool(pool);
                return NULL;
            }
            
            cycle->conf_ctx[ngx_modules[i]->index] = rv;
        }
    }

    senv = environ;

    ngx_memzero(&conf, sizeof(ngx_conf_t));
    /* STUB: init array ? */
    conf.args = ngx_array_create(pool, 10, sizeof(ngx_str_t));
    if (conf.args == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    conf.temp_pool = ngx_create_pool(NGX_CYCLE_POOL_SIZE, log);
    if (conf.temp_pool == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    conf.ctx = cycle->conf_ctx; //���������ngx_conf_param�������õ�ʱ�������conf.ctx��ֵ������ʵ���Ͼ��Ƕ�cycle->conf_ctx[i]��ֵ
    conf.cycle = cycle;
    conf.pool = pool;
    conf.log = log;
    conf.module_type = NGX_CORE_MODULE;
    conf.cmd_type = NGX_MAIN_CONF;

#if 0
    log->log_level = NGX_LOG_DEBUG_ALL;
#endif

    if (ngx_conf_param(&conf) != NGX_CONF_OK) { //��ʱ���confָ�����cycle
        environ = senv;
        ngx_destroy_cycle_pools(&conf);
        return NULL;
    }
        
    if (ngx_conf_parse(&conf, &cycle->conf_file) != NGX_CONF_OK) {//��ʱ���confָ�����cycle
        environ = senv;
        ngx_destroy_cycle_pools(&conf);
        return NULL;
    }
        
    if (ngx_test_config && !ngx_quiet_mode) {
        ngx_log_stderr(0, "the configuration file %s syntax is ok",
                       cycle->conf_file.data);
    }

    /*
    ��������NGX_CORE_MODULE����ģ���init_conf��������һ�����Ŀ�����������к���ģ���ڽ������������������ۺ��Դ���
    */
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_CORE_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->init_conf) {
            if (module->init_conf(cycle, cycle->conf_ctx[ngx_modules[i]->index])
                == NGX_CONF_ERROR)
            {
                environ = senv;
                ngx_destroy_cycle_pools(&conf);
                return NULL;
            }
        }
    }

    if (ngx_process == NGX_PROCESS_SIGNALLER) {
        return cycle;
    }

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (ngx_test_config) {

        if (ngx_create_pidfile(&ccf->pid, log) != NGX_OK) {
            goto failed;
        }

    } else if (!ngx_is_init_cycle(old_cycle)) {

        /*
         * we do not create the pid file in the first ngx_init_cycle() call
         * because we need to write the demonized process pid
         */

        old_ccf = (ngx_core_conf_t *) ngx_get_conf(old_cycle->conf_ctx,
                                                   ngx_core_module);
        if (ccf->pid.len != old_ccf->pid.len
            || ngx_strcmp(ccf->pid.data, old_ccf->pid.data) != 0)
        {
            /* new pid file name */

            if (ngx_create_pidfile(&ccf->pid, log) != NGX_OK) {
                goto failed;
            }

            ngx_delete_pidfile(old_cycle);
        }
    }


    if (ngx_test_lockfile(cycle->lock_file.data, log) != NGX_OK) {
        goto failed;
    }

    //������nginx.conf���漰���ļ�·�����ļ���
    if (ngx_create_paths(cycle, ccf->user) != NGX_OK) {
        goto failed;
    }

    if (ngx_log_open_default(cycle) != NGX_OK) {
        goto failed;
    }

    /* open the new files */

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].name.len == 0) {
            continue;
        }

        file[i].fd = ngx_open_file(file[i].name.data,
                                   NGX_FILE_APPEND,
                                   NGX_FILE_CREATE_OR_OPEN,
                                   NGX_FILE_DEFAULT_ACCESS);

        ngx_log_debug3(NGX_LOG_DEBUG_CORE, log, 0,
                       "log: %p %d \"%s\"",
                       &file[i], file[i].fd, file[i].name.data);

        if (file[i].fd == NGX_INVALID_FILE) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                          ngx_open_file_n " \"%s\" failed",
                          file[i].name.data);
            goto failed;
        }

#if !(NGX_WIN32)
        if (fcntl(file[i].fd, F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                          "fcntl(FD_CLOEXEC) \"%s\" failed",
                          file[i].name.data);
            goto failed;
        }
#endif
    }

    /**/
    //�ڽ��������ļ���error_logǰ��ʹ������ɵ�Ĭ��log,�����������ļ��󣬻����°���error_log�����ļ���д��־
    //new_log��Ϣͨ��ǰ���ngx_log_open_default����new_log��Ϣ�����������part = &cycle->open_files.part;�д�������ļ������fd
    cycle->log = &cycle->new_log;
    pool->log = &cycle->new_log;

    /*  �ߵ������ʱ�����е������Ѿ�������ϣ��������"zone" proxy_cache_path fastcgi_cache_path����Ҫ���������ڴ棬�����������δ�����Ӧ�Ĺ����ڴ�  */

    /* create shared memory */
    part = &cycle->shared_memory.part;
    shm_zone = part->elts;
    //ngx_cycle_t�ṹ���shared_mem ory�����н��Ὺʼ��ʼ�����ڽ��̼�ͨ�ŵĹ����ڴ档
    for (i = 0; /* void */ ; i++) {
        if (i >= part->nelts) { //��������nginx��ʱ��Ż�ִ���������ش��룬Ĭ��i��part->nelts������0
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            shm_zone = part->elts;
            i = 0;
        }

        if (shm_zone[i].shm.size == 0) {
            ngx_log_error(NGX_LOG_EMERG, log, 0,
                          "zero size shared memory zone \"%V\"",
                          &shm_zone[i].shm.name);
            goto failed;
        }

        shm_zone[i].shm.log = cycle->log;

        opart = &old_cycle->shared_memory.part;
        oshm_zone = opart->elts;

        for (n = 0; /* void */ ; n++) {

            if (n >= opart->nelts) {
                if (opart->next == NULL) {
                    break;
                }
                opart = opart->next;
                oshm_zone = opart->elts;
                n = 0;
            }

            if (shm_zone[i].shm.name.len != oshm_zone[n].shm.name.len) {
                continue;
            }

            if (ngx_strncmp(shm_zone[i].shm.name.data,
                            oshm_zone[n].shm.name.data,
                            shm_zone[i].shm.name.len) //
                != 0)
            {
                continue;
            }

            if (shm_zone[i].tag == oshm_zone[n].tag
                && shm_zone[i].shm.size == oshm_zone[n].shm.size
                && !shm_zone[i].noreuse)
            {
                shm_zone[i].shm.addr = oshm_zone[n].shm.addr;
#if (NGX_WIN32)
                shm_zone[i].shm.handle = oshm_zone[n].shm.handle;
#endif

                if (shm_zone[i].init(&shm_zone[i], oshm_zone[n].data)
                    != NGX_OK)
                {
                    goto failed;
                }

                goto shm_zone_found;
            }

            ngx_shm_free(&oshm_zone[n].shm);

            break;
        }

        if (ngx_shm_alloc(&shm_zone[i].shm) != NGX_OK) {
            goto failed;
        }

        if (ngx_init_zone_pool(cycle, &shm_zone[i]) != NGX_OK) {
            goto failed;
        }

        if (shm_zone[i].init(&shm_zone[i], NULL) != NGX_OK) {
            goto failed;
        }

    shm_zone_found:

        continue;
    }


    /* handle the listening sockets */
/*
���е�ģ�鶼�Ѿ��������Լ���Ҫ�����Ķ˿ڣ���HTTPģ���Ѿ��ڽ���http{������}������ʱ�õ���Ҫ�����Ķ˿ڣ�����ӵ�
listening�������ˡ���һ������ǰ���listening�����е�ÿһ��ngx_listening_tԪ������socket����������˿ڣ�ʵ���ϣ���һ�������Ҫ�������ǵ�
�ñ�8-2�е�ngx_open_listening_sockets��������
*/
    if (old_cycle->listening.nelts) {
        ls = old_cycle->listening.elts; //�ɵ�listen,���������̳й�����sock,��ngx_add_inherited_sockets
        for (i = 0; i < old_cycle->listening.nelts; i++) {
            ls[i].remain = 0;
        }

        nls = cycle->listening.elts; //�������ļ�nginx.conf��listen��server ip�Ͷ˿ڻ�ȡ������Ϣ
        for (n = 0; n < cycle->listening.nelts; n++) {

            for (i = 0; i < old_cycle->listening.nelts; i++) {
                if (ls[i].ignore) {
                    continue;
                }

                if (ls[i].remain) {
                    continue;
                }

                if (ngx_cmp_sockaddr(nls[n].sockaddr, nls[n].socklen,
                                     ls[i].sockaddr, ls[i].socklen, 1)
                    == NGX_OK)
                {
                    nls[n].fd = ls[i].fd;
                    nls[n].previous = &ls[i];
                    ls[i].remain = 1;

                    if (ls[i].backlog != nls[n].backlog) {
                        nls[n].listen = 1;
                    }

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)

                    /*
                     * FreeBSD, except the most recent versions,
                     * could not remove accept filter
                     */
                    nls[n].deferred_accept = ls[i].deferred_accept;

                    if (ls[i].accept_filter && nls[n].accept_filter) {
                        if (ngx_strcmp(ls[i].accept_filter,
                                       nls[n].accept_filter)
                            != 0)
                        {
                            nls[n].delete_deferred = 1;
                            nls[n].add_deferred = 1;
                        }

                    } else if (ls[i].accept_filter) {
                        nls[n].delete_deferred = 1;

                    } else if (nls[n].accept_filter) {
                        nls[n].add_deferred = 1;
                    }
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)

                    if (ls[i].deferred_accept && !nls[n].deferred_accept) {
                        nls[n].delete_deferred = 1;

                    } else if (ls[i].deferred_accept != nls[n].deferred_accept)
                    {
                        nls[n].add_deferred = 1;
                    }
#endif

#if (NGX_HAVE_REUSEPORT)
                    if (nls[n].reuseport && !ls[i].reuseport) {
                        nls[n].add_reuseport = 1;
                    }
#endif

                    break;
                }
            }

            if (nls[n].fd == (ngx_socket_t) -1) {
                nls[n].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
                if (nls[n].accept_filter) {
                    nls[n].add_deferred = 1;
                }
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
                if (nls[n].deferred_accept) {
                    nls[n].add_deferred = 1;
                }
#endif
            }
        }

    } else {
        ls = cycle->listening.elts;
        for (i = 0; i < cycle->listening.nelts; i++) {
            ls[i].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
            if (ls[i].accept_filter) {
                ls[i].add_deferred = 1;
            }
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
            if (ls[i].deferred_accept) {
                ls[i].add_deferred = 1;
            }
#endif
        }
    }

    if (ngx_open_listening_sockets(cycle) != NGX_OK) {
        goto failed;
    }

    if (!ngx_test_config) {
        ngx_configure_listening_sockets(cycle);
    }


    /* commit the new cycle configuration */

    if (!ngx_use_stderr) {
        (void) ngx_log_redirect_stderr(cycle);
    }

    pool->log = cycle->log;

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->init_module) {
            if (ngx_modules[i]->init_module(cycle) != NGX_OK) {
                /* fatal */
                exit(1);
            }
        }
    }


    /* close and delete stuff that lefts from an old cycle */

    /* free the unnecessary shared memory */

    opart = &old_cycle->shared_memory.part;
    oshm_zone = opart->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= opart->nelts) {
            if (opart->next == NULL) {
                goto old_shm_zone_done;
            }
            opart = opart->next;
            oshm_zone = opart->elts;
            i = 0;
        }

        part = &cycle->shared_memory.part;
        shm_zone = part->elts;

        for (n = 0; /* void */ ; n++) {

            if (n >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }
                part = part->next;
                shm_zone = part->elts;
                n = 0;
            }

            if (oshm_zone[i].shm.name.len == shm_zone[n].shm.name.len
                && ngx_strncmp(oshm_zone[i].shm.name.data,
                               shm_zone[n].shm.name.data,
                               oshm_zone[i].shm.name.len)
                == 0)
            {
                goto live_shm_zone;
            }
        }

        ngx_shm_free(&oshm_zone[i].shm);

    live_shm_zone:

        continue;
    }

old_shm_zone_done:


    /* close the unnecessary listening sockets */

    ls = old_cycle->listening.elts;
    for (i = 0; i < old_cycle->listening.nelts; i++) {

        if (ls[i].remain || ls[i].fd == (ngx_socket_t) -1) {
            continue;
        }

        if (ngx_close_socket(ls[i].fd) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                          ngx_close_socket_n " listening socket on %V failed",
                          &ls[i].addr_text);
        }

#if (NGX_HAVE_UNIX_DOMAIN)

        if (ls[i].sockaddr->sa_family == AF_UNIX) {
            u_char  *name;

            name = ls[i].addr_text.data + sizeof("unix:") - 1;

            ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                          "deleting socket %s", name);

            if (ngx_delete_file(name) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno,
                              ngx_delete_file_n " %s failed", name);
            }
        }

#endif
    }


    /* close the unnecessary open files */

    part = &old_cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].fd == NGX_INVALID_FILE || file[i].fd == ngx_stderr) {
            continue;
        }

        if (ngx_close_file(file[i].fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }
    }

    ngx_destroy_pool(conf.temp_pool);
//����ǵ�һ�μ��أ�������ngx_is_init_cycle�������reload����������ԭ����nginx���̵�ngx_process == NGX_PROCESS_MASTER    
    if (ngx_process == NGX_PROCESS_MASTER || ngx_is_init_cycle(old_cycle)) { 

        /*
         * perl_destruct() frees environ, if it is not the same as it was at
         * perl_construct() time, therefore we save the previous cycle
         * environment before ngx_conf_parse() where it will be changed.
         */
        env = environ;
        environ = senv;

        ngx_destroy_pool(old_cycle->pool);
        cycle->old_cycle = NULL;

        environ = env;

        return cycle;
    }


    if (ngx_temp_pool == NULL) {
        ngx_temp_pool = ngx_create_pool(128, cycle->log);
        if (ngx_temp_pool == NULL) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                          "could not create ngx_temp_pool");
            exit(1);
        }

        n = 10;
        ngx_old_cycles.elts = ngx_pcalloc(ngx_temp_pool,
                                          n * sizeof(ngx_cycle_t *));
        if (ngx_old_cycles.elts == NULL) {
            exit(1);
        }
        ngx_old_cycles.nelts = 0;
        ngx_old_cycles.size = sizeof(ngx_cycle_t *);
        ngx_old_cycles.nalloc = n;
        ngx_old_cycles.pool = ngx_temp_pool;

        ngx_cleaner_event.handler = ngx_clean_old_cycles;
        ngx_cleaner_event.log = cycle->log;
        ngx_cleaner_event.data = &dumb;
        dumb.fd = (ngx_socket_t) -1;
    }

    ngx_temp_pool->log = cycle->log;

    old = ngx_array_push(&ngx_old_cycles);
    if (old == NULL) {
        exit(1);
    }
    *old = old_cycle;

    if (!ngx_cleaner_event.timer_set) {
        ngx_add_timer(&ngx_cleaner_event, 30000, NGX_FUNC_LINE);
        ngx_cleaner_event.timer_set = 1;
    }

    return cycle;


failed:

    if (!ngx_is_init_cycle(old_cycle)) {
        old_ccf = (ngx_core_conf_t *) ngx_get_conf(old_cycle->conf_ctx,
                                                   ngx_core_module);
        if (old_ccf->environment) {
            environ = old_ccf->environment;
        }
    }

    /* rollback the new cycle configuration */

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].fd == NGX_INVALID_FILE || file[i].fd == ngx_stderr) {
            continue;
        }

        if (ngx_close_file(file[i].fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }
    }

    if (ngx_test_config) {
        ngx_destroy_cycle_pools(&conf);
        return NULL;
    }

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
        if (ls[i].fd == (ngx_socket_t) -1 || !ls[i].open) {
            continue;
        }

        if (ngx_close_socket(ls[i].fd) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                          ngx_close_socket_n " %V failed",
                          &ls[i].addr_text);
        }
    }

    ngx_destroy_cycle_pools(&conf);

    return NULL;
}


static void
ngx_destroy_cycle_pools(ngx_conf_t *conf)
{
    ngx_destroy_pool(conf->temp_pool);
    ngx_destroy_pool(conf->pool);
}


static ngx_int_t
ngx_init_zone_pool(ngx_cycle_t *cycle, ngx_shm_zone_t *zn)
{
    u_char           *file;
    ngx_slab_pool_t  *sp;

    //�����ڴ����ʼ��ַ��ʼ��sizeof(ngx_slab_pool_t)�ֽ��������洢�������ڴ��slab poll��
    sp = (ngx_slab_pool_t *) zn->shm.addr; //�����ڴ���ʼ��ַ

    if (zn->shm.exists) {

        if (sp == sp->addr) {
            return NGX_OK;
        }

#if (NGX_WIN32)

        /* remap at the required address */

        if (ngx_shm_remap(&zn->shm, sp->addr) != NGX_OK) {
            return NGX_ERROR;
        }

        sp = (ngx_slab_pool_t *) zn->shm.addr;

        if (sp == sp->addr) {
            return NGX_OK;
        }

#endif

        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "shared zone \"%V\" has no equal addresses: %p vs %p",
                      &zn->shm.name, sp->addr, sp);
        return NGX_ERROR;
    }

    sp->end = zn->shm.addr + zn->shm.size;
    sp->min_shift = 3;
    sp->addr = zn->shm.addr;

#if (NGX_HAVE_ATOMIC_OPS)

    file = NULL;

#else

    file = ngx_pnalloc(cycle->pool, cycle->lock_file.len + zn->shm.name.len);
    if (file == NULL) {
        return NGX_ERROR;
    }

    (void) ngx_sprintf(file, "%V%V%Z", &cycle->lock_file, &zn->shm.name);

#endif

    //���������ڴ���
    if (ngx_shmtx_create(&sp->mutex, &sp->lock, file) != NGX_OK) {
        return NGX_ERROR;
    }

    ngx_slab_init(sp);

    return NGX_OK;
}


ngx_int_t
ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log)
{
    size_t      len;
    ngx_uint_t  create;
    ngx_file_t  file;
    u_char      pid[NGX_INT64_LEN + 2];

    if (ngx_process > NGX_PROCESS_MASTER) {
        return NGX_OK;
    }

    ngx_memzero(&file, sizeof(ngx_file_t));

    file.name = *name;
    file.log = log;

    create = ngx_test_config ? NGX_FILE_CREATE_OR_OPEN : NGX_FILE_TRUNCATE;

    file.fd = ngx_open_file(file.name.data, NGX_FILE_RDWR,
                            create, NGX_FILE_DEFAULT_ACCESS);

    if (file.fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", file.name.data);
        return NGX_ERROR;
    }

    if (!ngx_test_config) {
        len = ngx_snprintf(pid, NGX_INT64_LEN + 2, "%P%N", ngx_pid) - pid;

        if (ngx_write_file(&file, pid, len, 0) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    if (ngx_close_file(file.fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", file.name.data);
    }

    return NGX_OK;
}


void
ngx_delete_pidfile(ngx_cycle_t *cycle)
{
    u_char           *name;
    ngx_core_conf_t  *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    name = ngx_new_binary ? ccf->oldpid.data : ccf->pid.data;

    if (ngx_delete_file(name) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      ngx_delete_file_n " \"%s\" failed", name);
    }
}

/*
�ú������ã�
��ȡngx_core_moduleģ������ýṹngx_core_conf_t��
�������ýṹ�ҵ��乤�������ļ�����"/usr/local/nginx/logs/nginx.pid"(���ļ�����nginx����ID����pid)��
�򿪸��ļ�����ȡpid��
����ngx_os_signal_process()�����źţ�
*/
ngx_int_t
ngx_signal_process(ngx_cycle_t *cycle, char *sig)
{
    ssize_t           n;
    ngx_int_t         pid;
    ngx_file_t        file;
    ngx_core_conf_t  *ccf;
    u_char            buf[NGX_INT64_LEN + 2];

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "signal process started");

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    ngx_memzero(&file, sizeof(ngx_file_t));

    //�򿪴��master���̵��ļ�NGX_PID_PATH
    file.name = ccf->pid;
    file.log = cycle->log;

    file.fd = ngx_open_file(file.name.data, NGX_FILE_RDONLY,
                            NGX_FILE_OPEN, NGX_FILE_DEFAULT_ACCESS);

    if (file.fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, cycle->log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", file.name.data);
        return 1;
    }

    n = ngx_read_file(&file, buf, NGX_INT64_LEN + 2, 0);

    if (ngx_close_file(file.fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", file.name.data);
    }

    if (n == NGX_ERROR) {
        return 1;
    }

    while (n-- && (buf[n] == CR || buf[n] == LF)) { /* void */ }

    pid = ngx_atoi(buf, ++n); //master����id

    if (pid == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                      "invalid PID number \"%*s\" in \"%s\"",
                      n, buf, file.name.data);
        return 1;
    }

    return ngx_os_signal_process(cycle, sig, pid);

}


static ngx_int_t
ngx_test_lockfile(u_char *file, ngx_log_t *log)
{
#if !(NGX_HAVE_ATOMIC_OPS)
    ngx_fd_t  fd;

    fd = ngx_open_file(file, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);

    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", file);
        return NGX_ERROR;
    }

    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", file);
    }

    if (ngx_delete_file(file) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_delete_file_n " \"%s\" failed", file);
    }

#endif

    return NGX_OK;
}

//ngx_reopen��־λ�����Ϊ1�����ʾ��Ҫ���´������ļ�
void
ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user)
{
    ngx_fd_t          fd;
    ngx_uint_t        i;
    ngx_list_part_t  *part;
    ngx_open_file_t  *file;

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].name.len == 0) {
            continue;
        }

        if (file[i].flush) {
            file[i].flush(&file[i], cycle->log);
        }

        fd = ngx_open_file(file[i].name.data, NGX_FILE_APPEND,
                           NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "reopen file \"%s\", old:%d new:%d",
                       file[i].name.data, file[i].fd, fd);

        if (fd == NGX_INVALID_FILE) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          ngx_open_file_n " \"%s\" failed", file[i].name.data);
            continue;
        }

#if !(NGX_WIN32)
        if (user != (ngx_uid_t) NGX_CONF_UNSET_UINT) {
            ngx_file_info_t  fi;

            if (ngx_file_info((const char *) file[i].name.data, &fi)
                == NGX_FILE_ERROR)
            {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                              ngx_file_info_n " \"%s\" failed",
                              file[i].name.data);

                if (ngx_close_file(fd) == NGX_FILE_ERROR) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                                  ngx_close_file_n " \"%s\" failed",
                                  file[i].name.data);
                }

                continue;
            }

            if (fi.st_uid != user) {
                if (chown((const char *) file[i].name.data, user, -1) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                                  "chown(\"%s\", %d) failed",
                                  file[i].name.data, user);

                    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
                        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                                      ngx_close_file_n " \"%s\" failed",
                                      file[i].name.data);
                    }

                    continue;
                }
            }

            if ((fi.st_mode & (S_IRUSR|S_IWUSR)) != (S_IRUSR|S_IWUSR)) {

                fi.st_mode |= (S_IRUSR|S_IWUSR);

                if (chmod((const char *) file[i].name.data, fi.st_mode) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                                  "chmod() \"%s\" failed", file[i].name.data);

                    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
                        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                                      ngx_close_file_n " \"%s\" failed",
                                      file[i].name.data);
                    }

                    continue;
                }
            }
        }

        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) \"%s\" failed",
                          file[i].name.data);

            if (ngx_close_file(fd) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                              ngx_close_file_n " \"%s\" failed",
                              file[i].name.data);
            }

            continue;
        }
#endif

        if (ngx_close_file(file[i].fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }

        file[i].fd = fd;
    }

    (void) ngx_log_redirect_stderr(cycle);
}

//�ȴ�������ṹ�������洢�����ļ���ָ������Ҫ�Ĺ����ڴ�ռ���Ϣ��cycle->shared_memory��Ȼ����ngx_init_cycle�н����������ļ��󣬰������ļ���
//ָ������Ҫ�����ڴ���Ϣ��cycle->shared_memory����ȡ��������ngx_init_cycle�����������ļ���ͳһ���й����ڴ���䴴��
ngx_shm_zone_t *
ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name, size_t size, void *tag)
{
    ngx_uint_t        i;
    ngx_shm_zone_t   *shm_zone;
    ngx_list_part_t  *part;

    part = &cf->cycle->shared_memory.part;
    shm_zone = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            shm_zone = part->elts;
            i = 0;
        }

        if (name->len != shm_zone[i].shm.name.len) {
            continue;
        }

        if (ngx_strncmp(name->data, shm_zone[i].shm.name.data, name->len)
            != 0)
        {
            continue;
        }

        if (tag != shm_zone[i].tag) { //����proxy_cache abc, fastcgi abc��ͬʱ���ã��ͻᱨ��
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                            "the shared memory zone \"%V\" is "
                            "already declared for a different use",
                            &shm_zone[i].shm.name);
            return NULL;
        }

        if (shm_zone[i].shm.size == 0) {//������ͬ����С��һ�£�����  �����ڸ�����֮ǰ������proxy_cache xxx����xxx�����ڴ�Ĵ�С����0
            shm_zone[i].shm.size = size;
        }

        if (size && size != shm_zone[i].shm.size) {//֮ǰ�Ѿ�����name�����ڴ棬����������name��Ҫ�����Ĺ����ڴ��С��֮ǰ�Ĳ���ȣ���ͻ��
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                            "the size %uz of shared memory zone \"%V\" "
                            "conflicts with already declared size %uz",
                            size, &shm_zone[i].shm.name, shm_zone[i].shm.size);
            return NULL;
        }

        //�ҵ���һ����ͬ���ֵģ���������֪���Ĺ����ڴ��С��֮ǰ��һ������ֱ��ʹ�øù����ڴ棬��������cf->cycle->shared_memory.part�е��±ꡣ
        return &shm_zone[i];
    }

    /* ��Ҫ����һ���µ�ngx_shm_zone_t����ngx_init_cycle���������������ڴ� */
    shm_zone = ngx_list_push(&cf->cycle->shared_memory);

    if (shm_zone == NULL) {
        return NULL;
    }

    shm_zone->data = NULL;
    shm_zone->shm.log = cf->cycle->log;
    shm_zone->shm.size = size;
    shm_zone->shm.name = *name;
    shm_zone->shm.exists = 0;
    shm_zone->init = NULL;
    shm_zone->tag = tag;
    shm_zone->noreuse = 0;

    return shm_zone;
}


static void
ngx_clean_old_cycles(ngx_event_t *ev)
{
    ngx_uint_t     i, n, found, live;
    ngx_log_t     *log;
    ngx_cycle_t  **cycle;

    log = ngx_cycle->log;
    ngx_temp_pool->log = log;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, log, 0, "clean old cycles");

    live = 0;

    cycle = ngx_old_cycles.elts;
    for (i = 0; i < ngx_old_cycles.nelts; i++) {

        if (cycle[i] == NULL) {
            continue;
        }

        found = 0;

        for (n = 0; n < cycle[i]->connection_n; n++) {
            if (cycle[i]->connections[n].fd != (ngx_socket_t) -1) {
                found = 1;

                ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "live fd:%d", n);

                break;
            }
        }

        if (found) {
            live = 1;
            continue;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "clean old cycle: %d", i);

        ngx_destroy_pool(cycle[i]->pool);
        cycle[i] = NULL;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "old cycles status: %d", live);

    if (live) {
        ngx_add_timer(ev, 30000, NGX_FUNC_LINE);

    } else {
        ngx_destroy_pool(ngx_temp_pool);
        ngx_temp_pool = NULL;
        ngx_old_cycles.nelts = 0;
    }
}
