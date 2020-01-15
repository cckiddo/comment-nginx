
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static ngx_uint_t ngx_http_test_if_unmodified(ngx_http_request_t *r);
static ngx_uint_t ngx_http_test_if_modified(ngx_http_request_t *r);
static ngx_uint_t ngx_http_test_if_match(ngx_http_request_t *r,
    ngx_table_elt_t *header, ngx_uint_t weak);
static ngx_int_t ngx_http_not_modified_filter_init(ngx_conf_t *cf);


static ngx_http_module_t  ngx_http_not_modified_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_not_modified_filter_init,     /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

/*
��6-1  Ĭ�ϼ������Nginx��HTTP����ģ��
���������������������������������������ש�������������������������������������������������������������������
��Ĭ�ϼ������Nginx��HTTP����ģ��     ��    ����                                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ڷ���200�ɹ�ʱ������������If-              ��
��                                    ��Modified-Since����If-Unmodified-Sinceͷ��ȡ������������ļ���ʱ   ��
��ngx_http_not_modified_filter_module ��                                                                  ��
��                                    ���䣬�ٷ��������û��ļ�������޸�ʱ�䣬�Դ˾����Ƿ�ֱ�ӷ���304     ��
��                                    �� Not Modified��Ӧ���û�                                           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���������е�Range��Ϣ������Range�е�Ҫ�󷵻��ļ���һ���ָ�      ��
��ngx_http_range_body_filter_module   ��                                                                  ��
��                                    ���û�                                                              ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP�������������û����͵�ngx_chain_t�ṹ��HTTP��         ��
��                                    ���帴�Ƶ��µ�ngx_chain_t�ṹ�У����Ǹ���ָ��ĸ��ƣ�������ʵ��     ��
��ngx_http_copy_filter_module         ��                                                                  ��
��                                    ��HTTP��Ӧ���ݣ���������HTTP����ģ�鴦���ngx_chain_t���͵ĳ�       ��
��                                    ��Ա����ngx_http_copy_filter_moduleģ�鴦���ı���                 ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ������������ͨ���޸�nginx.conf�����ļ����ڷ���      ��
��ngx_http_headers_filter_module      ��                                                                  ��
��                                    �����û�����Ӧ����������HTTPͷ��                                  ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ�������������ִ��configure����ʱ�ᵽ��http_        ��
��ngx_http_userid_filter_module       ��                                                                  ��
��                                    ��userid moduleģ�飬������cookie�ṩ�˼򵥵���֤������           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���Խ��ı����ͷ��ظ��û�����Ӧ��������nginx��conf�е���������   ��
��ngx_http_charset_filter_module      ��                                                                  ��
��                                    �����б��룬�ٷ��ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ֧��SSI��Server Side Include����������Ƕ�룩���ܣ����ļ����ݰ�  ��
��ngx_http_ssi_filter_module          ��                                                                  ��
��                                    ��������ҳ�в����ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP����������5.5.2����ϸ���ܹ��ù���ģ�顣����Ӧ����     ��
��ngx_http_postpone_filter_module     ��subrequest��������������ʹ�ö��������ͬʱ��ͻ��˷�����Ӧʱ    ��
��                                    ���ܹ�������ν�ġ������ǿ����չ����������˳������Ӧ            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���ض���HTTP��Ӧ���壨����ҳ�����ı��ļ�������gzipѹ������      ��
��ngx_http_gzip_filter_module         ��                                                                  ��
��                                    ����ѹ��������ݷ��ظ��û�                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_range_header_filter_module ��  ֧��rangeЭ��                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_chunked_filter_module      ��  ֧��chunk����                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ù���ģ�齫���r->headers out�ṹ��        ��
��                                    ���еĳ�Ա���л�Ϊ���ظ��û���HTTP��Ӧ�ַ�����������Ӧ��(��         ��
��ngx_http_header_filter_module       ��                                                                  ��
��                                    ��HTTP/I.1 200 0K)����Ӧͷ������ͨ������ngx_http_write filter       ��
��                                    �� module����ģ���еĹ��˷���ֱ�ӽ�HTTP��ͷ���͵��ͻ���             ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_write_filter_module        ��  ����HTTP������������ģ�鸺����ͻ��˷���HTTP��Ӧ              ��
���������������������������������������ߩ�������������������������������������������������������������������

If-Modified-Since:
     �������Ͽ�, ����˵: �����ĳ��ʱ�������, ����ļ����޸���....
            1. �����ı��޸�: ��ô�Ϳ�ʼ����, ����������:200 OK
            2. ���û�б��޸�: ��ô�����贫��, ����������: 403 Not Modified.
     ��;:
             �ͻ��˳����������°汾���ļ�. ������ҳˢ��, ���ش�ͼ��ʱ��.
             ������: �����ͼƬ�����Ժ�û���ٱ��޸�, ��Ȼ��û��Ҫ����������!

If-Unmodified-Since:
     �������Ͽ�, ��˼��: �����ĳ��ʱ�������, �ļ�û�б��޸�.....
            1. ���û�б��޸�: ��ʼ`����'�����ļ�: ����������: 200 OK
            2. ����ļ����޸�: �򲻴���, ����������: 412 Precondition failed (Ԥ�������)
     ��;:

            �ϵ�����(һ���ָ��Range����). Ҫ��ϵ�����, ��ô�ļ���һ�����ܱ��޸�, ����Ͳ���ͬһ���ļ���, ��������ɶ����?

    ��֮һ�仰: һ�����޸��˲�����, һ����û�޸Ĳ�����.
*/
ngx_module_t  ngx_http_not_modified_filter_module = { 
//��ģ����ǽ��If-None-Match��ETag , If-Modified-Since��Last-Modified���������ж��Ƿ���304 no modified����ֱ�Ӱ��ļ����ݴ��͸��ͻ���
    NGX_MODULE_V1,
    &ngx_http_not_modified_filter_module_ctx, /* module context */
    NULL,                                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;

/*
{If-None-Match��ETag , If-Modified-Since��Last-Modified
    If-Modified-Since��������� = Last-Modified����������
    ���ã�������˵�һ�η��ʻ�÷�������Last-Modified����2�η��ʰ�������˻���ҳ�������޸�ʱ�䷢�͵�������ȥ�������������
    ��ʱ�����������ʵ���ļ�������޸�ʱ����жԱȡ����ʱ��һ�£���ô����304���ͻ��˾�ֱ��ʹ�ñ��ػ����ļ������ʱ�䲻һ�£���
    �᷵��200���µ��ļ����ݡ��ͻ��˽ӵ�֮�󣬻ᶪ�����ļ��������ļ���������������ʾ���������.


    If-None-Match��������� = ETag����������
    ����: If-None-Match��ETagһ����������ԭ������HTTP Response�����ETag��Ϣ�� ���û��ٴ��������Դʱ������HTTP Request �м���If-None-Match
    ��Ϣ(ETag��ֵ)�������������֤��Դ��ETagû�иı䣨����Դû�и��£���������һ��304״̬���߿ͻ���ʹ�ñ��ػ����ļ������򽫷���200״̬���µ���Դ��Etag. 
}


{
    ETags��If-None-Match��һ�ֳ��õ��ж���Դ�Ƿ�ı�ķ�����������Last-Modified��HTTP-If-Modified-Since������������ͬ����Last-Modified��HTTP-If-Modified-Sinceֻ�ж���Դ������޸�ʱ�䣬��ETags��If-None-Match��������Դ�κε��κ����ԡ�
    ETags��If-None-Match�Ĺ���ԭ������HTTPResponse�����ETags��Ϣ�����ͻ����ٴ��������Դʱ������HTTPRequest�м���If-None-Match��Ϣ��ETags��ֵ���������������֤��Դ��ETagsû�иı䣨����Դû�иı䣩��������һ��304״̬�����򣬷�����������200״̬�������ظ���Դ���µ�ETags��
}


{  
http��ӦLast-Modified��ETag

��������֪ʶ
1) ʲô�ǡ�Last-Modified��?
����    ���������һ������ĳһ��URLʱ���������˵ķ���״̬����200�����������������Դ��ͬʱ��һ��Last-Modified�����Ա�Ǵ��ļ��ڷ����ڶ�
    ����޸ĵ�ʱ�䣬��ʽ����������Last-Modified: Fri, 12 May 2006 18:53:33 GMT
�����ͻ��˵ڶ��������URLʱ������ HTTP Э��Ĺ涨�������������������� If-Modified-Since ��ͷ��ѯ�ʸ�ʱ��֮���ļ��Ƿ��б��޸Ĺ���
����If-Modified-Since: Fri, 12 May 2006 18:53:33 GMT
��������������˵���Դû�б仯�����Զ����� HTTP 304 ��Not Changed.��״̬�룬����Ϊ�գ������ͽ�ʡ�˴��������������������˴��뷢����
    ���������������ʱ�������·�����Դ�����غ͵�һ������ʱ���ơ��Ӷ���֤����ͻ����ظ�������Դ��Ҳ��֤���������б仯ʱ���ͻ����ܹ��õ����µ���Դ��
2) ʲô�ǡ�Etag��?
����HTTP Э����˵������ETagΪ�������������ʵ��ֵ�� ���μ� ���� �½� 14.19���� ��һ��˵���ǣ�ETag��һ��������Web��Դ�����ļǺţ�token�������͵�Web��Դ����һ��Webҳ����Ҳ������JSON��XML�ĵ������������������жϼǺ���ʲô���京�壬����HTTP��Ӧͷ�н��䴫�͵��ͻ��ˣ������Ƿ������˷��صĸ�ʽ��
����ETag: "50b1c1d4f775c61:df3"
�����ͻ��˵Ĳ�ѯ���¸�ʽ�������ģ�
����If-None-Match: W/"50b1c1d4f775c61:df3"
�������ETagû�ı䣬�򷵻�״̬304Ȼ�󲻷��أ���Ҳ��Last-Modifiedһ�������˲���Etag��Ҫ�ڶϵ�����ʱ�Ƚ����á�
����
Last-Modified��Etags��ΰ����������?
���������Ŀ����߻��Last-Modified ��ETags�����http��ͷһ��ʹ�ã����������ÿͻ��ˣ�������������Ļ��档��Ϊ���������Ȳ��� 
Last-Modified/Etag��ǣ������������Ժ�ʹ�������ж�ҳ���Ƿ��Ѿ����޸ġ������ϣ��ͻ���ͨ�����üǺŴ��ط�����Ҫ���������֤�䣨�ͻ��ˣ����档��������:
1.�ͻ�������һ��ҳ�棨A����
2.����������ҳ��A�����ڸ�A����һ��Last-Modified/ETag��
3.�ͻ���չ�ָ�ҳ�棬����ҳ����ͬLast-Modified/ETagһ�𻺴档
4.�ͻ��ٴ�����ҳ��A�������ϴ�����ʱ���������ص�Last-Modified/ETagһ�𴫵ݸ���������
5.����������Last-Modified��ETag�����жϳ���ҳ�����ϴοͻ�������֮��δ���޸ģ�ֱ�ӷ�����Ӧ304��һ���յ���Ӧ�塣
}
*/
static ngx_int_t
ngx_http_not_modified_header_filter(ngx_http_request_t *r)
{
    if (r->headers_out.status != NGX_HTTP_OK  //ֻ�з�������ΪOK�ĲŽ���304 not modified�ж�
        || r != r->main
        || r->disable_not_modified)
    {
        return ngx_http_next_header_filter(r);
    }
    

    /*
If-Unmodified-Since: �������Ͽ�, ��˼��: �����ĳ��ʱ�������, �ļ�û�б��޸�.....
    1. ���û�б��޸�: ��ʼ`����'�����ļ�: ����������: 200 OK
    2. ����ļ����޸�: �򲻴���, ����������: 412 Precondition failed (Ԥ�������)
��;:�ϵ�����(һ���ָ��Range����). Ҫ��ϵ�����, ��ô�ļ���һ�����ܱ��޸�, ����Ͳ���ͬһ���ļ���
*/
    if (r->headers_in.if_unmodified_since
        && !ngx_http_test_if_unmodified(r)) 
    /*
    �����ĳ��ʱ�������, �ļ����޸��ˣ��ͻ����������д���if_unmodified_since����ʾ����ļ������ĳ��ʱ�����û�б��޸ģ�
    �����200 OK���͸��ͻ��˰��塣���������ļ�ȴ���޸��ˣ���˷��ش���
    */
    {
        return ngx_http_filter_finalize_request(r, NULL,
                                                NGX_HTTP_PRECONDITION_FAILED);
    }

    if (r->headers_in.if_match
        && !ngx_http_test_if_match(r, r->headers_in.if_match, 0))
    {
        return ngx_http_filter_finalize_request(r, NULL,
                                                NGX_HTTP_PRECONDITION_FAILED);
    }

    if (r->headers_in.if_modified_since || r->headers_in.if_none_match) {

        if (r->headers_in.if_modified_since
            && ngx_http_test_if_modified(r)) //�ļ��з������޸�
        {
            return ngx_http_next_header_filter(r);
        }

        if (r->headers_in.if_none_match
            && !ngx_http_test_if_match(r, r->headers_in.if_none_match, 1)) //etag��ƥ�䣬˵���ļ�Ҳ�������޸�
        {
            return ngx_http_next_header_filter(r);
        }

        /* not modified */
        //����304��ʾ�ļ�û���޸�
        r->headers_out.status = NGX_HTTP_NOT_MODIFIED;
        r->headers_out.status_line.len = 0;
        r->headers_out.content_type.len = 0;
        ngx_http_clear_content_length(r);
        ngx_http_clear_accept_ranges(r);

        if (r->headers_out.content_encoding) {
            r->headers_out.content_encoding->hash = 0;
            r->headers_out.content_encoding = NULL;
        }

        return ngx_http_next_header_filter(r);
    }

    return ngx_http_next_header_filter(r);
}

//�����ĳ��ʱ�������, �ļ�û�б��޸ģ�����1
static ngx_uint_t
ngx_http_test_if_unmodified(ngx_http_request_t *r)
{
    time_t  iums;

    if (r->headers_out.last_modified_time == (time_t) -1) { //������ͷ���в���last_modified_time
        return 0;
    }

    iums = ngx_parse_http_time(r->headers_in.if_unmodified_since->value.data,
                               r->headers_in.if_unmodified_since->value.len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "http iums:%T lm:%T", iums, r->headers_out.last_modified_time);

    if (iums >= r->headers_out.last_modified_time) {
        return 1;
    }

    return 0;
}

/* ĳ��ʱ������ļ����޸� */
static ngx_uint_t
ngx_http_test_if_modified(ngx_http_request_t *r)
{
    time_t                     ims;
    ngx_http_core_loc_conf_t  *clcf;

    if (r->headers_out.last_modified_time == (time_t) -1) {
        return 1;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->if_modified_since == NGX_HTTP_IMS_OFF) { //���������if_modified_since��ֱ�ӷ���1
        return 1;
    }

    ims = ngx_parse_http_time(r->headers_in.if_modified_since->value.data,
                              r->headers_in.if_modified_since->value.len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http ims:%T lm:%T", ims, r->headers_out.last_modified_time);

    if (ims == r->headers_out.last_modified_time) { 
//�����ͨ��if_modified_since���͹�����ʱ�����nginx��������֮ǰͨ��last_modified_time���͸��������ʱ��,˵���ļ��ڿͻ������λ�ȡ�����л�û���޸�
        return 0;
    }

    if (clcf->if_modified_since == NGX_HTTP_IMS_EXACT //����Ϊ��ȷƥ�䣬������ims��r->headers_out.last_modified_time��ȣ���ʵ���ϲ����
        || ims < r->headers_out.last_modified_time)
    {
        return 1;
    }

    return 0;
}

//�Ƚ�etag�Ƿ����˱仯   r->headers_out.etagΪnginx�����������µ�etag��header->valueΪ���������ͷ������Я����etag��Ϣ
static ngx_uint_t
ngx_http_test_if_match(ngx_http_request_t *r, ngx_table_elt_t *header,
    ngx_uint_t weak) //ngx_http_test_if_match��֤�ͻ��˹�����etag, ngx_http_set_etag��������etag
{
    u_char     *start, *end, ch;
    ngx_str_t   etag, *list;

    list = &header->value;

    if (list->len == 1 && list->data[0] == '*') {
        return 1;
    }

    //����ͻ����ڵ�һ�������ļ��͵ڶ��������ļ����ʱ�䣬�ļ��޸��ˣ���etag�ᷢ���仯
    if (r->headers_out.etag == NULL) {
        return 0;
    }

    etag = r->headers_out.etag->value;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http im:\"%V\" etag:%V", list, &etag);

    if (weak
        && etag.len > 2
        && etag.data[0] == 'W'
        && etag.data[1] == '/') //��ngx_http_weak_etag
    {
        etag.len -= 2;
        etag.data += 2;
    } //ȥ��ǰ���W/�ַ���

    start = list->data;
    end = list->data + list->len;

    while (start < end) {

        if (weak
            && end - start > 2
            && start[0] == 'W'
            && start[1] == '/')
        {
            start += 2; //ȥ��ǰ���W/�ַ���
        }

        if (etag.len > (size_t) (end - start)) {
            return 0;
        }

        if (ngx_strncmp(start, etag.data, etag.len) != 0) {
            goto skip;
        }

        start += etag.len;

        while (start < end) {
            ch = *start;

            if (ch == ' ' || ch == '\t') {
                start++;
                continue;
            }

            break;
        }

        if (start == end || *start == ',') {
            return 1;
        }

    skip:

        while (start < end && *start != ',') { start++; }
        while (start < end) {
            ch = *start;

            if (ch == ' ' || ch == '\t' || ch == ',') {
                start++;
                continue;
            }

            break;
        }
    }

    return 0;
}


static ngx_int_t
ngx_http_not_modified_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_not_modified_header_filter;

    return NGX_OK;
}
