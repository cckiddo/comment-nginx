
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

//getconf PAGE_SIZE ������Բ鿴
ngx_uint_t  ngx_pagesize;//��ngx_os_init  ����һ����ҳ�Ĵ�С����λΪ�ֽ�(Byte)����ֵΪϵͳ�ķ�ҳ��С����һ�����Ӳ����ҳ��С��ͬ��
//ngx_pagesizeΪ4M��ngx_pagesize_shiftӦ��Ϊ12
ngx_uint_t  ngx_pagesize_shift; //ngx_pagesize������λ�Ĵ�������for (n = ngx_pagesize; n >>= 1; ngx_pagesize_shift++) { /* void */ }

/*
�����֪��CPU cache�еĴ�С����ô�Ϳ���������Ե������ڴ�Ķ���ֵ������������߳����Ч�ʡ�
Nginx�з����ڴ�صĽӿڣ�Nginx�Ὣ�ڴ�ر߽���뵽 CPU cache�д�С  32λƽ̨��ngx_cacheline_size=32
*/
ngx_uint_t  ngx_cacheline_size;//32λƽ̨��ngx_cacheline_size=32


void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "malloc(%uz) failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}


void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


#if (NGX_HAVE_POSIX_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);// ����һ�� size ��С���ڴ�

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
