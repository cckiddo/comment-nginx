
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/*
To quickly process static sets of data such as server names, map directive��s values, MIME types, names of request header strings, nginx 
uses hash tables. During the start and each re-configuration nginx selects the minimum possible sizes of hash tables such that the bucket 
size that stores keys with identical hash values does not exceed the configured parameter (hash bucket size). The size of a table is 
expressed in buckets. The adjustment is continued until the table size exceeds the hash max size parameter. Most hashes have the 
corresponding directives that allow changing these parameters, for example, for the server names hash they are server_names_hash_max_size 
and server_names_hash_bucket_size. 

The hash bucket size parameter is aligned to the size that is a multiple of the processor��s cache line size. This speeds up key search in 
a hash on modern processors by reducing the number of memory accesses. If hash bucket size is equal to one processor��s cache line size 
then the number of memory accesses during the key search will be two in the worst case �� first to compute the bucket address, and second 
during the key search inside the bucket. Therefore, if nginx emits the message requesting to increase either hash max size or hash bucket 
size then the first parameter should first be increased. 
*/
void *
ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len)
{
    ngx_uint_t       i;
    ngx_hash_elt_t  *elt;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "hf:\"%*s\"", len, name);
#endif

    elt = hash->buckets[key % hash->size];

    if (elt == NULL) {
        return NULL;
    }

    while (elt->value) {
        if (len != (size_t) elt->len) {
            goto next;
        }

        for (i = 0; i < len; i++) {
            if (name[i] != elt->name[i]) {
                goto next;
            }
        }

        return elt->value;

    next:

        elt = (ngx_hash_elt_t *) ngx_align_ptr(&elt->name[0] + elt->len,
                                               sizeof(void *));
        continue;
    }

    return NULL;
}

/*
nginxΪ�˴������ͨ�����������ƥ�����⣬ʵ����ngx_hash_wildcard_t������hash��������֧���������͵Ĵ���ͨ�����������һ����ͨ�����ǰ�ģ�
���磺��*.abc.com����Ҳ����ʡ�Ե��Ǻţ�ֱ��д�ɡ�.abc.com����������key������ƥ��www.abc.com��qqq.www.abc.com֮��ġ�����һ����ͨ�����ĩ
β�ģ����磺��mail.xxx.*�������ر�ע��ͨ�����ĩβ�Ĳ���λ�ڿ�ʼ��ͨ������Ա�ʡ�Ե���������ͨ���������ƥ��mail.xxx.com��mail.xxx.com.cn��
mail.xxx.net֮���������

��һ�����˵��������һ��ngx_hash_wildcard_t���͵�hash��ֻ�ܰ���ͨ�����ǰ��key������ͨ����ں��key������ͬʱ�����������͵�ͨ���
��key��ngx_hash_wildcard_t���ͱ����Ĺ�����ͨ������ngx_hash_wildcard_init��ɵģ�����ѯ��ͨ������ngx_hash_find_wc_head����
ngx_hash_find_wc_tail�����ġ�ngx_hash_find_wc_head�ǲ�ѯ����ͨ�����ǰ��key��hash��ģ���ngx_hash_find_wc_tail�ǲ�ѯ����ͨ����ں��key��hash��ġ�
*/
void *
ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ngx_uint_t   i, n, key;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wch:\"%*s\"", len, name);
#endif

    n = len;
    
    //�Ӻ���ǰ������һ��dot����n �� len-1 ��Ϊ�ؼ��������һ�� �ӹؼ���
    while (n) { //name���������ַ������� AA.BB.CC.DD���������ȡ���ľ���DD
        if (name[n - 1] == '.') {
            break;
        }

        n--;
    }

    key = 0;
    
    //n �� len-1 ��Ϊ�ؼ��������һ�� �ӹؼ��֣�������hashֵ
    for (i = n; i < len; i++) {
        key = ngx_hash(key, name[i]);
    }

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "key:\"%ui\"", key);
#endif

    //������ͨ�����ҵ��ؼ��ֵ�value���û��Զ�������ָ�룩
    value = ngx_hash_find(&hwc->hash, key, &name[n], len - n);

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer for both "example.com"
         *          and "*.example.com";
         *     01 - value is data pointer for "*.example.com" only;
         *     10 - value is pointer to wildcard hash allowing
         *          both "example.com" and "*.example.com";
         *     11 - value is pointer to wildcard hash allowing
         *          "*.example.com" only.
         */

        if ((uintptr_t) value & 2) {

            if (n == 0) { //�����������һ���ӹؼ�����û��ͨ�������"example.com"��example

                /* "example.com" */

                if ((uintptr_t) value & 1) {//value����λΪ11����Ϊ"*.example.com"��ָ�룬����û��ͨ�����û�е�������NULL
                    return NULL;
                }
             //value����λΪ10��Ϊ"example.com"��ָ�룬value������һ����ngx_hash_wildcard_t ��value�У�ȥ��Я���ĵ�2λ11    �ο�ngx_hash_wildcard_init
                hwc = (ngx_hash_wildcard_t *)
                                          ((uintptr_t) value & (uintptr_t) ~3);
                return hwc->value;
            }

            //��δ�����꣬����λΪ11��10������ȥ�¼�ngx_hash_wildcard_t������
            hwc = (ngx_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3); //����͵�������ַΪ��0����ȥ���ο�ngx_hash_wildcard_init

            //�������� �ؼ�����ʣ�ಿ�֣���"example.com"������ 0 �� n -1 ��Ϊ example
            value = ngx_hash_find_wc_head(hwc, name, n - 1);

            if (value) {//���ҵ����򷵻�
                return value;
            }

            return hwc->value; //����λΪ00 �ҵ�����Ϊwc->value
        }

        if ((uintptr_t) value & 1) { //����λΪ01

            if (n == 0) { //�ؼ���û��ͨ��������󷵻ؿ�

                /* "example.com" */

                return NULL;
            }

            return (void *) ((uintptr_t) value & (uintptr_t) ~3);//��ͨ�����ֱ�ӷ���
        }

        return value; //����λΪ00��ֱ�ӷ���
    }

    return hwc->value;
}

/*
nginxΪ�˴������ͨ�����������ƥ�����⣬ʵ����ngx_hash_wildcard_t������hash��������֧���������͵Ĵ���ͨ�����������һ����ͨ�����ǰ�ģ�
���磺��*.abc.com����Ҳ����ʡ�Ե��Ǻţ�ֱ��д�ɡ�.abc.com����������key������ƥ��www.abc.com��qqq.www.abc.com֮��ġ�����һ����ͨ�����ĩ
β�ģ����磺��mail.xxx.*�������ر�ע��ͨ�����ĩβ�Ĳ���λ�ڿ�ʼ��ͨ������Ա�ʡ�Ե���������ͨ���������ƥ��mail.xxx.com��mail.xxx.com.cn��
mail.xxx.net֮���������

��һ�����˵��������һ��ngx_hash_wildcard_t���͵�hash��ֻ�ܰ���ͨ�����ǰ��key������ͨ����ں��key������ͬʱ�����������͵�ͨ���
��key��ngx_hash_wildcard_t���ͱ����Ĺ�����ͨ������ngx_hash_wildcard_init��ɵģ�����ѯ��ͨ������ngx_hash_find_wc_head����
ngx_hash_find_wc_tail�����ġ�ngx_hash_find_wc_head�ǲ�ѯ����ͨ�����ǰ��key��hash��ģ���ngx_hash_find_wc_tail�ǲ�ѯ����ͨ����ں��key��hash��ġ�

hinit: ����һ��ͨ���hash���һЩ������һ�����ϡ����ڸò�����Ӧ�����͵�˵������μ�ngx_hash_t������ngx_hash_init������˵���� 

names: �����hash������е�ͨ���key�����顣�ر�Ҫע����������key�Ѿ����Ǳ�Ԥ������ġ����磺��*.abc.com�����ߡ�.abc.com��
��Ԥ��������Ժ󣬱���ˡ�com.abc.��������mail.xxx.*����Ԥ����Ϊ��mail.xxx.����Ϊʲô�ᱻ�������������ﲻ�ò��򵥵�����һ��
ͨ���hash���ʵ��ԭ������������͵�hash���ʱ��ʵ�����ǹ�����һ��hash���һ������������ͨ��hash���е�key�����ӡ������ġ�
���磺���ڡ�*.abc.com�����ṹ���2��hash����һ��hash������һ��keyΪcom�ı���ñ����value������ָ��ڶ���hash���ָ�룬
���ڶ���hash������һ������abc���ñ����value������ָ��*.abc.com��Ӧ��value��ָ�롣��ô��ѯ��ʱ�򣬱����ѯwww.abc.com��ʱ��
�Ȳ�com��ͨ����com�����ҵ��ڶ�����hash���ڵڶ���hash���У��ٲ���abc���������ƣ�ֱ����ĳһ����hash���в鵽�ı����Ӧ��value��
Ӧһ��������ֵ����һ��ָ����һ��hash���ָ���ʱ�򣬲�ѯ���̽�����������һ����Ҫ�ر�ע��ģ�����names������Ԫ�ص�value����Ӧ��
ֵ��Ҳ����������value���ڵĵ�ַ���������ܱ�4�����ģ�����˵����4�ı����ĵ�ַ���Ƕ���ġ���Ϊ���value��ֵ�ĵ���λbit�����õģ�
���Ա���Ϊ0�����������������������hash���ѯ������ȷ����� 

nelts: names����Ԫ�صĸ����� 

*/
/* 
@hwc  ��ʾ֧��ͨ����Ĺ�ϣ��Ľṹ��   
@name ��ʾʵ�ʹؼ��ֵ�ַ  
@len  ��ʾʵ�ʹؼ��ֳ���   

ngx_hash_find_wc_tail��ǰ��ͨ������Ҳ�࣬����value����λ�������ֱ�־�����Ӽ򵥣�

00 - value ��ָ�� �û��Զ�������
11 - value��ָ����һ����ϣ�� 
*/
void *
ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ngx_uint_t   i, key;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wct:\"%*s\"", len, name);
#endif

    key = 0;

    //��ǰ����������һ��dot����0 �� i ��Ϊ�ؼ����е�һ�� �ӹؼ���
    for (i = 0; i < len; i++) {
        if (name[i] == '.') {
            break;
        }

        key = ngx_hash(key, name[i]); //�����ϣֵ
    }

    if (i == len) {  //û��ͨ���������NULL
        return NULL;
    }

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = ngx_hash_find(&hwc->hash, key, name, i); //������ͨ�����ҵ��ؼ��ֵ�value���û��Զ�������ָ�룩

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "value:\"%p\"", value);
#endif

    /*
    ���ǵ��Ͻ���ngx_hash_wildcard_init�У���valueָ���2λ��Я����Ϣ����������������ģ����£�  
    * 00 - value ������ָ��   
    * 11 - value��ָ����һ����ϣ��   
    */
    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer;
         *     11 - value is pointer to wildcard hash allowing "example.*".
         */

        if ((uintptr_t) value & 2) {//��2λΪ11��value��ָ����һ����ϣ���ݹ�����

            i++;

            hwc = (ngx_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = ngx_hash_find_wc_tail(hwc, &name[i], len - i);

            if (value) { //�ҵ�����λ00������
                return value;
            }

            return hwc->value; //�Ҵ����λ11������hwc->value
        }

        return value;
    }

    return hwc->value; //��2λΪ00��ֱ�ӷ�������
}

//��hash���в��Ҷ�Ӧ��key - name
void *
ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key, u_char *name,
    size_t len)
{
    void  *value;

    if (hash->hash.buckets) {  //����ͨhash���в���
        value = ngx_hash_find(&hash->hash, key, name, len);

        if (value) {
            return value;
        }
    }

    if (len == 0) {
        return NULL;
    }

    if (hash->wc_head && hash->wc_head->hash.buckets) { //��ǰ��ͨ�����ϣ���в���
        value = ngx_hash_find_wc_head(hash->wc_head, name, len);

        if (value) {
            return value;
        }
    }

    if (hash->wc_tail && hash->wc_tail->hash.buckets) { //�ں���ͨ�����ϣ���в���
        value = ngx_hash_find_wc_tail(hash->wc_tail, name, len);

        if (value) {
            return value;
        }
    }

    return NULL;
}

/*
NGX_HASH_ELT_SIZE����������ngx_hash_elt_t�ṹ��С���������¡�
��32λƽ̨�ϣ�sizeof(void*)=4��(name)->key.len����ngx_hash_elt_t�ṹ��name���鱣������ݵĳ��ȣ����е�"+2"��Ҫ���ϸýṹ��len�ֶ�(u_short����)�Ĵ�С��
*/
#define NGX_HASH_ELT_SIZE(name)                                               \
    (sizeof(void *) + ngx_align((name)->key.len + 2, sizeof(void *)))

/*
��names������ngx_hash_key_t�ṹ�����飬����-ֵ��<key,value>���飬nelts��ʾ������Ԫ�صĸ���

�ú�����ʼ���Ľ�����ǽ�names���鱣��ļ�-ֵ��<key,value>��ͨ��hash�ķ�ʽ���������Ӧ��һ������hashͰ(�������е�buckets)�У�
��hash�����õ���hash����һ��Ϊngx_hash_key_lc�ȡ�hashͰ�����ŵ���ngx_hash_elt_t�ṹ��ָ��(hashԪ��ָ��)����ָ��ָ��һ������
�����������������������д�ŵ��Ǿ�hash֮��ļ�-ֵ��<key',value'>����ngx_hash_elt_t�ṹ�е��ֶ�<name,value>��ÿһ������������
����ŵļ�-ֵ��<key',value'>������һ��������
*/ //ngx_hash_init��names�������hashͰǰ����ṹ��ngx_hash_key_t��ʽ������hashͰ��������ݵ�ʱ�򣬻��ngx_hash_key_t����ĳ�Ա������ngx_hash_elt_t����Ӧ��Ա
//Դ���룬�Ƚϳ����ܵ����̼�Ϊ��Ԥ����Ҫ��Ͱ���� �C> ������Ҫ��Ͱ����->����Ͱ�ڴ�->��ʼ��ÿһ��ngx_hash_elt_t
 /* nginx hash�ṹ����������: 
11.           hash�ṹ����N��Ͱ, ÿ��Ͱ���N��Ԫ��(��<k,v>),���ڴ���, 
12.        ��һ��ָ�������¼N��Ͱ�ĵ�ַ,ÿ��Ͱ����һ�� ngx_hash_elt_t ���� 
13.        ָ������ �� ngx_hash_elt_t ���� ��һ���������ڴ���. 
14.        �ŵ�: ʹ���������Ѱַ�ٶ� 
15.        ȱ��: hash���ʼ����,ֻ�ܲ�ѯ,�����޸�. 
16. 
17.        ��Ȼhash�ṹ��������������ʽ,�����ڳ�ͻ��Ԫ�ش������������ʽ���,�ٹ��ص�hash������. 
18.        nginx��hash�ṹ����: 
19.        ���� ÿ��Ͱ�Ŀռ��С�̶� ͨ�� ngx_hash_init_t.bucket_size ָ��; 
20.        Ȼ�� ����Ԫ�صĸ�����Ͱ�Ĺ̶���С�������Ҫ���ٸ�Ͱ. 
21.        Ȼ�� ������ЩԪ�ش�ŵ��ĸ�Ͱ��,�������� (Ԫ�ص�hashֵ % Ͱ�ĸ���) 
22.        ��ʱ ��Ҫ���ٸ�Ͱ,��ЩͰ��Ҫ�����ڴ�ռ�,ÿ��Ͱ��Ŷ���Ԫ�أ���Ҫ�����ڴ�ռ��֪��, 
23.        ��������Ͱ���ڴ�ռ�,��Ϊ ngx_hash_init_t.hash.buckets ָ������. 
24.        ����ÿ��Ͱ���Ԫ�صĴ洢�ռ� = ��ͰԪ��ռ�õ��ڴ�ռ� + voidָ�� 
25.        Ϊ����߲�ѯЧ��,����һ�������ڴ�ռ��� ����Ͱ��Ԫ��. 
26.        Ȼ�����Ƭ�������ڴ�ռ�ӳ�䵽 ngx_hash_init_t.hash.buckets ָ������. 
27.        Ȼ��Ϊÿ��Ͱ��Ԫ�ظ�ֵ. 
28.        ���ÿ��Ͱ��"����Ԫ��"��ΪNULL 
29. 
30.        voidָ�����;: ΪͰ�Ľ������, �� ngx_hash_find() ����Ͱ���ж� etl->value �Ƿ�ΪNULLʱ�õ�. 
31.        ���������Ϊ "����Ԫ��"ֻ��һ��void*ָ��Ŀռ�ת���� ngx_hash_elt_t ��᲻��Խ�����? 
32.        ���ǲ����, void*ָ��Ŀռ�ת���� ngx_hash_elt_t ��ֻ���� ngx_hash_elt_t.value , 
33.        �� ngx_hash_elt_t.value �պ�ֻռ voidָ��ռ��С. 
34. 
35.        ָ��Ͱ�Ĵ�С�ĺô�: ��֤ÿ��Ͱ���Ԫ�صĸ���������һ��ֵ,Ŀ����Ϊ����߲�ѯЧ��. 
36.         
37.     */  
    //ʹ�÷������Բο�ngx_http_server_names
ngx_int_t
ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names, ngx_uint_t nelts)//ʹ�÷������Բο�ngx_http_server_names
{
    //�ο�http://www.oschina.net/question/234345_42065  http://www.bkjia.com/ASPjc/905190.html
    u_char          *elts;
    size_t           len;
    u_short         *test;
    ngx_uint_t       i, n, key, 
                     size,  //size��ʾʵ����ҪͰ�ĸ���
                     start, bucket_size;
    ngx_hash_elt_t  *elt, **buckets;

    for (n = 0; n < nelts; n++) {
        //���names�����ÿһ��Ԫ�أ��ж�Ͱ�Ĵ�С�Ƿ񹻷��� 
        //names[n]��Ա�ռ�һ��ҪС�ڵ���bucket_size /* ÿ��Ͱ�����ܴ��һ��Ԫ�� + һ��voidָ��  
        //Ҫ����sizeof(void *)����Ϊbucket�����Ҫk-v�Խ�����־����void * value�����ġ�
        if (hinit->bucket_size < NGX_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
        {
            //���κ�һ��Ԫ�أ�Ͱ�Ĵ�С����Ϊ��Ԫ�ط���ռ䣬���˳�   
            ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0,
                          "could not build the %s, you should "
                          "increase %s_bucket_size: %i",
                          hinit->name, hinit->name, hinit->bucket_size);
            return NGX_ERROR;
        }
    }

    //����2*max_size���ֽڵĿռ䱣��hash����(���ڴ�����������nginx���ڴ���н��У���Ϊtestֻ����ʱ��)    
    /* ���ڼ�¼ÿ��Ͱ����ʱ��С */  
    test = ngx_alloc(hinit->max_size * sizeof(u_short), hinit->pool->log);
    if (test == NULL) {
        return NGX_ERROR;
    }

    // ʵ�ʿ��ÿռ�Ϊ�����bucket_size��ȥĩβ��void *(��β��ʶ)��ĩβ��void* ָ��NULL
    bucket_size = hinit->bucket_size - sizeof(void *);

    /* �����⼸���Ǵ�Ĺ���һ�£�Ͱ����Ӧ�ôӶ��ٸ���ʼ�� */
    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;
    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

    //start��ʾ����Ͱ�ĸ�����Ͱ�ĸ�����������ģ���start��ʼ��max_sizeһ��һ�����ԣ�����Ҫ��֤ÿ��Ͱ�е�ʵ�ʿռ�hinit->bucket_size - sizeof(void *);Ҫ�ܹ�
    //�������ɢ�е���Ͱ�е�ngx_hash_elt_t�ռ�����͡�
    //��ʵ�����Ͱ�ĸ���(ͨ������Ԫ�ؿռ�С��hinit->max_size)����Ϊ�˱�֤ÿ��Ͱ�е�Ԫ�ظ�����̫�࣬�������Ա�֤�ڱ���hash���ʱ���ܹ������ҵ�����Ͱ�е�Ԫ��


    /*  max_size��bucket_size������
    max_size��ʾ������max_size��Ͱ��ÿ��Ͱ�е�Ԫ��(ngx_hash_elt_t)���� * NGX_HASH_ELT_SIZE(&names[n])���ܳ���bucket_size��С
    ʵ��ngx_hash_init�����ʱ�򲢲���ֱ����max_size��Ͱ�����Ǵ�size=1��max_sizeȥ�ԣ�ֻҪngx_hash_init�����е�names[]����������ȫ��hash
    ����size��Ͱ�У�������������:ÿ��Ͱ�е�Ԫ��(ngx_hash_elt_t)���� * NGX_HASH_ELT_SIZE(&names[n])������bucket_size��С,��˵����size
    ��Ͱ�͹����ˣ�Ȼ��ֱ��ʹ��x��Ͱ�洢�� ��ngx_hash_init
     */
    
    for (size = start; size <= hinit->max_size; size++) { //size��ʾʵ����ҪͰ�ĸ���

        ngx_memzero(test, size * sizeof(u_short));

        
        //���1���˿�����Ǽ��bucket��С�Ƿ񹻷���hash����   
        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            
            //����key��names������name���ȣ���������test[key]��   
            key = names[n].key_hash % size;//��size=1����keyһֱΪ0  
            //test[i]��ʾ��i��Ͱ���Ѿ�ʹ���˵�ngx_hash_elt_t�ռ��ܴ�С
            test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n])); 

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %ui %ui \"%V\"",
                          size, key, test[key], &names[n].key);
#endif
            //���������õ���bucket_size���������ֵ����˵�����size�����ʰ�goto next��������Ͱ����Ŀ
            if (test[key] > (u_short) bucket_size) { 
                //��������Ͱ�Ĵ�С������һ��Ͱ���¼���   
                goto next;
            }
        }

        goto found;

    next:

        continue;
    }

    size = hinit->max_size;
    
    //�ߵ�������棬��names�е�Ԫ����hashͰ��ʱ�򣬿��ܻ����ĳЩhashͰ�����ÿռ���ʵ�ʵ�bucket_size��
    ngx_log_error(NGX_LOG_WARN, hinit->pool->log, 0,
                  "could not build optimal %s, you should increase "
                  "either %s_max_size: %i or %s_bucket_size: %i; "
                  "ignoring %s_bucket_size",
                  hinit->name, hinit->name, hinit->max_size,
                  hinit->name, hinit->bucket_size, hinit->name);

found://�ҵ����ʵ�bucket   

    //�����������е�test[i]���鸳ֵΪ4��Ԥ����NULLָ��
    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *);//��test����ǰsize��Ԫ�س�ʼ��Ϊ4����ǰ��ֵ4��ԭ���ǣ�hashͰ�ĳ�Ա�б�β������һ��NULL����ǰ����4�ֽڿռ�Ԥ�� 
    }

   /* ���2������1���������ͬ�����˿�������ٴμ�������hash���ݵ��ܳ���(���1�ļ����ͨ��)  
      ���˴���test[i]�ѱ���ʼ��Ϊ4�����൱�ں����ļ����ټ���һ��voidָ��Ĵ�С��  
    */ //����ÿ��Ͱ�еĳ�Ա�ռ��С�ܺ�
    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        //����key��names������name���ȣ���������test[key]��   
        key = names[n].key_hash % size;//��size=1����keyһֱΪ0   
        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }
    
    //����hash���ݵ��ܳ��ȣ�����Ͱ�����ݿռ䳤�Ⱥ�   
    len = 0;

    //len��ʾ����names[]��������һ����Ҫx��ngx_hash_elt_t�ṹ�洢����x��ngx_hash_elt_t���õĿռ��ܺͣ�len��ʵ�ʴ洢����Ԫ�صĿռ䣬Ҳ���Ǵ�������Ͱ�е�Ԫ�����õĿռ�
    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {//��test[i]��Ϊ��ʼ����ֵ4����û�б仯�������  
            continue;
        }
        
        //��test[i]��ngx_cacheline_size����(32λƽ̨��ngx_cacheline_size=32)   
        test[i] = (u_short) (ngx_align(test[i], ngx_cacheline_size));

        len += test[i];
    }

    //����ÿһ��Ͱ��ָ��������ݲ��ֵ�ָ��ͷ
    if (hinit->hash == NULL) {
        //���ڴ���з���hashͷ��buckets����(size��ngx_hash_elt_t*�ṹ)   
        hinit->hash = ngx_pcalloc(hinit->pool, sizeof(ngx_hash_wildcard_t)
                                             + size * sizeof(ngx_hash_elt_t *)); //size��ʾʵ����ҪͰ�ĸ���������Ŀռ�պþ���ÿ��Ͱ��ͷ��ָ��
        if (hinit->hash == NULL) {
            ngx_free(test);
            return NGX_ERROR;
        }
        
        //����buckets����ʾλ��(��ngx_hash_wildcard_t�ṹ֮��)  
        buckets = (ngx_hash_elt_t **)
                      ((u_char *) hinit->hash + sizeof(ngx_hash_wildcard_t));

    } else { //���ڴ���з���buckets����(size��ngx_hash_elt_t*�ṹ)   
        buckets = ngx_pcalloc(hinit->pool, size * sizeof(ngx_hash_elt_t *)); //size��ʾʵ����ҪͰ�ĸ���������Ŀռ�պþ���ÿ��Ͱ��ͷ��ָ��
        if (buckets == NULL) {
            ngx_free(test);
            return NGX_ERROR;
        }
    }
    
    //���ŷ���elts����СΪlen+ngx_cacheline_size���˴�Ϊʲô+32����������Ҫ��32�ֽڶ���   
    elts = ngx_palloc(hinit->pool, len + ngx_cacheline_size); 
    //len��ʾ����names[](����������x����Ա)��������һ����Ҫx��ngx_hash_elt_t�ṹ�洢����x��ngx_hash_elt_t���õĿռ��ܺ�
    if (elts == NULL) {
        ngx_free(test);
        return NGX_ERROR;
    }
    
    //��elts��ַ��ngx_cacheline_size=32����   
    elts = ngx_align_ptr(elts, ngx_cacheline_size);

    for (i = 0; i < size; i++) {//��buckets��������Ӧelts��Ӧ����  
        if (test[i] == sizeof(void *)) {
            continue;
        }

        //ÿ��Ͱͷָ��buckets[i]ָ���Լ�Ͱ�еĳ�Ա�׵�ַ
        buckets[i] = (ngx_hash_elt_t *) elts;
        elts += test[i];
    }

    for (i = 0; i < size; i++) {
        //test������0   
        test[i] = 0;
    }

    //�����е�name�������hash����
    for (n = 0; n < nelts; n++) {//����������ÿһ��hash���ݴ���hash�� 
        if (names[n].key.data == NULL) {
            continue;
        }

        
        //����key��������hash�������ڵڼ���bucket�����������Ӧ��eltsλ�ã�Ҳ�����ڸ�buckets[i]Ͱ�еľ���λ��
        key = names[n].key_hash % size;
        elt = (ngx_hash_elt_t *) ((u_char *) buckets[key] + test[key]);

        
        //��ngx_hash_elt_t�ṹ��ֵ   
        elt->value = names[n].value;
        elt->len = (u_short) names[n].key.len;

        //ÿ���ƶ�test[]��ʱ�򣬶����ƶ�NGX_HASH_ELT_SIZE(&names[n])�������и�nameԤ��name�ַ������ȿռ�
        ngx_strlow(elt->name, names[n].key.data, names[n].key.len); 
        
        //������һ��Ҫ��hash�����ݵĳ���ƫ�ƣ���һ�ξʹӸ�Ͱ����һ��λ�ô洢   
        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }

    //Ϊÿ��Ͱ�ĳ�Ա�б���β�����һ��ngx_hash_elt_t��Ա����value=NULL����ʶ���Ǹ�Ͱ�е����һ��ngx_hash_elt_t
    for (i = 0; i < size; i++) {
        if (buckets[i] == NULL) {
            continue;
        }
        
        //test[i]�൱�����б�hash�������ܳ���   
        elt = (ngx_hash_elt_t *) ((u_char *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    ngx_free(test);//�ͷŸ���ʱ�ռ�   

    hinit->hash->buckets = buckets;
    hinit->hash->size = size;

#if 0

    for (i = 0; i < size; i++) {
        ngx_str_t   val;
        ngx_uint_t  key;

        elt = buckets[i];

        if (elt == NULL) {
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: NULL", i);
            continue;
        }

        while (elt->value) {
            val.len = elt->len;
            val.data = &elt->name[0];

            key = hinit->key(val.data, val.len);

            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %p \"%V\" %ui", i, elt, &val, key);

            elt = (ngx_hash_elt_t *) ngx_align_ptr(&elt->name[0] + elt->len,
                                                   sizeof(void *));
        }
    }

#endif

    return NGX_OK;
}

/*
nginxΪ�˴������ͨ�����������ƥ�����⣬ʵ����ngx_hash_wildcard_t������hash��������֧���������͵Ĵ���ͨ�����������һ����ͨ�����ǰ�ģ�
���磺��*.abc.com����Ҳ����ʡ�Ե��Ǻţ�ֱ��д�ɡ�.abc.com����������key������ƥ��www.abc.com��qqq.www.abc.com֮��ġ�����һ����ͨ�����ĩ
β�ģ����磺��mail.xxx.*�������ر�ע��ͨ�����ĩβ�Ĳ���λ�ڿ�ʼ��ͨ������Ա�ʡ�Ե���������ͨ���������ƥ��mail.xxx.com��mail.xxx.com.cn��
mail.xxx.net֮���������

��һ�����˵��������һ��ngx_hash_wildcard_t���͵�hash��ֻ�ܰ���ͨ�����ǰ��key������ͨ����ں��key������ͬʱ�����������͵�ͨ���
��key��ngx_hash_wildcard_t���ͱ����Ĺ�����ͨ������ngx_hash_wildcard_init��ɵģ�����ѯ��ͨ������ngx_hash_find_wc_head����
ngx_hash_find_wc_tail�����ġ�ngx_hash_find_wc_head�ǲ�ѯ����ͨ�����ǰ��key��hash��ģ���ngx_hash_find_wc_tail�ǲ�ѯ����ͨ����ں��key��hash��ġ�

�ر�Ҫע����������key�Ѿ����Ǳ�Ԥ������ġ����磺��*.abc.com�����ߡ�.abc.com����Ԥ��������Ժ󣬱���ˡ�com.abc.��������mail.xxx.*����Ԥ����Ϊ��mail.xxx.��

���ȿ�һ��ngx_hash_wildcard_init ���ڴ�ṹ������������͵�hash���ʱ��ʵ�����ǹ�����һ��hash���һ������������ͨ��hash���е�key�����ӡ�
�����ġ����磺���ڡ�*.abc.com�����ṹ���2��hash����һ��hash������һ��keyΪcom�ı���ñ����value������ָ��ڶ���hash���ָ�룬
���ڶ���hash������һ������abc���ñ����value������ָ��*.abc.com��Ӧ��value��ָ�롣��ô��ѯ��ʱ�򣬱����ѯwww.abc.com��ʱ���Ȳ�com��
ͨ����com�����ҵ��ڶ�����hash���ڵڶ���hash���У��ٲ���abc���������ƣ�ֱ����ĳһ����hash���в鵽�ı����Ӧ��value��Ӧһ��������ֵ����
һ��ָ����һ��hash���ָ���ʱ�򣬲�ѯ���̽�����
*/
ngx_int_t
ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts) //�ο�:http://www.bkjia.com/ASPjc/905190.html    //ʹ�÷������Բο�ngx_http_server_names
{//�ο�http://www.bkjia.com/ASPjc/905190.html ͼ��
    size_t                len, dot_len;
    ngx_uint_t            i, 
                          n,  //n��ʾ��ǰ�������names[]�����еĵڼ�����Ա
                          dot; //��ǰ������names[i]�еĵ�i��Ԫ���ַ�����.�ַ���λ��
    ngx_array_t           curr_names, next_names;
    ngx_hash_key_t       *name, *next_name;
    ngx_hash_init_t       h;
    ngx_hash_wildcard_t  *wdc;

    //��ʼ����ʱ��̬����curr_names,curr_names�Ǵ�ŵ�ǰ�ؼ��ֵ�����
    if (ngx_array_init(&curr_names, hinit->temp_pool, nelts, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    //��ʼ����ʱ��̬����next_names,next_names�Ǵ�Źؼ���ȥ����ʣ��ؼ���
    if (ngx_array_init(&next_names, hinit->temp_pool, nelts,
                       sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (n = 0; n < nelts; n = i) {

#if 0
        ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                      "wc0: \"%V\"", &names[n].key);
#endif

        dot = 0;

        for (len = 0; len < names[n].key.len; len++) {//���� dot��len�ĳ���Ϊ.ǰ����ַ�������
            if (names[n].key.data[len] == '.') {
                dot = 1;
                break;
            }
        }

        name = ngx_array_push(&curr_names);
        if (name == NULL) {
            return NGX_ERROR;
        }

        //���ؼ���dot��ǰ�Ĺؼ��ַ���curr_names
        //names[]�ַ�������key�д洢�� 

        /* ȡֵaa.bb.cc�е�aa�ַ����洢��key�У�������aa��Ӧ��key_hashֵ���������еݹ飬Ȼ��ȡ��bb��cc�ַ����ֱ�浽name������ */
        name->key.len = len; //lenΪ.dotǰ����ַ���
        name->key.data = names[n].key.data;
        name->key_hash = hinit->key(name->key.data, name->key.len);
        name->value = names[n].value; //�������hash����value���ں���ָ����hash

#if 0
        ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                      "wc1: \"%V\" %ui", &name->key, dot);
#endif

        dot_len = len + 1;

        if (dot) {
            len++;//lenָ��dot��ʣ��ؼ���
        }

        next_names.nelts = 0;

        //���names[n] dot����ʣ��ؼ��֣���ʣ��ؼ��ַ���next_names��
        if (names[n].key.len != len) {//ȡ����aa.bb.cc�е�aa�ַ����浽curr_names[]�����У�ʣ�µ�bb.cc�ַ����浽next_names������
            next_name = ngx_array_push(&next_names);
            if (next_name == NULL) {
                return NGX_ERROR;
            }

            next_name->key.len = names[n].key.len - len;
            next_name->key.data = names[n].key.data + len;
            next_name->key_hash = 0;
            next_name->value = names[n].value;

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "wc2: \"%V\"", &next_name->key);
#endif
        }

        //��������������Ĺؼ���û��dot����n+1����names�����ؼ��ֱ�������ȫ������next_name

        /*  
            ����names[0]Ϊaa.bb,names[1]Ϊaa.cc�����ҵ�ǰ�������names[0],��aa�浽curr_names[]�����У�bb��cc�浽next_name�����У�
            Ҳ���ǰ�aa.bb��aa.cc�ϲ���
          */
        for (i = n + 1; i < nelts; i++) {
            if (ngx_strncmp(names[n].key.data, names[i].key.data, len) != 0) {//ǰlen���ؼ�����ͬ
                break;
            }

            if (!dot
                && names[i].key.len > len
                && names[i].key.data[len] != '.')
            {
                break;
            }

            next_name = ngx_array_push(&next_names);
            if (next_name == NULL) {
                return NGX_ERROR;
            }

            next_name->key.len = names[i].key.len - dot_len;
            next_name->key.data = names[i].key.data + dot_len;
            next_name->key_hash = 0;
            next_name->value = names[i].value;

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "wc3: \"%V\"", &next_name->key);
#endif
        }

        //�������name[i]Ϊaa.bb.cc��ǰ��ȡ����aa,ʣ�µ�bb.cc�浽��next_names�����У���ݹ�ú����Ӷ���bb��cc����ͬ���Ĳ���
        if (next_names.nelts) {//���next_name�ǿ�

            h = *hinit;
            h.hash = NULL;

            if (ngx_hash_wildcard_init(&h, (ngx_hash_key_t *) next_names.elts,
                                       next_names.nelts)//�ݹ飬����һ���µĹ�����
                != NGX_OK)
            {
                return NGX_ERROR;
            }

            wdc = (ngx_hash_wildcard_t *) h.hash;

            if (names[n].key.len == len) { //����ͼ�����û�valueֵ�����µ�hash��Ҳ����hinit��
                wdc->value = names[n].value;
            }

            /*
                ����ָ�붼��void*����СΪ4���ֽڶ����ˣ���2λ�϶�Ϊ0�����ֲ�����name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2)) �� 
                �����ʹ����ָ��ĵ�λЯ��������Ϣ����ʡ���ڴ棬���˲��ò����ngx����ߵ���������
                */    
            name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2)); //������ǰvalueֵָ���µ�hash��

        } else if (dot) {
            name->value = (void *) ((uintptr_t) name->value | 1); //��ʾ�����Ѿ�û����hash��,valueָ������key-value�е�value�ַ���
        }
    }
    
    //�������hash��ʼ��
    if (ngx_hash_init(hinit, (ngx_hash_key_t *) curr_names.elts,
                      curr_names.nelts)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}

/*
�����ש�������������������������������������������������������������������������������������������������������
��  ��    ��7-8 Nginx�ṩ������ɢ�з���                                                                     ��
�ǩ��贈�������������������������������������������������ש���������������������������������������������������
��  ��    ɢ�з���                                      ��    ����                                          ��
�ǩ��ߩ��������������������������������������������������贈��������������������������������������������������
��ngx_uint_t ngx_hash_key(u_char *data, size_t len)     ��  ʹ��BKDR�㷨�����ⳤ�ȵ��ַ���ӳ��Ϊ����        ��
�ǩ������������������������������������������������������贈��������������������������������������������������
��                                                      ��  ���ַ���ȫСд����ʹ��BKDR�㷨�����ⳤ�ȵ���  ��
��.gx_uint_t ngx_hash_key_lc(I_char *data, size_t len)  ������ӳ��Ϊ����                                    ��
���������������������������������������������������������ߩ���������������������������������������������������
*/
/*
ngx_hash_key�����ļ���ɱ���Ϊ���й�ʽ��

Key[0] = data[0]
Key[1] = data[0]*31 + data[1]
Key[2] = (data[0]*31 + data[1])*31 + data[2]
...
Key[len-1] = ((((data[0]*31 + data[1])*31 + data[2])*31) ... data[len-2])*31 + data[len-1]key[len-1]��Ϊ����Ĳ���data��Ӧ��hashֵ
*/
//�������ַ���data�����keyֵ
ngx_uint_t
ngx_hash_key(u_char *data, size_t len)
{
    ngx_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ngx_hash(key, data[i]);
    }

    return key;
}

/*
�����ש�������������������������������������������������������������������������������������������������������
��  ��    ��7-8 Nginx�ṩ������ɢ�з���                                                                     ��
�ǩ��贈�������������������������������������������������ש���������������������������������������������������
��  ��    ɢ�з���                                      ��    ����                                          ��
�ǩ��ߩ��������������������������������������������������贈��������������������������������������������������
��ngx_uint_t ngx_hash_key(u_char *data, size_t len)     ��  ʹ��BKDR�㷨�����ⳤ�ȵ��ַ���ӳ��Ϊ����        ��
�ǩ������������������������������������������������������贈��������������������������������������������������
��                                                      ��  ���ַ���ȫСд����ʹ��BKDR�㷨�����ⳤ�ȵ���  ��
��.gx_uint_t ngx_hash_key_lc(I_char *data, size_t len)  ������ӳ��Ϊ����                                    ��
���������������������������������������������������������ߩ���������������������������������������������������
*/
ngx_uint_t
ngx_hash_key_lc(u_char *data, size_t len)
{
    ngx_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ngx_hash(key, ngx_tolower(data[i]));
    }

    return key;
}


ngx_uint_t
ngx_hash_strlow(u_char *dst, u_char *src, size_t n)
{
    ngx_uint_t  key;

    key = 0;

    while (n--) {
        *dst = ngx_tolower(*src);
        key = ngx_hash(key, *dst);
        dst++;
        src++;
    }

    return key;
}

/*
��ʼ��ngx_hash_keys_arrays_t �ṹ�壬type��ȡֵ��Χֻ��������NGX_HASH_SMALL��ʾ��ʼ��Ԫ�ؽ��٣�NGX_HASH_LARGE��ʾ��ʼ��Ԫ�ؽ϶࣬
����ha�м���ʱ������ô˷�����
*/ //ngx_hash_keys_array_initһ���ngx_hash_add_key���ʹ�ã�ǰ�߱�ʾ��ʼ��ngx_hash_keys_arrays_t����ռ䣬���������洢��Ӧ��key�������еĶ�Ӧhash��������
ngx_int_t
ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type) //ʹ�÷������Բο�ngx_http_server_names
{
    ngx_uint_t  asize;

    if (type == NGX_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

    } else {
        asize = NGX_HASH_LARGE_ASIZE;
        ha->hsize = NGX_HASH_LARGE_HSIZE;
    }
    
    //��ʼ�� ��ŷ�ͨ����ؼ��ֵ�����
    if (ngx_array_init(&ha->keys, ha->temp_pool, asize, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    //��ʼ�����ǰ��ͨ�������õĹؼ��� ����
    if (ngx_array_init(&ha->dns_wc_head, ha->temp_pool, asize, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }
    
    //��ʼ����ź���ͨ�������õĹؼ��� ����
    if (ngx_array_init(&ha->dns_wc_tail, ha->temp_pool, asize, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

//�����⼸��ʵ������hashͨ�ĸ���Ͱ��ͷ��ָ�룬ÿ��hash��ha->hsize��Ͱͷ��ָ�룬��ngx_hash_add_key��ʱ��ͷ��ָ��ָ��ÿ��Ͱ�о���ĳ�Ա�б�
    
  /*
  ��ʼ����λ���飬��������ŵĵ�һ��ά�ȴ������bucket�ı�ţ���ôkeys_hash[i]�д�ŵ������е�key�������hashֵ��hsizeȡ
  ģ�Ժ��ֵΪi��key��������3��key,�ֱ���key1,key2��key3����hashֵ������Ժ��hsizeȡģ��ֵ����i����ô������key��ֵ��˳���
  ����keys_hash[i][0],keys_hash[i][1], keys_hash[i][2]����ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��keyֵ��Ҳ�����Ƿ����ظ���
  */  
    ha->keys_hash = ngx_pcalloc(ha->temp_pool, sizeof(ngx_array_t) * ha->hsize); 
    //ֻ�����ж��ٸ�Ͱ��Ӧ��ͷ��ÿ��Ͱ�������洢���ݵĿռ��ں����ngx_hash_add_key��Ƭ�ռ�
    if (ha->keys_hash == NULL) {
        return NGX_ERROR;
    }
    
    // �������ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��ǰ��ͨ�����keyֵ��Ҳ�����Ƿ����ظ���
    ha->dns_wc_head_hash = ngx_pcalloc(ha->temp_pool,
                                       sizeof(ngx_array_t) * ha->hsize);
    if (ha->dns_wc_head_hash == NULL) {
        return NGX_ERROR;
    }
    
   // �������ڵ��õĹ�������������ͼ���Ƿ��г�ͻ�ĺ���ͨ�����keyֵ��Ҳ�����Ƿ����ظ��� 
    ha->dns_wc_tail_hash = ngx_pcalloc(ha->temp_pool,
                                       sizeof(ngx_array_t) * ha->hsize);
    if (ha->dns_wc_tail_hash == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

/*
��key value��ӵ�ha��Ӧ��array����������
*/ //ngx_hash_add_key�ǽ����򲻴�ͨ�����keyת��������ngx_hash_keys_arrays_t��Ӧ��
/*
��ϣͨ����Ĳ���: 
Nginx��ϣ֧���������͵�ͨ��: 
"*.example.com", ".example.com", and "www.example.*"

����Щ�ַ������й�ϣǰ������Ԥ����(ngx_hash_add_key): 
"*.example.com", ����Ԥ���������: "com.example.\0"
".example.com"  ����Ԥ���������: "com.example\0"
"www.example.*" ����Ԥ���������:  "www.example\0"

ͨ���hash���ʵ��ԭ�� �� ����������͵�hash���ʱ��ʵ�����ǹ�����һ��hash���һ������������ͨ��hash���е�key�����ӡ������ġ�
���磺���ڡ�*.example.com�����ṹ���2��hash����һ��hash������һ��keyΪcom�ı���ñ����value������ָ��ڶ���hash���ָ�룬
���ڶ���hash������һ������abc���ñ����value������ָ*.example.com��Ӧ��value��ָ�롣��ô��ѯ��ʱ�򣬱����ѯwww.example.com��ʱ��
�Ȳ�com��ͨ����com�����ҵ��ڶ�����hash���ڵڶ���hash���У��ٲ���example���������ƣ�ֱ����ĳһ����hash���в鵽�ı����Ӧ��value��
Ӧһ��������ֵ����һ��ָ����һ��hash���ָ���ʱ�򣬲�ѯ���̽����������ҵ���������value��ַ�������bit��ʾ: (��Ҳ���������ڴ�ʱҪ
��4�ֽڶ����ԭ��, �����bit��0, ���Ա��޸�����ʾ�������)

ͷ��ͨ�����:
        / *
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer for both "example.com"
         *          and "*.example.com";
         *     01 - value is data pointer for "*.example.com" only;
         *     10 - value is pointer to wildcard hash allowing
         *          both "example.com" and "*.example.com";
         *     11 - value is pointer to wildcard hash allowing
         *          "*.example.com" only.
         * /
β��ͨ�����:

        / *
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer;
         *     11 - value is pointer to wildcard hash allowing "example.*".
         * /

*/
/*
ngx_hash_add_key�ǽ����򲻴�ͨ�����keyת�������������ṹ�еģ��������:
    �ȿ�����ĵ�����������־������key�ǲ���NGX_HASH_WILDCARD_KEY�� 
    ������ǣ�����ha->keys_hash�м���Ƿ��ͻ����ͻ�ͷ���NGX_BUSY�����򣬾ͽ���һ����뵽ha->keys�С�
    ����ǣ����ж�ͨ������ͣ�֧�ֵ�ͳ��������֡�*.example.com��, ��.example.com��, and ��www.example.*����
    Ȼ�󽫵�һ��ת��Ϊ"com.example.�������뵽ha->dns_wc_head�У���������ת��Ϊ"www.example"�����뵽ha->dns_wc_tail�У�
    �Եڶ��ֱȽ����⣬��Ϊ���ȼ��ڡ�*.example.com��+��example.com��,���Ի�һ��ת��Ϊ"com.example.�����뵽ha->dns_wc_head��
    һ��Ϊ"example.com"���뵽ha->keys�С���Ȼ����ǰ�������Ƿ��ͻ��
*/ //ngx_hash_keys_array_initһ���ngx_hash_add_key���ʹ�ã�ǰ�߱�ʾ��ʼ��ngx_hash_keys_arrays_t����ռ䣬���������洢��Ӧ��key�������еĶ�Ӧhash��������

/*

    ��ֵ��ngx_hash_add_key
    
    ԭʼkey                  ��ŵ�hashͰ(keys_hash��dns_wc_head_hash                 ��ŵ�������(keys��dns_wc_head��
                                    ��dns_wc_tail_hash)                                     dns_wc_tail)
                                    
 www.example.com                 www.example.com(����keys_hash)                        www.example.com (����keys�����Աngx_hash_key_t��Ӧ��key��)
  .example.com             example.com(�浽keys_hash��ͬʱ����dns_wc_tail_hash)        com.example  (����dns_wc_head�����Աngx_hash_key_t��Ӧ��key��)
 www.example.*                     www.example. (����dns_wc_tail_hash)                 www.example  (����dns_wc_tail�����Աngx_hash_key_t��Ӧ��key��)
 *.example.com                     example.com  (����dns_wc_head_hash)                 com.example. (����dns_wc_head�����Աngx_hash_key_t��Ӧ��key��)
*/
ngx_int_t
ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key, void *value,
    ngx_uint_t flags)//ʹ�÷������Բο�ngx_http_server_names
{
    size_t           len;
    u_char          *p;
    ngx_str_t       *name;
    ngx_uint_t       i, k, n, 
        skip, // 1 -- ".example.com"  2 -- "*.example.com"  0 -- "www.example.*"
        last;
    ngx_array_t     *keys, *hwc;
    ngx_hash_key_t  *hk;

    last = key->len;

    if (flags & NGX_HASH_WILDCARD_KEY) {

        /*
         * supported wildcards:
         *     "*.example.com", ".example.com", and "www.example.*"
         */

        n = 0;

        for (i = 0; i < key->len; i++) {

            if (key->data[i] == '*') {
                if (++n > 1) { //ͨ���*ֻ�ܳ���һ�Σ����ֶ��˵�����󷵻�
                    return NGX_DECLINED;
                }
            }

            //���ܳ�������������..�����ǳ���ֱ�ӷ���
            if (key->data[i] == '.' && key->data[i + 1] == '.') {
                return NGX_DECLINED;
            }
        }
        
        if (key->len > 1 && key->data[0] == '.') {//���ַ���.��".example.com"˵����ǰ��ͨ���
            skip = 1;
            goto wildcard;
        }

        if (key->len > 2) {

            if (key->data[0] == '*' && key->data[1] == '.') {//"*.example.com"
                skip = 2;
                goto wildcard;
            }

            if (key->data[i - 2] == '.' && key->data[i - 1] == '*') {//"www.example.*"
                skip = 0;
                last -= 2;
                goto wildcard;
            }
        }

        if (n) {
            return NGX_DECLINED;
        }
    }

    /* exact hash */
    /* ˵���Ǿ�ȷƥ��server_name���ͣ�����"www.example.com" */
    k = 0;

    //���ַ���keyΪԴ������hash��һ���ַ�һ���ַ�����
    for (i = 0; i < last; i++) {
        if (!(flags & NGX_HASH_READONLY_KEY)) {
            key->data[i] = ngx_tolower(key->data[i]);
        }
        k = ngx_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    /* check conflicts in exact hash */

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            if (last != name[i].len) {
                continue;
            }

            if (ngx_strncmp(key->data, name[i].data, last) == 0) { //�Ѿ�����һ����ͬ��
                return NGX_BUSY;
            }
        }

    } else {
        //ÿ��Ͱ�е�Ԫ�ظ���Ĭ��
        if (ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                           sizeof(ngx_str_t)) //Ͱ��ͷ��ָ����ngx_hash_keys_array_init���䣬Ͱ�д洢���ݵĿռ����������
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    /*
    key�浽ha->keys_hash[]��Ӧ��hashͰ��, key-value�浽ha->keys[]�����еĶ�Ӧkey��Ա��value��Ա��

     */

    //���key��ha->keys_hash[]Ͱ��
    name = ngx_array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return NGX_ERROR;
    }

    *name = *key;

    hk = ngx_array_push(&ha->keys);
    if (hk == NULL) {
        return NGX_ERROR;
    }
    //ha->keys�д�ŵ���key  value��
    hk->key = *key;
    hk->key_hash = ngx_hash_key(key->data, last);
    hk->value = value;

    return NGX_OK;

wildcard:

    /* wildcard hash */
    //�Բ����е�key�ַ�������hash key
    k = ngx_hash_strlow(&key->data[skip], &key->data[skip], last - skip);

    k %= ha->hsize;

    if (skip == 1) { //".example.com"��".example.com"������ӵ�hashͰkeys_hash[]�⣬������ӵ�dns_wc_tail_hash[]Ͱ�У�

        /* check conflicts in exact hash for ".example.com" */

        name = ha->keys_hash[k].elts;

        if (name) {
            len = last - skip; //��ȥ��ͷ��.
            //�ַ������Ⱥ�������ȫƥ��
            for (i = 0; i < ha->keys_hash[k].nelts; i++) {
                if (len != name[i].len) {
                    continue;
                }

                if (ngx_strncmp(&key->data[1], name[i].data, len) == 0) {
                    return NGX_BUSY;
                }
            }

        } else {
            if (ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                               sizeof(ngx_str_t)) //ÿ�����е�Ԫ�ظ���Ĭ��4�ֽ�
            //����ÿ�����еĿռ䣬������Ŷ�Ӧ�Ľڵ㵽�ò��У��۵�ͷ�ڵ���ngx_hash_keys_array_init���Ѿ������
                != NGX_OK)
            {
                return NGX_ERROR;
            }
        }

        name = ngx_array_push(&ha->keys_hash[k]);
        if (name == NULL) {
            return NGX_ERROR;
        }

        name->len = last - 1; //��".example.com"�ַ�����ͷ��.ȥ��
        name->data = ngx_pnalloc(ha->temp_pool, name->len);
        if (name->data == NULL) {
            return NGX_ERROR;
        }

        //".example.com"ȥ����ͷ��.���Ϊ"example.com"�洢��name�У�����key����ԭ����".example.com"
        ngx_memcpy(name->data, &key->data[1], name->len);//".example.com"ȥ����ͷ��.���Ϊ"example.com"�洢��ha->keys_hash[i]Ͱ��
    }


    if (skip) { //ǰ��ƥ���ͨ���"*.example.com"  ".example.com"
        /*
         * convert "*.example.com" to "com.example.\0"
         *      and ".example.com" to "com.example\0"
         */
        p = ngx_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NGX_ERROR;
        }

        len = 0;
        n = 0;

        for (i = last - 1; i; i--) {
            if (key->data[i] == '.') {
                ngx_memcpy(&p[n], &key->data[i + 1], len);
                n += len;
                p[n++] = '.';
                len = 0;
                continue;
            }

            len++;
        }

        if (len) {
            ngx_memcpy(&p[n], &key->data[1], len);
            n += len;
        }

        /* key������"*.example.com"��p������"com.example.\0"   key������".example.com" p������"com.example\0" */
        p[n] = '\0'; 

        hwc = &ha->dns_wc_head;
        keys = &ha->dns_wc_head_hash[k];
    
    }  else {//����ƥ��
        last++; //+1�������洢\0�ַ�

        p = ngx_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NGX_ERROR;
        }

        //key������Ϊ"www.example.*"�� p������Ϊ"www.example\0"
        ngx_cpystrn(p, key->data, last); 

        hwc = &ha->dns_wc_tail;
        keys = &ha->dns_wc_tail_hash[k];
    }

    /* check conflicts in wildcard hash */  
    name = keys->elts;

    if (name) {
        len = last - skip;
        //�鿴�Ƿ��Ѿ��д��ڵ���
        for (i = 0; i < keys->nelts; i++) {
            if (len != name[i].len) {
                continue;
            }

            if (ngx_strncmp(key->data + skip, name[i].data, len) == 0) {
                return NGX_BUSY;
            }
        }

    } else {//˵���ǵ�һ�γ���ǰ��ͨ������ߺ���ͨ���
        //��ʼ��Ͱha->dns_wc_head_hash[i]����Ͱha->dns_wc_tail_hash[i]�е�Ԫ�ظ���
        if (ngx_array_init(keys, ha->temp_pool, 4, sizeof(ngx_str_t)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    name = ngx_array_push(keys);
    if (name == NULL) {
        return NGX_ERROR;
    }

    name->len = last - skip;
    name->data = ngx_pnalloc(ha->temp_pool, name->len);
    if (name->data == NULL) {
        return NGX_ERROR;
    }
    
    /* ǰ��ƥ��key�ַ�����ŵ�&ha->dns_wc_head; ����ƥ��key�ַ�����ŵ�&ha->dns_wc_tail hash���� */
    ngx_memcpy(name->data, key->data + skip, name->len);

    /*
    ����valid_referers none blocked server_names .example.com  www.example.*
    ��������valid_referers none blocked server_names *.example.com  www.example.*
    name:example.com, kye:*.example.com
    name:www.example., kye:www.example.*
    name:example.com, kye:.example.com

    *.example.com  --- example.com
    www.example.*  --- www.example.
    .example.com   --- example.com
    ngx_log_debugall(ngx_cycle->log, 0, "name:%V, kye:%V", name, key);
     */
    /* add to wildcard hash */

    hk = ngx_array_push(hwc);
    if (hk == NULL) {
        return NGX_ERROR;
    }

    hk->key.len = last - 1;
    //������,p�е����ݾ���Դkey"*.example.com", ".example.com", and "www.example.*"��Ϊ��"com.example.\0" "com.example\0"  "www.example\0"
    hk->key.data = p;
    hk->key_hash = 0;
    hk->value = value; //��ngx_http_add_refererΪ��������keyΪ��*.example.com/test/xxx,��valueΪ�ַ���/test/xxx������ΪNGX_HTTP_REFERER_NO_URI_PART

/*
    ����valid_referers none blocked server_names .example.com  www.example.*
    ��������valid_referers none blocked server_names *.example.com  www.example.*

    [yangya  [debug] 25843#25843: name:example.com, kye:.example.com, p:com.example
    [yangya  [debug] 25843#25843: name:www.example., kye:www.example.*, p:www.example
    [yangya  [debug] 25844#25844: name:example.com, kye:*.example.com, p:com.example.


    ngx_log_debugall(ngx_cycle->log, 0, "name:%V, kye:%V, p:%V", name, key, &hk->key);
*/
    return NGX_OK;
}

