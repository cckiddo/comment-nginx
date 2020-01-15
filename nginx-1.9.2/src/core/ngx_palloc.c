
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/*
ngx_create_pool������pool
ngx_destory_pool������ pool
ngx_reset_pool������pool�еĲ�������
ngx_palloc/ngx_pnalloc����pool�з���һ���ڴ�
ngx_pool_cleanup_add��Ϊpool���cleanup����
*/
ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;

    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log); //// ����һ�� size ��С���ڴ�  �ڴ�ռ�16�ֽڶ���
    if (p == NULL) {
        return NULL;
    }

    // ��pool�е��������ʼֵ
    p->d.last = (u_char *) p + sizeof(ngx_pool_t); //���ÿռ�Ҫ��ȥ���ͷ�� ��sizeof(ngx_pool_t)����pool��header��Ϣ��header��Ϣ�еĸ����ֶ����ڹ�������pool
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL; //���ܳ���NGX_MAX_ALLOC_FROM_POOL// pool �������ô�С
    
    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p; //ָ��ռ����ͷ��
}

/*
ngx_create_pool������pool
ngx_destroy_pool������ pool
ngx_reset_pool������pool�еĲ�������
ngx_palloc/ngx_pnalloc����pool�з���һ���ڴ�
ngx_pool_cleanup_add��Ϊpool���cleanup����
*/
void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) { //cleanup��ngx_pool_cleanup_add��ֵ
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {

        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);

        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

/*
ngx_create_pool������pool
ngx_destory_pool������ pool
ngx_reset_pool������pool�еĲ�������
ngx_palloc/ngx_pnalloc����pool�з���һ���ڴ�
ngx_pool_cleanup_add��Ϊpool���cleanup����
*/ //�����Ҫ���ͷŴ���ڴ����ݣ�ͬʱ���Դ���ʹ�����ݿ��е�����
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}

/*
ʵ���ϣ���r�п��Ի������ڴ�ض�����Щ�ڴ�صĴ�С�����弰�����ڸ�����ͬ����3���ֻ��漰����ڴ�أ�����ʹ��r->pool�ڴ�ؼ��ɡ�����ngx_pool_t�����
���Դ��ڴ���з����ڴ档
���У�ngx_palloc���������pool�ڴ���з��䵽size�ֽڵ��ڴ棬����������ڴ����ʼ��ַ���������NULL��ָ�룬���ʾ����ʧ�ܡ�
����һ����װ��ngx_palloc�ĺ���ngx_pcalloc����������һ���£����ǰ�ngx_palloc���뵽���ڴ��ȫ����Ϊ0����Ȼ����������¸��ʺ���ngx_pcalloc�������ڴ档
*/ //����ʹ��ngx_palloc���ڴ���л�ȡһ���ڴ�
//ngx_palloc��ngx_palloc�������Ƿ�ƬС���ڴ�ʱ�Ƿ���Ҫ�ڴ����
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;
    
    // �ж� size �Ƿ���� pool ����ʹ���ڴ��С
    if (size <= pool->max) {

        p = pool->current; //��current���ڵ�pool���ݽڵ㿪ʼ�������Ѱ���Ǹ��ڵ���Է���size�ڴ�

        do {
            m = ngx_align_ptr(p->d.last, NGX_ALIGNMENT);// �� m ���䵽�ڴ�����ַ
            if ((size_t) (p->d.end - m) >= size) {// �ж� pool ��ʣ���ڴ��Ƿ���
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;//�����ǰ�ڴ治����������һ���ڴ���з���ռ�

        } while (p);

        return ngx_palloc_block(pool, size);
    }

    /*
    �����������һ�����������Ҫ���ڴ����pool���ɷ����ڴ��Сʱ����ʱ�����ж�size�Ѿ�����pool->max�Ĵ�С�ˣ�����ֱ�ӵ���ngx_palloc_large���д��ڴ���䣬���ǽ�ע����ת���������
    ��ƪ������Դ�� Linux������վ(www.linuxidc.com)  ԭ�����ӣ�http://www.linuxidc.com/Linux/2011-08/41860.htm
    */
    return ngx_palloc_large(pool, size);
}

//ngx_palloc��ngx_palloc�������Ƿ�ƬС���ڴ�ʱ�Ƿ���Ҫ�ڴ����
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ngx_palloc_block(pool, size);
    }

    return ngx_palloc_large(pool, size);
}

//���ǰ�濪�ٵ�pool�ռ��Ѿ����꣬����¿��ٿռ�ngx_pool_t
static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

    // ��ǰ������ pool �Ĵ�С
    psize = (size_t) (pool->d.end - (u_char *) pool);

    //// ���ڴ�����˵�ǰ���£��·���һ���ڴ�
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size;

    // �ж��ڵ�ǰ pool �����ڴ��ʧ�ܴ������������ܸ��õ�ǰ pool �Ĵ�����
    // ������� 4 �Σ�������ڴ� pool ���ٴγ��Է����ڴ棬�����Ч��
    //���ʧ�ܴ�������4��������4���������currentָ�룬��������pool���ڴ������ʹ��
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;// ���� current ָ�룬 ÿ�δ�pool�з����ڴ��ʱ���Ǵ�curren��ʼ����pool�ڵ��ȡ�ڴ��
        }
    }

    // �þ�ָ���������� next ָ���·���� pool
    p->d.next = new;

    return m;
}

/*
����Ҫ���ڴ����pool���ɷ����ڴ��Сʱ����ʱ�����ж�size�Ѿ�����pool->max�Ĵ�С�ˣ�����ֱ�ӵ���ngx_palloc_large���д��ڴ���䣬
��ƪ������Դ�� Linux������վ(www.linuxidc.com)  ԭ�����ӣ�http://www.linuxidc.com/Linux/2011-08/41860.htm
*/
static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    /*
    // ��������һ���СΪ size �����ڴ�
    // ע�⣺�˴���ʹ�� ngx_memalign ��ԭ���ǣ��·�����ڴ�ϴ󣬶���Ҳû̫���Ҫ
    //  ���Һ����ṩ�� ngx_pmemalign ������ר���û���������˵��ڴ�
    */
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    // ����largt�����Ͽ����large ָ��
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) { //�������û�õ�large
            large->alloc = p;
            return p;
        }

        /*
         // �����ǰ large �󴮵� large �ڴ����Ŀ���� 3 ��������3����
        // ��ֱ��ȥ��һ���������ڴ棬���ٲ�����
        */
        if (n++ > 3) {//Ҳ����˵���pool->largeͷ��������4��large��allocָ�붼�����ˣ�����������һ���µ�pool_larg���ŵ�pool->largeͷ��
            break; //????? �о�ûɶ�ã���Ϊ����ÿ��alloc��large��Ӧ��alloc���Ǹ�ֵ�˵�
        }
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    // ���·���� large �����������
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/*
ngx_create_pool������pool
ngx_destory_pool������ pool
ngx_reset_pool������pool�еĲ�������
ngx_palloc/ngx_pnalloc����pool�з���һ���ڴ�
ngx_pool_cleanup_add��Ϊpool���cleanup����
*/ //��pool�з������handler���ڴ棬
/*
�Ի���fileΪ��:

���Կ�����ngx_pool_cleanup_file_t�еĶ�����ngx_buf_t��������file�ṹ���ж����ֹ��ˣ�����Ҳ����ͬ�ġ�����file�ṹ�壬�������ڴ�����Ѿ�Ϊ��������ڴ棬
ֻ�����������ʱ�Ż��ͷţ���ˣ�����򵥵�����file��ĳ�Ա���ɡ������ļ�����������������¡�
ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
if (cln == NULL) {
 return NGX_ERROR;
}

cln->handler = ngx_pool_cleanup_file;
ngx_pool_cleanup_file_t  *clnf = cln->data;

clnf->fd = b->file->fd;
clnf->name = b->file->name.data;
clnf->log = r->pool->log;

ngx_pool_cleanup_add���ڸ���HTTP��ܣ����������ʱ����cln��handler����������Դ��
*///poll��������ngx_pool_cleanup_add, ngx_http_request_t��������ngx_http_cleanup_add
ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

/*
Nginx���첽�ؽ������ļ���Ч�ط��͸��û����������Ǳ���Ҫ��HTTP�������Ӧ������Ϻ�ر��Ѿ��򿪵��ļ���������򽫻���־��й¶���⡣
���������ļ����Ҳ�ܼ򵥣�ֻ��Ҫ����һ��ngx_pool_cleanup_t�ṹ�壨������򵥵ķ�����HTTP��ܻ��ṩ��������ʽ�����������ʱ�ص�����HTTPģ���cleanup���������ڵ�11�½��ܣ���
�����Ǹյõ����ļ��������Ϣ������������Nginx�ṩ��ngx_pool_cleanup_file�������õ�����handler�ص������м��ɡ�

*/
//ngx_pool_cleanup_file�������ǰ��ļ�����رա��������ʵ���п��Կ�����ngx_pool_cleanup_file������Ҫһ��ngx_pool_cleanup_file_t���͵Ĳ�����
//��ô������ṩ��������أ���ngx_pool_cleanup_t�ṹ���data��Ա�ϸ�ֵ���ɡ��������һ��ngx_pool_cleanup_file_t�Ľṹ��

/*
���Կ�����ngx_pool_cleanup_file_t�еĶ�����ngx_buf_t��������file�ṹ���ж����ֹ��ˣ�����Ҳ����ͬ�ġ�����file�ṹ�壬�������ڴ�����Ѿ�Ϊ��������ڴ棬
ֻ�����������ʱ�Ż��ͷţ���ˣ�����򵥵�����file��ĳ�Ա���ɡ������ļ�����������������¡�
ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
if (cln == NULL) {
 return NGX_ERROR;
}

cln->handler = ngx_pool_cleanup_file;
ngx_pool_cleanup_file_t  *clnf = cln->data;

clnf->fd = b->file->fd;
clnf->name = b->file->name.data;
clnf->log = r->pool->log;

ngx_pool_cleanup_add���ڸ���HTTP��ܣ����������ʱ����cln��handler����������Դ��
*/
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif

