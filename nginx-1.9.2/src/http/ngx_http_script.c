
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static ngx_int_t ngx_http_script_init_arrays(ngx_http_script_compile_t *sc);
static ngx_int_t ngx_http_script_done(ngx_http_script_compile_t *sc);
static ngx_int_t ngx_http_script_add_copy_code(ngx_http_script_compile_t *sc,
    ngx_str_t *value, ngx_uint_t last);
static ngx_int_t ngx_http_script_add_var_code(ngx_http_script_compile_t *sc,
    ngx_str_t *name);
static ngx_int_t ngx_http_script_add_args_code(ngx_http_script_compile_t *sc);
#if (NGX_PCRE)
static ngx_int_t ngx_http_script_add_capture_code(ngx_http_script_compile_t *sc,
     ngx_uint_t n);
#endif
static ngx_int_t
     ngx_http_script_add_full_name_code(ngx_http_script_compile_t *sc);
static size_t ngx_http_script_full_name_len_code(ngx_http_script_engine_t *e);
static void ngx_http_script_full_name_code(ngx_http_script_engine_t *e);


#define ngx_http_script_exit  (u_char *) &ngx_http_script_exit_code

static uintptr_t ngx_http_script_exit_code = (uintptr_t) NULL;


void
ngx_http_script_flush_complex_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *val)
{
    ngx_uint_t *index;

    index = val->flushes;

    if (index) {
        while (*index != (ngx_uint_t) -1) {

            if (r->variables[*index].no_cacheable) {
                r->variables[*index].valid = 0;
                r->variables[*index].not_found = 0;
            }

            index++;
        }
    }
}

//��val->lengths��val->values(һ�㶼��һЩ��������������ʽ)  �������code�н�������Ӧ����ͨ�ַ���ֵ��value��
ngx_int_t
ngx_http_complex_value(ngx_http_request_t *r, ngx_http_complex_value_t *val,
    ngx_str_t *value)
{
    size_t                        len;
    ngx_http_script_code_pt       code;
    ngx_http_script_len_code_pt   lcode;
    ngx_http_script_engine_t      e;

    if (val->lengths == NULL) {
        *value = val->value;
        return NGX_OK;
    }

    ngx_http_script_flush_complex_value(r, val);

    ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

    e.ip = val->lengths;
    e.request = r;
    e.flushed = 1;

    len = 0;

    while (*(uintptr_t *) e.ip) {
        lcode = *(ngx_http_script_len_code_pt *) e.ip;
        len += lcode(&e);
    }

    value->len = len;
    value->data = ngx_pnalloc(r->pool, len);
    if (value->data == NULL) {
        return NGX_ERROR;
    }

    e.ip = val->values;
    e.pos = value->data;
    e.buf = *value;

    while (*(uintptr_t *) e.ip) {
        code = *(ngx_http_script_code_pt *) e.ip;
        code((ngx_http_script_engine_t *) &e);
    }

    *value = e.buf;

    return NGX_OK;
}


ngx_int_t
ngx_http_compile_complex_value(ngx_http_compile_complex_value_t *ccv)
{
    ngx_str_t                  *v;
    ngx_uint_t                  i, n, nv, nc;
    ngx_array_t                 flushes, lengths, values, *pf, *pl, *pv;
    ngx_http_script_compile_t   sc;

    v = ccv->value;

    nv = 0;
    nc = 0;

    for (i = 0; i < v->len; i++) {
        if (v->data[i] == '$') {
            if (v->data[i + 1] >= '1' && v->data[i + 1] <= '9') {
                nc++;

            } else {
                nv++;
            }
        }
    }

    if ((v->len == 0 || v->data[0] != '$')
        && (ccv->conf_prefix || ccv->root_prefix))
    {
        if (ngx_conf_full_name(ccv->cf->cycle, v, ccv->conf_prefix) != NGX_OK) {
            return NGX_ERROR;
        }

        ccv->conf_prefix = 0;
        ccv->root_prefix = 0;
    }

    ccv->complex_value->value = *v;
    ccv->complex_value->flushes = NULL;
    ccv->complex_value->lengths = NULL;
    ccv->complex_value->values = NULL;

    if (nv == 0 && nc == 0) {
        return NGX_OK;
    }

    n = nv + 1;

    if (ngx_array_init(&flushes, ccv->cf->pool, n, sizeof(ngx_uint_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    n = nv * (2 * sizeof(ngx_http_script_copy_code_t)
                  + sizeof(ngx_http_script_var_code_t))
        + sizeof(uintptr_t);

    if (ngx_array_init(&lengths, ccv->cf->pool, n, 1) != NGX_OK) {
        return NGX_ERROR;
    }

    n = (nv * (2 * sizeof(ngx_http_script_copy_code_t)
                   + sizeof(ngx_http_script_var_code_t))
                + sizeof(uintptr_t)
                + v->len
                + sizeof(uintptr_t) - 1)
            & ~(sizeof(uintptr_t) - 1);

    if (ngx_array_init(&values, ccv->cf->pool, n, 1) != NGX_OK) {
        return NGX_ERROR;
    }

    pf = &flushes;
    pl = &lengths;
    pv = &values;

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = ccv->cf;
    sc.source = v;
    sc.flushes = &pf;
    sc.lengths = &pl;
    sc.values = &pv;
    sc.complete_lengths = 1;
    sc.complete_values = 1;
    sc.zero = ccv->zero;
    sc.conf_prefix = ccv->conf_prefix;
    sc.root_prefix = ccv->root_prefix;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_ERROR;
    }

    if (flushes.nelts) {
        ccv->complex_value->flushes = flushes.elts;
        ccv->complex_value->flushes[flushes.nelts] = (ngx_uint_t) -1;
    }

    ccv->complex_value->lengths = lengths.elts;
    ccv->complex_value->values = values.elts;

    return NGX_OK;
}

//secure_link md5_str, 120����
char *
ngx_http_set_complex_value_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t                          *value;
    ngx_http_complex_value_t          **cv;
    ngx_http_compile_complex_value_t    ccv;

    cv = (ngx_http_complex_value_t **) (p + cmd->offset);

    if (*cv != NULL) {
        return "duplicate";
    }

    *cv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
    if (*cv == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = *cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

//ngx_http_set_predicate_slot��ֵ,��ngx_http_test_predicates����
ngx_int_t
ngx_http_test_predicates(ngx_http_request_t *r, ngx_array_t *predicates)
{
    ngx_str_t                  val;
    ngx_uint_t                 i;
    ngx_http_complex_value_t  *cv;

    if (predicates == NULL) {
        return NGX_OK;
    }

    cv = predicates->elts;

    for (i = 0; i < predicates->nelts; i++) {
        if (ngx_http_complex_value(r, &cv[i], &val) != NGX_OK) {
            return NGX_ERROR;
        }

        if (val.len && (val.len != 1 || val.data[0] != '0')) {
            return NGX_DECLINED;
        }
    }

    return NGX_OK;
}

//proxy_cache_bypass fastcgi_cache_bypass ����ngx_http_set_predicate_slot��ֵ
//proxy_cache_bypass  xx1 xx2���õ�xx2��Ϊ�ջ��߲�Ϊ0���򲻻�ӻ�����ȡ������ֱ�ӳ��˶�ȡ
//proxy_no_cache  xx1 xx2���õ�xx2��Ϊ�ջ��߲�Ϊ0�����˻��������ݲ��ᱻ����
char * //ngx_http_set_predicate_slot��ֵ,��ngx_http_test_predicates����
ngx_http_set_predicate_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t                          *value;
    ngx_uint_t                          i;
    ngx_array_t                       **a;
    ngx_http_complex_value_t           *cv;
    ngx_http_compile_complex_value_t    ccv;

    a = (ngx_array_t **) (p + cmd->offset);

    if (*a == NGX_CONF_UNSET_PTR) {
        *a = ngx_array_create(cf->pool, 1, sizeof(ngx_http_complex_value_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {
        cv = ngx_array_push(*a);
        if (cv == NULL) {
            return NGX_CONF_ERROR;
        }

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &value[i];
        ccv.complex_value = cv;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}

/*
ngx_http_script_compile_t:/�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣
ngx_http_script_copy_code_t - �������ò������еĹ̶��ַ���ԭ�ⲻ���Ŀ����������ַ�����
ngx_http_script_var_code_t - �������ò������еı���ȡֵ����ӵ������ַ�����

�ű�������غ���
ngx_http_script_variables_count - ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_http_script_compile - �����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��
ngx_http_script_done - ��ӽ�����־֮�����β������
ngx_http_script_add_copy_code - ��������еĹ̶��ַ�������Щ�ַ���Ҫ�ͱ�����ֵƴ�ӳ����ղ���ֵ��
ngx_http_script_run - 
ngx_http_script_add_var_code - Ϊ��������ȡֵ��Ҫ�Ľű�����ʵ�ʱ���ȡֵ�����У�Ϊ��ȷ�����������Ĳ����ڲ���ȡֵ����Ҫ���ڴ���С��
Nginx ��ȡֵ���̷ֳ������ű���һ��������������ֵ���ȣ���һ������ȡ����Ӧ��ֵ��
*/
//һ��������ȡ����������б����м���$���� ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_uint_t
ngx_http_script_variables_count(ngx_str_t *value)
{
    ngx_uint_t  i, n;

    for (n = 0, i = 0; i < value->len; i++) {
        if (value->data[i] == '$') {
            n++;
        }
    }

    return n;
}

/*
ngx_http_script_compile_t:/�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣
ngx_http_script_copy_code_t - �������ò������еĹ̶��ַ���ԭ�ⲻ���Ŀ����������ַ�����
ngx_http_script_var_code_t - �������ò������еı���ȡֵ����ӵ������ַ�����

�ű�������غ���
ngx_http_script_variables_count - ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_http_script_compile - �����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��
ngx_http_script_done - ��ӽ�����־֮�����β������
ngx_http_script_add_copy_code - ��������еĹ̶��ַ�������Щ�ַ���Ҫ�ͱ�����ֵƴ�ӳ����ղ���ֵ��
ngx_http_script_run - 
ngx_http_script_add_var_code - Ϊ��������ȡֵ��Ҫ�Ľű�����ʵ�ʱ���ȡֵ�����У�Ϊ��ȷ�����������Ĳ����ڲ���ȡֵ����Ҫ���ڴ���С��
Nginx ��ȡֵ���̷ֳ������ű���һ��������������ֵ���ȣ���һ������ȡ����Ӧ��ֵ��
*/
//�����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��

/*����set $name ${value}abc���֣���name������ֵ��ͨ����������ngx_http_script_add_var_code��ngx_http_script_add_copy_code�Ѷ�Ӧ��code����
ngx_http_rewrite_loc_conf_t->codes�У���ngx_http_rewrite_handler�б���ִ��codes�Ϳ��԰�$value�ַ�����abc�ַ���ƴ����һ�𣬴���name����
��Ӧ��������r->variables[code->index]
*/
//�����еı��� �ַ��������ȶ�ͨ���ú����Ѷ�Ӧ��code��ӵ�codes������
ngx_int_t 
ngx_http_script_compile(ngx_http_script_compile_t *sc)
//ngx_http_script_compile �����Բ���������������� ngx_http_log_script_t �����У����Բο�access_log��ngx_http_log_set_log
{
    u_char       ch;
    ngx_str_t    name;
    ngx_uint_t   i, bracket;

    if (ngx_http_script_init_arrays(sc) != NGX_OK) {
        return NGX_ERROR;
    }

    for (i = 0; i < sc->source->len; /* void */ ) {

        name.len = 0;

        if (sc->source->data[i] == '$') {
             // ��'$'��β�����д���ģ���Ϊ���ﴦ��Ķ��Ǳ���������������(��������ĩβ��$������˼��) 
            if (++i == sc->source->len) { //$����û�������ַ��ˣ�
                goto invalid_variable;
            }

#if (NGX_PCRE)
            {
                ngx_uint_t  n;
               /* 
                  ע�⣬��������ν�ı��������֣�һ����$������ַ����ģ�һ���Ǹ����ֵġ� 
                  �����ж��Ƿ���������ʽ�ı����� 
                 */ 
                if (sc->source->data[i] >= '1' && sc->source->data[i] <= '9') {//��λ�Ƶ���ʽ����$1,$2...$9�ȱ���

                    n = sc->source->data[i] - '0';

                    //��λ�Ƶ���ʽ����$1,$2...$9�ȱ���������Ӧλ������1����ʾ����Ҫ��������Ϊdup_capture׼���� 
                    if (sc->captures_mask & (1 << n)) {
                        sc->dup_capture = 1;
                    }

                        
                     /* 
                        ��sc->captures_mask�н����ֶ�Ӧ��λ��1����ôcaptures_mask��������ʲô�� �ں����sc�ṹ�����ʱ���ᵽ�� 
                        */  
                    sc->captures_mask |= 1 << n;
                
                    if (ngx_http_script_add_capture_code(sc, n) != NGX_OK) {
                        return NGX_ERROR;
                    }

                    i++;

                    continue;
                }
            }
#endif

          /* 
             * �����Ǹ�����˼�ĵط����ٸ����ӣ������и�����һ������proxy_pass $host$uritest�� 
             * ����������ʵ������nginx���������ñ�����host��uri�����Ƕ���$uritest��˵��������� 
             * ���Ӵ�����ô�ں���������ԻὫuritest���������Ϊһ������������Ȼ����������Ҫ�ġ� 
             * ����ô���أ�nginx����ʹ��"{}"����һЩ��������������������������ַ�������һ���ڴ˴� 
             * ���ǿ���������${uri}test����Ȼ����֮�������֣���ĸ�����»���֮����ַ����б�Ҫ�������� 
             * ���������ֵĺ����ԡ� 
             */
            if (sc->source->data[i] == '{') {
                bracket = 1;

                if (++i == sc->source->len) {
                    goto invalid_variable;
                }

                //name��������һ��������ı���   
                name.data = &sc->source->data[i]; 

            } else {
                bracket = 0;
                name.data = &sc->source->data[i];
            }

            for ( /* void */ ; i < sc->source->len; i++, name.len++) {
                ch = sc->source->data[i];
                
                // ��"{}"�е��ַ����ᱻ�������(��break���)�������������ַ�������һ��   
                if (ch == '}' && bracket) {
                    i++;
                    bracket = 0;
                    break;
                }

                 /* 
                     ������������ֵ��ַ��������ַ������Ǳ������ַ������Կո��ǿ������ֱ����ġ� 
                     ��������������ﾭ�����Ըо�����������ԭ�������������ʾ���� 
                    */ 
                if ((ch >= 'A' && ch <= 'Z')
                    || (ch >= 'a' && ch <= 'z')
                    || (ch >= '0' && ch <= '9')
                    || ch == '_')
                {
                    continue;
                }

                break;
            }

            if (bracket) { //�������{�ͱ�����}��β
                ngx_conf_log_error(NGX_LOG_EMERG, sc->cf, 0,
                                   "the closing bracket in \"%V\" "
                                   "variable is missing", &name);
                return NGX_ERROR;
            }

            if (name.len == 0) {
                goto invalid_variable;
            }

            sc->variables++;// ��������   

            // �õ�һ��������������   
            if (ngx_http_script_add_var_code(sc, &name) != NGX_OK) { 
                return NGX_ERROR;
            }

            continue;
        }

       /*  
          ����������ζ��һ�������������(����ͨ�ַ���)�����߻�û������������һЩ�Ǳ������ַ��������ﲻ����Ϊ�������ַ����� 
          �����漰������������ֵĴ����Ƚϼ򵥡�����ط�һ������һ�η���������߳��������󣬺������'?'����� 
          ��صĴ�������ngx_http_script_add_args_code�����á� 
         */ 
        if (sc->source->data[i] == '?' && sc->compile_args) {
            sc->args = 1;
            sc->compile_args = 0;

            if (ngx_http_script_add_args_code(sc) != NGX_OK) {
                return NGX_ERROR;
            }

            i++;

            continue;
        }
        
        // ����name����һ����ν�ġ������ַ�����   
        name.data = &sc->source->data[i];
        // ����ó����ַ���   
        while (i < sc->source->len) {

            if (sc->source->data[i] == '$') {// ����'$'��ζ����������һ������ 
                break;
            }

            
            /* 
                 �˴���ζ��������һ�������ַ������������������'?'��������ǲ���Ҫ��������������⴦��Ļ��� 
                 ��sc->compile_args = 0����ô���Ǿͽ�����Ϊ�����ַ�����һ�������������򣬵�ǰ�ĳ����ַ����� 
                 ��'?'�����ضϣ��ֳ������֡�
               */  
            if (sc->source->data[i] == '?') {

                sc->args = 1;

                if (sc->compile_args) {
                    break;
                }
            }

            i++;
            name.len++;
        }
        
        // һ�������ַ���������ϣ�sc->sizeͳ�������ַ���(��sc->source)�У������ַ������ܳ���   
        sc->size += name.len;
        // �����ַ����Ĵ��������������������
        if (ngx_http_script_add_copy_code(sc, &name, (i == sc->source->len))
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    // ����compile��������һЩ��β�ƺ�����  
    return ngx_http_script_done(sc);

invalid_variable:

    ngx_conf_log_error(NGX_LOG_EMERG, sc->cf, 0, "invalid variable name");

    return NGX_ERROR;
}

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
u_char *
ngx_http_script_run(ngx_http_request_t *r, ngx_str_t *value,
    void *code_lengths, size_t len, void *code_values)
{
    ngx_uint_t                    i;
    ngx_http_script_code_pt       code;
    ngx_http_script_len_code_pt   lcode;
    ngx_http_script_engine_t      e;
    ngx_http_core_main_conf_t    *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    for (i = 0; i < cmcf->variables.nelts; i++) {
        if (r->variables[i].no_cacheable) {
            r->variables[i].valid = 0;
            r->variables[i].not_found = 0;
        }
    }

    ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

    e.ip = code_lengths;
    e.request = r;
    e.flushed = 1;

    while (*(uintptr_t *) e.ip) {
        lcode = *(ngx_http_script_len_code_pt *) e.ip;
        len += lcode(&e);
    }


    value->len = len;
    value->data = ngx_pnalloc(r->pool, len);
    if (value->data == NULL) {
        return NULL;
    }

    e.ip = code_values;
    e.pos = value->data;

    while (*(uintptr_t *) e.ip) {
        code = *(ngx_http_script_code_pt *) e.ip;
        code((ngx_http_script_engine_t *) &e);
    }

    return e.pos;
}


void
ngx_http_script_flush_no_cacheable_variables(ngx_http_request_t *r,
    ngx_array_t *indices)
{
    ngx_uint_t  n, *index;

    if (indices) {
        index = indices->elts;
        for (n = 0; n < indices->nelts; n++) {
            if (r->variables[index[n]].no_cacheable) {
                r->variables[index[n]].valid = 0;
                r->variables[index[n]].not_found = 0;
            }
        }
    }
}

static ngx_int_t
ngx_http_script_init_arrays(ngx_http_script_compile_t *sc)
{
    ngx_uint_t   n;

    if (sc->flushes && *sc->flushes == NULL) {
        n = sc->variables ? sc->variables : 1;
        *sc->flushes = ngx_array_create(sc->cf->pool, n, sizeof(ngx_uint_t));
        if (*sc->flushes == NULL) {
            return NGX_ERROR;
        }
    }

    if (*sc->lengths == NULL) {
        n = sc->variables * (2 * sizeof(ngx_http_script_copy_code_t)
                             + sizeof(ngx_http_script_var_code_t))
            + sizeof(uintptr_t);

        //����n��1�ֽڿռ�����飬ÿ�������Ա��1���ֽ�
        *sc->lengths = ngx_array_create(sc->cf->pool, n, 1);
        if (*sc->lengths == NULL) {
            return NGX_ERROR;
        }
    }

    if (*sc->values == NULL) {
        n = (sc->variables * (2 * sizeof(ngx_http_script_copy_code_t)
                              + sizeof(ngx_http_script_var_code_t))
                + sizeof(uintptr_t)
                + sc->source->len
                + sizeof(uintptr_t) - 1)
            & ~(sizeof(uintptr_t) - 1);

        *sc->values = ngx_array_create(sc->cf->pool, n, 1);
        if (*sc->values == NULL) {
            return NGX_ERROR;
        }
    }

    sc->variables = 0;

    return NGX_OK;
}

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
static ngx_int_t
ngx_http_script_done(ngx_http_script_compile_t *sc)//��ӽ�����־֮�����β������
{
    ngx_str_t    zero;
    uintptr_t   *code;

    if (sc->zero) {

        zero.len = 1;
        zero.data = (u_char *) "\0";

        if (ngx_http_script_add_copy_code(sc, &zero, 0) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    if (sc->conf_prefix || sc->root_prefix) {
        if (ngx_http_script_add_full_name_code(sc) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    if (sc->complete_lengths) {
        code = ngx_http_script_add_code(*sc->lengths, sizeof(uintptr_t), NULL);
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;
    }

    if (sc->complete_values) {
        code = ngx_http_script_add_code(*sc->values, sizeof(uintptr_t),
                                        &sc->main);
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;
    }

    return NGX_OK;
}


void *
ngx_http_script_start_code(ngx_pool_t *pool, ngx_array_t **codes, size_t size)
{
    if (*codes == NULL) {
        *codes = ngx_array_create(pool, 256, 1);
        if (*codes == NULL) {
            return NULL;
        }
    }

    return ngx_array_push_n(*codes, size);
}

/* �� codes�ڴ���п��� size �ֽ� �Ŀռ������ڻ�ȡ������Ӧ��ֵ���ȵĽű�
 */
void *
ngx_http_script_add_code(ngx_array_t *codes, size_t size, void *code)
{
    u_char  *elts, **p;
    void    *new;

    elts = codes->elts;

    new = ngx_array_push_n(codes, size);//codes->elts���ܻ�仯�ġ���������Ѿ�������Ҫ����һ�����ڴ�
    if (new == NULL) {
        return NULL;
    }

    if (code) {
        if (elts != codes->elts) { //����ڴ�仯�ˣ�
            p = code; //��Ϊcode���������&sc->main���֣�Ҳ����ָ����������ݣ������Ҫ����һ��λ����Ϣ��
            *p += (u_char *) codes->elts - elts; //����ʲô��˼����������������ڴ��λ�ơ�
        }
    }

    return new;
}

//�����е�$name���ֱ����ַ�������ngx_http_script_add_var_code����ͨ���ַ�������ngx_http_script_add_copy_code
//ngx_http_script_add_copy_code - ��������еĹ̶��ַ�������Щ�ַ���Ҫ�ͱ�����ֵƴ�ӳ����ղ���ֵ��
static ngx_int_t
ngx_http_script_add_copy_code(ngx_http_script_compile_t *sc, ngx_str_t *value,
    ngx_uint_t last) //last����Ƿ��ǲ����б��еĵ����һ������
{
    u_char                       *p;
    size_t                        size, len, zero;
    ngx_http_script_copy_code_t  *code;

    zero = (sc->zero && last);
    len = value->len + zero;

    //����lengths������ֻ�Ǵ�ı������Ⱥ�code��values�����д��б������ȣ�����ֵ�Ͷ�Ӧ��code

    
    /* �� lengths �ڴ���п��� sizeof(ngx_http_script_copy_code_t) �ֽڵĿռ����ڴ�Ź̶��ַ����ĳ��� */
    code = ngx_http_script_add_code(*sc->lengths,
                                    sizeof(ngx_http_script_copy_code_t), NULL);
    if (code == NULL) {
        return NGX_ERROR;
    }

    
    /* ������ʱ���� len */ //��lengths�е�code��ֵ
    code->code = (ngx_http_script_code_pt) ngx_http_script_copy_len_code;
    code->len = len;

    
    /* �̶��ַ��������ڻ�ȡ�˹̶��ַ����Ľű���Ҫ�Ĵ洢�ռ� �������ngx_http_script_copy_code_t�ṹ�������size - ngx_http_script_copy_code_t�ռ������value����*/
    size = (sizeof(ngx_http_script_copy_code_t) + len + sizeof(uintptr_t) - 1)
            & ~(sizeof(uintptr_t) - 1);
    
    /* �� values �ڴ���п��� size �ֽڵĿռ����ڴ洢�̶��ַ����Ͳ����ű� */
    code = ngx_http_script_add_code(*sc->values, size, &sc->main);
    if (code == NULL) {
        return NGX_ERROR;
    }
    /* ������ʱ�����Ĺ̶��ַ������� */
    code->code = ngx_http_script_copy_code;
    code->len = len;

    
    /* ���̶��ַ����ݴ��� values �� */
    p = ngx_cpymem((u_char *) code + sizeof(ngx_http_script_copy_code_t),
                   value->data, value->len); //��value���ݿ�����ngx_http_script_copy_code_tƨ�ɺ��棬

    if (zero) {
        *p = '\0';
        sc->zero = 0;
    }

    return NGX_OK;
}


size_t
ngx_http_script_copy_len_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_copy_code_t  *code;

    code = (ngx_http_script_copy_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_copy_code_t);

    return code->len;
}

//ngx_http_script_copy_codeΪ����������p buf�У�ngx_http_script_copy_var_codeΪ����������Ӧ��value
void
ngx_http_script_copy_code(ngx_http_script_engine_t *e)
{
    u_char                       *p;
    ngx_http_script_copy_code_t  *code;

    code = (ngx_http_script_copy_code_t *) e->ip;

    p = e->pos;

    if (!e->skip) {//�ڸú��������追������
        e->pos = ngx_copy(p, e->ip + sizeof(ngx_http_script_copy_code_t),
                          code->len);
    }

    e->ip += sizeof(ngx_http_script_copy_code_t)
          + ((code->len + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t) - 1));

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script copy: \"%*s\"", e->pos - p, p);
}

/*
ngx_http_script_compile_t:/�ű��������ṹ�� - ��Ϊ�ű������������Ĳ���ʱ��ͳһ���롣
ngx_http_script_copy_code_t - �������ò������еĹ̶��ַ���ԭ�ⲻ���Ŀ����������ַ�����
ngx_http_script_var_code_t - �������ò������еı���ȡֵ����ӵ������ַ�����

�ű�������غ���
ngx_http_script_variables_count - ���� $ �����ڵĴ���ͳ�Ƴ�һ������������г����˶��ٸ�������
ngx_http_script_compile - �����������Ĳ����ű������Ա���Ҫ�Բ������廯ʱ���ýű�������ֵ��
ngx_http_script_done - ��ӽ�����־֮�����β������
ngx_http_script_run - 
ngx_http_script_add_copy_code - ��������еĹ̶��ַ�������Щ�ַ���Ҫ�ͱ�����ֵƴ�ӳ����ղ���ֵ��
ngx_http_script_add_var_code - Ϊ��������ȡֵ��Ҫ�Ľű�����ʵ�ʱ���ȡֵ�����У�Ϊ��ȷ�����������Ĳ����ڲ���ȡֵ����Ҫ���ڴ���С��
Nginx ��ȡֵ���̷ֳ������ű���һ��������������ֵ���ȣ���һ������ȡ����Ӧ��ֵ��
*/ //ͨ��name���ҵ�variables�е�С�꣬Ȼ��������л�ȡngx_http_script_var_code_t�ڵ㣬��index��code��ֵ����
//�����е�$name���ֱ����ַ�������ngx_http_script_add_var_code����ͨ���ַ�������ngx_http_script_add_copy_code

/*
�����set $xxx $bbb�ú������ngx_http_script_complex_value_code��ȡ��ngx_http_script_complex_value_code����code->lengths(ngx_http_script_copy_var_len_code)
��ȷ�������ַ����ĳ��ȣ�Ȼ�󿪱ٶ�Ӧ��e->buf�ռ䣬�����ngx_http_script_copy_var_code�аѱ�����������e->buf�ռ���
*/
static ngx_int_t
ngx_http_script_add_var_code(ngx_http_script_compile_t *sc, ngx_str_t *name)//name��ӵ�flushes  lengths  values��
{
    ngx_int_t                    index, *p;
    ngx_http_script_var_code_t  *code;

    //���ݱ������֣���ȡ����&cmcf->variables������±ꡣ���û�У����½�����
    index = ngx_http_get_variable_index(sc->cf, name);

    if (index == NGX_ERROR) {
        return NGX_ERROR;
    }

    if (sc->flushes) {
        p = ngx_array_push(*sc->flushes);
        if (p == NULL) {
            return NGX_ERROR;
        }

        *p = index;
    }

    
    /* �� lengths �ڴ���п��� sizeof(ngx_http_script_var_code_t) �ֽ�
          �Ŀռ������ڻ�ȡ������Ӧ��ֵ���ȵĽű�
        */ //lengths�����е�ÿ����Ա��һ�ֽ�
    code = ngx_http_script_add_code(*sc->lengths,
                                    sizeof(ngx_http_script_var_code_t), NULL); //lengths�еĽڵ���ngx_http_script_complex_value_codeִ��
    if (code == NULL) {
        return NGX_ERROR;
    }

    code->code = (ngx_http_script_code_pt) ngx_http_script_copy_var_len_code;
    code->index = (uintptr_t) index;

    code = ngx_http_script_add_code(*sc->values,
                                    sizeof(ngx_http_script_var_code_t),
                                    &sc->main);
    if (code == NULL) {
        return NGX_ERROR;
    }
    
    /* ngx_http_script_copy_var_code ���ڻ�ȡ index ��Ӧ�ı���ȡֵ */
    code->code = ngx_http_script_copy_var_code;
    code->index = (uintptr_t) index; //ngx_http_variable_t->index

    return NGX_OK;
}

//��ȡinex��Ӧ����ֵ���ַ�������
size_t
ngx_http_script_copy_var_len_code(ngx_http_script_engine_t *e)
{
    ngx_http_variable_value_t   *value;
    ngx_http_script_var_code_t  *code;

    code = (ngx_http_script_var_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_var_code_t);

    if (e->flushed) {
        value = ngx_http_get_indexed_variable(e->request, code->index);

    } else {
        value = ngx_http_get_flushed_variable(e->request, code->index);
    }

    if (value && !value->not_found) {
        return value->len;
    }

    return 0;
}

// /* ngx_http_script_copy_var_code ���ڻ�ȡ index ��Ӧ�ı���ȡֵ */
void //��ȡ����ֵ  //ngx_http_script_copy_codeΪ����������p buf�У�ngx_http_script_copy_var_codeΪ����������Ӧ��value
ngx_http_script_copy_var_code(ngx_http_script_engine_t *e)
{
    u_char                      *p;
    ngx_http_variable_value_t   *value;
    ngx_http_script_var_code_t  *code;

    code = (ngx_http_script_var_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_var_code_t);

    if (!e->skip) {

        if (e->flushed) {
            value = ngx_http_get_indexed_variable(e->request, code->index);

        } else {
            value = ngx_http_get_flushed_variable(e->request, code->index);
        }

        if (value && !value->not_found) {
            p = e->pos;
            e->pos = ngx_copy(p, value->data, value->len); //ת��index��Ӧ�ı���ֵ��e->buf��

            ngx_log_debug2(NGX_LOG_DEBUG_HTTP,
                           e->request->connection->log, 0,
                           "http script var: \"%*s\"", e->pos - p, p);
        }
    }
}

//set $ttt  xxxx?bbb���֣������Ĳ����д���?��ִ�иú�����
static ngx_int_t
ngx_http_script_add_args_code(ngx_http_script_compile_t *sc)
{
    uintptr_t   *code;

    code = ngx_http_script_add_code(*sc->lengths, sizeof(uintptr_t), NULL);
    if (code == NULL) {
        return NGX_ERROR;
    }

    *code = (uintptr_t) ngx_http_script_mark_args_code;

    code = ngx_http_script_add_code(*sc->values, sizeof(uintptr_t), &sc->main);
    if (code == NULL) {
        return NGX_ERROR;
    }

    *code = (uintptr_t) ngx_http_script_start_args_code;

    return NGX_OK;
}


size_t
ngx_http_script_mark_args_code(ngx_http_script_engine_t *e)
{
    e->is_args = 1;
    e->ip += sizeof(uintptr_t);

    return 1;
}


void
ngx_http_script_start_args_code(ngx_http_script_engine_t *e)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script args");

    e->is_args = 1;
    e->args = e->pos;
    e->ip += sizeof(uintptr_t);
}


#if (NGX_PCRE)

/*
1.	����������ʽ�������URL�����У����ƥ��ʧ�ܣ���e->ip += code->next;�õ��÷�������һ�����ʽ����н�����
2.����ɹ�������code->lengths���Ӷ���ȡ������ʽ�滻����ַ������ȣ��Ա��ڴ˺������غ��code�����������ܹ��洢���ַ������ȡ�
*/
//ngx_http_script_regex_start_code��ngx_http_script_regex_end_code���ʹ��
void
ngx_http_script_regex_start_code(ngx_http_script_engine_t *e)
{
//ƥ��������ʽ������Ŀ���ַ������Ȳ�����ռ䡣���������ÿ��rewrite������ȵ��õĽ���������
//����������ƥ�䣬��Ŀ���ַ������ȼ��㣬����lengths lcodes�������
    size_t                         len;
    ngx_int_t                      rc;
    ngx_uint_t                     n;
    ngx_http_request_t            *r;
    ngx_http_script_engine_t       le;
    ngx_http_script_len_code_pt    lcode;
    ngx_http_script_regex_code_t  *code;

    code = (ngx_http_script_regex_code_t *) e->ip;

    r = e->request;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http script regex: \"%V\"", &code->name);

    if (code->uri) { //rewrite����,���ǱȽ�uri��rewrite xxx yyy break;�е�yyy�Ƿ�ƥ�䣬
        e->line = r->uri;
    } else { //if����
        e->sp--;
        e->line.len = e->sp->len;
        e->line.data = e->sp->data;
    }

    //�������Ѿ������regex ��e->lineȥƥ�䣬�����Ƿ�ƥ��ɹ���
    rc = ngx_http_regex_exec(r, code->regex, &e->line);

    if (rc == NGX_DECLINED) {
        if (e->log || (r->connection->log->log_level & NGX_LOG_DEBUG_HTTP)) {
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                          "\"%V\" does not match \"%V\"",
                          &code->name, &e->line);
        }

        r->ncaptures = 0;

        if (code->test) { //if{}���òŻ���1
            if (code->negative_test) {   
                e->sp->len = 1;
                e->sp->data = (u_char *) "1";

            } else {
                e->sp->len = 0;
                e->sp->data = (u_char *) "";
            }

            e->sp++; //�ƶ�����һ���ڵ㡣���ء�

            e->ip += sizeof(ngx_http_script_regex_code_t); 
            return;
        }
        
        //next�ĺ���Ϊ;�����ǰcodeƥ��ʧ�ܣ���ô��һ��code��λ������ʲô�ط�����Щ����ȫ������һ����������ġ�
        e->ip += code->next;
        return;
    }

    if (rc == NGX_ERROR) {
        e->ip = ngx_http_script_exit;
        e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    if (e->log || (r->connection->log->log_level & NGX_LOG_DEBUG_HTTP)) {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                      "\"%V\" matches \"%V\"", &code->name, &e->line);
    }

    if (code->test) {//if{}���òŻ���1 //���ƥ��ɹ��ˣ�������һ����־�ɣ�����������ifƥ���ʱ�����ͨ���鿴��ջ��ֵ��֪���Ƿ�ɹ���
        if (code->negative_test) {
            e->sp->len = 0;
            e->sp->data = (u_char *) "";

        } else {
            e->sp->len = 1;
            e->sp->data = (u_char *) "1";
        }

        e->sp++;

        e->ip += sizeof(ngx_http_script_regex_code_t);
        return;
    }

    if (code->status) { //�����rewrite�������ã���ngx_http_rewrite��ֵ
        e->status = code->status;

        if (!code->redirect) {
            e->ip = ngx_http_script_exit;
            return;
        }
    }

    if (code->uri) {
        r->internal = 1;
        r->valid_unparsed_uri = 0;

        if (code->break_cycle) { //rewrite���Ĳ�����break����rewrite��ĵ�ַ�ڵ�ǰlocation��ǩ��ִ��
            r->valid_location = 0;
            r->uri_changed = 0; //��uri_changed����Ϊ0��Ҳ�ͱ�־˵URLû�б仯����ô��
            //��ngx_http_core_post_rewrite_phase�оͲ���ִ�������if��䣬Ҳ�Ͳ����ٴ��ߵ�find config�Ĺ����ˣ����Ǽ����������ġ�
            //��Ȼ���������rewrite�ɹ����ǻ�������һ�εģ��൱��һ��ȫ�µ�����

        } else {
            r->uri_changed = 1; //��Ҫrewrite����ѭ������ngx_http_core_post_rewrite_phase  ���½����ض�����ҹ���
        }
    }

    if (code->lengths == NULL) {//������沿���Ǽ��ַ������� rewrite ^(.*)$ http://chenzhenianqing.cn break;
        e->buf.len = code->size;

        if (code->uri) {
            if (r->ncaptures && (r->quoted_uri || r->plus_in_uri)) {
                e->buf.len += 2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len,
                                                 NGX_ESCAPE_ARGS);
            }
        }

        for (n = 2; n < r->ncaptures; n += 2) {
            e->buf.len += r->captures[n + 1] - r->captures[n];
        }

    } else { /*һ����ȥ�����ӱ��ʽ������������ʵֻ����һ�´�С�ģ�
        ���������ݿ������ϲ��code��ȡ������ rewrite ^(.*)$ http://$http_host.mp4 break;
        //�����ֲ��ģ�ƴװ�������url,������������ӣ�Ϊ
			ngx_http_script_copy_len_code		7
			ngx_http_script_copy_var_len_code 	18
			ngx_http_script_copy_len_code		4	=== 29 
		����ֻ����һ�³��ȣ�����lengths�󳤶ȡ����ݿ�����ngx_http_rewrite_handler�У����������غ�͵������¹��̿�������: 
			ngx_http_script_copy_code		����"http://" ��e->buf
			ngx_http_script_copy_var_code	����"115.28.34.175:8881"
			ngx_http_script_copy_code 		����".mp4"
        */
        ngx_memzero(&le, sizeof(ngx_http_script_engine_t));

        le.ip = code->lengths->elts;
        le.line = e->line;
        le.request = r;
        le.quote = code->redirect;

        len = 0;
 
        while (*(uintptr_t *) le.ip) { //���ַ������ܳ���
            lcode = *(ngx_http_script_len_code_pt *) le.ip;  
            len += lcode(&le);
        }

        e->buf.len = len;//��ס�ܳ��ȡ�
    }

    if (code->add_args && r->args.len) { //�Ƿ���Ҫ�Զ����Ӳ�������������еĺ�����ʾ�ļ�����?���ţ���nginx����׷�Ӳ�����
        e->buf.len += r->args.len + 1; //Ϊ�����ngx_http_script_regex_end_code��׼��
    }

    //Ϊǰ����Щcode�����ĵ��Ĳ���ֵ���ܳ��ȼ�������ˣ�������ִ��ngx_http_script_regex_end_code��ʱ�������ַ���������e->buf
    e->buf.data = ngx_pnalloc(r->pool, e->buf.len);
    if (e->buf.data == NULL) {
        e->ip = ngx_http_script_exit;
        e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    e->quote = code->redirect;

    e->pos = e->buf.data;

    e->ip += sizeof(ngx_http_script_regex_code_t);
}


//ngx_http_script_regex_start_code��ngx_http_script_regex_end_code���ʹ��
void
ngx_http_script_regex_end_code(ngx_http_script_engine_t *e)
{//ò��û��ʲô���飬�����redirect����������һ��ͷ��header��location����302�ˡ�
    u_char                            *dst, *src;
    ngx_http_request_t                *r;
    ngx_http_script_regex_end_code_t  *code;

    code = (ngx_http_script_regex_end_code_t *) e->ip;

    r = e->request;

    e->quote = 0;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http script regex end");

    if (code->redirect) {

        dst = e->buf.data;
        src = e->buf.data;

        ngx_unescape_uri(&dst, &src, e->pos - e->buf.data,
                         NGX_UNESCAPE_REDIRECT);

        if (src < e->pos) {
            dst = ngx_movemem(dst, src, e->pos - src); //��ȡ������Ľ��
        }

        e->pos = dst;

        if (code->add_args && r->args.len) {//���uri�д�?,�����?���ʺź���Ĳ�������������ַ������棬��ʵ���ǰ������uri��?ǰ����ַ���������
            *e->pos++ = (u_char) (code->args ? '&' : '?');
            e->pos = ngx_copy(e->pos, r->args.data, r->args.len); //
        }

        e->buf.len = e->pos - e->buf.data;

        if (e->log || (r->connection->log->log_level & NGX_LOG_DEBUG_HTTP)) {
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                          "rewritten redirect: \"%V\"", &e->buf);
        }

        ngx_http_clear_location(r);

        r->headers_out.location = ngx_list_push(&r->headers_out.headers);
        if (r->headers_out.location == NULL) {
            e->ip = ngx_http_script_exit;
            e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            return;
        }

        /*
          rewrite ^(.*)$ http://$1.mp4 break; ���uriΪhttp://10.135.0.1/aaa,��location�д洢����aaa.mp4
          */
        r->headers_out.location->hash = 1;
        ngx_str_set(&r->headers_out.location->key, "Location");
        r->headers_out.location->value = e->buf; //ָ��rewrite����µ��ض���uri

        e->ip += sizeof(ngx_http_script_regex_end_code_t);
        return;
    }

    if (e->args) {
        e->buf.len = e->args - e->buf.data;

        if (code->add_args && r->args.len) {
            *e->pos++ = '&';
            e->pos = ngx_copy(e->pos, r->args.data, r->args.len);
        }

        r->args.len = e->pos - e->args;
        r->args.data = e->args;

        e->args = NULL;

    } else {
        e->buf.len = e->pos - e->buf.data;

        if (!code->add_args) {
            r->args.len = 0;
        }
    }

    /* 
      ����Ϊ:
      location ~* /1mytest  {			
            rewrite   ^.*$ www.11.com/ last;		
       }  
      uriΪ:http://10.135.10.167/1mytest ,���ߵ������uri���Ϊwww.11.com/
     */
    if (e->log || (r->connection->log->log_level & NGX_LOG_DEBUG_HTTP)) {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                      "rewritten data: \"%V\", args: \"%V\"",
                      &e->buf, &r->args);
    }

    if (code->uri) {
        r->uri = e->buf; //uriָ���µ�rewrite���uri

        if (r->uri.len == 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "the rewritten URI has a zero length");
            e->ip = ngx_http_script_exit;
            e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            return;
        }

        ngx_http_set_exten(r);
    }

    e->ip += sizeof(ngx_http_script_regex_end_code_t);
}

//$1,$2...$9�ȱ���  ���ngx_http_script_complex_value_code�Ķ�
static ngx_int_t
ngx_http_script_add_capture_code(ngx_http_script_compile_t *sc, ngx_uint_t n) //nΪ$3�е�3
{
    ngx_http_script_copy_capture_code_t  *code;

    code = ngx_http_script_add_code(*sc->lengths,
                                    sizeof(ngx_http_script_copy_capture_code_t),
                                    NULL);
    if (code == NULL) {
        return NGX_ERROR;
    }

    code->code = (ngx_http_script_code_pt)
                      ngx_http_script_copy_capture_len_code; //��λ$1����ֵ���ַ�������
    code->n = 2 * n;


    code = ngx_http_script_add_code(*sc->values,
                                    sizeof(ngx_http_script_copy_capture_code_t),
                                    &sc->main);
    if (code == NULL) {
        return NGX_ERROR;
    }

    code->code = ngx_http_script_copy_capture_code; //��ȡ����$1����ֵ���ַ���
    code->n = 2 * n;

    if (sc->ncaptures < n) {
        sc->ncaptures = n;
    }

    return NGX_OK;
}


size_t
ngx_http_script_copy_capture_len_code(ngx_http_script_engine_t *e)
{
    int                                  *cap;
    u_char                               *p;
    ngx_uint_t                            n;
    ngx_http_request_t                   *r;
    ngx_http_script_copy_capture_code_t  *code;

    r = e->request;

    code = (ngx_http_script_copy_capture_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_copy_capture_code_t);

    n = code->n;

    if (n < r->ncaptures) {

        cap = r->captures;

        if ((e->is_args || e->quote)
            && (e->request->quoted_uri || e->request->plus_in_uri))
        {
            p = r->captures_data;

            return cap[n + 1] - cap[n]
                   + 2 * ngx_escape_uri(NULL, &p[cap[n]], cap[n + 1] - cap[n],
                                        NGX_ESCAPE_ARGS);
        } else {
            return cap[n + 1] - cap[n];
        }
    }

    return 0;
}


void
ngx_http_script_copy_capture_code(ngx_http_script_engine_t *e)
{
    int                                  *cap;
    u_char                               *p, *pos;
    ngx_uint_t                            n;
    ngx_http_request_t                   *r;
    ngx_http_script_copy_capture_code_t  *code;

    r = e->request;

    code = (ngx_http_script_copy_capture_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_copy_capture_code_t);

    n = code->n;

    pos = e->pos;

    if (n < r->ncaptures) {

        cap = r->captures;
        p = r->captures_data;

        if ((e->is_args || e->quote)
            && (e->request->quoted_uri || e->request->plus_in_uri))
        {
            e->pos = (u_char *) ngx_escape_uri(pos, &p[cap[n]],
                                               cap[n + 1] - cap[n],
                                               NGX_ESCAPE_ARGS);
        } else {
            e->pos = ngx_copy(pos, &p[cap[n]], cap[n + 1] - cap[n]);
        }
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script capture: \"%*s\"", e->pos - pos, pos);
}

#endif


static ngx_int_t
ngx_http_script_add_full_name_code(ngx_http_script_compile_t *sc)
{
    ngx_http_script_full_name_code_t  *code;

    code = ngx_http_script_add_code(*sc->lengths,
                                    sizeof(ngx_http_script_full_name_code_t),
                                    NULL);
    if (code == NULL) {
        return NGX_ERROR;
    }

    code->code = (ngx_http_script_code_pt) ngx_http_script_full_name_len_code;
    code->conf_prefix = sc->conf_prefix;

    code = ngx_http_script_add_code(*sc->values,
                                    sizeof(ngx_http_script_full_name_code_t),
                                    &sc->main);
    if (code == NULL) {
        return NGX_ERROR;
    }

    code->code = ngx_http_script_full_name_code;
    code->conf_prefix = sc->conf_prefix;

    return NGX_OK;
}


static size_t
ngx_http_script_full_name_len_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_full_name_code_t  *code;

    code = (ngx_http_script_full_name_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_full_name_code_t);

    return code->conf_prefix ? ngx_cycle->conf_prefix.len:
                               ngx_cycle->prefix.len;
}


static void
ngx_http_script_full_name_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_full_name_code_t  *code;

    ngx_str_t  value, *prefix;

    code = (ngx_http_script_full_name_code_t *) e->ip;

    value.data = e->buf.data;
    value.len = e->pos - e->buf.data;

    prefix = code->conf_prefix ? (ngx_str_t *) &ngx_cycle->conf_prefix:
                                 (ngx_str_t *) &ngx_cycle->prefix;

    if (ngx_get_full_name(e->request->pool, prefix, &value) != NGX_OK) {
        e->ip = ngx_http_script_exit;
        e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    e->buf = value;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script fullname: \"%V\"", &value);

    e->ip += sizeof(ngx_http_script_full_name_code_t);
}

//����return code�����÷�����Ӧ��
//��ngx_http_rewrite_handler(ngx_http_request_t *r) ����Ķ���e->ip = rlcf->codes->elts; 
void
ngx_http_script_return_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_return_code_t  *code;

    code = (ngx_http_script_return_code_t *) e->ip;

    if (code->status < NGX_HTTP_BAD_REQUEST
        || code->text.value.len
        || code->text.lengths)
    {
        e->status = ngx_http_send_response(e->request, code->status, NULL,
                                           &code->text);
    } else {
        e->status = code->status;
    }

    e->ip = ngx_http_script_exit; //ִ�и�code�󣬲�����ִ��֮ǰ�����ڸ�code�������������code
}


void
ngx_http_script_break_code(ngx_http_script_engine_t *e)
{
    e->request->uri_changed = 0; //��ֵΪ0����ʾ�������ظ�rewrite find

    e->ip = ngx_http_script_exit; //ֹͣ����ĸ��ֱ�����ֵ�����ȣ�Ҳ����ֹͣ�ű�����
}


void
ngx_http_script_if_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_if_code_t  *code;

    code = (ngx_http_script_if_code_t *) e->ip;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script if");

    e->sp--;

    if (e->sp->len && (e->sp->len != 1 || e->sp->data[0] != '0')) {
        if (code->loc_conf) {
            e->request->loc_conf = code->loc_conf;
            ngx_http_update_location_config(e->request);
        }

        e->ip += sizeof(ngx_http_script_if_code_t);
        return;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script if: false");

    e->ip += code->next;
}


void
ngx_http_script_equal_code(ngx_http_script_engine_t *e)
{
    ngx_http_variable_value_t  *val, *res;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script equal");

    e->sp--;
    val = e->sp;
    res = e->sp - 1;

    e->ip += sizeof(uintptr_t);

    if (val->len == res->len
        && ngx_strncmp(val->data, res->data, res->len) == 0)
    {
        *res = ngx_http_variable_true_value;
        return;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script equal: no");

    *res = ngx_http_variable_null_value;
}


void
ngx_http_script_not_equal_code(ngx_http_script_engine_t *e)
{
    ngx_http_variable_value_t  *val, *res;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script not equal");

    e->sp--;
    val = e->sp;
    res = e->sp - 1;

    e->ip += sizeof(uintptr_t);

    if (val->len == res->len
        && ngx_strncmp(val->data, res->data, res->len) == 0)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                       "http script not equal: no");

        *res = ngx_http_variable_null_value;
        return;
    }

    *res = ngx_http_variable_true_value;
}


void
ngx_http_script_file_code(ngx_http_script_engine_t *e)
{
    ngx_str_t                     path;
    ngx_http_request_t           *r;
    ngx_open_file_info_t          of;
    ngx_http_core_loc_conf_t     *clcf;
    ngx_http_variable_value_t    *value;
    ngx_http_script_file_code_t  *code;

    value = e->sp - 1;

    code = (ngx_http_script_file_code_t *) e->ip;
    e->ip += sizeof(ngx_http_script_file_code_t);

    path.len = value->len - 1;
    path.data = value->data;

    r = e->request;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http script file op %p \"%V\"", code->op, &path);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.read_ahead = clcf->read_ahead;
    of.directio = clcf->directio;
    of.valid = clcf->open_file_cache_valid;
    of.min_uses = clcf->open_file_cache_min_uses;
    of.test_only = 1;
    of.errors = clcf->open_file_cache_errors;
    of.events = clcf->open_file_cache_events;

    if (ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK) {
        e->ip = ngx_http_script_exit;
        e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool)
        != NGX_OK)
    {
        if (of.err != NGX_ENOENT
            && of.err != NGX_ENOTDIR
            && of.err != NGX_ENAMETOOLONG)
        {
            ngx_log_error(NGX_LOG_CRIT, r->connection->log, of.err,
                          "%s \"%s\" failed", of.failed, value->data);
        }

        switch (code->op) {

        case ngx_http_script_file_plain:
        case ngx_http_script_file_dir:
        case ngx_http_script_file_exists:
        case ngx_http_script_file_exec:
             goto false_value;

        case ngx_http_script_file_not_plain:
        case ngx_http_script_file_not_dir:
        case ngx_http_script_file_not_exists:
        case ngx_http_script_file_not_exec:
             goto true_value;
        }

        goto false_value;
    }

    switch (code->op) {
    case ngx_http_script_file_plain:
        if (of.is_file) {
             goto true_value;
        }
        goto false_value;

    case ngx_http_script_file_not_plain:
        if (of.is_file) {
            goto false_value;
        }
        goto true_value;

    case ngx_http_script_file_dir:
        if (of.is_dir) {
             goto true_value;
        }
        goto false_value;

    case ngx_http_script_file_not_dir:
        if (of.is_dir) {
            goto false_value;
        }
        goto true_value;

    case ngx_http_script_file_exists:
        if (of.is_file || of.is_dir || of.is_link) {
             goto true_value;
        }
        goto false_value;

    case ngx_http_script_file_not_exists:
        if (of.is_file || of.is_dir || of.is_link) {
            goto false_value;
        }
        goto true_value;

    case ngx_http_script_file_exec:
        if (of.is_exec) {
             goto true_value;
        }
        goto false_value;

    case ngx_http_script_file_not_exec:
        if (of.is_exec) {
            goto false_value;
        }
        goto true_value;
    }

false_value:

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http script file op false");

    *value = ngx_http_variable_null_value;
    return;

true_value:

    *value = ngx_http_variable_true_value;
    return;
}

//ngx_http_rewrite_handler��ִ��
/*
���valueֵ��$name������������ú������ngx_http_script_add_var_code��ȡ��ngx_http_script_complex_value_code����code->lengths(ngx_http_script_copy_var_len_code)
��ȷ�������ַ����ĳ��ȣ�Ȼ�󿪱ٶ�Ӧ��e->buf�ռ䣬�����ngx_http_script_copy_var_code�аѱ�����������e->buf�ռ���

��������д���?�������ջ���ִ��ngx_http_script_add_args_code

���������$1,�����ջ�ִ��ngx_http_script_add_capture_code
*/void
ngx_http_script_complex_value_code(ngx_http_script_engine_t *e)
{
    size_t                                 len;
    ngx_http_script_engine_t               le;
    ngx_http_script_len_code_pt            lcode;
    ngx_http_script_complex_value_code_t  *code;

    code = (ngx_http_script_complex_value_code_t *) e->ip;// e->ip����֮ǰ�ڽ���ʱ���õĸ��ֽṹ��  

    e->ip += sizeof(ngx_http_script_complex_value_code_t);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script complex value");

    ngx_memzero(&le, sizeof(ngx_http_script_engine_t));

    le.ip = code->lengths->elts;
    le.line = e->line;
    le.request = e->request;
    le.quote = e->quote;

    //���ngx_http_rewrite_value->ngx_http_script_compile->ngx_http_script_add_var_code�Ķ�
    for (len = 0; *(uintptr_t *) le.ip; len += lcode(&le)) {//��ȡ�ñ����ַ����ĳ���
        lcode = *(ngx_http_script_len_code_pt *) le.ip; 
    }

    e->buf.len = len;
    e->buf.data = ngx_pnalloc(e->request->pool, len);
    if (e->buf.data == NULL) {
        e->ip = ngx_http_script_exit;
        e->status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    e->pos = e->buf.data;

    e->sp->len = e->buf.len;
    e->sp->data = e->buf.data;
    e->sp++;
}

//ngx_http_rewrite_handler��ִ��   
void
ngx_http_script_value_code(ngx_http_script_engine_t *e) //e�Ǵ�ngx_http_rewrite_loc_conf_t->codes->elts������ȡ����
//��ȡvalueֵ��ŵ�e->sp�У�sp�����ƶ�һλָ����һ����Ҫ�����ngx_http_script_xxx_code_t,һ��value_code_t����һ��Ϊset_var_code
{
    ngx_http_script_value_code_t  *code;

    code = (ngx_http_script_value_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_value_code_t);

    e->sp->len = code->text_len;
    e->sp->data = (u_char *) code->text_data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script value: \"%v\"", e->sp);

    e->sp++;
}

//ngx_http_rewrite_handler��ִ��
void //��ִ��ngx_http_script_value_code�󣬻�ȡ��valueֵ����e�У������Ϳ���ͨ��set_val_code��e�е�value��ֵ����Ӧ������ngx_http_request_t
ngx_http_script_set_var_code(ngx_http_script_engine_t *e) //e�Ǵ�ngx_http_rewrite_loc_conf_t->codes->elts������ȡ����
{
    ngx_http_request_t          *r;
    ngx_http_script_var_code_t  *code;

    code = (ngx_http_script_var_code_t *) e->ip;// e->ip����֮ǰ�ڽ���ʱ���õĸ��ֽṹ��   

    e->ip += sizeof(ngx_http_script_var_code_t);

    r = e->request;

    // e->sp��ͨ�������õ��ı�����������һ�����飬�����ķ���˳���ip�е�˳��һ�£��������Ŵ��������������Ϊ�˱����д����һ����(����   
    // �Ϳ��Ա�֤���ط�ʹ��һ�µĴ���ʽ)������sp���Ϳ��Եõ�֮ǰ�Ĵ���ֵ���õ�������Ҫ�Ľ���ˡ� 
    e->sp--; 
    //һ��_value_code��set_var��һ�ԣ���������Ҫ�ص�ǰ���sp��Ҫ��set_var������һ��sp�ڵ��ˣ���sp�ǲ����ȡֵ�ģ����Բ�Ӧ��ռ��һ���ڵ㣬����ȷ��ֻ��value_code������


    /*
     ����code->index��ʾNginx����$file��cmcf->variables�����ڵ��±꣬��Ӧÿ������ı���ֵ�洢�ռ��Ϊr->variables[code->index]��
     �����e->spջ��ȡ�����ݲ�����C���Ա�����ͨ�����ϵĸ�ֵ
     */
    //�Ѵ�ǰ���_value_code�л�ȡ����ֵ��ֵ����Ӧ��r->variables[code->index]
    r->variables[code->index].len = e->sp->len;
    r->variables[code->index].valid = 1;
    r->variables[code->index].no_cacheable = 0;
    r->variables[code->index].not_found = 0;
    r->variables[code->index].data = e->sp->data;

#if (NGX_DEBUG)
    {
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    v = cmcf->variables.elts;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script set $%V", &v[code->index].name);
    }
#endif
}


void
ngx_http_script_var_set_handler_code(ngx_http_script_engine_t *e)
{
    ngx_http_script_var_handler_code_t  *code;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script set var handler");

    code = (ngx_http_script_var_handler_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_var_handler_code_t);

    e->sp--;

    code->handler(e->request, e->sp, code->data);
}


void
ngx_http_script_var_code(ngx_http_script_engine_t *e)
{
    ngx_http_variable_value_t   *value;
    ngx_http_script_var_code_t  *code;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script var");

    code = (ngx_http_script_var_code_t *) e->ip;

    e->ip += sizeof(ngx_http_script_var_code_t);

    value = ngx_http_get_flushed_variable(e->request, code->index);

    if (value && !value->not_found) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                       "http script var: \"%v\"", value);

        *e->sp = *value;
        e->sp++;

        return;
    }

    *e->sp = ngx_http_variable_null_value;
    e->sp++;
}


void
ngx_http_script_nop_code(ngx_http_script_engine_t *e)
{
    e->ip += sizeof(uintptr_t);
}
