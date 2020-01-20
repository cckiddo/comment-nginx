
/*
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Valentin V. Bartenev
 */


#ifndef _NGX_HTTP_V2_MODULE_H_INCLUDED_
#define _NGX_HTTP_V2_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    //����ÿһ��worker�����뻺������С
    size_t                          recv_buffer_size; //http2_recv_buffer_size������ָ��  Ĭ��ֵ256 * 1024
    u_char                         *recv_buffer; //����recv_buffer_size�����ڴ棬��ֵ��ngx_http_v2_init
} ngx_http_v2_main_conf_t;


typedef struct {
    size_t                          pool_size; //http2_pool_size������ָ�����ռ������ngx_http_v2_init
    /* һ��������ͬʱ�����������޶ȣ���Ч��ngx_http_v2_state_headers */
    ngx_uint_t                      concurrent_streams; //http2_max_concurrent_streams������ָ�� Ĭ��128
    //���ƾ���HPACKѹ��������ͷ�е���name:value�ֶε����ߴ硣
    size_t                          max_field_size; //http2_max_field_size������ָ��  Ĭ��4096
    /* header�����е�����name+value֮�ͳ������ֵ����Ч��ngx_http_v2_state_process_header 
    ���ƾ���HPACKѹ������������ͷ�����ߴ硣
    */
    size_t                          max_header_size; //http2_max_header_size������ָ�� Ĭ��16384
    ngx_uint_t                      streams_index_mask; //http2_streams_index_size������ָ�� Ĭ��32-1
    ngx_msec_t                      recv_timeout; //http2_recv_timeout������ָ��  Ĭ��30000
    /* ���ÿ������ӹرյĳ�ʱʱ�䡣 */
    ngx_msec_t                      idle_timeout; //http2_idle_timeout������ָ��  Ĭ��180000
} ngx_http_v2_srv_conf_t;

typedef struct {
    /* ������Ӧ�������ݣ�response body����Ƭ����󳤶ȡ�������ֵ��С������������ߵĿ�����
        ���ֵ������ᵼ����ͷ���������⡣Ĭ�ϴ�С8k�� */
    size_t                          chunk_size; //http2_chunk_size������ָ��
} ngx_http_v2_loc_conf_t;


extern ngx_module_t  ngx_http_v2_module;


#endif /* _NGX_HTTP_V2_MODULE_H_INCLUDED_ */
