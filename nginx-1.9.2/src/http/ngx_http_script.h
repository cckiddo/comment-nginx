
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_SCRIPT_H_INCLUDED_
#define _NGX_HTTP_SCRIPT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    /*
    ����pos && code: ÿ�ε���code,���Ὣ���������µ��ַ�������posָ����ַ�������Ȼ��pos����ƶ����´ν����ʱ�򣬻��Զ�������׷�ӵ�����ġ�
	����ipҲ�����ԭ��code����Ὣe->ip����ƶ����ƶ��Ĵ�С���ݲ�ͬ�ı���������ء�ipָ��һ���ڴ棬������Ϊ������ص�һ���ṹ�壬��
	��ngx_http_script_copy_capture_code_t���ṹ��֮��������һ��ip�ĵ�ַ�������ƶ�ʱ�������� :code = (ngx_http_script_copy_capture_code_t *) e->ip;
     e->ip += sizeof(ngx_http_script_copy_capture_code_t);//�ƶ���ô��λ�ơ�
	*/ 
	/* �����������ý������������õ�һЩ����ṹ�壬�����rlcf->codes��һ�����飬ע����ǣ���Щ�ṹ��ĵ�һ����Ա����һ������handler��
    ���ﴦ��ʱ�����Ὣ�ýṹ������ǿת���õ��䴦��handler��Ȼ����˳������ִ��֮   */
    u_char                     *ip; //�ο�ngx_http_rewrite_handler  IPʵ�����Ǻ���ָ������
    u_char                     *pos; //pos֮ǰ�����ݾ��ǽ����ɹ��ģ���������ݽ�׷�ӵ�pos���档posָ����Ǻ����buf����ĩβ��
    //����ò������sp�������м��������籣�浱ǰ��һ���Ľ��ȣ�����һ������e->sp--���ҵ���һ���Ľ����
    /* sp��һ��ngx_http_variable_value_t�����飬���汣���˴������з������һЩ����   
    ��һЩ�м������ڵ�ǰ�����п��Կ��Է�����õ�֮ǰ����֮��ı���(ͨ��sp--����sp++)  */
    ngx_http_variable_value_t  *sp; //�ο�ngx_http_rewrite_handler����ռ�, ��������ʳ���valueֵ�ģ����ն���ͨ��ngx_http_script_set_var_code������r->variables[code->index]��

    ngx_str_t                   buf;//��Ž����Ҳ����buffer��posָ�����С�  �ο�ngx_http_script_complex_value_code
    
    ngx_str_t                   line; //��¼������URI  e->line = r->uri;

    /* the start of the rewritten arguments */
    u_char                     *args; //��¼?����Ĳ�����Ϣ

    unsigned                    flushed:1;
    unsigned                    skip:1; //��Ч��ngx_http_script_copy_code����1��ʾû����Ҫ���������ݣ�ֱ���������ݿ�������
    unsigned                    quote:1; //��ngx_http_script_regex_code_t->redirectһ������ngx_http_script_regex_start_code
    unsigned                    is_args:1; //ngx_http_script_mark_args_code��ngx_http_script_start_args_code����1����ʾ�������Ƿ����?
    unsigned                    log:1;

    ngx_int_t                   status; //��ִ����ngx_http_rewrite_handler�е�����codeʱ���᷵�ظ�status����
    ngx_http_request_t         *request; //����������   // ��Ҫ���������  
    
} ngx_http_script_engine_t;


/*
��ȡ������ֵ
�ű�����
ʲô�����棿�ӻ�е������˵���ǰ�����ת���ɻ�е�˶����豸���ڼ����ѧ��������һ����ָ����������ת����������ʽ����ʽ����������ģ�顣

    Nginx ���������������������ת����һϵ�нű������ں��ʵ�ʱ����ͨ���ű�����������Щ�ű����õ�����������ֵ�����仰˵���ű����渺�𽫲�
���еı����ں��ʵ�ʱ��ȡֵ���Ͳ����еĹ̶��ַ���ƴ�ӳ������ַ�����

    �Բ����Ľű���������Ҳ�������������������ɡ�Ϊ�˱����壬ѡ�������һ�����������з�������������ʹ Nginx ���õ������Ӧ�� Host 
��Ϣ�󣬱��磬"example.com"�����ű����洦��ƴ�ӳ� "example.com/access.log" ���ú��� access log ��Ŀ���ļ�����
    access_log  ${host}/access.log;            ������ȡ���ű�������̿��Բο�ngx_http_script_compile_t������������̼�ngx_http_variable_s����½�
*/ 

/*
ngx_http_script_compile_t:/�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣
ngx_http_script_copy_code_t - �������ò������еĹ̶��ַ���ԭ�ⲻ���Ŀ����������ַ�����
ngx_http_script_var_code_t - �������ò������еı���ȡֵ����ӵ������ַ�����

�ű�������غ���
ngx_http_script_variables_count - ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_http_script_compile - �����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��
ngx_http_script_done - ��ӽ�����־֮�����β������
ngx_http_script_run - 
ngx_http_script_add_var_code - Ϊ��������ȡֵ��Ҫ�Ľű�����ʵ�ʱ���ȡֵ�����У�Ϊ��ȷ�����������Ĳ����ڲ���ȡֵ����Ҫ���ڴ���С��
Nginx ��ȡֵ���̷ֳ������ű���һ��������������ֵ���ȣ���һ������ȡ����Ӧ��ֵ��

��������ָ�� access_log ${host}access.log ������ָ���������һ�±����Ӷ��嵽ʹ�õ��������̡�
�����ý����������� access_log ָ��󣬻����������ص����� ngx_http_log_set_log ���������������
*/
//������access_logΪ�����ο�ngx_http_log_set_log
//�ο�:http://ialloc.org/posts/2013/10/20/ngx-notes-http-variables/    http://blog.csdn.net/brainkick/article/details/7065244
//ngx_http_rewrite�У�rewrite aaa bbb break;�����У�aaa����ʹ��ngx_regex_compile_t��bbb����ʹ��ngx_http_script_compile_t
//��ֵ��ngx_http_rewrite
typedef struct {//�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣   ngx_http_script_init_arrays�з�������ڴ�
    
    ngx_conf_t                 *cf;
    ngx_str_t                  *source; /* �����ļ��е�ԭʼ�����ַ���  ����http://$http_host/aa.mp4*/
        
    //ngx_http_script_add_copy_code   ngx_http_script_add_var_code  ngx_http_script_add_args_code
    ngx_array_t               **flushes;//��ngx_http_variable_t->variables�л�ȡ�� ������洢���Ǳ�����index��ţ���ngx_http_script_add_var_code
    /*
    ngx_http_rewrite_value�и�ֵΪngx_http_script_complex_value_code_t->lengths��ngx_http_script_complex_value_code��ִ�и������еĽڵ�pt,
     */  
    ngx_array_t               **lengths; /* ������ڻ�ȡ������Ӧ��ֵ���ȵĽű� */ //  �����е�ÿ����Ա��1�ֽ� 
    /*
    ngx_http_rewrite_value�и�ֵΪngx_http_rewrite_loc_conf_t->codes,�ڵ�pt����ngx_http_rewrite_handler��õ�ִ��
     */ //��Աpt������ngx_http_script_copy_var_code����ngx_http_script_add_var_code
     //��ngx_http_script_compile_t->valuesһ������ngx_http_rewrite_value  
    ngx_array_t               **values; /* ������ڻ�ȡ������Ӧ��ֵ�Ľű� */ // �����е�ÿ����Ա��1�ֽ�  
    
    
    // ��ͨ�����ĸ�����������������(args������$n�����Լ������ַ���)   
    ngx_uint_t                  variables; /* ԭʼ�����ַ��г��ֵı������� */  //�ο�ngx_http_script_compile
    ngx_uint_t                  ncaptures; // ��ǰ����ʱ�����ֵ�$n���������ֵ�������õ����Ϊ$3����ôncaptures�͵���3   
    
    /* 
       * ��λ�Ƶ���ʽ����$1,$2...$9�ȱ���������Ӧλ������1����ʾ����Ҫ��������Ϊdup_capture׼���� 
       * �����������mask�Ĵ��ڣ��űȽ����׵õ��Ƿ����ظ���$n���֡� 
     */  
    ngx_uint_t                  captures_mask; //��ֵ��ngx_http_script_compile
    ngx_uint_t                  size;// ��compile���ַ����У��������ַ��������ܳ���  

    /*  
     ����main�����Ա�������Ҫ�ھ�Ķ�����mainһ������ָ��һ��ngx_http_script_regex_code_t�Ľṹ 
     */  //ngx_http_rewriteָ��ngx_http_script_regex_code_t
    void                       *main; //������ʽ�ṹ���Ƕ���ı��ʽ�����������lengths�ȡ�

    // �Ƿ���Ҫ����������� 
    unsigned                    compile_args:1; //���?����ͨ�ַ������������ַ����ο�ngx_http_script_compile

    //������Щ����β��������ngx_http_script_done
    unsigned                    complete_lengths:1;// �Ƿ�����lengths�������ֹ������NULL     ��ngx_http_script_done
    unsigned                    complete_values:1; // �Ƿ�����values�������ֹ��  
    unsigned                    zero:1; // values��������ʱ���õ����ַ����Ƿ�׷��'\0'��β   
    unsigned                    conf_prefix:1; // �Ƿ������ɵ��ļ���ǰ��׷��·��ǰ׺   
    unsigned                    root_prefix:1; // ͬconf_prefix   

   /* 
     ������λ��Ҫ��rewriteģ����ʹ�ã���ngx_http_rewrite�У� 
     if (sc.variables == 0 && !sc.dup_capture) { 
         regex->lengths = NULL; 
     } 
     û���ظ���$n����ôregex->lengths����ΪNULL��������úܹؼ����ں��� ngx_http_script_regex_start_code�о���ͨ����regex->lengths���жϣ�
     ������ͬ�Ĵ�����Ϊ��û���ظ���$n��ʱ�򣬿���ͨ�����������captures��������ȡ$n��һ�������ظ��ģ� ��ôpcre���������captures������
     �������ǵ�Ҫ��������Ҫ���Լ�handler������ 
    */
    unsigned                    dup_capture:1;
    unsigned                    args:1; //��ǲ����Ƿ����?  �ο�ngx_http_script_compile   // ��compile���ַ������Ƿ�����'?'
} ngx_http_script_compile_t;


typedef struct {
    ngx_str_t                   value;
    ngx_uint_t                 *flushes;
    void                       *lengths;
    void                       *values;
} ngx_http_complex_value_t;


typedef struct {
    ngx_conf_t                 *cf;
    ngx_str_t                  *value;
    ngx_http_complex_value_t   *complex_value;

    unsigned                    zero:1;
    unsigned                    conf_prefix:1;
    unsigned                    root_prefix:1;
} ngx_http_compile_complex_value_t;


typedef void (*ngx_http_script_code_pt) (ngx_http_script_engine_t *e);
typedef size_t (*ngx_http_script_len_code_pt) (ngx_http_script_engine_t *e);

/*
ngx_http_script_compile_t:/�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣
ngx_http_script_copy_code_t - �������ò������еĹ̶��ַ���ԭ�ⲻ���Ŀ����������ַ�����
ngx_http_script_var_code_t - �������ò������еı���ȡֵ����ӵ������ַ�����

�ű�������غ���
ngx_http_script_variables_count - ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_http_script_compile - �����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��
ngx_http_script_done - ��ӽ�����־֮�����β������
ngx_http_script_run - 
ngx_http_script_add_var_code - Ϊ��������ȡֵ��Ҫ�Ľű�����ʵ�ʱ���ȡֵ�����У�Ϊ��ȷ�����������Ĳ����ڲ���ȡֵ����Ҫ���ڴ���С��
Nginx ��ȡֵ���̷ֳ������ű���һ��������������ֵ���ȣ���һ������ȡ����Ӧ��ֵ��
*/
typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   len;
} ngx_http_script_copy_code_t; //ʵ���ϴ�ngx_http_script_compile_t->values����ռ��ʱ�򣬻���������������ݵĿռ䣬��ngx_http_script_add_copy_code


typedef struct {
    /* ��ֵΪ��������ngx_http_script_copy_var_len_code   ngx_http_script_set_var_code  ngx_http_script_var_code*/
    ngx_http_script_code_pt     code;
    //����ngx_http_script_var_code_t->index��ʾNginx����$file��cmcf->variables�����ڵ��±꣬��Ӧÿ������ı���ֵ�洢�ռ��Ϊr->variables[code->index],�ο�ngx_http_script_set_var_code
    uintptr_t                   index; //��ngx_http_variable_t->index�е�λ��
} ngx_http_script_var_code_t;


typedef struct {
    ngx_http_script_code_pt     code;
    ngx_http_set_variable_pt    handler;
    uintptr_t                   data;
} ngx_http_script_var_handler_code_t;


typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   n;
} ngx_http_script_copy_capture_code_t;


#if (NGX_PCRE)

typedef struct { //�����ռ丳ֵ��ngx_http_rewrite
    //��ǰ��code����һ��������Ϊngx_http_script_regex_start_code
    ngx_http_script_code_pt     code; //rewrite xxx bbb break����ʱ��Ϊngx_http_script_regex_start_code��if���õ�ʱ��Ϊngx_http_script_regex_start_code
    ngx_http_regex_t           *regex;//�������������ʽ��
    //��rewriteΪ����������沿���Ǽ��ַ������� rewrite ^(.*)$ http://chenzhenianqing.cn break;��lengthΪNULL
    ngx_array_t                *lengths; //�����������ʽ��Ӧ��lengths�������������� �ڶ����� rewrite ^(.*)$ http://$http_host.mp4 break;
    									//lengths�������һϵ��code,������Ŀ��url�Ĵ�С�ġ�
    uintptr_t                   size; // ��compile���ַ����У��������ַ��������ܳ���   ��Դ��ngx_http_rewrite->ngx_http_script_compile
    //rewrite aaa bbb op;�е�bbbΪ"http://" "https://" "$scheme"����opΪredirect permanent��status�ḳֵ����ngx_http_rewrite
    uintptr_t                   status; //����NGX_HTTP_MOVED_TEMPORARILY  
    uintptr_t                   next; //next�ĺ���Ϊ;�����ǰcodeƥ��ʧ�ܣ���ô��һ��code��λ������ʲô�ط�����Щ����ȫ������һ����������ġ�

    uintptr_t                   test:1;//����Ҫ�����Ƿ�����ƥ��ɹ��������ƥ���ʱ��ǵ÷Ÿ���������ջ� if{}���õ�ʱ����Ҫ
    uintptr_t                   negative_test:1;
    uintptr_t                   uri:1;//�Ƿ���URIƥ�䡣  rewrite����ʱ����1��ʾ��Ҫ����uriƥ��
    uintptr_t                   args:1;

    /* add the r->args to the new arguments */
    uintptr_t                   add_args:1;//�Ƿ��Զ�׷�Ӳ�����rewrite���档���Ŀ�������������ʺý�β����nginx���´�������������

    //rewrite aaa bbb op;�е�bbbΪ"http://" "https://" "$scheme"����opΪredirect permanent��redirect��1����ngx_http_rewrite
    uintptr_t                   redirect:1;//nginx�жϣ��������http://�ȿ�ͷ��rewrite���ʹ����ǿ����ض��򡣻���302����
    //rewrite���Ĳ�����break����rewrite��ĵ�ַ�ڵ�ǰlocation��ǩ��ִ�С�����ο�ngx_http_script_regex_start_code
    uintptr_t                   break_cycle:1;

    //������ʽ��䣬����rewrite  ^(?<name1>/download/.*)/media/(?<name2>.*)/(abc)\..*$   xxx  break;�е� ^(?<name1>/download/.*)/media/(?<name2>.*)/(abc)\..*$ 
    ngx_str_t                   name; 
} ngx_http_script_regex_code_t;


typedef struct { //ngx_http_rewrite�з���ռ䣬��ֵ
    ngx_http_script_code_pt     code;

    uintptr_t                   uri:1;
    uintptr_t                   args:1;

    /* add the r->args to the new arguments */
    uintptr_t                   add_args:1; //��ngx_http_script_regex_code_t->add_args��ȣ���ngx_http_rewrite

    uintptr_t                   redirect:1;//��ngx_http_script_regex_code_t->redirect��ȣ���ngx_http_rewrite
} ngx_http_script_regex_end_code_t;

#endif


typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   conf_prefix;
} ngx_http_script_full_name_code_t;


typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   status; //return code�е�codeת���������
    ngx_http_complex_value_t    text;
} ngx_http_script_return_code_t; //�����ռ�͸�ֵ��ngx_http_rewrite_return


typedef enum {
    ngx_http_script_file_plain = 0,
    ngx_http_script_file_not_plain,
    ngx_http_script_file_dir,
    ngx_http_script_file_not_dir,
    ngx_http_script_file_exists,
    ngx_http_script_file_not_exists,
    ngx_http_script_file_exec,
    ngx_http_script_file_not_exec
} ngx_http_script_file_op_e;


typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   op;
} ngx_http_script_file_code_t;


typedef struct {
    ngx_http_script_code_pt     code;
    uintptr_t                   next;
    void                      **loc_conf;
} ngx_http_script_if_code_t;

/*����set $variable value�����value����ͨ�ַ���������ngx_http_script_value_code_t�����value��һ��$xx�����������
ngx_http_script_complex_value_code_t����ngx_http_rewrite_value,���ն������ngx_http_rewrite_loc_conf_t->codes�У��ο�ngx_http_rewrite_value */
typedef struct {
    ngx_http_script_code_pt     code; //ngx_http_script_complex_value_code
    //��ngx_http_script_compile_t->lengthsһ������ngx_http_rewrite_value  
    ngx_array_t                *lengths; //ngx_http_script_complex_value_code��ִ�и������еĽڵ�pt,
} ngx_http_script_complex_value_code_t;

/*����set $variable value�����value����ͨ�ַ���������ngx_http_script_value_code_t�����value��һ��$xx�����������
ngx_http_script_complex_value_code_t���ο�ngx_http_rewrite_value */
typedef struct { //��ngx_http_rewrite_value
    ngx_http_script_code_pt     code; 
    uintptr_t                   value; //����������ַ��������ֵΪ�����ַ���ת��Ϊ���ֺ��ֵ
    uintptr_t                   text_len; //value�ַ����ĳ���
    uintptr_t                   text_data; //value�ַ���
} ngx_http_script_value_code_t;


void ngx_http_script_flush_complex_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *val);
ngx_int_t ngx_http_complex_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *val, ngx_str_t *value);
ngx_int_t ngx_http_compile_complex_value(ngx_http_compile_complex_value_t *ccv);
char *ngx_http_set_complex_value_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


ngx_int_t ngx_http_test_predicates(ngx_http_request_t *r,
    ngx_array_t *predicates);
char *ngx_http_set_predicate_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

ngx_uint_t ngx_http_script_variables_count(ngx_str_t *value);
ngx_int_t ngx_http_script_compile(ngx_http_script_compile_t *sc);
u_char *ngx_http_script_run(ngx_http_request_t *r, ngx_str_t *value,
    void *code_lengths, size_t reserved, void *code_values);
void ngx_http_script_flush_no_cacheable_variables(ngx_http_request_t *r,
    ngx_array_t *indices);

void *ngx_http_script_start_code(ngx_pool_t *pool, ngx_array_t **codes,
    size_t size);
void *ngx_http_script_add_code(ngx_array_t *codes, size_t size, void *code);

size_t ngx_http_script_copy_len_code(ngx_http_script_engine_t *e);
void ngx_http_script_copy_code(ngx_http_script_engine_t *e);
size_t ngx_http_script_copy_var_len_code(ngx_http_script_engine_t *e);
void ngx_http_script_copy_var_code(ngx_http_script_engine_t *e);
size_t ngx_http_script_copy_capture_len_code(ngx_http_script_engine_t *e);
void ngx_http_script_copy_capture_code(ngx_http_script_engine_t *e);
size_t ngx_http_script_mark_args_code(ngx_http_script_engine_t *e);
void ngx_http_script_start_args_code(ngx_http_script_engine_t *e);
#if (NGX_PCRE)
void ngx_http_script_regex_start_code(ngx_http_script_engine_t *e);
void ngx_http_script_regex_end_code(ngx_http_script_engine_t *e);
#endif
void ngx_http_script_return_code(ngx_http_script_engine_t *e);
void ngx_http_script_break_code(ngx_http_script_engine_t *e);
void ngx_http_script_if_code(ngx_http_script_engine_t *e);
void ngx_http_script_equal_code(ngx_http_script_engine_t *e);
void ngx_http_script_not_equal_code(ngx_http_script_engine_t *e);
void ngx_http_script_file_code(ngx_http_script_engine_t *e);
void ngx_http_script_complex_value_code(ngx_http_script_engine_t *e);
void ngx_http_script_value_code(ngx_http_script_engine_t *e);
void ngx_http_script_set_var_code(ngx_http_script_engine_t *e);
void ngx_http_script_var_set_handler_code(ngx_http_script_engine_t *e);
void ngx_http_script_var_code(ngx_http_script_engine_t *e);
void ngx_http_script_nop_code(ngx_http_script_engine_t *e);


#endif /* _NGX_HTTP_SCRIPT_H_INCLUDED_ */
