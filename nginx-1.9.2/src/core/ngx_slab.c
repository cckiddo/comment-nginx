
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>

/*
����ָ����4�ı���,��ô����λһ��Ϊ0,��ʱ���ǿ�������ָ��ĺ���λ�����,������ÿռ�.
��nginx��slab��,����ʹ��ngx_slab_page_s�ṹ���е�ָ��pre�ĺ���λ�����,����ָʾ��pageҳ���slot������ngx_slab_exact_size�Ĺ�ϵ.
��page���ֵ�slot��С��32ʱ��,pre�ĺ���λΪNGX_SLAB_SMALL.
��page���ֵ�slot�����32ʱ��,pre�ĺ���λΪNGX_SLAB_EXACT
��page���ֵ�slot����32��ʱ��,pre�ĺ���λΪNGX_SLAB_BIG
��pageҳ�治����slotʱ��,��������ҳ�������û�,pre�ĺ���λΪNGX_SLAB_PAGE
*/
#define NGX_SLAB_PAGE_MASK   3
#define NGX_SLAB_PAGE        0
#define NGX_SLAB_BIG         1
#define NGX_SLAB_EXACT       2
#define NGX_SLAB_SMALL       3

#if (NGX_PTR_SIZE == 4)

#define NGX_SLAB_PAGE_FREE   0
//�����������������page�������Ҳ�����page������һ�η���3��page,�����pageΪ[1-3]����page[1].slab=3  page[2].slab=page[3].slab=NGX_SLAB_PAGE_BUSY��¼
#define NGX_SLAB_PAGE_BUSY   0xffffffff
#define NGX_SLAB_PAGE_START  0x80000000

#define NGX_SLAB_SHIFT_MASK  0x0000000f
#define NGX_SLAB_MAP_MASK    0xffff0000
#define NGX_SLAB_MAP_SHIFT   16

#define NGX_SLAB_BUSY        0xffffffff

#else /* (NGX_PTR_SIZE == 8) */

#define NGX_SLAB_PAGE_FREE   0
#define NGX_SLAB_PAGE_BUSY   0xffffffffffffffff
#define NGX_SLAB_PAGE_START  0x8000000000000000

#define NGX_SLAB_SHIFT_MASK  0x000000000000000f
#define NGX_SLAB_MAP_MASK    0xffffffff00000000
#define NGX_SLAB_MAP_SHIFT   32

#define NGX_SLAB_BUSY        0xffffffffffffffff

#endif


#if (NGX_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)     ngx_memset(p, 0xA5, size)

#elif (NGX_HAVE_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)                                                \
    if (ngx_debug_malloc)          ngx_memset(p, 0xA5, size)

#else

#define ngx_slab_junk(p, size)

#endif

static ngx_slab_page_t *ngx_slab_alloc_pages(ngx_slab_pool_t *pool,
    ngx_uint_t pages);
static void ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages);
static void ngx_slab_error(ngx_slab_pool_t *pool, ngx_uint_t level,
    char *text);

//slabҳ��Ĵ�С,32λLinux��Ϊ4k,  
static ngx_uint_t  ngx_slab_max_size;//����ngx_slab_max_size = 2048B�����һ��ҳҪ��Ŷ��obj����obj sizeҪС�������ֵ 
/*
����ָ����4�ı���,��ô����λһ��Ϊ0,��ʱ���ǿ�������ָ��ĺ���λ�����,������ÿռ�.
��nginx��slab��,����ʹ��ngx_slab_page_s�ṹ���е�ָ��pre�ĺ���λ�����,����ָʾ��pageҳ���slot������ngx_slab_exact_size�Ĺ�ϵ.
��page���ֵ�slot��С��32ʱ��,pre�ĺ���λΪNGX_SLAB_SMALL.
��page���ֵ�slot�����32ʱ��,pre�ĺ���λΪNGX_SLAB_EXACT
��page���ֵ�slot����32��ʱ��,pre�ĺ���λΪNGX_SLAB_BIG
��pageҳ�治����slotʱ��,��������ҳ�������û�,pre�ĺ���λΪNGX_SLAB_PAGE
*/ //����32��slot��,ÿ��slot�Ĵ�С����ngx_slab_exact_size  
static ngx_uint_t  ngx_slab_exact_size;//����ngx_slab_exact_size = 128B���ֽ��Ƿ�Ҫ�ڻ������������ռ��bitmap  
static ngx_uint_t  ngx_slab_exact_shift;//ngx_slab_exact_shift = 7����128��λ��ʾ //ÿ��slot���С��λ����ngx_slab_exact_shift  

/*
ע�⣬��ngx_slab_pool_t�������������͵�slab page����Ȼ����ngx_slab_page_t����Ľṹ�����ǹ��ܲ�����ͬ��һ����slots��������ʾ��
�Ž�Сobj���ڴ��(���ҳ��С�� 4096B������<2048B��obj����С��1/2ҳ)����һ������ʾ��Ҫ����Ŀռ��ڻ�������λ�á�Nginx�ѻ���obj��
�ɴ�� (>=2048B)��С��(<2048B)��ÿ�θ����obj����һ����ҳ�����Ѷ��Сobj�����һ��ҳ�м䣬��bitmap�ȷ�������ʾ ��ռ���������С
��obj�ַ�Ϊ3�֣�С��128B������128B������128B��С��2048B������С��128B��obj��Ҫ��ʵ�ʻ������������ bitmap�ռ�����ʾ�ڴ�ʹ�����
(��Ϊslab��Աֻ��4��byte��32bit��һ������ҳ4KB���Դ�ŵ�obj����32�������Բ�����slab ����ʾ)�����������һ���Ŀռ���ʧ�����ڻ��
��128B��obj��Ϊ������һ��32bit����������ʾ��״̬�����ԾͿ���ֱ����slab��Ա��ÿ�η��� �Ŀռ���2^n����С��8byte��8��16��32��64��
128��256��512��1024��2048��С��2^i�Ҵ���2^(i-1)��obj�ᱻ�� ��һ��2^i�Ŀռ䣬����56byte��obj�ͻ����һ��64byte�Ŀռ䡣
*/ //http://adam281412.blog.163.com/blog/static/33700067201111283235134/
/*
�����ڴ����ʵ��ַ��ʼ������:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(����ʵ�ʵ����ݲ��֣�
ÿ��ngx_pagesize����ǰ���һ��ngx_slab_page_t���й�������ÿ��ngx_pagesize��ǰ�˵�һ��obj��ŵ���һ�����߶��int����bitmap�����ڹ���ÿ������ȥ���ڴ�)
*/

    //ͼ�λ����ο�:http://blog.csdn.net/u013009575/article/details/17743261
void
ngx_slab_init(ngx_slab_pool_t *pool)//poolָ��������������ڴ�ռ����ʼ��ַ   slab�ṹ����Ϲ����ڴ�ʹ�õ� ������limit reqģ��Ϊ�����ο�ngx_http_limit_req_module
{
    u_char           *p;
    size_t            size;
    ngx_int_t         m;
    ngx_uint_t        i, n, pages;
    ngx_slab_page_t  *slots;

    /*
     //����ÿ��page��4KB  
    //����ngx_slab_max_size = 2048B�����һ��ҳҪ��Ŷ��obj����obj sizeҪС�������ֵ  
    //����ngx_slab_exact_size = 128B���ֽ��Ƿ�Ҫ�ڻ������������ռ��bitmap  
    //ngx_slab_exact_shift = 7����128��λ��ʾ  
     */
    /* STUB */
    if (ngx_slab_max_size == 0) {
        ngx_slab_max_size = ngx_pagesize / 2;
        ngx_slab_exact_size = ngx_pagesize / (8 * sizeof(uintptr_t));
        for (n = ngx_slab_exact_size; n >>= 1; ngx_slab_exact_shift++) {
            /* void */
        }
    }
    /**/

    pool->min_size = 1 << pool->min_shift; //��С����Ŀռ���8byte  

    p = (u_char *) pool + sizeof(ngx_slab_pool_t); //�����ڴ�ǰ���sizeof(ngx_slab_pool_t)�Ǹ�slab poll��
    size = pool->end - p; //size���ܵĹ����ڴ� - sizeof(ngx_slab_pool_t)�ֽں�ĳ���

    ngx_slab_junk(p, size);

    slots = (ngx_slab_page_t *) p;
    n = ngx_pagesize_shift - pool->min_shift;//12-3=9

/*
��Щslab page�Ǹ���СΪ8��16��32��64��128��256��512��1024��2048byte���ڴ�� ��Щslab page��λ������pool->pages��ǰ���ʼ��  
�����ڴ����ʵ��ַ��ʼ������:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(����ʵ�ʵ����ݲ��֣�
ÿ��ngx_pagesize����ǰ���һ��ngx_slab_page_t���й�������ÿ��ngx_pagesize��ǰ�˵�һ��obj��ŵ���һ�����߶��int����bitmap�����ڹ���ÿ������ȥ���ڴ�)
*/
    for (i = 0; i < n; i++) { //��9��slots[]��9 * sizeof(ngx_slab_page_t)ָ��
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(ngx_slab_page_t); //����������Щslab page  

    //**��������ռ��ܹ����Է���Ļ���ҳ(4KB)��������ÿ��ҳ��overhead��һ��slab page�Ĵ�С  
    //**�����overhead��������֮���<128B��������bitmap�����  

    //���� + sizeof(ngx_slab_page_t)��ԭ����ÿ��ngx_pagesize���ж�Ӧ��ngx_slab_page_t���й���
    pages = (ngx_uint_t) (size / (ngx_pagesize + sizeof(ngx_slab_page_t))); //�����size�ǲ���Ӧ�ð�ͷ��n * sizeof(ngx_slab_page_t)��ȥ�������������׼ȷ?
    //��ÿ������ҳ��ǰ���sizeof(ngx_slab_page_t)�ֽڶ�Ӧ��slab page��0  
    ngx_memzero(p, pages * sizeof(ngx_slab_page_t));

    pool->pages = (ngx_slab_page_t *) p;

    //��ʼ��free��free.next���´η���ҳʱ������  
    pool->free.prev = 0;
    pool->free.next = (ngx_slab_page_t *) p;

    //���µ�һ��slab page��״̬�����slab��Ա��¼��������������ҳ��Ŀ  
    pool->pages->slab = pages; //��һ��pages->slabָ���˹����ڴ��г�ȥͷ����ʣ��ҳ�ĸ���
    pool->pages->next = &pool->free;
    pool->pages->prev = (uintptr_t) &pool->free;

    //ʵ�ʻ�����(ҳ)�Ŀ�ͷ������   //��Ϊ�����ԭ��,ʹ��m_page�������������֮�������Щ�ڴ��޷�ʹ��,  
    pool->start = (u_char *)
                  ngx_align_ptr((uintptr_t) p + pages * sizeof(ngx_slab_page_t),
                                 ngx_pagesize);

    //����ʵ�ʻ������Ŀ�ʼ�ͽ�β�ٴθ����ڴ�ҳ����Ŀ  
    //�����ڴ�������(pool->start���ڴ����),���ܵ���pages����,  
    //����Ҫ����.mΪ������pageҳ��ļ�С��.  
    //��ʵ���漸�д�����൱��:  
    // pages =(pool->end - pool->start) / ngx_pagesize  
    //pool->pages->slab = pages;  
    m = pages - (pool->end - pool->start) / ngx_pagesize;
    if (m > 0) {
        pages -= m;
        pool->pages->slab = pages;
    }

    //����pages * sizeof(ngx_slab_page_t)��Ҳ����ָ��ʵ�ʵ�����ҳpages*ngx_pagesize
    pool->last = pool->pages + pages;

    pool->log_nomem = 1;
    pool->log_ctx = &pool->zero;
    pool->zero = '\0';
}

//�����ǹ����ڴ棬�����ڽ��̼���Ҫ����������ͬ��
void *
ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size)
{
    void  *p;

    ngx_shmtx_lock(&pool->mutex);

    p = ngx_slab_alloc_locked(pool, size);

    ngx_shmtx_unlock(&pool->mutex);

    return p;
}

/*
���ڸ���size,��slab_pool�з����ڴ�.
1.���size���ڵ���һҳ,��ô��m_page�в���,�������ֱ�ӷ���,����ʧ��.
2.���sizeС��һҳ,����������п���slot��.
     (1).���size����ngx_slab_exact_size,
a.����slab�ĸ�16λ(32λϵͳ)���solt��Ӧ��map,���Ҹö�ӦΪmap�ĵ�λ��Ӧpage�и�λ��slot��.����1110��ӦΪ��1��slot�ǿ��õ�,2-4�鲻����.slab�ĵ�16Ϊ�洢slot���С��λ��.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_BIG.
c.���page��ȫ��slot����ʹ����,��ô����ҳ���m_slot����Ԫ�ص��������Ƴ�.
   (2).���size����ngx_slab_exact_size
a.����slab�洢slot��map,ͬ��slab�еĵ�λ��Ӧ��λ�õ�slot.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_EXACT.
c.���page��ȫ��slot����ʹ����,��ô����ҳ���m_slot����Ԫ�ص��������Ƴ�.
   (3).���sizeС��ngx_slab_exact_size
a.��page�е�ǰ����slot���slot��map,ͬ����λ��Ӧ��λ.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_SMALL.
b.���page��ȫ��slot����ʹ����,��ô����ҳ���m_slot����Ԫ�ص��������Ƴ�.
 3.���������û�п����slot��,����free�������ҵ�һ�����е�ҳ������m_slot����Ԫ���е�����.
   (1).���size����ngx_slab_exact_size,
a.����slab�ĸ�16λ(32λϵͳ)���solt��Ӧ��map,���Ҹö�ӦΪmap�ĵ�λ��Ӧpage�и�λ��slot��.slab�ĵ�16Ϊ�洢slot���С��λ��.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_BIG.
c.�������ҳ������m_slot����Ԫ�ص�������.
   (2).���size����ngx_slab_exact_size
a.����slab�洢slot��map,ͬ��slab�еĵ�λ��Ӧ��λ�õ�slot.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_EXACT.
c.�������ҳ������m_slot����Ԫ�ص�������.
   (3).���sizeС��ngx_slab_exact_size
a.��page�е�ǰ����slot���slot��map,ͬ����λ��Ӧ��λ.
b.����m_pageԪ�ص�preָ��ΪNGX_SLAB_SMALL.
c.�������ҳ������m_slot����Ԫ�ص�������.
4.�ɹ��򷵻ط�����ڴ��,��ָ��p,����ʧ��,����null.

*/
//ͼ�λ����ο�:http://blog.csdn.net/u013009575/article/details/17743261
//���ص�ֵ����Ҫ����Ŀռ����ڴ滺������λ��  

/*
��һ��pageҳ�л�ȡС��2048��obj���ʱ�򣬶�������page->next = &slots[slot]; page->prev = &slots[slot]��slots[slot].next = page;Ҳ������Ϊobj���ҳ
page[]����Ͷ�Ӧ��slots[]����������Ƿ������2048�Ŀռ䣬����������ҳ����page[]��slots��û�й�ϵ
����pageҳ�����������°�page[]��next��prev��ΪNULL��ͬʱ�Ѷ�Ӧ��slot[]��next��prevָ��slot[]����
��page������ͷ�����һ��obj���лָ�Ϊpage->next = &slots[slot]; page->prev = &slots[slot]��slots[slot].next = page;
*/
void * 
ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size)
{ //�������page_size��4KB  
    size_t            s;
    uintptr_t         p, n, m, mask, *bitmap;
    ngx_uint_t        i, slot, shift, map;
    ngx_slab_page_t  *page, *prev, *slots;

    if (size > ngx_slab_max_size) { //�����large obj, size >= 2048B  

        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0,
                       "slab alloc: %uz", size);

        //����1�������ڴ�ҳ  
        page = ngx_slab_alloc_pages(pool, (size >> ngx_pagesize_shift)
                                          + ((size % ngx_pagesize) ? 1 : 0)); //����size�պ���4K,��page=1,�����4K+1����page=2
        if (page) {
            //���page�����page[0]��ƫ��������m_page��page�������໥��Ӧ��,��m_page[0]����page[0]ҳ��,m_page[1]����page[1]ҳ��.  
            //���Ի��page�����m_page[0]��ƫ�����Ϳ��Ը���start�õ���Ӧҳ���ƫ����.  
            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start; //�õ�ʵ�ʷ����ҳ����ʼ��ַ

        } else {
            p = 0;
        }

        goto done;
    }

    //��С��obj, size < 2048B������Ҫ�����size��ȷ����slots��λ�ã�ÿ��slot���һ�ִ�С��obj�ļ��ϣ���slots[0]��ʾ8byte�Ŀռ䣬
    //slots[3]��ʾ64byte�Ŀռ����obj��С(<1B)��slot��λ����1B�ռ��λ�ã�����С����1B  
    if (size > pool->min_size) { //����ʹ���ĸ�slots[]��Ҳ������Ҫ����Ŀռ��Ƕ���  ����size=9,���ʹ��slot[1]��Ҳ����16�ֽ�
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    //ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t) + pages * sizeof(ngx_slab_page_t) +pages*ngx_pagesize(����ʵ�ʵ����ݲ���)
    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0,
                   "slab alloc: %uz slot: %ui", size, slot);
                   
    //ָ��9 * sizeof(ngx_slab_page_t) ��Ҳ����slots[0-8]���� = 8 - 2048
    slots = (ngx_slab_page_t *) ((u_char *) pool + sizeof(ngx_slab_pool_t));
    //��ngx_slab_init��slots[]->nextĬ����ָ���Լ���
    page = slots[slot].next;//����m_slot[slot]�����Ӧ��m_pageԪ��,Ȼ���ҵ���Ӧҳ��.  

    if (page->next != page) { //���֮ǰ�Ѿ��д����Сobj���Ǹ��Ѿ�������ڴ滺��ҳ��δ��  9 ��ngx_slab_page_t����û������

        if (shift < ngx_slab_exact_shift) { //Сobj��size < 128B�������ڴ滺��ҳ�е�bitmap�������ش�����Ŀռ��ڻ����λ��  

            do {
                p = (page - pool->pages) << ngx_pagesize_shift; 
                bitmap = (uintptr_t *) (pool->start + p);//pool->start��ʼ��128�ֽ�Ϊ��Ҫ����Ŀռ䣬������һ��int 4�ֽڿռ�����bitmap

                /*  
               ����Ҫ�����sizeΪ54�ֽڣ�����ǰ��������shift��Ӧ���ֽ���Ӧ����64�ֽڣ�����һ��ҳ��ȫ��64�ֽ�obj��С������һ����64
               ��64�ֽڵ�obj��64��obj������Ҫ64λ����ʾÿ��obj�Ƿ���ʹ�ã����������Ҫ64λ(Ҳ����8�ֽڣ�2��int),��������Ҫ����һ��64
               �ֽ�obj���洢��bitmap��Ϣ����һ��64�ֽ�objʵ����ֻ����8�ֽڣ�����56�ֽ�δ��
               */
                //������Ҫ���ٸ��ֽ����洢bitmap  
                map = (1 << (ngx_pagesize_shift - shift))
                          / (sizeof(uintptr_t) * 8);

                for (n = 0; n < map; n++) {

                    if (bitmap[n] != NGX_SLAB_BUSY) {//���page��obj�����,Ҳ����bitmapָ���32��obj�Ƿ��Ѿ��������ȥ��  
                        //�������pageҳ������slab�Ѿ����꣬����ں����ngx_slab_alloc_pages���»�ȡ�ռ䲢����slab��Ȼ�����
                        for (m = 1, i = 0; m; m <<= 1, i++) {//���obj�鱻ռ��,��������  ����32��obj���ҳ���һ��δ�������ȥ��obj
                            if ((bitmap[n] & m)) {
                                continue;
                            }

                            //�ҵ��ˣ���bitmap��Ӧ�ĵ�n��(ע����λ�Ʋ������m)δʹ�ã�ʹ������ͬʱ��λ��λ����ʾ��bitmp[n]�Ѿ������ٱ����䣬��Ϊ�Ѿ����η����ȥ��
                            bitmap[n] |= m;

                            i = ((n * sizeof(uintptr_t) * 8) << shift)
                                + (i << shift); //��bit����������bitmap�еĵڼ�λ(������Ҫ3��bitmap��ʾ���е�slab�飬�������ǵ�����bitmap�ĵ�һλ����i=32+32+1-1,bit��0��ʼ)

                            if (bitmap[n] == NGX_SLAB_BUSY) { //�����32λͼ�����ȡ������31λ(0-31)��ʱ��ǰ���bitmap[n] |= m��;ʹ��պ�NGX_SLAB_BUSY��Ҳ����λͼ����
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != NGX_SLAB_BUSY) {//�����bitmap����ļ���bitmap��û�����꣬��ֱ�ӷ��ظ�bitmap��ַ
                                         p = (uintptr_t) bitmap + i;

                                         goto done;
                                     }
                                }
                                //& ~NGX_SLAB_PAGE_MASK�����ԭ������Ҫ�ָ�ԭ���ĵ�ַ����Ϊ����λ�ڵ�һ�λ�ȡ�ռ��ʱ�������������崦��
                                prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK); //Ҳ���Ǹ�obj��Ӧ��slot_m[]

                                //pages_m[i]��slot_m[i]ȡ����Ӧ�����ù�ϵ����Ϊ��pages_m[i]ָ���ҳpage�Ѿ�������
                                prev->next = page->next;
                                page->next->prev = page->prev; //slot_m[i]�ṹ��next��prevָ���Լ�

                                page->next = NULL; //page��next��prevָ��NULL����ʾ���ٿ���������slot[]��Ӧ��obj
                                page->prev = NGX_SLAB_SMALL;
                            }

                            p = (uintptr_t) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == ngx_slab_exact_shift) {
            //size == 128B����Ϊһ��ҳ���Է�32������slab page��slab��Ա����עÿ���ڴ��ռ�����������Ҫ�������ڴ滺��������bitmap��
            //�����ش�����Ŀռ��ڻ����λ��  

            do {
                if (page->slab != NGX_SLAB_BUSY) { //��page�Ƿ��Ѿ�����

                    for (m = 1, i = 0; m; m <<= 1, i++) { //�������pageҳ������slab�Ѿ����꣬����ں����ngx_slab_alloc_pages���»�ȡ�ռ䲢����slab��Ȼ�����
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m; //��ǵ�m��slab���ڷ����ȥ��

                        if (page->slab == NGX_SLAB_BUSY) {//ִ����page->slab |= m;���п���page->slab == NGX_SLAB_BUSY����ʾ���һ��obj�Ѿ������ȥ��
                            //pages_m[i]��slot_m[i]ȡ����Ӧ�����ù�ϵ����Ϊ��pages_m[i]ָ���ҳpage�Ѿ�������
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_EXACT;
                        }

                        p = (page - pool->pages) << ngx_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start; //���ظ�obj��Ӧ�ĵ�ַ

                        goto done; 
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > ngx_slab_exact_shift */

   //size > 128B��Ҳ�Ǹ���slab page��slab��Ա��������ҪԤ������slab�Ĳ���bit����Ϊһ��ҳ��obj����С��32���������ش�����Ŀռ��ڻ����λ��  
            n = ngx_pagesize_shift - (page->slab & NGX_SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((uintptr_t) 1 << n) - 1;
            mask = n << NGX_SLAB_MAP_SHIFT;

            do {//�������pageҳ������slab�Ѿ����꣬����ں����ngx_slab_alloc_pages���»�ȡ�ռ䲢����slab��Ȼ�����
                if ((page->slab & NGX_SLAB_MAP_MASK) != mask) {

                    for (m = (uintptr_t) 1 << NGX_SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & NGX_SLAB_MAP_MASK) == mask) {
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_BIG;
                        }

                        p = (page - pool->pages) << ngx_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    //�ֳ�һҳ���뵽m_slot�����ӦԪ����  
    page = ngx_slab_alloc_pages(pool, 1);

    /*  
       ����Ҫ�����sizeΪ54�ֽڣ�����ǰ��������shift��Ӧ���ֽ���Ӧ����64�ֽڣ�����һ��ҳ��ȫ��64�ֽ�obj��С������һ����64
       ��64�ֽڵ�obj��64��obj������Ҫ64λ����ʾÿ��obj�Ƿ���ʹ�ã����������Ҫ64λ(Ҳ����8�ֽڣ�2��int),��������Ҫ����һ��64
       �ֽ�obj���洢��bitmap��Ϣ����һ��64�ֽ�objʵ����ֻ����8�ֽڣ�����56�ֽ�δ��
     */
    if (page) {
        //size<128
        if (shift < ngx_slab_exact_shift) {
            p = (page - pool->pages) << ngx_pagesize_shift;//slot���map�洢��page��slot�ж�λ����Ӧ��page  
            bitmap = (uintptr_t *) (pool->start + p); //pageҳ����ʼ��ַ��һ��uintptr_t����4�ֽ������洢bitmap��Ϣ

            /*  
               ����Ҫ�����sizeΪ54�ֽڣ�����ǰ��������shift��Ӧ���ֽ���Ӧ����64�ֽڣ�����һ��ҳ��ȫ��64�ֽ�obj��С������һ����64
               ��64�ֽڵ�obj��64��obj������Ҫ64λ����ʾÿ��obj�Ƿ���ʹ�ã����������Ҫ64λ(Ҳ����8�ֽڣ�2��int),��������Ҫ����һ��64
               �ֽ�obj���洢��bitmap��Ϣ����һ��64�ֽ�objʵ����ֻ����8�ֽڣ�����56�ֽ�δ��
               */
            s = 1 << shift;//s��ʾÿ��slot��Ĵ�С,�ֽ�Ϊ��λ  
            n = (1 << (ngx_pagesize_shift - shift)) / 8 / s;  //����bitmap��Ҫ���ٸ�slot obj��  
        
            if (n == 0) {
                n = 1; //������Ҫһ��4Mҳ���С��һ��obj(2<<shift�ֽ�)���洢bitmap��Ϣ
            }

            //���ö�Ӧ��slot���map,���ڴ��map��slot����Ϊ1,��ʾ��ʹ�ò������õ�һ�����õ�slot��(�������ڼ�¼map��slot��)
            //���Ϊ1,��Ϊ���μ���ʹ��.
            bitmap[0] = (2 << n) - 1; //�����ǻ�ȡ��ҳ�ĵڶ���obj����Ϊ��һ���Ѿ�����bitmap�ˣ����Ե�һ���͵ڶ����������ʾ���ã�bitmap[0]=3

            //��������obj��λͼ��Ҫ���ٸ�uintptr_t�洢������ÿ��obj��СΪ64�ֽڣ���4K������64��obj��Ҳ����Ҫ8�ֽڣ�����bitmap
            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0; //�ӵڶ���bitmap��ʼ������λ����0
            }

            page->slab = shift; //��ҳ��һ��obj��Ӧ���ֽ���λ����С
            /* ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize */
            page->next = &slots[slot];//ָ�������slots_m[i],����obj��С64�ֽڣ���ָ��slots[2]   slots[0-8] -----8-2048
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL; //��Ǹ�ҳ�д洢����С��128��obj

            slots[slot].next = page;

            p = ((page - pool->pages) << ngx_pagesize_shift) + s * n;
            //���ض�Ӧ��ַ.  ����Ϊ64�ֽ�obj���򷵻ص�startΪ�ڶ�����ʼ��obj���´η���ӵڶ�����ʼ��ȡ��ַ�ռ�obj
            p += (uintptr_t) pool->start;//���ض�Ӧ��ַ.,

            goto done;

        } else if (shift == ngx_slab_exact_shift) {

            page->slab = 1;//slab����Ϊ1   page->slab�洢obj��bitmap,��������Ϊ1����ʾ˵��һ��obj�����ȥ�� 4K��32��128�ֽ�obj,���һ��slabλͼ�պÿ��Ա�ʾ��32��obj
            page->next = &slots[slot];
            //��ָ��ĺ���λ�����,��NGX_SLAB_SMALL��ʾslot��С��ngx_slab_exact_shift  
            /*
                ����slab�ĸ�16λ(32λϵͳ)���solt��Ӧ��map,���Ҹö�ӦΪmap�ĵ�λ��Ӧpage�и�λ��slot��.slab�ĵ�16Ϊ�洢slot���С��λ��.
               */ 
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT; 

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;//���ض�Ӧ��ַ.  

            goto done;

        } else { /* shift > ngx_slab_exact_shift */

            //����128��Ҳ��������256�,4K���Ҳ��16��256�����ֻ��Ҫslab�ĸ�16λ��ʾobjλͼ����
            page->slab = ((uintptr_t) 1 << NGX_SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            //��ָ��ĺ���λ�����,��NGX_SLAB_BIG��ʾslot�����ngx_slab_exact_shift  
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab alloc: %p", p);

    return (void *) p;
}


void *
ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size)
{
    void  *p;

    ngx_shmtx_lock(&pool->mutex);

    p = ngx_slab_calloc_locked(pool, size);

    ngx_shmtx_unlock(&pool->mutex);

    return p;
}


void *
ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size)
{
    void  *p;

    p = ngx_slab_alloc_locked(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


void
ngx_slab_free(ngx_slab_pool_t *pool, void *p)
{
    ngx_shmtx_lock(&pool->mutex);

    ngx_slab_free_locked(pool, p);

    ngx_shmtx_unlock(&pool->mutex);
}

/*
���ݸ�����ָ��p,�ͷ���Ӧ�ڴ��.
1.�ҵ�p��Ӧ���ڴ��Ͷ�Ӧ��m_page����Ԫ��,
2.����m_page����Ԫ�ص�preָ��ȷ��ҳ������
     (1).���NGX_SLAB_SMALL����,��sizeС��ngx_slab_exact_size
a.������Ӧslot��Ϊ����
b.���������ҳ�涼����,��ҳ�����free��
    (2).���NGX_SLAB_EXACT����,��size����ngx_slab_exact_size
a.������Ӧslot��Ϊ����
b.���������ҳ�涼����,��ҳ�����free��
    (3).���NGX_SLAB_BIG����,��size����ngx_slab_exact_size
a.������Ӧslot��Ϊ����
b.���������ҳ�涼����,��ҳ�����free��
     (4).���NGX_SLAB_PAGE����,��size��С���ڵ���һ��ҳ��
a.������Ӧҳ���Ϊ����
b.��ҳ�����free��
*/
void
ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p)
{
    size_t            size;
    uintptr_t         slab, m, *bitmap;
    ngx_uint_t        n, type, slot, shift, map;
    ngx_slab_page_t  *slots, *page;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab free: %p", p);

    if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
        ngx_slab_error(pool, NGX_LOG_ALERT, "ngx_slab_free(): outside of pool");
        goto fail;
    }

    //����p�ҵ���Ҫ�ͷŵ�m_pageԪ��  
    n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab; //��������ʱ��һ���Է�����page�����һ��page��slabָ������һ���Է����˶��ٸ�ҳpage
    //��pre�ĵ���λ���жϸ�ҳ���е�slot��С��ngx_slab_exact_size�Ĵ�С��ϵ  
    type = page->prev & NGX_SLAB_PAGE_MASK;

    switch (type) {

    case NGX_SLAB_SMALL://slotС��ngx_slab_exact_size  

        //��obj��С��ƫ��  
        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;//�����obj�Ĵ�С  

        if ((uintptr_t) p & (size - 1)) {//���ڶ���,p�ĵ�ַһ����obj��С��������  
            goto wrong_chunk;
        }

        n = ((uintptr_t) p & (ngx_pagesize - 1)) >> shift;//���p��Ӧ��obj���λ��  
        m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));////�����uintptr_t��,p��ƫ��,�������uintptr_t�еĵڼ�λ  
        //����map�Ǹ���uintptr_t���ֵ�,��������ÿ��Ӧ��uintptr_t��ƫ��,������ڼ���uintptr_t  
        n /= (sizeof(uintptr_t) * 8); //��obj���ڵ����Ǹ�bitmap������һ��ҳ64��obj������Ҫ2��bitmap��ʾ��64��obj��λͼ
        bitmap = (uintptr_t *)
                             ((uintptr_t) p & ~((uintptr_t) ngx_pagesize - 1));//���p��Ӧ��pageҳ����λͼ��λ��  

        if (bitmap[n] & m) {//�����mλȷʵΪ1,  

            if (page->next == NULL) { //���ҳ��ĵ�ǰ״̬��ȫ����ʹ��,�Ͱ�������slot_m[]��
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));//��λslot_m����  

                //�ҵ���Ӧ��slot_m[]��Ԫ��  
                slot = shift - pool->min_shift;

                //�����Ӧ��slot[]�У���ʾ��ҳ���Լ���ʹ���ˣ���ngx_slab_calloc_locked�ֿ��Ա�������ҳ�����з���obj
                page->next = slots[slot].next;
                slots[slot].next = page;

                //����m_page��pre  
                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL;
                page->next->prev = (uintptr_t) page | NGX_SLAB_SMALL;
            }

             //ҳ��ĵ�ǰ״̬�ǲ�����ʹ��,���Ѿ���slot��  ����slot��Ӧλ��Ϊ����,��0  
            bitmap[n] &= ~m;

            //����Ĳ�����Ҫ�ǲ鿴���ҳ���Ƿ�û��, �����ûʹ��,��ҳ�����free��  
            
            n = (1 << (ngx_pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {//������Ҫ���ٸ�4�ֽ�bitmap���洢λͼ������64��obj��Ҫ2��bitmap����ʾ64λ,��Ҫ���ٸ�obj���洢������bitmap��һ���һ��obj���洢����bitmap��Ҳ������64�ֽ��е�8�ֽ�
                n = 1;
            }

            //�鿴��һ��uintptr_t�еĿ�obj�Ƿ񶼿���  
            if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
                goto done;
            }

             //����λͼʹ���˶��ٸ�uintptr_t���洢
            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (n = 1; n < map; n++) {//�鿴����uintptr_t�Ƿ�ûʹ��  
                if (bitmap[n]) {
                    goto done;
                }
            }

            ngx_slab_free_pages(pool, page, 1); //����ҳ�涼û��ʹ�ã��黹��free 

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_EXACT:
        //p����Ӧ����slab(objλͼ)�е�λ��.  
        m = (uintptr_t) 1 <<
                (((uintptr_t) p & (ngx_pagesize - 1)) >> ngx_slab_exact_shift);
        size = ngx_slab_exact_size;

        if ((uintptr_t) p & (size - 1)) {//���pΪpage�е�obj��,��ôһ����size������  
            goto wrong_chunk;
        }

        if (slab & m) {//slab(λͼ)�ж�Ӧ��λΪ1  
            if (slab == NGX_SLAB_BUSY) {//�������ҳ���е�����obj�鶼��ʹ��,���ҳpage[]��slot[]û�ж�Ӧ��ϵ,�����Ҫ��ҳpage[]��slot[]��Ӧ��ϵ����
                 //��λslot[]����  
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = ngx_slab_exact_shift - pool->min_shift;//�����ҳ��Ҫ����slot[]���ĸ�����  

                //����page[]Ԫ�غ�slot[]�Ķ�Ӧ��ϵ��ͨ��prev��nextָ��
                page->next = slots[slot].next;
                slots[slot].next = page;
                
                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;
                page->next->prev = (uintptr_t) page | NGX_SLAB_EXACT;
            }

            page->slab &= ~m;//��slab���Ӧλ������Ϊ0  

            if (page->slab) {//pageҳ���л�������ʹ�õ�obj��,��Ϊslabλͼ��Ϊ0
                goto done;
            }

            ngx_slab_free_pages(pool, page, 1);//pageҳ��������slab�鶼û��ʹ��  

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_BIG://�û������ڴ�Ĵ�С����ngx_slab_exact_size  
        //slab�ĸ�16λ��slot���λͼ,��16λ���ڴ洢slot���С��ƫ��  

        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }
        //�ҵ���slab����λͼ�е�λ��.����Ҫע��һ��,  
        //λͼ�洢��slab�ĸ�16λ,����Ҫ+16(��+ NGX_SLAB_MAP_SHIFT)  
        m = (uintptr_t) 1 << ((((uintptr_t) p & (ngx_pagesize - 1)) >> shift)
                              + NGX_SLAB_MAP_SHIFT);

        if (slab & m) {//��slab��ȷʵ���ڱ�ʹ��  

            if (page->next == NULL) {//�������ҳ���е�����obj�鶼��ʹ��,���ҳpage[]��slot[]û�ж�Ӧ��ϵ,�����Ҫ��ҳpage[]��slot[]��Ӧ��ϵ����
                //��λslot[]����  
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = shift - pool->min_shift;
                //�ҵ�slot[]�����ж�Ӧ��λ��,���slot[]��page[]�Ķ�Ӧ��ϵ
                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;
                page->next->prev = (uintptr_t) page | NGX_SLAB_BIG;
            }

            page->slab &= ~m;//����slab���Ӧ��λͼλ��Ϊ0,������  
            
            //���slabҳ����slot�黹�ڱ�ʹ��  
            if (page->slab & NGX_SLAB_MAP_MASK) {
                goto done;
            }

            //���pageҳ������slab�鶼����ʹ�þͽ���ҳ������free��  
            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_PAGE://�û��黹����ҳ��  

        if ((uintptr_t) p & (ngx_pagesize - 1)) { //p��Ҳ����ģ������
            goto wrong_chunk;
        }

        if (slab == NGX_SLAB_PAGE_FREE) { 
        
            ngx_slab_error(pool, NGX_LOG_ALERT,
                           "ngx_slab_free(): page is already free");
            goto fail;
        }

        if (slab == NGX_SLAB_PAGE_BUSY) {
        //˵��������������page�ķ��׸�page������ֱ���ͷţ������⼸��pageһ���ͷţ����pָ��ָ���������page
            ngx_slab_error(pool, NGX_LOG_ALERT,
                           "ngx_slab_free(): pointer to wrong page");
            goto fail;
        }

        //����ҳ���Ӧ��page[]��  
        n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
        size = slab & ~NGX_SLAB_PAGE_START;//����黹page�ĸ���  

        ngx_slab_free_pages(pool, &pool->pages[n], size); //�黹ҳ��  

        ngx_slab_junk(p, size << ngx_pagesize_shift);

        return;
    }

    /* not reached */

    return;

done:

    ngx_slab_junk(p, size);

    return;

wrong_chunk:

    ngx_slab_error(pool, NGX_LOG_ALERT,
                   "ngx_slab_free(): pointer to wrong chunk");

    goto fail;

chunk_already_free:

    ngx_slab_error(pool, NGX_LOG_ALERT,
                   "ngx_slab_free(): chunk is already free");

fail:

    return;
}

/*
����һ��slab page�����slab page֮��ᱻ����ȷ���������Ŀռ����ڴ滺���λ�� 

�����ܹ���6��page[]��ngx_slab_init��page[0]��next��prev��ָ��free��free��nextҲָ��page[0]��������ngx_slab_alloc_pages���ȡ3��pages��ʱ��
��ǰ����pages(page[0], page[1], page[2])�ᱻ����ã���ĩβpage[5]��prevָ��page[3],����page[3]��slabָ������ֻ��6-3=3��page���Է����ˣ�
Ȼ��page[3]��next��prevָ��free,free��next��prevҲָ��page[3]��Ҳ�����´�ֻ�ܴ�page[3]��ʼ��ȡҳ

*/ //����һ��ҳ��,����ҳ���free��ժ��.

/* 
-------------------------------------------------------------------
| page1  | page2 | page3 | page4| page5) | page6 | page7 | page8 |
--------------------------------------------------------------------
��ʼ״̬: pool->freeָ��page1,page1ָ��poll->free,������page��next��prevĬ��ָ��NULL,Ҳ����pool->freeָ������page�壬page1->slab=8

1.�����һ��ngx_slab_alloc_pages��ȡ����page����page1��page2������ȥ��page1Ϊ������page����page1->slabֻ������������page��һ������,
    page2->slab = NGX_SLAB_PAGE_BUSY;��ʾ���Ǹ���page1һ������ȥ�ģ����ұ�page������page����ʱ��pool->freeָ��page3,����ʾpage3->slab=6��
    ��ʾpage3��ʼ����6��page����ʹ��
2.����ڶ����ֻ�ȡ��3��page(page3-5),��page3����page,page3->slab=3,ͬʱpage4,page5->slab = NGX_SLAB_PAGE_BUSY;��ʾ���Ǹ���page1һ
    ������ȥ�ģ����ұ�page������page����ʱ��pool->freeָ��page6,����ʾpage6->slab=3����ʾpage6��ʼ����3��page����ʹ��
3. ͬ���ٻ�ȡ1��page,page6����ʱ��pool->freeָ��page7,����ʾpage7->slab=2����ʾpage7��ʼ����2��page����ʹ��
4. �����ͷŵ�1��page1��ʼ��2��page������ngx_slab_free_pages�л�ѵ�3����ʣ�������δ�����page7(ʵ�����ǰ�page7��ʼ��2��page��ʶΪ1����page)
   ��page1(page1ʵ������ʱ���ʶ���ǵ�һ���е�page1��ʼ��2��page)��pool->free�γ�һ��˫�����������Լ�
  
                      pool->free  
                    ----------  /                 
 -----------------\|         | --------------------------------------  
|  ----------------|         | -----------------------------------   |
|  |                 -----------                                  |  |
|  |                                                              |  |
|  |     page1 2                                   page7 2        |  |
|  |  \ ----------                          \  -----------   /    |  |
|   --- |    |    |----------------------------|    |     | -------  | 
------- |    |    |--------------------------- |    |     | ---------- 
        ----------  \                           ---------

5.���ͷ�ngx_slab_free_pagespage3��ʼ��3��pageҳ��page3Ҳ�����ӵ�˫�򻷱��У�����pool->free��page1[i]֮�䣬ע����ʱ���page1��page2�ǽ���һ���page
  ��û�ж����ǽ��кϲ���page1->slab����=2  page2->slab����=3����û�а����Ǻϲ�Ϊһ������page->slab=5,����´���allocһ��4page�Ŀռ䣬��
  ���䲻�ɹ���
*/
static ngx_slab_page_t *
ngx_slab_alloc_pages(ngx_slab_pool_t *pool, ngx_uint_t pages) //��ngx_slab_free_pages����Ķ����
{
    ngx_slab_page_t  *page, *p;

    //��ʼ����ʱ��pool->free.nextĬ��ָ���һ��pool->pages
    //��pool->free.next��ʼ��ÿ��ȡ(slab page) page = page->next  
    for (page = pool->free.next; page != &pool->free; page = page->next) { //���һ������pageҳ��û�У��Ͳ������ѭ����

    /*
    ����slab pageʣ�µĻ���ҳ��Ŀ>=��Ҫ����Ļ���ҳ��Ŀpages����Է��䣬�����������free,ֱ����һ����page���������page���ʹ��ڵ�����Ҫ�����pages�����ſ��Է��� 
    slab���״η���page��ʼ��slab��ҳ��ʱ��ָ���ģ����ͷŵ�ʱ��slab�����״η���ʱ���slab������䣬Ҳ����˵�ͷ�page�󲻻�����ڵ�����pageҳ��slab���ϲ���
    �����״ο���page1��ʼ��3��pageҳ�ռ䣬page1->slab=3,�����ſ���page2��ʼ��2��pageҳ�ռ䣬page2->slab=2,�������ͷ�page1��page2��Ӧ�Ŀռ�����ǻ���
    ����������page[]�ռ䣬slab�ֱ���2��3,������������������ռ���кϲ�Ϊ1��(Ҳ�����µ�page3,page3�׵�ַ����page2������page3->slab=3+2=5)
    */
        if (page->slab >= pages) {

            //�����ܹ���6��page[]��ngx_slab_init��page[0]��next��prev��ָ��free��free��nextҲָ��page[0]��������ngx_slab_alloc_pages���ȡ3��pages��ʱ��
            //��ǰ����pages(page[0], page[1], page[2])�ᱻ����ã���ĩβpage[5]��prevָ��page[3],����page[3]��slabָ������ֻ��6-3=3��page���Է����ˣ�
            //Ȼ��page[3]��next��prevָ��free,free��next��prevҲָ��page[3]��Ҳ�����´�ֻ�ܴ�page[3]��ʼ��ȡҳ
            if (page->slab > pages) {  
                page[page->slab - 1].prev = (uintptr_t) &page[pages];

                //���´ӱ���slab page��ʼ���µ�pages��slab page�Ļ���ҳ��ĿΪ����slab page��Ŀ��ȥpages  
                //���´ο��Դ�page[pages]��ʼ�����ҳ��next��prevָ��pool->free,ֻҪҳ��next��prevָ����free�����ʾ���ԴӸ�ҳ��ʼ����ҳpage
                page[pages].slab = page->slab - pages; //�´ο�ʼ�����page[]����ʼ��ȡҳ���������ڿ���pageֻ��page->slab - pages����
                page[pages].next = page->next; //�ÿ���ҳ��nextָ��pool->free.next
                page[pages].prev = page->prev; //�ÿ���ҳ��prevָ��pool->free.next

                p = (ngx_slab_page_t *) page->prev;
                p->next = &page[pages];//����pool->free.next = &page[pages]���´δӵ�pages��slab page��ʼ���������for()ѭ������
                page->next->prev = (uintptr_t) &page[pages]; //poll->free->prev = &page[pages]ָ���´ο��Է���ҳ��ҳ��ַ

            } else { //pageҳ�������ˣ���free��next��prev��ָ���Լ��������´��ٽ���ú�������for()ѭ����ʱ���޷�����ѭ�����У�Ҳ���䲻��page
                //���freeָ���pageҳ����ҳ��СΪ2�����ú���Ҫ���ȡ3��ҳ����ֱ�Ӱ�������ҳ���س�ȥ������˵������ҳ����������ţ��ܱ�û�кá�
                p = (ngx_slab_page_t *) page->prev; //��ȡpoll->free
                p->next = page->next; //poll->free->next = poll->free
                page->next->prev = page->prev; ////poll->free->prev = poll->free   free��next��prev��ָ�����Լ���˵��û�ж���ռ������
            }

            //NGX_SLAB_PAGE_START���page�Ƿ����pages��ҳ�ĵ�һ��ҳ�����ڵ�һ��ҳpage�м�¼�����������pages��ҳ��һ������
            page->slab = pages | NGX_SLAB_PAGE_START; //���±������page slab�еĵ�һ����slab��Ա����ҳ�ĸ�����ռ�����  
            //page��next��prev���൱��ָ����NULL�ˣ�
            page->next = NULL; 
            page->prev = NGX_SLAB_PAGE; //pageҳ�治����slotʱ��,��������ҳ�������û�,pre�ĺ���λΪNGX_SLAB_PAGE

            if (--pages == 0) { //pagesΪ1����ֱ�ӷ��ظ�page
                return page;
            }

            for (p = page + 1; pages; pages--) {
                //��������ҳ��pages>1�����º���page slab��slab��ԱΪNGX_SLAB_PAGE_BUSY  
                p->slab = NGX_SLAB_PAGE_BUSY; 
                //�����������������page�������Ҳ�����page������һ�η���3��page,�����pageΪ[1-3]����page[1].slab=3  page[2].slab=page[3].slab=NGX_SLAB_PAGE_BUSY��¼
                p->next = NULL;
                p->prev = NGX_SLAB_PAGE;
                p++;
            }

            return page;
        }
    }

    if (pool->log_nomem) {
        ngx_slab_error(pool, NGX_LOG_CRIT,
                       "ngx_slab_alloc() failed: no memory");
    }

    //û���ҵ������ҳ  
    return NULL;
}

//�ͷ�pageҳ��ʼ��pages��ҳ��
static void
ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages) //�ͷ�pages��ҳ��,����ҳ�����free��
{
    ngx_uint_t        type;
    ngx_slab_page_t  *prev, *join;

    page->slab = pages--; //�ͷŵ�pagesҳ��-1������Ҫ����?

    if (pages) {
        ngx_memzero(&page[1], pages * sizeof(ngx_slab_page_t)); //���һ���Է����˴��ڵ���2��page������Ҫ����page�������page�ָ���0����
    }

    if (page->next) { 
        //���slot[]��page[]�Ĺ�������slot[]��next��prevָ��slot[]����
        prev = (ngx_slab_page_t *) (page->prev & ~NGX_SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }
    
    //����Ҫ�ͷŵ�ҳpages�����һ��page���п���һ���ͷ�3��ҳ����join���������ҳ����ʼ��ַ,���pagesΪ1����joinֱ��ָ������page
    join = page + page->slab; 

    if (join < pool->last) { //join����pool�����һ��page 
        type = join->prev & NGX_SLAB_PAGE_MASK;
        
        if (type == NGX_SLAB_PAGE) {//��ngx_slab_alloc_pages����Ķ���
            
            if (join->next != NULL) { //��ifӦ��ʼ�����㲻��???/  ��alloc��ʱ����ҳpage��next��ָ��NULL
                pages += join->slab;
                page->slab += join->slab;

                prev = (ngx_slab_page_t *) (join->prev & ~NGX_SLAB_PAGE_MASK);
                prev->next = join->next;
                join->next->prev = join->prev;

                join->slab = NGX_SLAB_PAGE_FREE;
                join->next = NULL;
                join->prev = NGX_SLAB_PAGE;
            }
        }
    }

    if (page > pool->pages) {
        join = page - 1;
        type = join->prev & NGX_SLAB_PAGE_MASK;

        if (type == NGX_SLAB_PAGE) {

            if (join->slab == NGX_SLAB_PAGE_FREE) {
                join = (ngx_slab_page_t *) (join->prev & ~NGX_SLAB_PAGE_MASK);
            }

            if (join->next != NULL) {
                pages += join->slab;
                join->slab += page->slab;

                prev = (ngx_slab_page_t *) (join->prev & ~NGX_SLAB_PAGE_MASK);
                prev->next = join->next;
                join->next->prev = join->prev;

                page->slab = NGX_SLAB_PAGE_FREE;
                page->next = NULL;
                page->prev = NGX_SLAB_PAGE;

                page = join;
            }
        }
    }

    if (pages) { //����һ��alloc 3��page��������page[3 4 5],��page[5]��prevָ��page[3]
        page[pages].prev = (uintptr_t) page;
    }

    /*
    freepage[i]Ϊ�����ͷŵ�freepage[i]��ʼ��pages��pageҳ
    unusepage[i]Ϊ��δ����ʹ�õ�ԭʼpageҳ��ʼ�������ܻ��ж��page[]�������
                          pool->free  
                        ----------  /                 
     -----------------\|         | --------------------------------------  
    |  ----------------|         | -----------------------------------   |
    |  |                 -----------                                  |  |
    |  |                                                              |  |
    |  |     freepage[i]                           unusepage[i]       |  |
    |  |  \ ----------                          \  -----------   /    |  |
    |   --- |    |    |----------------------------|    |     | -------  | 
    ------- |    |    |--------------------------- |    |     | ---------- 
            ----------  \                           ---------

    �ߵ������pool->freeָ���˸��ͷŵ�page[]��ע����ʱ����page->slab����

   
     */
    

    //��ngx_slab_alloc_pages����Ķ��� �����ͷź�ͻ��֮ǰfreeָ��Ŀ���pageҳ���ͷŵ�pageҳ�Լ�pool->free�γ�һ����������
    page->prev = (uintptr_t) &pool->free;
    page->next = pool->free.next;

    page->next->prev = (uintptr_t) page;

    pool->free.next = page;
}


static void
ngx_slab_error(ngx_slab_pool_t *pool, ngx_uint_t level, char *text)
{
    ngx_log_error(level, ngx_cycle->log, 0, "%s%s", text, pool->log_ctx);
}
