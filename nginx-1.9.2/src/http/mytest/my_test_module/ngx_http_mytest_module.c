#include <ngx_config.h>  
#include <ngx_core.h>  
#include <ngx_http.h>  

/*
��ngx_http_mytest_handler������������ngx_http_request_t�����о������������ڴ�ع���������Ƕ��ڴ�صĲ��������Ի���
�������У���������������������ʱ���ڴ�ط�����ڴ�Ҳ���ᱻ�ͷš�
*/

/*
��ngx_http_mytest_handler�ķ���ֵ�У������������HTTP�����룬Nginx�ͻᰴ�չ淶����Ϸ�����Ӧ�����͸��û���
���磬�������PUT�����ݲ�֧�֣���ô���ڴ������з��ַ�������PUTʱ������NGX_HTTP_NOT_ALLOWED������NginxҲ�ͻṹ�������������Ӧ�����û���
*/

/*
Nginx�ڵ��������е�ngx_http_mytest_handler����ʱ������������Nginx
���̵ģ�����ngx_http_mytest_handler�����ƵĴ��������ǲ����к�ʱ�ܳ��Ĳ����ġ�
*/
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)  
{  
    printf("yang test:xxxxxxxxx <%s, %u>\n",  __FUNCTION__, __LINE__);
    // Only handle GET/HEAD method  ////������GET����HEAD���������򷵻�405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {  
        return NGX_HTTP_NOT_ALLOWED;  
    }  

    // Discard request body  
    ngx_int_t rc = ngx_http_discard_request_body(r);  
    //������봦�������еİ��壬��ô���Ե���ngx_http_discard_request_body�����������Կͻ��˵�HTTP���嶪������
    if (rc != NGX_OK) {  
        return rc;  
    }  
  
    // Send response header  
    ngx_str_t type = ngx_string("text/plain");  
    ngx_str_t response = ngx_string("Hello World!!!11111111111");  
    r->headers_out.status = NGX_HTTP_OK;  
    r->headers_out.content_length_n = response.len;  
    r->headers_out.content_type = type;  

    /*
    ��������Ϻ���Ҫ���û�����HTTP��Ӧ����֪�ͻ���Nginx��ִ�н����HTTP��Ӧ��Ҫ������Ӧ�С���Ӧͷ�������������֡�
    ����HTTP��Ӧʱ��Ҫִ�з���HTTPͷ��������HTTPͷ��ʱҲ�ᷢ����Ӧ�У��ͷ���HTTP��������������
    */
    rc = ngx_http_send_header(r);  
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {  
        return rc;  
    }  
    // Send response body  
    ngx_buf_t *b;  
    b = ngx_create_temp_buf(r->pool, response.len);  
    if (b == NULL) {  
        return NGX_HTTP_INTERNAL_SERVER_ERROR;  
    }  
    ngx_memcpy(b->pos, response.data, response.len);  
    b->last = b->pos + response.len;  
    b->last_buf = 1;  
  
    ngx_chain_t out;  
    out.buf = b;  
    out.next = NULL;  

    return ngx_http_output_filter(r, &out);  
}  
  
static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)  
{  
    ngx_http_core_loc_conf_t *clcf;  


    /*
    �����ҵ�mytest���������������ÿ飬clcf����ȥ����location���ڵ����ݽṹ����ʵ��Ȼ����������main��srv����loc���������Ҳ����˵��
    ��ÿ��http{}��server{}��Ҳ����һ��ngx_http_core_loc_conf_t�ṹ��
    */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);  

    /*HTTP����ڴ����û�������е�NGX_HTTP_CONTENT_PHASE�׶�ʱ��������������������URI��mytest���������ڵ����ÿ���ƥ�䣬�ͽ���������ʵ�ֵ�ngx_http_mytest_handler���������������*/
    //�ú�����ngx_http_core_content_phase�е�ngx_http_finalize_request(r, r->content_handler(r));�����r->content_handler(r)ִ��
    clcf->handler = ngx_http_mytest_handler;   //HTTP����ڽ�����HTTP�����ͷ���󣬻����handlerָ��ķ���

    return NGX_CONF_OK;  
}  
  
static ngx_command_t ngx_http_mytest_commands[] = {  
    {  
        ngx_string("mytest"),  
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS, 

/*
ngx_http_mytest��ngx_command_t�ṹ���е�set��Ա����������Ϊchar *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);����
����ĳ�����ÿ��г���mytest������ʱ��Nginx�������ngx_http_mytest���������濴һ�����ʵ��ngx_http_mytest������
*/
        ngx_http_mytest,  
        NGX_HTTP_LOC_CONF_OFFSET,  
        0,  
        NULL  
    },  
    ngx_null_command      
};  

//���Ҳ������ʵ�ִ��������ngx_http_mytest_handler���������û��ʲô�����Ǳ�����HTTP��ܳ�ʼ��ʱ��ɵģ��ǾͲ���ʵ��ngx_http_module_t��8���ص�������������������������ngx_http_module_t�ӿڡ�
static ngx_http_module_t ngx_http_mytest_module_ctx = {  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL  
};  

//����mytestģ�飺 
ngx_module_t ngx_http_mytest_module = {  
    NGX_MODULE_V1,  
    &ngx_http_mytest_module_ctx,  
    ngx_http_mytest_commands,  
    NGX_HTTP_MODULE,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NGX_MODULE_V1_PADDING  
};  

