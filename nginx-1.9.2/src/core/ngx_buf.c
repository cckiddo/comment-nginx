
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


ngx_buf_t *
ngx_create_temp_buf(ngx_pool_t *pool, size_t size)
{
    ngx_buf_t *b;

    b = ngx_calloc_buf(pool); //��������Ϊngx_buf_tͷ������Ŀռ�
    if (b == NULL) {
        return NULL;
    }

    b->start = ngx_palloc(pool, size); //��������������洢���ݵĿռ�
    if (b->start == NULL) {
        return NULL;
    }

    /*
     * set by ngx_calloc_buf():
     *
     *     b->file_pos = 0;
     *     b->file_last = 0;
     *     b->file = NULL;
     *     b->shadow = NULL;
     *     b->tag = 0;
     *     and flags
     */

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;

    return b;
}


ngx_chain_t *
ngx_alloc_chain_link(ngx_pool_t *pool)
{
    ngx_chain_t  *cl;

    cl = pool->chain;

    if (cl) {
        pool->chain = cl->next; //���ͷŵ�ngx_chain_t��ͨ��ngx_free_chain��ӵ�poll->chain�ϵ�
        return cl;
    }

    cl = ngx_palloc(pool, sizeof(ngx_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}


ngx_chain_t *
ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs)
{
    u_char       *p;
    ngx_int_t     i;
    ngx_buf_t    *b;
    ngx_chain_t  *chain, *cl, **ll;

    p = ngx_palloc(pool, bufs->num * bufs->size);
    if (p == NULL) {
        return NULL;
    }

    ll = &chain;

    for (i = 0; i < bufs->num; i++) {

        b = ngx_calloc_buf(pool);
        if (b == NULL) {
            return NULL;
        }

        /*
         * set by ngx_calloc_buf():
         *
         *     b->file_pos = 0;
         *     b->file_last = 0;
         *     b->file = NULL;
         *     b->shadow = NULL;
         *     b->tag = 0;
         *     and flags
         *
         */

        b->pos = p;
        b->last = p;
        b->temporary = 1;

        b->start = p;
        p += bufs->size;
        b->end = p;

        cl = ngx_alloc_chain_link(pool);
        if (cl == NULL) {
            return NULL;
        }

        cl->buf = b;
        *ll = cl;
        ll = &cl->next;
    }

    *ll = NULL;

    return chain;
}

//��in��ӵ�chain�ĺ��棬ƴ������
ngx_int_t
ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain, ngx_chain_t *in)
{
    ngx_chain_t  *cl, **ll;

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) { // ѭ��������ll ָ�����һ�� chain �� next��next ����ָ�룬���� ll �Ƕ���ָ��
        ll = &cl->next;
    }

    while (in) {
        cl = ngx_alloc_chain_link(pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = in->buf;
        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return NGX_OK;
}


ngx_chain_t *
ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free)
{
    ngx_chain_t  *cl;

    if (*free) { //���free ���������п����ngx_chain_t�ڵ㣬��ֱ��ʹ�ø������е�ngx_chain_t�ڵ�ռ䣬������濪�ٿռ�
        cl = *free;
        *free = cl->next;
        cl->next = NULL;
        return cl;
    }

    cl = ngx_alloc_chain_link(p);
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = ngx_calloc_buf(p);
    if (cl->buf == NULL) {
        return NULL;
    }

    cl->next = NULL;

    return cl;
}

/*
��Ϊnginx������ǰflush�����������Щbuf�������Ϳ����ظ�ʹ�ã����Ա����ط��䣬���ϵͳ���ܣ�����Ϊfree_buf����û�б������
buf����busy_buf��nginxû���ر�ļ���������Ե����������ṩ��һ������ngx_chain_update_chains������������ά������������������
*/
//�ú������ܾ��ǰ��¶�����out������ӵ�busy��β����Ȼ���busy�����Ѿ�������ϵ�buf�ڵ��busy����ժ����Ȼ��ŵ�free��ͷ��
//δ���ͳ�ȥ��buf�ڵ�Ȼ���out�����У�Ҳ����busy������
void
ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free, ngx_chain_t **busy,
    ngx_chain_t **out, ngx_buf_tag_t tag)
{
    ngx_chain_t  *cl;

    if (*busy == NULL) {
        // busy ָ�� out ָ��ĵط�
        *busy = *out;

    } else {
        for (cl = *busy; cl->next; cl = cl->next) { /* void */ } // cl ָ�� busy chain ���������һ��
        

        cl->next = *out; //out�ڵ���ӵ�busy���е����һ���ڵ�
    }

    *out = NULL;

    while (*busy) {
        cl = *busy;

        // buf ��С���� 0��˵����û�������request body �е� bufs ������õģ�����������bufs ��ָ��� buf �� busy ָ��� buf ������һģһ����
        if (ngx_buf_size(cl->buf) != 0) { //pos��last����ȣ�˵����buf�е�����û�д�����
            break;
        }

        if (cl->buf->tag != tag) {// tag �д洢���� ����ָ��
            *busy = cl->next;
            ngx_free_chain(p, cl);
            continue;
        }

        //�Ѹÿռ��pos last��ָ��start��ʼ������ʾ��ngx_buf_tû�����������棬��˿��԰����ӵ�free���У����Լ�����ȡ���ݵ�free�е�ngx_buf_t�ڵ���
        cl->buf->pos = cl->buf->start;
        cl->buf->last = cl->buf->start;

        *busy = cl->next; //��cl��busy�в����Ȼ����ӵ�freeͷ��
        cl->next = *free;
        *free = cl; // ��� chain �ŵ� free �б����ǰ�棬��ӵ�freeͷ��
    }
}


off_t
ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit)
{
    off_t         total, size, aligned, fprev;
    ngx_fd_t      fd;
    ngx_chain_t  *cl;

    total = 0;

    cl = *in;
    fd = cl->buf->file->fd;

    do {
        size = cl->buf->file_last - cl->buf->file_pos;

        if (size > limit - total) {
            size = limit - total;

            aligned = (cl->buf->file_pos + size + ngx_pagesize - 1)
                       & ~((off_t) ngx_pagesize - 1);

            if (aligned <= cl->buf->file_last) {
                size = aligned - cl->buf->file_pos;
            }
        }

        total += size;
        fprev = cl->buf->file_pos + size;
        cl = cl->next;

    } while (cl
             && cl->buf->in_file
             && total < limit
             && fd == cl->buf->file->fd
             && fprev == cl->buf->file_pos);

    *in = cl;

    return total;
}

//���㱾�ε���ngx_writev���ͳ�ȥ��send�ֽ���in�������������ݵ��Ǹ�λ��
ngx_chain_t *
ngx_chain_update_sent(ngx_chain_t *in, off_t sent)
{
    off_t  size;

    for ( /* void */ ; in; in = in->next) {
        //�ֱ���һ��������ӣ�Ϊ���ҵ��ǿ�ֻ�ɹ�������һ�������ݵ��ڴ�飬����������ʼ���͡�
        if (ngx_buf_special(in->buf)) {
            continue;
        }

        if (sent == 0) {
            break;
        }

        size = ngx_buf_size(in->buf);

        if (sent >= size) { //˵����in->buf�����Ѿ�ȫ�����ͳ�ȥ
            sent -= size;//��Ǻ��滹�ж����������ҷ��͹���

            if (ngx_buf_in_memory(in->buf)) {//˵����in->buf�����Ѿ�ȫ�����ͳ�ȥ
                in->buf->pos = in->buf->last;//�������ڴ档��������һ��
            }

            if (in->buf->in_file) {
                in->buf->file_pos = in->buf->file_last;
            }

            continue;
        }

        if (ngx_buf_in_memory(in->buf)) { //˵�����ͳ�ȥ�����һ�ֽ����ݵ���һ�ֽ�������in->buf->pos+sendλ�ã��´δ����λ�ÿ�ʼ����
            in->buf->pos += (size_t) sent;//����ڴ�û����ȫ������ϣ����磬�»صô����￪ʼ��
        }

        if (in->buf->in_file) {
            in->buf->file_pos += sent;
        }

        break;
    }

    return in; //�´δ����in��ʼ����in->buf->pos
}
