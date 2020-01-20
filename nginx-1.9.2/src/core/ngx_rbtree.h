
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_RBTREE_H_INCLUDED_
#define _NGX_RBTREE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

/*
��7-4 NginxΪ������Ѿ�ʵ�ֺõ�3��������ӷ��� (ngx_rbtree_insert_ptָ���������ַ���)
���������������������������������������ש��������������������������������������ש�������������������������������
��    ������                          ��    ��������                          ��    ִ������                  ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_value        ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     �����ݽڵ�Ĺؼ��ֶ���Ψһ�ģ�  ��
��ngx_rbtree_node_t *node,            ����ָ�룻sentinel����ú������ʼ��    ��������ͬһ���ؼ����ж���ڵ�  ��
��ngx_rbtree_node_t *sentinel)        ��ʱ�ڱ��ڵ��ָ��                      ��������                        ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_timer_value  ��  root�Ǻ����������ָ�룻node��      ��                              ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     ��  ������������ݽڵ㣬ÿ��  ��
��ngx_rbtree_node_t *node,            ����ָ�룬����Ӧ�Ĺؼ�����ʱ�����      �����ݽڵ�Ĺؼ��ֱ�ʾʱ������  ��
��                                    ��ʱ�������Ǹ�����sentinel�����    ��ʱ���                        ��
��ngx_rbtree_node_t *sentinel)        ��                                      ��                              ��
��                                    ���������ʼ��ʱ���ڱ��ڵ�              ��                              ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_str_rbtree_insert_value    ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *temp,           �������Ԫ�ص�ngx_str_node_t��Ա��      �����ݽڵ�Ĺؼ��ֿ��Բ���Ψһ  ��
��ngx_rbtree_node_t *node,            ��ָ�루ngx- rbtree_node_t���ͻ�ǿ��ת  ���ģ������������ַ�����ΪΨһ  ��
��                                    ����Ϊngx_str_node_t���ͣ���sentinel��  ���ı�ʶ�������ngx_str_node_t  ��
��ngx_rbtree_node t *sentinel)        ��                                      ��                              ��
��                                    ����ú������ʼ��ʱ�ڱ��ڵ��ָ��      ���ṹ���str��Ա��             ��
���������������������������������������ߩ��������������������������������������ߩ�������������������������������
    ͬʱ������ngx_str_node_t�ڵ㣬Nginx���ṩ��ngx_str_rbtree_lookup�������ڼ���
������ڵ㣬��������һ�����Ķ��壬�������¡�
    ngx_str_node_t  *ngx_str_rbtree_lookup(ngx_rbtree t  *rbtree,  ngx_str_t *name, uint32_t hash)��
    ���У�hash������Ҫ��ѯ�ڵ��key�ؼ��֣���name��Ҫ��ѯ���ַ����������ͬ��
������Ӧ��ͬkey�ؼ��ֵ����⣩�����ص��ǲ�ѯ���ĺ�����ڵ�ṹ�塣
    ���ں���������ķ�������7-5��
��7-5  ����������ṩ�ķ���
���������������������������������������ש��������������������������������ש�������������������������������������
��    ������                          ��    ��������                    ��    ִ������                        ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��                                    ��  tree�Ǻ����������ָ�룻s��   ��  ��ʼ���������������ʼ������      ��
��                                    ���ڱ��ڵ��ָ�룻i��ngx_rbtree_  ��                                    ��
��ngx_rbtree_init(tree, s, i)         ��                                ���㡢�ڱ��ڵ㡢ngx_rbtree_insert_pt  ��
��                                    ��insert_pt���͵Ľڵ���ӷ������� ���ڵ���ӷ���                        ��
��                                    �������7-4                       ��                                    ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��void ngx_rbtree_insert(ngx_rbtree_t ��  tree�Ǻ����������ָ�룻node  ��  ����������ӽڵ㣬�÷�����      ��
��*tree, ngx_rbtree node_t *node)     ������Ҫ��ӵ�������Ľڵ�ָ��    ��ͨ����ת�������������ƽ��          ��
�ǩ������������������������������������贈�������������������������������贈������������������������������������
��void ngx_rbtree_delete(ngx_rbtree_t ��  tree�Ǻ����������ָ�룻node  ��  �Ӻ������ɾ���ڵ㣬�÷�����      ��
��*tree, ngx_rbtree node_t *node)     ���Ǻ��������Ҫɾ���Ľڵ�ָ��    ��ͨ����ת�������������ƽ��          ��
���������������������������������������ߩ��������������������������������ߩ�������������������������������������
    �ڳ�ʼ�������ʱ����Ҫ�ȷ���ñ���������ngx_rbtree_t�ṹ�壬�Լ�ngx_rbtree_
node_t���͵��ڱ��ڵ㣬��ѡ������Զ���ngx_rbtree_insert_pt���͵Ľڵ���Ӻ�����
    ���ں������ÿ���ڵ���˵�����Ƕ��߱���7-6���е�7�����������ֻ�����˽����
ʹ�ú��������ôֻ��Ҫ�˽�ngx_rbtree_min������


���������������������������������������ש����������������������������������ש���������������������������������������
��    ������                          ��    ��������                      ��    ִ������                          ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbt_red(node)                   ��                                  ��  ����node�ڵ����ɫΪ��ɫ            ��
��                                    �� t���͵Ľڵ�ָ��                  ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbt_black(node)                 ��                                  ��  ����node�ڵ����ɫΪ��ɫ            ��
��                                    ��t���͵Ľڵ�ָ��                   ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��  ��node�ڵ����ɫΪ��ɫ���򷵻ط�O   ��
��ngx_rbt_is_red(node)                ��                                  ��                                      ��
��                                    ��t���͵Ľڵ�ָ��                   ����ֵ�����򷵻�O                       ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��ngx_rbt_is_black(node)              ��  node�Ǻ������ngx_rbtree_node_  ��  ��node�ڵ����ɫΪ��ɫ���򷵻ط�0   ��
��                        I           ��t���͵Ľڵ�ָ��                   ����ֵ�����򷵻�O                       ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��               I                    ��  nl��n2���Ǻ������ngx_rbtree_   ��                                      ��
��ngx_rbt_copy_color(nl, n2)          ��                                  ��  ��n2�ڵ����ɫ���Ƶ�nl�ڵ�          ��
��                                 I  ��nodej���͵Ľڵ�ָ��               ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��ngx_rbtree_node_t *                 ��  node�Ǻ������ngx_rbtree_node_  ��                                      ��
��ngx_rbtree_min                      ��t���͵Ľڵ�ָ�룻sentinel����ú� ��  �ҵ���ǰ�ڵ㼰�������е���С�ڵ�    ��
��(ngx_rbtree_node_tľnode,           ���������ڱ��ڵ�                    ��������key�ؼ��֣�                     ��
��ngx_rbtree_node_t *sentinel)        ��                                  ��                                      ��
�ǩ������������������������������������贈���������������������������������贈��������������������������������������
��                                    ��  node�Ǻ������ngx_rbtree_node_  ��  ��ʼ���ڱ��ڵ㣬ʵ���Ͼ��ǽ��ýڵ�  ��
��ngx_rbtree_sentinel_init(node)      ��                                  ��                                      ��
��                                    ��t���͵Ľڵ�ָ��                   ����ɫ��Ϊ��ɫ                          ��
���������������������������������������ߩ����������������������������������ߩ���������������������������������������

ʹ�ú�����ļ�����
    ������һ���򵥵�������˵�����ʹ�ú����������������ջ�з���rbtree���������
�ṹ���Լ��ڱ��ڵ�sentinel����Ȼ��Ҳ����ʹ���ڴ�ػ��ߴӽ��̶��з��䣩�������еĽ�
����ȫ��key�ؼ�����Ϊÿ���ڵ��Ψһ��ʶ�������Ϳ��Բ���Ԥ���ngx_rbtree insert
value�����ˡ����ɵ���ngx_rbtree_init������ʼ�������������������ʾ��
    ngx_rbtree_node_t  sentinel ;
    ngx_rbtree_init ( &rbtree, &sentinel,ngx_str_rbtree_insert_value)
    ���������ڵ�Ľṹ�彫ʹ��7.5.3���н��ܵ�TestRBTreeNode�ṹ�壬ÿ��Ԫ�ص�key�ؼ��ְ���1��6��8��11��13��15��17��22��25��27��˳
��һһ����������ӣ�����������ʾ��
    rbTreeNode [0] .num=17;
    rbTreeNode [1] .num=22;
    rbTreeNode [2] .num=25;
    rbTreeNode [3] .num=27;
    rbTreeNode [4] .num=17;
    rbTreeNode [7] .num=22;
    rbTreeNode [8] .num=25;
    rbTreeNode [9] .num=27;
    for(i=0j i<10; i++)
    {
        rbTreeNode [i].node. key=rbTreeNode[i]. num;
        ngx_rbtree_insert(&rbtree,&rbTreeNode[i].node);
    )

ngx_rbtree_node_t *tmpnode   =   ngx_rbtree_min ( rbtree . root ,    &sentinel )  ;
    ��Ȼ�������������ʹ�ø��ڵ����ʹ����һ���ڵ�Ҳ�ǿ��Եġ���������һ�����
����1���ڵ㣬��ȻNginx�Դ˲�û���ṩԤ��ķ����������ַ��������ṩ��ngx_str_
rbtree_lookup��������������ʵ���ϼ����Ƿǳ��򵥵ġ�������Ѱ��key�ؼ���Ϊ13�Ľڵ�
Ϊ��������˵����
    ngx_uint_t lookupkey=13;
    tmpnode=rbtree.root;
    TestRBTreeNode *lookupNode;
    while (tmpnode  !=&sentinel)  {
        if (lookupkey!-tmpnode->key)  (
        ��������key�ؼ����뵱ǰ�ڵ�Ĵ�С�Ƚϣ������Ǽ�������������������
        tmpnode=  (lookupkey<tmpnode->key)  ?tmpnode->left:tmpnode->right;
        continue��
        )
        �����ҵ���ֵΪ13�����ڵ�
        lookupNode=  (TestRBTreeNode*)  tmpnode;
        break;
    )
    �Ӻ������ɾ��1���ڵ�Ҳ�Ƿǳ��򵥵ģ���Ѹո��ҵ���ֵΪ13�Ľڵ��rbtree��
ɾ����ֻ�����ngx_rbtree_delete������
ngx_rbtree_delete ( &rbtree , &lookupNode->node);

*/


typedef ngx_uint_t  ngx_rbtree_key_t;
typedef ngx_int_t   ngx_rbtree_key_int_t;


typedef struct ngx_rbtree_node_s  ngx_rbtree_node_t;

/*
ngx_rbtree_node_t�Ǻ����ʵ���б����õ������ݽṹ��һ�����ǰ����ŵ��ṹ���е�
��1����Ա�У�����������Զ���Ľṹ��ǿ��ת����ngx_rbtree_node_t���͡����磺
typedef struct  {
    //һ�㶼��ngx_rbtree_node_t�ڵ�ṹ������������������͵ĵ�1λ���Է������͵�ǿ��ת�� 
    ngx_rbtree_node_t node;
    ngx_uint_t num;
) TestRBTreeNode;
    �������ϣ��������Ԫ�ص�����������TestRBTreeNode����ôֻ��Ҫ�ڵ�1����Ա��
����ngx_rbtree_node_t���͵�node���ɡ��ڵ���ͼ7-7��ngx_rbtree_t�������ṩ�ķ���
ʱ����Ҫ�Ĳ�������ngx_rbtree_node_t���ͣ���ʱ��TestRBTreeNode���͵�ָ��ǿ��ת��
��ngx_rbtree_node_t���ɡ�
*/
struct ngx_rbtree_node_s {
    /* key��Ա��ÿ��������ڵ�Ĺؼ��֣������������͡��������������Ҫ����key��Ա */
    ngx_rbtree_key_t       key; //�޷������͵Ĺؼ���  �ο�ngx_http_file_cache_exists  ��ʵ����ngx_http_cache_t->key��ǰ4�ֽ�
    ngx_rbtree_node_t     *left;
    ngx_rbtree_node_t     *right;
    ngx_rbtree_node_t     *parent;
    u_char                 color; //�ڵ����ɫ��0��ʾ��ɫ��l��ʾ��ɫ
    u_char                 data;//��1���ֽڵĽڵ����ݡ����ڱ�ʾ�Ŀռ�̫С������һ�����ʹ��
};

typedef struct ngx_rbtree_s  ngx_rbtree_t;

/*
�������һ��ͨ�õ����ݽṹ�����Ľڵ㣨���߳�Ϊ������Ԫ�أ������ǰ�������������ڵ������ṹ�塣���ڲ�ͬ�Ľṹ�壬�ܶೡ��
��������ͬ�Ľڵ�ӵ����ͬ�Ĺؼ��ֵģ��������磬��ͬ���ַ�������
��ɢ�г���ͬ�Ĺؼ��֣���ʱ�����ں�����еĹؼ�������ͬ�ģ�Ȼ���������ǲ�ͬ�Ľڵ㣬���������ʱ�Ͳ����Ը���ԭ��ͬ���ؼ��ֽڵ㣬
������Ϊ�²���Ľڵ���ڡ���ˣ������Ԫ��ʱ����Ҫ���ǵ���������������Ԫ�صķ��������ngx_rbtree_insert_pt����ָ����Ժܺ�
��ʵ����һ˼�룬�û�Ҳ�������ض����Լ�����Ϊ��Nginx�����û�ʵ����3�ּ���Ϊ����ӽڵ㷽��

Ϊ�����ͬ�ڵ㺬����ͬ�ؼ��ֵ�Ԫ�س�ͻ���⣬�����������ngx_rbtree_insert_ptָ�룬������������ӳ�ͻԪ��
*/

/*
��7-4 NginxΪ������Ѿ�ʵ�ֺõ�3��������ӷ��� (ngx_rbtree_insert_ptָ���������ַ���)
���������������������������������������ש��������������������������������������ש�������������������������������
��    ������                          ��    ��������                          ��    ִ������                  ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_value        ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     �����ݽڵ�Ĺؼ��ֶ���Ψһ�ģ�  ��
��ngx_rbtree_node_t *node,            ����ָ�룻sentinel����ú������ʼ��    ��������ͬһ���ؼ����ж���ڵ�  ��
��ngx_rbtree_node_t *sentinel)        ��ʱ�ڱ��ڵ��ָ��                      ��������                        ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_rbtree_insert_timer_value  ��  root�Ǻ����������ָ�룻node��      ��                              ��
��(ngx_rbtree_node_t *root,           �������Ԫ�ص�ngx_rbtree_node_t��Ա     ��  ������������ݽڵ㣬ÿ��  ��
��ngx_rbtree_node_t *node,            ����ָ�룬����Ӧ�Ĺؼ�����ʱ�����      �����ݽڵ�Ĺؼ��ֱ�ʾʱ������  ��
��                                    ��ʱ�������Ǹ�����sentinel�����    ��ʱ���                        ��
��ngx_rbtree_node_t *sentinel)        ��                                      ��                              ��
��                                    ���������ʼ��ʱ���ڱ��ڵ�              ��                              ��
�ǩ������������������������������������贈�������������������������������������贈������������������������������
��void ngx_str_rbtree_insert_value    ��  root�Ǻ����������ָ�룻node��      ��  ������������ݽڵ㣬ÿ��  ��
��(ngx_rbtree_node_t *temp,           �������Ԫ�ص�ngx_str_node_t��Ա��      �����ݽڵ�Ĺؼ��ֿ��Բ���Ψһ  ��
��ngx_rbtree_node_t *node,            ��ָ�루ngx- rbtree_node_t���ͻ�ǿ��ת  ���ģ������������ַ�����ΪΨһ  ��
��                                    ����Ϊngx_str_node_t���ͣ���sentinel��  ���ı�ʶ�������ngx_str_node_t  ��
��ngx_rbtree_node t *sentinel)        ��                                      ��                              ��
��                                    ����ú������ʼ��ʱ�ڱ��ڵ��ָ��      ���ṹ���str��Ա��             ��
���������������������������������������ߩ��������������������������������������ߩ�������������������������������
*/
typedef void (*ngx_rbtree_insert_pt) (ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel); //node����root�ķ���  ִ�еط���ngx_rbtree_insert

struct ngx_rbtree_s {
    ngx_rbtree_node_t     *root;      //ָ�����ĸ��ڵ㡣ע�⣬���ڵ�Ҳ������Ԫ��
    ngx_rbtree_node_t     *sentinel;  //ָ��NIL�ͱ��ڵ�  �ڱ��ڵ����������²��Ҷ�ӽڵ㶼ָ��һ��NULL�սڵ㣬ͼ�λ��ο�:http://blog.csdn.net/xzongyuan/article/details/22389185
    ngx_rbtree_insert_pt   insert;    //��ʾ��������Ԫ�صĺ���ָ�룬������������½ڵ�ʱ����Ϊ�������滻��������
};

//sentinel�ڱ������ⲿ�ڵ㣬���е�Ҷ���Լ������ĸ��ڵ㣬��ָ�����Ψһ���ڱ�nil���ڱ�����ɫΪ��ɫ

#define ngx_rbtree_init(tree, s, i)                                           \
    ngx_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i

void ngx_rbtree_insert(ngx_rbtree_t *tree, ngx_rbtree_node_t *node);
void ngx_rbtree_delete(ngx_rbtree_t *tree, ngx_rbtree_node_t *node);
void ngx_rbtree_insert_value(ngx_rbtree_node_t *root, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel);
void ngx_rbtree_insert_timer_value(ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel); 
    


#define ngx_rbt_red(node)               ((node)->color = 1)
#define ngx_rbt_black(node)             ((node)->color = 0)
#define ngx_rbt_is_red(node)            ((node)->color)
#define ngx_rbt_is_black(node)          (!ngx_rbt_is_red(node))
#define ngx_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define ngx_rbtree_sentinel_init(node)  ngx_rbt_black(node)


static ngx_inline ngx_rbtree_node_t *
ngx_rbtree_min(ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}

#endif /* _NGX_RBTREE_H_INCLUDED_ */

