
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


/*
 * open file cache caches
 *    open file handles with stat() info;
 *    directories stat() info;
 *    files and directories errors: not found, access denied, etc.
 */


#define NGX_MIN_READ_AHEAD  (128 * 1024)


static void ngx_open_file_cache_cleanup(void *data);
#if (NGX_HAVE_OPENAT)
static ngx_fd_t ngx_openat_file_owner(ngx_fd_t at_fd, const u_char *name,
    ngx_int_t mode, ngx_int_t create, ngx_int_t access, ngx_log_t *log);
#if (NGX_HAVE_O_PATH)
static ngx_int_t ngx_file_o_path_info(ngx_fd_t fd, ngx_file_info_t *fi,
    ngx_log_t *log);
#endif
#endif
static ngx_fd_t ngx_open_file_wrapper(ngx_str_t *name,
    ngx_open_file_info_t *of, ngx_int_t mode, ngx_int_t create,
    ngx_int_t access, ngx_log_t *log);
static ngx_int_t ngx_file_info_wrapper(ngx_str_t *name,
    ngx_open_file_info_t *of, ngx_file_info_t *fi, ngx_log_t *log);
static ngx_int_t ngx_open_and_stat_file(ngx_str_t *name,
    ngx_open_file_info_t *of, ngx_log_t *log);
static void ngx_open_file_add_event(ngx_open_file_cache_t *cache,
    ngx_cached_open_file_t *file, ngx_open_file_info_t *of, ngx_log_t *log);
static void ngx_open_file_cleanup(void *data);
static void ngx_close_cached_file(ngx_open_file_cache_t *cache,
    ngx_cached_open_file_t *file, ngx_uint_t min_uses, ngx_log_t *log);
static void ngx_open_file_del_event(ngx_cached_open_file_t *file);
static void ngx_expire_old_cached_files(ngx_open_file_cache_t *cache,
    ngx_uint_t n, ngx_log_t *log);
static void ngx_open_file_cache_rbtree_insert_value(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel);
static ngx_cached_open_file_t *
    ngx_open_file_lookup(ngx_open_file_cache_t *cache, ngx_str_t *name,
    uint32_t hash);
static void ngx_open_file_cache_remove(ngx_event_t *ev);


ngx_open_file_cache_t *
ngx_open_file_cache_init(ngx_pool_t *pool, ngx_uint_t max, time_t inactive)
{
    ngx_pool_cleanup_t     *cln;
    ngx_open_file_cache_t  *cache;

    cache = ngx_palloc(pool, sizeof(ngx_open_file_cache_t));
    if (cache == NULL) {
        return NULL;
    }

    ngx_rbtree_init(&cache->rbtree, &cache->sentinel,
                    ngx_open_file_cache_rbtree_insert_value);

    ngx_queue_init(&cache->expire_queue);

    cache->current = 0;
    cache->max = max;
    cache->inactive = inactive;

    cln = ngx_pool_cleanup_add(pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_open_file_cache_cleanup;
    cln->data = cache;

    return cache;
}

//����ȥ�����ںͶ��е���Ϣ�����ν���ngx_close_cached_file�ĵ������ر��ļ���
static void
ngx_open_file_cache_cleanup(void *data)
{
    ngx_open_file_cache_t  *cache = data;

    ngx_queue_t             *q;
    ngx_cached_open_file_t  *file;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "open file cache cleanup");

    for ( ;; ) {

        if (ngx_queue_empty(&cache->expire_queue)) {
            break;
        }

        q = ngx_queue_last(&cache->expire_queue);

        file = ngx_queue_data(q, ngx_cached_open_file_t, queue);

        ngx_queue_remove(q);

        ngx_rbtree_delete(&cache->rbtree, &file->node);

        cache->current--;

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                       "delete cached open file: %s", file->name);

        if (!file->err && !file->is_dir) {
            file->close = 1;
            file->count = 0;
            ngx_close_cached_file(cache, file, 0, ngx_cycle->log);

        } else {
            ngx_free(file->name);
            ngx_free(file);
        }
    }

    if (cache->current) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                      "%ui items still leave in open file cache",
                      cache->current);
    }

    if (cache->rbtree.root != cache->rbtree.sentinel) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                      "rbtree still is not empty in open file cache");

    }
}

/*ͨ����name��hash�ں�����в����Ƿ��и�ngx_cached_open_file_s�ڵ㣬û���򴴽���Ӧ�Ľڵ㣬����NGX_OK������Ѿ����������ļ�
���������Ժ�֮ǰ������д洢���ļ������Ƿ����仯�������仯����¸�������name�ļ�����,����NGX_OK*/

//�Ի���KEY�ķ�����Ҫ���жϻ����key�Ƿ��б仯���߹��ڡ������ҪupdateȻ����ת��found������ֱ��ת��found��
ngx_int_t
ngx_open_cached_file(ngx_open_file_cache_t *cache, ngx_str_t *name,
    ngx_open_file_info_t *of, ngx_pool_t *pool) //����и��ļ�name���ڣ�����ȡname�ļ�������stat���ԣ�����NGX_OK�����򷵻�NGX_ERROR
{
    time_t                          now;
    uint32_t                        hash;
    ngx_int_t                       rc;
    ngx_file_info_t                 fi;
    ngx_pool_cleanup_t             *cln;
    ngx_cached_open_file_t         *file;
    ngx_pool_cleanup_file_t        *clnf;
    ngx_open_file_cache_cleanup_t  *ofcln;

    of->fd = NGX_INVALID_FILE;
    of->err = 0;

    if (cache == NULL) { // ���cache�ṹû�б���ʼ���� ���ȡname�ļ�stat��Ϣ��
    /* 
    ���û������open_file_cache max=1000 inactive=20s;��Ҳ����˵û�л���cache�����ļ���Ӧ���ļ�stat��Ϣ����ÿ�ζ�Ҫ���´��ļ���ȡ�ļ�stat��Ϣ��
    ���������open_file_cache�����Ѵ򿪵�cache�����ļ�stat��Ϣ����ngx_crc32_long��hash����ӵ�ngx_cached_open_file_t->rbtree�У������´��������
    uri����Ͳ����ٴ�open�ļ�����stat��ȡ�ļ������ˣ������������Ч��,�ο�ngx_open_cached_file 
    */ 
        if (of->test_only) {//���ֻ�ǲ�����  �������index module��ʱ�򣬾�������

            if (ngx_file_info_wrapper(name, of, &fi, pool->log) //�Ը��ļ����ļ���Ϣ���в�ѯ�ͷ��أ�����ʵ�ʴ���
                == NGX_FILE_ERROR)
            {
                return NGX_ERROR;
            }

            of->uniq = ngx_file_uniq(&fi);
            of->mtime = ngx_file_mtime(&fi);
            of->size = ngx_file_size(&fi);
            of->fs_size = ngx_file_fs_size(&fi);
            of->is_dir = ngx_is_dir(&fi);
            of->is_file = ngx_is_file(&fi);
            of->is_link = ngx_is_link(&fi);
            of->is_exec = ngx_is_exec(&fi);

            return NGX_OK;
        }
        //ֱ�Ӵ�����ļ��������ûص������ڴ���ͷ�ʱ�رո��ļ�
        cln = ngx_pool_cleanup_add(pool, sizeof(ngx_pool_cleanup_file_t));
        if (cln == NULL) {
            return NGX_ERROR;
        }

        ////��ȡname�ļ������ngx_open_file_info_t��Ϣ,Ҳ���ǻ�ȡ�ļ�������Ϣ
        rc = ngx_open_and_stat_file(name, of, pool->log); //����ͨ��stat������ȡstat��Ϣ

        if (rc == NGX_OK && !of->is_dir) {
            cln->handler = ngx_pool_cleanup_file;
            clnf = cln->data; //ָ��ǰ���sizeof(ngx_pool_cleanup_file_t)�ռ䣬��ngx_pool_cleanup_add

            clnf->fd = of->fd;
            clnf->name = name->data;
            clnf->log = pool->log;
        }

        return rc;
    }

    cln = ngx_pool_cleanup_add(pool, sizeof(ngx_open_file_cache_cleanup_t));
    if (cln == NULL) {
        return NGX_ERROR;
    }

    now = ngx_time();

    hash = ngx_crc32_long(name->data, name->len);//�ļ�����hash

    file = ngx_open_file_lookup(cache, name, hash); //��hash�в��ң�����û�и��ļ���stat��Ϣ

    if (file) { //�ҵ�����������и��ļ�
        //�ҵ���������ļ���

        file->uses++;
        //������ɾ������ļ��������ջ����²�����У�����������ʵ��ڶ���ͷ
        ngx_queue_remove(&file->queue);

        if (file->fd == NGX_INVALID_FILE && file->err == 0 && !file->is_dir) {

            /* file was not used often enough to keep open */
            rc = ngx_open_and_stat_file(name, of, pool->log); //�򿪸��ļ���������Ϣ

            if (rc != NGX_OK && (of->err == 0 || !of->errors)) {
                goto failed;
            }
            
            //�����¼������ļ��������仯
            goto add_event;
        }


        /*
          ����ʹ�������ֻ��ƣ������ֻ����ǻ���ġ�һ�����ļ��¼������ƣ���kqueue�²��еġ�
          һ���Ƕ�ʱ�����ƣ�now - file->created < of->valid��
          �����ʱ���û�����⣬���ofû��uniqֵ��ô������ͨ���ˣ�����Ա�uniqֵ
          ���ֵ�����ļ������е�st_ino(ͬһ���豸�е�ÿ���ļ������ֵ���ǲ�ͬ�ģ���
          ���ֵ��Ҫ�����ж��ļ��Ƿ��޸�(��������޸��Ǹ�������ģ��������open�򿪣�Ȼ��д��Ļ������ֵ����һ����)
          */
        if (file->use_event //use_eventֻ��kqueue����Ч
            || (file->event == NULL
                && (of->uniq == 0 || of->uniq == file->uniq) 
                //����ļ���Ϣuniqû�з����仯�����Ҹýڵ㻹û��ʧЧ����ֱ�Ӵ�ԭ���ĺ������ȡ��������of,�Ӷ����Ч��
                && now - file->created < of->valid //˵��û�й��� of.valid = clcf->open_file_cache_valid;   open_file_cache_valid 60s��������Ч
#if (NGX_HAVE_OPENAT)
                && of->disable_symlinks == file->disable_symlinks
                && of->disable_symlinks_from == file->disable_symlinks_from
#endif
            ))
        {
            if (file->err == 0) {
                //û�����ֱ�ӵ�found������ҵ��Ĳ�����   ֱ�Ӵӻ����п�������

                of->fd = file->fd;
                of->uniq = file->uniq;
                of->mtime = file->mtime;
                of->size = file->size;

                of->is_dir = file->is_dir;
                of->is_file = file->is_file;
                of->is_link = file->is_link;
                of->is_exec = file->is_exec;
                of->is_directio = file->is_directio;

                if (!file->is_dir) {
                    //��������ļ��¼����
                    file->count++;
                    ngx_open_file_add_event(cache, file, of, pool->log);
                }

            } else {
                of->err = file->err;
#if (NGX_HAVE_OPENAT)
                of->failed = file->disable_symlinks ? ngx_openat_file_n
                                                    : ngx_open_file_n;
#else
                of->failed = ngx_open_file_n;
#endif
            }
            ngx_log_debugall(pool->log, 0, "ngx open cache file, direct update stat info, not open file and exec stat()");
            goto found;
        }

        ngx_log_debug4(NGX_LOG_DEBUG_CORE, pool->log, 0,
                       "retest open file: %s, fd:%d, c:%d, e:%d",
                       file->name, file->fd, file->count, file->err);

        if (file->is_dir) {
            //�ļ����ı��ˣ������ǵ�����

            /*
             * chances that directory became file are very small
             * so test_dir flag allows to use a single syscall
             * in ngx_file_info() instead of three syscalls
             */

            of->test_dir = 1;
        }
        
        //���´򿪼���of��Ϣ
        of->fd = file->fd;
        of->uniq = file->uniq;

        rc = ngx_open_and_stat_file(name, of, pool->log);//��ȡ�ļ����µ����ԣ�file����֮ǰ�����������е�����

        if (rc != NGX_OK && (of->err == 0 || !of->errors)) {
            goto failed;
        }

        /*
            ������Ҫ�Ǽ���ļ������Ƿ��ͱ仯���������:
            ����name��Ŀ¼������֮ǰ���ļ���Ҳ�����ļ���Ŀ¼�ˣ������仯�ˡ�
            Ŀ¼���ļ������Ҽ��uniq�Ƿ��ͱ仯
          */

        if (of->is_dir) {
            //���ļ�ǰ��״̬�Աȵļ��

            if (file->is_dir || file->err) {
                //Ŀ¼����Ŀ¼��ֱ��update,found
                goto update;
            }
            //�ļ����Ŀ¼����鲻ͨ��

            /* file became directory */

        } else if (of->err == 0) {  /* file */ 
            //�ļ����Ŀ¼����������¼����м�飬����update,found 
            if (file->is_dir || file->err) {
                goto add_event;
            }
            //�ļ���uniqֵδ�����仯������update,found
            if (of->uniq == file->uniq) {

                if (file->event) {
                    file->use_event = 1;
                }

                of->is_directio = file->is_directio;

                goto update;
            }
            //�ļ��仯�ˣ���鲻ͨ��
            /* file was changed */

        } else { /* error to cache */
            //�ļ������˴��������ǰҲ�Ǵ�����ôupdate,found
            if (file->err || file->is_dir) {
                goto update;
            }
            //�ļ���ǰû�д���˵���ļ���ɾ���ˣ���ô��鲻ͨ��

            /* file was removed, etc. */
        }

        //�ļ������Ѿ������仯
        
        //��鲻ͨ���������ü���Ϊ0����ô�ر��ļ����Ҽ����¼�������Ȼ��update,found

        if (file->count == 0) {

            ngx_open_file_del_event(file);

            if (ngx_close_file(file->fd) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_ALERT, pool->log, ngx_errno,
                              ngx_close_file_n " \"%V\" failed", name);
            }

            goto add_event;
        }
        
//���ü�����Ϊ0����ô�ں������ɾ������ڵ㣬��cache������Ŀ��һ�����ļ�����close��ǣ�Ȼ����ǰ�ĵ�no found������������ӵ��ļ�cache�У�
        ngx_rbtree_delete(&cache->rbtree, &file->node);//ע������û�дӹ��ڶ���ɾ����file�ڵ㣬�����ڹ��ں��Ǻ�ӹ��ڶ���expire_queue��ɾ��

        cache->current--;

        file->close = 1;

        goto create; //Ϊʲô��Ҫ���´�����������ֱ�Ӹ�����?
    }

    /* not found */
    //��ȡname�ļ���Ӧ��stat������Ϣ
    rc = ngx_open_and_stat_file(name, of, pool->log);

    if (rc != NGX_OK && (of->err == 0 || !of->errors)) {
        goto failed;
    }

create:

    if (cache->current >= cache->max) { //������нڵ���������ˣ�ɾ�����ϵ�node�ڵ�
        ngx_expire_old_cached_files(cache, 0, pool->log);
    }

    file = ngx_alloc(sizeof(ngx_cached_open_file_t), pool->log);

    if (file == NULL) {
        goto failed;
    }

    file->name = ngx_alloc(name->len + 1, pool->log);

    if (file->name == NULL) {
        ngx_free(file);
        file = NULL;
        goto failed;
    }

    ngx_cpystrn(file->name, name->data, name->len + 1);

    file->node.key = hash;//�ļ�������hash

    ngx_rbtree_insert(&cache->rbtree, &file->node); //�ļ���Ϣ����������������

    cache->current++; //�ļ���������

    file->uses = 1;
    file->count = 0;
    file->use_event = 0;
    file->event = NULL;

add_event:

    ngx_open_file_add_event(cache, file, of, pool->log);

update: //����name���µ��ļ�����

    file->fd = of->fd; //�����ļ���Ϣ
    file->err = of->err;
#if (NGX_HAVE_OPENAT)
    file->disable_symlinks = of->disable_symlinks;
    file->disable_symlinks_from = of->disable_symlinks_from;
#endif
    //�ɹ��򿪾ͽ�����Ϣ����
    if (of->err == 0) {
        file->uniq = of->uniq;
        file->mtime = of->mtime;
        file->size = of->size;

        file->close = 0;

        file->is_dir = of->is_dir;
        file->is_file = of->is_file;
        file->is_link = of->is_link;
        file->is_exec = of->is_exec;
        file->is_directio = of->is_directio;

        if (!of->is_dir) {
            file->count++; //˵��name���ļ�
        }
    }
    
    //���´���ʱ��
    file->created = now;

found:
    //���·���ʱ��
    file->accessed = now;
    
    //������ڶ���
    ngx_queue_insert_head(&cache->expire_queue, &file->queue);

    ngx_log_debug5(NGX_LOG_DEBUG_CORE, pool->log, 0,
                   "cached open file: %s, fd:%d, c:%d, e:%d, u:%d",
                   file->name, file->fd, file->count, file->err, file->uses);

    if (of->err == 0) {
        
        //�趨�������ٻص�
        if (!of->is_dir) {//�������Ŀ¼���ļ�������Ҫ����ļ���cleanup
            //ͨ��ǰ���ngx_pool_cleanup_add��ӵ�pool->cleanup��
            cln->handler = ngx_open_file_cleanup;
            ofcln = cln->data; //ָ��ǰ��cln = ngx_pool_cleanup_add(pool, sizeof(ngx_open_file_cache_cleanup_t));�п��ٵĿռ�ngx_open_file_cache_cleanup_t

            //cln->dataָ��ngx_open_file_cache_cleanup_t������ֵ
            ofcln->cache = cache;
            ofcln->file = file;
            ofcln->min_uses = of->min_uses;
            ofcln->log = pool->log;
        }

        return NGX_OK;
    }

    return NGX_ERROR;

failed:

    if (file) {
        ngx_rbtree_delete(&cache->rbtree, &file->node);

        cache->current--;

        if (file->count == 0) {

            if (file->fd != NGX_INVALID_FILE) {
                if (ngx_close_file(file->fd) == NGX_FILE_ERROR) {
                    ngx_log_error(NGX_LOG_ALERT, pool->log, ngx_errno,
                                  ngx_close_file_n " \"%s\" failed",
                                  file->name);
                }
            }

            ngx_free(file->name);
            ngx_free(file);

        } else {
            file->close = 1;
        }
    }

    if (of->fd != NGX_INVALID_FILE) {
        if (ngx_close_file(of->fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, pool->log, ngx_errno,
                          ngx_close_file_n " \"%V\" failed", name);
        }
    }

    return NGX_ERROR;
}


#if (NGX_HAVE_OPENAT)

static ngx_fd_t
ngx_openat_file_owner(ngx_fd_t at_fd, const u_char *name,
    ngx_int_t mode, ngx_int_t create, ngx_int_t access, ngx_log_t *log)
{
    ngx_fd_t         fd;
    ngx_err_t        err;
    ngx_file_info_t  fi, atfi;

    /*
     * To allow symlinks with the same owner, use openat() (followed
     * by fstat()) and fstatat(AT_SYMLINK_NOFOLLOW), and then compare
     * uids between fstat() and fstatat().
     *
     * As there is a race between openat() and fstatat() we don't
     * know if openat() in fact opened symlink or not.  Therefore,
     * we have to compare uids even if fstatat() reports the opened
     * component isn't a symlink (as we don't know whether it was
     * symlink during openat() or not).
     */

    fd = ngx_openat_file(at_fd, name, mode, create, access);

    if (fd == NGX_INVALID_FILE) {
        return NGX_INVALID_FILE;
    }

    if (ngx_file_at_info(at_fd, name, &atfi, AT_SYMLINK_NOFOLLOW)
        == NGX_FILE_ERROR)
    {
        err = ngx_errno;
        goto failed;
    }

#if (NGX_HAVE_O_PATH)
    if (ngx_file_o_path_info(fd, &fi, log) == NGX_ERROR) {
        err = ngx_errno;
        goto failed;
    }
#else
    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        err = ngx_errno;
        goto failed;
    }
#endif

    if (fi.st_uid != atfi.st_uid) {
        err = NGX_ELOOP;
        goto failed;
    }

    return fd;

failed:

    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_close_file_n " \"%V\" failed", name);
    }

    ngx_set_errno(err);

    return NGX_INVALID_FILE;
}


#if (NGX_HAVE_O_PATH)

static ngx_int_t
ngx_file_o_path_info(ngx_fd_t fd, ngx_file_info_t *fi, ngx_log_t *log)
{
    static ngx_uint_t  use_fstat = 1;

    /*
     * In Linux 2.6.39 the O_PATH flag was introduced that allows to obtain
     * a descriptor without actually opening file or directory.  It requires
     * less permissions for path components, but till Linux 3.6 fstat() returns
     * EBADF on such descriptors, and fstatat() with the AT_EMPTY_PATH flag
     * should be used instead.
     *
     * Three scenarios are handled in this function:
     *
     * 1) The kernel is newer than 3.6 or fstat() with O_PATH support was
     *    backported by vendor.  Then fstat() is used.
     *
     * 2) The kernel is newer than 2.6.39 but older than 3.6.  In this case
     *    the first call of fstat() returns EBADF and we fallback to fstatat()
     *    with AT_EMPTY_PATH which was introduced at the same time as O_PATH.
     *
     * 3) The kernel is older than 2.6.39 but nginx was build with O_PATH
     *    support.  Since descriptors are opened with O_PATH|O_RDONLY flags
     *    and O_PATH is ignored by the kernel then the O_RDONLY flag is
     *    actually used.  In this case fstat() just works.
     */

    if (use_fstat) {
        if (ngx_fd_info(fd, fi) != NGX_FILE_ERROR) {
            return NGX_OK;
        }

        if (ngx_errno != NGX_EBADF) {
            return NGX_ERROR;
        }

        ngx_log_error(NGX_LOG_NOTICE, log, 0,
                      "fstat(O_PATH) failed with EBADF, "
                      "switching to fstatat(AT_EMPTY_PATH)");

        use_fstat = 0;
    }

    if (ngx_file_at_info(fd, "", fi, AT_EMPTY_PATH) != NGX_FILE_ERROR) {
        return NGX_OK;
    }

    return NGX_ERROR;
}

#endif

#endif /* NGX_HAVE_OPENAT */

//open���ļ���Ȼ���ڸú�������ȡ�ļ�stat������Ϣ
static ngx_fd_t
ngx_open_file_wrapper(ngx_str_t *name, ngx_open_file_info_t *of,
    ngx_int_t mode, ngx_int_t create, ngx_int_t access, ngx_log_t *log)
{
    ngx_fd_t  fd;

#if !(NGX_HAVE_OPENAT)

    fd = ngx_open_file(name->data, mode, create, access);

    if (fd == NGX_INVALID_FILE) {
        of->err = ngx_errno;
        of->failed = ngx_open_file_n;
        return NGX_INVALID_FILE;
    }

    return fd;

#else

    u_char           *p, *cp, *end;
    ngx_fd_t          at_fd;
    ngx_str_t         at_name;

    if (of->disable_symlinks == NGX_DISABLE_SYMLINKS_OFF) {
        fd = ngx_open_file(name->data, mode, create, access);

        if (fd == NGX_INVALID_FILE) {
            of->err = ngx_errno;
            of->failed = ngx_open_file_n;
            return NGX_INVALID_FILE;
        }

        return fd;
    }

    //������з��������ļ����

    
    //�ļ���
    p = name->data;
    end = p + name->len;

    at_name = *name;

    if (of->disable_symlinks_from) {

        cp = p + of->disable_symlinks_from;

        *cp = '\0';

        at_fd = ngx_open_file(p, NGX_FILE_SEARCH|NGX_FILE_NONBLOCK,
                              NGX_FILE_OPEN, 0);

        *cp = '/';

        if (at_fd == NGX_INVALID_FILE) {
            of->err = ngx_errno;
            of->failed = ngx_open_file_n;
            return NGX_INVALID_FILE;
        }

        at_name.len = of->disable_symlinks_from;
        p = cp + 1;

    } else if (*p == '/') {

        at_fd = ngx_open_file("/",
                              NGX_FILE_SEARCH|NGX_FILE_NONBLOCK,
                              NGX_FILE_OPEN, 0);

        if (at_fd == NGX_INVALID_FILE) {
            of->err = ngx_errno;
            of->failed = ngx_openat_file_n;
            return NGX_INVALID_FILE;
        }

        at_name.len = 1;
        p++;

    } else {
        at_fd = NGX_AT_FDCWD;
    }

    for ( ;; ) {
        cp = ngx_strlchr(p, end, '/');
        if (cp == NULL) {
            break;
        }

        if (cp == p) {
            p++;
            continue;
        }

        *cp = '\0';

        if (of->disable_symlinks == NGX_DISABLE_SYMLINKS_NOTOWNER) {
            fd = ngx_openat_file_owner(at_fd, p,
                                       NGX_FILE_SEARCH|NGX_FILE_NONBLOCK,
                                       NGX_FILE_OPEN, 0, log);

        } else {
            fd = ngx_openat_file(at_fd, p,
                           NGX_FILE_SEARCH|NGX_FILE_NONBLOCK|NGX_FILE_NOFOLLOW,
                           NGX_FILE_OPEN, 0);
        }

        *cp = '/';

        if (fd == NGX_INVALID_FILE) {
            of->err = ngx_errno;
            of->failed = ngx_openat_file_n;
            goto failed;
        }

        if (at_fd != NGX_AT_FDCWD && ngx_close_file(at_fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%V\" failed", &at_name);
        }

        p = cp + 1;
        at_fd = fd;
        at_name.len = cp - at_name.data;
    }

    if (p == end) {

        /*
         * If pathname ends with a trailing slash, assume the last path
         * component is a directory and reopen it with requested flags;
         * if not, fail with ENOTDIR as per POSIX.
         *
         * We cannot rely on O_DIRECTORY in the loop above to check
         * that the last path component is a directory because
         * O_DIRECTORY doesn't work on FreeBSD 8.  Fortunately, by
         * reopening a directory, we don't depend on it at all.
         */

        fd = ngx_openat_file(at_fd, ".", mode, create, access);
        goto done;
    }

    if (of->disable_symlinks == NGX_DISABLE_SYMLINKS_NOTOWNER
        && !(create & (NGX_FILE_CREATE_OR_OPEN|NGX_FILE_TRUNCATE)))
    {
        fd = ngx_openat_file_owner(at_fd, p, mode, create, access, log);

    } else {
        fd = ngx_openat_file(at_fd, p, mode|NGX_FILE_NOFOLLOW, create, access);
    }

done:

    if (fd == NGX_INVALID_FILE) {
        of->err = ngx_errno;
        of->failed = ngx_openat_file_n;
    }

failed:

    if (at_fd != NGX_AT_FDCWD && ngx_close_file(at_fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_close_file_n " \"%V\" failed", &at_name);
    }

    return fd;
#endif
}

//open���ļ���Ȼ���ȡ�ļ�stat������Ϣ
static ngx_int_t
ngx_file_info_wrapper(ngx_str_t *name, ngx_open_file_info_t *of,
    ngx_file_info_t *fi, ngx_log_t *log)
{
    ngx_int_t  rc;

#if !(NGX_HAVE_OPENAT)

    rc = ngx_file_info(name->data, fi);

    if (rc == NGX_FILE_ERROR) {
        of->err = ngx_errno;
        of->failed = ngx_file_info_n;
        return NGX_FILE_ERROR;
    }

    return rc;

#else

    ngx_fd_t  fd;

    if (of->disable_symlinks == NGX_DISABLE_SYMLINKS_OFF) { //������������

        rc = ngx_file_info(name->data, fi);

        if (rc == NGX_FILE_ERROR) {
            of->err = ngx_errno;
            of->failed = ngx_file_info_n;
            return NGX_FILE_ERROR;
        }

        return rc;
    }

    fd = ngx_open_file_wrapper(name, of, NGX_FILE_RDONLY|NGX_FILE_NONBLOCK,
                               NGX_FILE_OPEN, 0, log); //open���ļ���Ȼ���ȡ�ļ�stat������Ϣ

    if (fd == NGX_INVALID_FILE) { //�ļ�������ֱ�ӷ���NGX_FILE_ERROR
        return NGX_FILE_ERROR;
    }

    rc = ngx_fd_info(fd, fi);

    if (rc == NGX_FILE_ERROR) {
        of->err = ngx_errno;
        of->failed = ngx_fd_info_n;
    }

    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_close_file_n " \"%V\" failed", name);
    }

    return rc;
#endif
}

//��ȡname�ļ������ngx_open_file_info_t��Ϣ��Ҳ������Ҫ��ȡ�ļ�����stat��Ϣ�����û�и��ļ����ڣ���᷵��NGX_ERROR
static ngx_int_t //����ͨ��stat������ȡ�ļ�stat��Ϣ
ngx_open_and_stat_file(ngx_str_t *name, ngx_open_file_info_t *of,
    ngx_log_t *log)
{
    ngx_fd_t         fd;
    ngx_file_info_t  fi;

    //of->fd�Ƿ��Ѿ���Ч�˵�������
    
    if (of->fd != NGX_INVALID_FILE) { //���֮ǰ�Ѿ��򿪹�name�ļ�������of->uniq

        if (ngx_file_info_wrapper(name, of, &fi, log) == NGX_FILE_ERROR) {
            of->fd = NGX_INVALID_FILE;
            return NGX_ERROR;
        }

        if (of->uniq == ngx_file_uniq(&fi)) {
            goto done;
        }

    } else if (of->test_dir) {

        if (ngx_file_info_wrapper(name, of, &fi, log) == NGX_FILE_ERROR) {
            of->fd = NGX_INVALID_FILE;
            return NGX_ERROR;
        }

        if (ngx_is_dir(&fi)) {
            goto done;
        }
    }

    if (!of->log) {

        /*
         * Use non-blocking open() not to hang on FIFO files, etc.
         * This flag has no effect on a regular files.
         */

        fd = ngx_open_file_wrapper(name, of, NGX_FILE_RDONLY|NGX_FILE_NONBLOCK,
                                   NGX_FILE_OPEN, 0, log);

    } else {
        fd = ngx_open_file_wrapper(name, of, NGX_FILE_APPEND,
                                   NGX_FILE_CREATE_OR_OPEN,
                                   NGX_FILE_DEFAULT_ACCESS, log);
    }

    if (fd == NGX_INVALID_FILE) {
        of->fd = NGX_INVALID_FILE;
        return NGX_ERROR;
    }

    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_CRIT, log, ngx_errno,
                      ngx_fd_info_n " \"%V\" failed", name);

        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%V\" failed", name);
        }

        of->fd = NGX_INVALID_FILE;

        return NGX_ERROR;
    }

    if (ngx_is_dir(&fi)) {
        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%V\" failed", name);
        }

        of->fd = NGX_INVALID_FILE;

    } else {
        of->fd = fd;

        if (of->read_ahead && ngx_file_size(&fi) > NGX_MIN_READ_AHEAD) {
            if (ngx_read_ahead(fd, of->read_ahead) == NGX_ERROR) {
                ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                              ngx_read_ahead_n " \"%V\" failed", name);
            }
        }

        if (of->directio <= ngx_file_size(&fi)) {
            if (ngx_directio_on(fd) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                              ngx_directio_on_n " \"%V\" failed", name);

            } else {
                of->is_directio = 1;
            }
        }
    }

done:

    //��ȡ�ļ��������Ϣ
    of->uniq = ngx_file_uniq(&fi);
    of->mtime = ngx_file_mtime(&fi);
    of->size = ngx_file_size(&fi);
    of->fs_size = ngx_file_fs_size(&fi);
    of->is_dir = ngx_is_dir(&fi);
    of->is_file = ngx_is_file(&fi);
    of->is_link = ngx_is_link(&fi);
    of->is_exec = ngx_is_exec(&fi);

    return NGX_OK;
}


/*
 * we ignore any possible event setting error and
 * fallback to usual periodic file retests
 */
/*
�����eventָ�ľ���open_file_cache_events��ֻ����kqueue��������á���unfinished code�������Ǽ���ļ��������ı仯��
*/
static void
ngx_open_file_add_event(ngx_open_file_cache_t *cache,
    ngx_cached_open_file_t *file, ngx_open_file_info_t *of, ngx_log_t *log)
{
    ngx_open_file_cache_event_t  *fev;

    if (!(ngx_event_flags & NGX_USE_VNODE_EVENT)
        || !of->events
        || file->event
        || of->fd == NGX_INVALID_FILE
        || file->uses < of->min_uses)
    {
        return;
    }

    file->use_event = 0;

    file->event = ngx_calloc(sizeof(ngx_event_t), log);
    if (file->event== NULL) {
        return;
    }

    fev = ngx_alloc(sizeof(ngx_open_file_cache_event_t), log);
    if (fev == NULL) {
        ngx_free(file->event);
        file->event = NULL;
        return;
    }

    fev->fd = of->fd;
    fev->file = file;
    fev->cache = cache;

    file->event->handler = ngx_open_file_cache_remove;
    file->event->data = fev;

    /*
     * although vnode event may be called while ngx_cycle->poll
     * destruction, however, cleanup procedures are run before any
     * memory freeing and events will be canceled.
     */

    file->event->log = ngx_cycle->log;

    char tmpbuf[256];
        
        snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_VNODE_EVENT(et) read add", NGX_FUNC_LINE);
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, tmpbuf);
    if (ngx_add_event(file->event, NGX_VNODE_EVENT, NGX_ONESHOT_EVENT)
        != NGX_OK)
    {
        ngx_free(file->event->data);
        ngx_free(file->event);
        file->event = NULL;
        return;
    }

    /*
     * we do not set file->use_event here because there may be a race
     * condition: a file may be deleted between opening the file and
     * adding event, so we rely upon event notification only after
     * one file revalidation on next file access
     */

    return;
}

//nginx�����˳���ʱ����ͷ�pool�Ӷ�ִ�иú����������ڵ�ngx_cached_open_file_s����ngx_destroy_pool�� 
static void
ngx_open_file_cleanup(void *data) //����ǿͻ��������ͷ���Դ��ʱ��ִ�У�����ľ���ͻ��˵�r->pool->cleanup
{
    ngx_open_file_cache_cleanup_t  *c = data;
    
    //���ļ������ü���count��һ��ngxclosecached_file�᳢��ȥ�ر�����ļ���
    c->file->count--;

    //�����c->min_uses����open_file_cache_min_uses���õ�ʱ��
    ngx_close_cached_file(c->cache, c->file, c->min_uses, c->log);

    /* drop one or two expired open files */
    ngx_expire_old_cached_files(c->cache, 1, c->log);
}


static void
ngx_close_cached_file(ngx_open_file_cache_t *cache,
    ngx_cached_open_file_t *file, ngx_uint_t min_uses, ngx_log_t *log)
{
    ngx_log_debug5(NGX_LOG_DEBUG_CORE, log, 0,
                   "close cached open file: %s, fd:%d, c:%d, u:%d, %d",
                   file->name, file->fd, file->count, file->uses, file->close);

    if (!file->close) { //�ļ�����Ҫ���رգ�Ϊ1����Ҫ��if�ⱻ�ر�

        file->accessed = ngx_time();

        ngx_queue_remove(&file->queue);  //�ѽڵ�Ӷ���ɾ��������ļ�ͷ

        ngx_queue_insert_head(&cache->expire_queue, &file->queue);

         //�����min_uses����open_file_cache_min_uses���õ�ʱ��
    //file->uses >= min_uses��ʾֻҪ��ngx_cached_open_file_s file�ڵ㱻�������Ĵ����ﵽmin_uses�Σ�����Զ����ر��ļ������Ǹ�cache nodeʧЧ����ngx_open_file_cleanup  ngx_close_cached_file
        if (file->uses >= min_uses || file->count) { //file->count > 0˵�����пͻ���������ʹ�ø�node�ڵ�
            //�ļ���ʹ�ô����������ֵ�����ļ������������þ�ֱ�ӷ��أ���ʱ����Ҫ���ر�
            return;
        }
    }

    ngx_open_file_del_event(file);
    
    //�ļ���Ҫ���رգ������ļ��������þ�ֱ�ӷ���
    if (file->count) {
        return;
    }

    if (file->fd != NGX_INVALID_FILE) {//��������ļ���Ҫ���ر�

        if (ngx_close_file(file->fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", file->name);
        }

        file->fd = NGX_INVALID_FILE;
    }

    if (!file->close) {
        return;
    }

    
    //��Ҫ���رգ�������Ĺر��ˣ���ô�ͷ��ڴ�
    ngx_free(file->name);
    ngx_free(file);
}


static void
ngx_open_file_del_event(ngx_cached_open_file_t *file)
{
    if (file->event == NULL) {
        return;
    }

    (void) ngx_del_event(file->event, NGX_VNODE_EVENT,
                         file->count ? NGX_FLUSH_EVENT : NGX_CLOSE_EVENT);

    ngx_free(file->event->data);
    ngx_free(file->event);
    file->event = NULL;
    file->use_event = 0;
}

/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s(ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   )��ngx_expire_old_cached_files����ʧЧ�ж�, 
�����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)ngx_http_file_cache_node_t(ngx_http_file_cache_s->sh�еĳ�Ա)��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/

//ɾ��������͹��ڶ����й��ڵ��ļ�

//�ڿͻ������ӶϿ��ͷ���Դ��ʱ����ã������ڴ���node�ڵ��ʱ���ֳ���open_file_cache max=1000�����ˣ����ǿ��ɾ����ɵ�
static void
ngx_expire_old_cached_files(ngx_open_file_cache_t *cache, ngx_uint_t n,
    ngx_log_t *log) //����n˵����ǿ��ɾ�����Ƿ�ǿ��ɾ����0ǿ��ɾ��
//����open_file_cache max=1000���õ�1000�ﵽ����(ngx_open_file_cache_t->rbtree������еĽڵ������ﵽ����)�����߿ͻ��������ͷ�pool��Դ��ʱ���ִ�е��ú���
{
    time_t                   now;
    ngx_queue_t             *q;
    ngx_cached_open_file_t  *file;

    now = ngx_time();

    /*
     * n == 1 deletes one or two inactive files
     * n == 0 deletes least recently used file by force
     *        and one or two inactive files
     */

    while (n < 3) {

        if (ngx_queue_empty(&cache->expire_queue)) {
            //�ն���ֱ�ӷ���
            return;
        }
        
        //ȡ�����һ���ļ��������п��ܳ�ʱ���ļ�
        q = ngx_queue_last(&cache->expire_queue);

        file = ngx_queue_data(q, ngx_cached_open_file_t, queue);


        /*
          ������������Ļ�ֱ���˳���ʱ����,����������������������
          n = 0��Ȼ���жϺ���ı��ʽǿ��ɾ��
          n = 1,2 �ж�ʱ��������ɾ��
          
          ���Ե�ǿ��ɾ������ʱ����ǿ���ͷ�һ����Ȼ��ɾ��1��2���ļ��� ������ʱ��ɾ��1��2���ļ�
          ���ļ�ռ����������ʱ��϶����ͷ�һ����ȥ����һ������˲������й©�����
          */
        
        //���n��Ϊ0��������ļ�û�й��ڣ���ôֱ�ӷ��أ�
        if (n++ != 0 && now - file->accessed <= cache->inactive) {//�� cache->inactive���ʱ������Ƿ��з��ʸû��棬�������ֱ�ӷ��أ�˵��û�й���
            return;
        }
        
        //�ļ�����ɾ��
        ngx_queue_remove(q);

        ngx_rbtree_delete(&cache->rbtree, &file->node);

        cache->current--;

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                       "expire cached open file: %s", file->name);

        if (!file->err && !file->is_dir) {
            file->close = 1;
            ngx_close_cached_file(cache, file, 0, log);

        } else {
            ngx_free(file->name);
            ngx_free(file);
        }
    }
}


static void
ngx_open_file_cache_rbtree_insert_value(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t       **p;
    ngx_cached_open_file_t    *file, *file_temp;

    for ( ;; ) {

        if (node->key < temp->key) {

            p = &temp->left;

        } else if (node->key > temp->key) {

            p = &temp->right;

        } else { /* node->key == temp->key */

            file = (ngx_cached_open_file_t *) node;
            file_temp = (ngx_cached_open_file_t *) temp;

            p = (ngx_strcmp(file->name, file_temp->name) < 0)
                    ? &temp->left : &temp->right;
        }

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


static ngx_cached_open_file_t *
ngx_open_file_lookup(ngx_open_file_cache_t *cache, ngx_str_t *name,
    uint32_t hash)
{
    ngx_int_t                rc;
    ngx_rbtree_node_t       *node, *sentinel;
    ngx_cached_open_file_t  *file;

    node = cache->rbtree.root;
    sentinel = cache->rbtree.sentinel;

    while (node != sentinel) {

        if (hash < node->key) {
            node = node->left;
            continue;
        }

        if (hash > node->key) {
            node = node->right;
            continue;
        }

        /* hash == node->key */

        file = (ngx_cached_open_file_t *) node;

        rc = ngx_strcmp(name->data, file->name);

        if (rc == 0) {
            return file;
        }

        node = (rc < 0) ? node->left : node->right;
    }

    return NULL;
}

//kqueue���������
static void
ngx_open_file_cache_remove(ngx_event_t *ev)
{
    ngx_cached_open_file_t       *file;
    ngx_open_file_cache_event_t  *fev;

    fev = ev->data;
    file = fev->file;

    ngx_queue_remove(&file->queue);

    ngx_rbtree_delete(&fev->cache->rbtree, &file->node);

    fev->cache->current--;

    /* NGX_ONESHOT_EVENT was already deleted */
    file->event = NULL;
    file->use_event = 0;

    file->close = 1;

    ngx_close_cached_file(fev->cache, file, 0, ev->log);

    /* free memory only when fev->cache and fev->file are already not needed */

    ngx_free(ev->data);
    ngx_free(ev);
}
