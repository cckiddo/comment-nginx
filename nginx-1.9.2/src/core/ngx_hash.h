
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 http://www.cnblogs.com/chengxuyuancc/p/3782808.html
 http://www.bkjia.com/ASPjc/905190.html
 http://www.oschina.net/question/234345_42065
 */
 
#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*
ngx_hash_elt_t��uchar name[1]����ƣ������name�̵ܶ�����£�name�� ushort ���ֽڶ������ֻ��ռ��һ���ֽڣ������ͱȷ�һ
��uchar* ��ָ����ռ��һ���ֽڣ����Կ���ngx��������ڴ��Ͽ��ǣ���ʡÿһ���ڴ�����߲�����
*/
//hash����Ԫ��ngx_hash_elt_t Ԥ��ӹ�ϣɢ��Ԫ�ؽṹ ngx_hash_key_t
typedef struct {//ʵ��һ��ngx_hash_elt_t���õĿռ�ΪNGX_HASH_ELT_SIZE����sizeof(ngx_hash_elt_t)����Ϊ��Ҫ�洢������ַ���name
    //���value=NULL,��ʾ�Ǹþ���hashͰbucket[i]�е����һ��ngx_hash_elt_t��Ա
    void             *value;//value����ĳ��key��Ӧ��ֵ����<key,value>�е�value   
    u_short           len;//name����  key�ĳ���    
    //ÿ���ƶ�test[]��ʱ�򣬶����ƶ�NGX_HASH_ELT_SIZE(&names[n])�������и�nameԤ��name�ַ������ȿռ䣬��ngx_hash_init
    u_char            name[1]; //ĳ��Ҫhash������(��nginx�б���Ϊ�ַ���)����<key,value>�е�key     ����Сд��ĸ

//ngx_hash_elt_t��ngx_hash_t->buckets[i]Ͱ�еľ����Ա
} ngx_hash_elt_t; //hashԪ�ؽṹ   //ngx_hash_init��names�������hashͰǰ����ṹ��ngx_hash_key_t��ʽ������hashͰ��������ݵ�ʱ�򣬻��ngx_hash_key_t����ĳ�Ա������ngx_hash_elt_t����Ӧ��Ա  


/*
�������ngx_hash_keys_arrays_tͰkeys_hash  dns_wc_head_hash  dns_wc_tail_hash����Ӧ�ľ���Ͱ�еĿռ������飬�����С����ǰ���������ʼ����ʱ�����úõ�
ngx_hash_t->buckets[]�еľ���Ͱ�еĳ�Ա�Ǹ���ʵ�ʳ�Ա���������Ŀռ�
*/

//�ڴ���hashͰ��ʱ��ֵ����ngx_hash_init
typedef struct { //hashͰ�������Բο�ngx_hash_find  
    ngx_hash_elt_t  **buckets; //hashͰ(��size��Ͱ)    ָ�����Ͱ��ͷ��ָ�룬Ҳ����bucket[]���飬bucket[I]��ָ��ÿ��Ͱ�еĵ�һ��ngx_hash_elt_t��Ա����ngx_hash_init
    ngx_uint_t        size;//hashͰ������ע����Ͱ�ĸ���������ÿ��Ͱ�еĳ�Ա��������ngx_hash_init
} ngx_hash_t;

/*
����ṹ��Ҫ���ڰ���ͨ�����hash������ṹ���ngx_hash_t�ṹ���Ƕ���һ��valueָ�룬value����ֶ����������ĳ���Ѿ��ﵽĩβ��ͨ���url��Ӧ��valueֵ��
���ͨ���urlû�дﵽĩβ������ֶ�ΪNULL��

ngx_hash_wildcard_tר���ڱ�ʾǣ�ƻ����ͨ����Ĺ�ϣ���磺ǰ��*.test.com������:www.test.* ����ֻ�Ƕ�ngx_hash_t�ļ򵥷�װ��
����һ��������ϣ��hash��һ�������valueָ�룬��ʹ��ngx_hash_wildcard_tͨ�����ϣ����Ϊ����Ԫ��ʱ������ʹ��valueָ���û����ݡ�
*/
/*
nginxΪ�˴������ͨ�����������ƥ�����⣬ʵ����ngx_hash_wildcard_t������hash��������֧���������͵Ĵ���ͨ�����������һ����ͨ�����ǰ�ģ�
���磺��*.abc.com����Ҳ����ʡ�Ե��Ǻţ�ֱ��д�ɡ�.abc.com����������key������ƥ��www.abc.com��qqq.www.abc.com֮��ġ�����һ����ͨ�����ĩ
β�ģ����磺��mail.xxx.*�������ر�ע��ͨ�����ĩβ�Ĳ���λ�ڿ�ʼ��ͨ������Ա�ʡ�Ե���������ͨ���������ƥ��mail.xxx.com��mail.xxx.com.cn��
mail.xxx.net֮���������

��һ�����˵��������һ��ngx_hash_wildcard_t���͵�hash��ֻ�ܰ���ͨ�����ǰ��key������ͨ����ں��key������ͬʱ�����������͵�ͨ���
��key��ngx_hash_wildcard_t���ͱ����Ĺ�����ͨ������ngx_hash_wildcard_init��ɵģ�����ѯ��ͨ������ngx_hash_find_wc_head����
ngx_hash_find_wc_tail�����ġ�ngx_hash_find_wc_head�ǲ�ѯ����ͨ�����ǰ��key��hash��ģ���ngx_hash_find_wc_tail�ǲ�ѯ����ͨ����ں��key��hash��ġ�
*/
typedef struct {
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;


//hash����Ԫ��ngx_hash_elt_t Ԥ��ӹ�ϣɢ��Ԫ�ؽṹ ngx_hash_key_t
typedef struct { //��ֵ�ο�ngx_hash_add_key
    ngx_str_t         key; //key��Ϊnginx���ַ����ṹ   
    ngx_uint_t        key_hash; //�ɸ�key�������hashֵ(ͨ��hash������ngx_hash_key_lc())  
    void             *value;  //��key��Ӧ��ֵ�����һ����-ֵ��<key,value>    ��ͨ���hash��Ҳ����ָ����һ��ͨ���hash,��ngx_hash_wildcard_init
} ngx_hash_key_t; //ngx_hash_init��names�������hashͰǰ����ṹ��ngx_hash_key_t��ʽ������hashͰ��������ݵ�ʱ�򣬻��ngx_hash_key_t����ĳ�Ա������ngx_hash_elt_t����Ӧ��Ա


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
typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);

/*
Nginx����server- name������ͨ�����֧�ֹ���
    ���ȣ�ѡ�������ַ�����ȫƥ���server name����www.testweb.com��
    ��Σ�ѡ��ͨ�����ǰ���server name����*.testweb.com��
    �ٴΣ�ѡ��ͨ����ں����server name����www.testweb.*��

ngx_hash_combined_t����3����ϣ����ɣ�һ����ͨhash��hash��һ������ǰ��ͨ�����hash��wc_head��һ����������ͨ�����hash�� wc_tail��
*/ //ngx_http_virtual_names_t�а����ýṹ
typedef struct { //�������hash��Ϣ��ngx_http_server_names�д洢��hash���е�server_name��������server{}������ctx,server_nameΪkey��������ctxΪvalue
    //��hash�д�ŵ���ngx_hash_keys_arrays_t->keys[]�����еĳ�Ա
    ngx_hash_t            hash; //��ͨhash����ȫƥ��
    //��wc_head hash�д洢����ngx_hash_keys_arrays_t->dns_wc_head�����еĳ�Ա
    ngx_hash_wildcard_t  *wc_head; //ǰ��ͨ���hash
    //��wc_head hash�д洢����ngx_hash_keys_arrays_t->dns_wc_tail�����еĳ�Ա
    ngx_hash_wildcard_t  *wc_tail; //����ͨ���hash
} ngx_hash_combined_t;

//�ο�http://www.oschina.net/question/234345_42065  http://www.bkjia.com/ASPjc/905190.html
typedef struct {//���ʹ���Լ�ʹ�÷������Բο�ngx_http_referer_merge_conf
    ngx_hash_t       *hash;//ָ�����ʼ����hash�ṹ
    ngx_hash_key_pt   key; //hash����ָ��   

    /*  max_size��bucket_size������
    max_size��ʾ������max_size��Ͱ��ÿ��Ͱ�е�Ԫ��(ngx_hash_elt_t)���� * NGX_HASH_ELT_SIZE(&names[n])���ܳ���bucket_size��С
    ʵ��ngx_hash_init�����ʱ�򲢲���ֱ����max_size��Ͱ�����Ǵ�x=1��max_sizeȥ�ԣ�ֻҪngx_hash_init�����е�names[]����������ȫ��hash
    ����x��Ͱ�У�������������:ÿ��Ͱ�е�Ԫ��(ngx_hash_elt_t)���� * NGX_HASH_ELT_SIZE(&names[n])������bucket_size��С,��˵����x
    ��Ͱ�͹����ˣ�Ȼ��ֱ��ʹ��x��Ͱ�洢�� ��ngx_hash_init
     */
    
    ngx_uint_t        max_size;//�����Ҫ��ô���Ͱ��ʵ����Ͱ�ĸ�������ͨ������õ��ģ��ο�ngx_hash_init
    //���䵽�þ���Ͱ�еĳ�Ա���� * NGX_HASH_ELT_SIZE(&names[n])ҪС��bucket_size����ʾ��Ͱ�����洢��ô���ngx_hash_elt_t��Ա
    //��Ч�ط���ngx_hash_init�е�:if (test[key] > (u_short) bucket_size) { }
    ngx_uint_t        bucket_size; //��ʾÿ��hashͰ��(hash->buckets[i->��Ա[i]])��Ӧ�ĳ�Ա����ngx_hash_elt_t��Ա���ÿռ�͵����ֵ������ÿ��Ͱ���õ����пռ����ֵ��ͨ�����ֵ������Ҫ���ٸ�Ͱ
    char             *name; //��hash�ṹ������(���ڴ�����־��ʹ��,���ڵ��Դ�ӡ��)   
    ngx_pool_t       *pool;  //��hash�ṹ��poolָ����ڴ���з���   
    ngx_pool_t       *temp_pool; //������ʱ���ݿռ���ڴ��   
} ngx_hash_init_t; //hash��ʼ���ṹ   

#define NGX_HASH_SMALL            1 //NGX_HASH_SMALL��ʾ��ʼ��Ԫ�ؽ���
#define NGX_HASH_LARGE            2 //NGX_HASH_LARGE��ʾ��ʼ��Ԫ�ؽ϶࣬

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1 //ͨ�������
#define NGX_HASH_READONLY_KEY     2


/*
typedef struct  {
    ���������keys_hash��dna- wc head hash��dns wc tail_ hash���Ǽ���ɢ�б���hsizeָ��
��ɢ�б�Ĳ۸����������ɢ�з���Ҳ��Ҫ��hsize����+��
    ngx_uint_t hsize;
    //�ڴ�أ����ڷ����������ڴ棬��Ŀǰ��Nginx�汾Ϊֹ����pool��Աû���κ�����+��
    ngx_pool_t *pool;
    //��ʱ�ڴ�أ�����Ķ�̬������Ҫ���ڴ涼��temp_pool�ڴ�ط���
    ngx_pool_t *temp_pool;
    //�ö�̬������ngx_hash_key_t�ṹ�屣���Ų�����ͨ����ؼ��ֵ�Ԫ��
    ngx_array_t keys;
   //һ��������׵�ɢ�б������������ʽ������hsize��Ԫ�أ�ÿ��Ԫ�ض���ngx_array_t��̬��
�顣���û���ӵ�Ԫ�ع����У�����ݹؼ��뽫�û���ngx_str_t���͵Ĺؼ�����ӵ�ngx_array_t��̬�����С�
�������е��û�Ԫ�صĹؼ��ֶ������Դ�ͨ�������ʾ��ȷƥ��+��
    ngx��array_t *keys_hash;
   //�ö�̬������ngx-- hash- key_�߽ṹ�屣���ź���ǰ��ͨ����ؼ��ֵ�Ԫ�����ɵ��м�ؼ���+��
    ngx_array_t dns_wc_head;
    //һ��������׵�ɢ�б������������ʽ������hsize��Ԫ�أ�ÿ��Ԫ������ngx��array��t��̬��
�顣���û����Ԫ�ع����У�����ݹؼ��뽫�û���ngx_str_t���͵Ĺؼ�����ӵ�ngx��arrayһ�߶�̬�����С�
�������е��û�Ԫ�صĹؼ��ֶ���ǰ��ͨ���
    ngx_array_t   *dns_wc_head_haah;
    //�ö�̬������ngx_ha sh_key_t�ṹ�屣���ź��к���ͨ����ؼ��ֵ�Ԫ�����ɵ��м�ؼ���t��
    ngx_array_t dns_wc_tail;
    //һ��������׵�ɢ�б������������ʽ������hsize��Ԫ�أ�ÿ��Ԫ�ض���ngx��array_�߶�̬��
�顣���û����Ԫ�ع����У�����ݹؼ��뽫�û���ngx_ str_�����͵Ĺؼ�����ӵ�ngx_array_t��̬�����С�
�������е��û�Ԫ�صĹؼ��ֶ�������ͨ�������
    ngx��array_t *dns_wc_tail_hash;
    }  ngx_hash_keys_arrays_t,

*/
/*
hsize: ��Ҫ������hash���Ͱ�ĸ���������ʹ������ṹ�а�������Ϣ�������������͵�hash����ʹ�ô˲����� 
pool: ������Щhash��ʹ�õ�pool�� 
temp_pool: �ڹ�����������Լ����յ�����hash������п����õ���ʱpool����temp_pool�����ڹ�������Ժ󣬱����ٵ�������ֻ�Ǵ����ʱ��һЩ�ڴ����ġ� 
keys: ������з�ͨ���key�����顣 
keys_hash: ���Ǹ���ά���飬��һ��ά�ȴ������bucket�ı�ţ���ôkeys_hash[i]�д�ŵ������е�key�������hashֵ��hsizeȡģ�Ժ��ֵΪi��key��������3��key,�ֱ���key1,key2��key3����hashֵ������Ժ��hsizeȡģ��ֵ����i����ô������key��ֵ��˳������keys_hash[i][0],keys_hash[i][1], keys_hash[i][2]����ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��keyֵ��Ҳ�����Ƿ����ظ��� 
dns_wc_head: ��ǰ��ͨ���key����������Ժ��ֵ�����磺��*.abc.com�� ����������Ժ󣬱�� ��com.abc.�� ������ڴ������С� 
dns_wc_tail: ��ź���ͨ���key����������Ժ��ֵ�����磺��mail.xxx.*�� ����������Ժ󣬱�� ��mail.xxx.�� ������ڴ������С� 
dns_wc_head_hash: ��ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��ǰ��ͨ�����keyֵ��Ҳ�����Ƿ����ظ��� 
dns_wc_tail_hash: ��ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ�ĺ���ͨ�����keyֵ��Ҳ�����Ƿ����ظ��� 
*/

/*
ע��keys��dns_wc_head��dns_wc_tail�����������д�ŵ�Ԫ��ʱngx_hash_key_t���͵ģ���keys_hash,dns_wc_head_hash��dns_wc_tail_hash��
������ά�����д�ŵ�Ԫ����ngx_str_t���͵ġ�
ngx_hash_keys_array_init����Ϊ�����ṹ����ռ䡣
*/ //�ýṹֻ����������ȫƥ��   ǰ��ƥ��  ����ƥ��ͨ���ýṹ��ȫ���洢�ڸýṹ���Ӧ��hash��������
typedef struct { //�ýṹһ�������洢������Ϣ
    //ɢ���в�����  ����Ǵ�hashͰ��ʽ����hsize=NGX_HASH_LARGE_HSIZE,СhashͰ��ʽ��hsize=107,��ngx_hash_keys_array_init
    ngx_uint_t        hsize; 

    ngx_pool_t       *pool;//�ڴ�أ����ڷ��������Ե��ڴ�
    ngx_pool_t       *temp_pool; //��ʱ�ڴ�أ��������ʱ��̬���鶼������ʱ�ڴ�ط���

    //�����⼸��ʵ������hashͨ�ĸ���Ͱ��ͷ��ָ�룬ÿ��hash��ha->hsize��Ͱͷ��ָ�룬��ngx_hash_add_key��ʱ��ͷ��ָ��ָ��ÿ��Ͱ�о���ĳ�Ա�б�
    //�������Щ���Բο�ngx_hash_add_key
     /*
    keys_hash���Ǹ���ά���飬��һ��ά�ȴ������bucket�ı�ţ���ôkeys_hash[i]�д�ŵ������е�key�������hashֵ��hsizeȡģ�Ժ��ֵΪi��key��
    ������3��key,�ֱ���key1,key2��key3����hashֵ������Ժ��hsizeȡģ��ֵ����i����ô������key��ֵ��˳������keys_hash[i][0],
    keys_hash[i][1], keys_hash[i][2]����ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��keyֵ��Ҳ�����Ƿ����ظ���
    */ //��ȫƥ��keys����ֻ���key�ַ������ڵ�����Ϊngx_str_t��keys_hash hash����key��Ӧ��key-value�У�hash�������ݽڵ�����Ϊngx_hash_key_t

   /* 
    hashͰkeys_hash  dns_wc_head_hash   dns_wc_tail_hashͷ��ָ����Ϣ��ʼ����ngx_hash_keys_array_init�����еľ���
    Ͱkeys_hash[i] dns_wc_head_hash[i]  dns_wc_tail_hash[i]�е���������Ϊngx_str_t��ÿ��Ͱ�����ݳ�ԱĬ��4������ngx_hash_add_key��
    Ͱ�д洢��������Ϣ����ngx_hash_add_key������key�����ַ���
    
    ����keys[] dns_wc_head[] dns_wc_tail[]�е���������Ϊngx_hash_key_t����ngx_hash_keys_array_init��
    ngx_hash_key_t�е�key��value�ֱ�洢ngx_hash_add_key�е�key������value����
    */

    /*

    ��ֵ��ngx_hash_add_key
    
    ԭʼkey                  ��ŵ�hashͰ(keys_hash��dns_wc_head_hash                 ��ŵ�������(keys��dns_wc_head��
                                    ��dns_wc_tail_hash)                                     dns_wc_tail)
                                    
 www.example.com                 www.example.com(����keys_hash)                        www.example.com (����keys�����Աngx_hash_key_t��Ӧ��key��)
  .example.com             example.com(�浽keys_hash��ͬʱ����dns_wc_tail_hash)        com.example  (����dns_wc_head�����Աngx_hash_key_t��Ӧ��key��)
 www.example.*                     www.example. (����dns_wc_tail_hash)                 www.example  (����dns_wc_tail�����Աngx_hash_key_t��Ӧ��key��)
 *.example.com                     example.com  (����dns_wc_head_hash)                 com.example. (����dns_wc_head�����Աngx_hash_key_t��Ӧ��key��)

    //�������ngx_hash_keys_arrays_tͰkeys_hash  dns_wc_head_hash  dns_wc_tail_hash����Ӧ�ľ���Ͱ�еĿռ������飬�����С����ǰ���������ʼ����ʱ�����úõ�
    ngx_hash_t->buckets[]�еľ���Ͱ�еĳ�Ա�Ǹ���ʵ�ʳ�Ա���������Ŀռ�
    */
    ngx_array_t       keys;//�����Աngx_hash_key_t
    ngx_array_t      *keys_hash;//keys_hash[i]��ӦhashͰͷ��ָ�룬������Ͱ�г�Ա����ngx_str_t

    ngx_array_t       dns_wc_head; //�����Աngx_hash_key_t
    ngx_array_t      *dns_wc_head_hash;//dns_wc_head_hash[i]��ӦhashͰͷ��ָ�룬����Ͱ�г�Ա����ngx_str_t

    ngx_array_t       dns_wc_tail; //�����Աngx_hash_key_t
    ngx_array_t      *dns_wc_tail_hash; //dns_wc_tail_hash[i]��ӦhashͰͷ��ָ�룬����Ͱ�г�Ա����ngx_str_t
} ngx_hash_keys_arrays_t; //ngx_http_referer_conf_t�е�keys��Ա

/*
ngx_table_elt_t���ݽṹ������ʾ��
typedef struct {
    ngx_uint_t        hash;
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;

���Կ�����ngx_table_elt_t����һ��key/value�ԣ�ngx_str_t ���͵�key��value��Ա�ֱ�洢�������֡�ֵ�ַ�����
hash��Ա����ngx_table_elt_tҲ������ĳ��ɢ�б����ݽṹ��ngx_hash_t���ͣ��еĳ�Ա��ngx_uint_t ���͵�hash
��Ա������ngx_hash_t�и�����ҵ���ͬkey��ngx_table_elt_t���ݡ�lowcase_keyָ�����ȫСд��key�ַ�����

�Զ��׼���ngx_table_elt_t��ΪHTTPͷ���������ơ��ģ�����key�洢ͷ�����ƣ���Content-Length����value�洢��Ӧ��ֵ���硰1024������
lowcase_key��Ϊ�˺���HTTPͷ�����ƵĴ�Сд�����磬��Щ�ͻ��˷�����HTTP����ͷ����content-length��Nginxϣ�������Сд���е�
Content-Length����ͬ��������ȫСд��lowcase_key��Ա��Ϳ��Կ��ٴ��Ŀ���ˣ���hash���ڿ��ټ���ͷ���������÷���3.6.3���н�����������
*/
typedef struct {
    ngx_uint_t        hash; //����ngx_http_request_s->header_hash ,����ͨ��key value�ַ����������hashֵ
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key; //��ŵ��Ǳ��ṹ����key��Сд��ĸ�ַ���
} ngx_table_elt_t;


void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key,
    void *value, ngx_uint_t flags);

#endif /* _NGX_HASH_H_INCLUDED_ */

