
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
/*
����˫���������������
    ���ڶ���һ���򵥵�������ʹ��ngx_queue_sort����������Ԫ���������������
�У����Կ�����ζ��塢��ʼ��ngx_queue_t��������ζ����������͵�����Ԫ�أ���α�
����������Զ������򷽷���ִ������
    ���ȣ���������Ԫ�صĽṹ�壬�������TestNode�ṹ�壺
typedef struct {
                      u_char* str;
                      ngx_queue_t qEle;
                    I           int num;
} TestNode;

220��ڶ�������α�дHTTPģ��
    ����Ԫ�ؽṹ���б������ngx_queue_t���͵ĳ�Ա����Ȼ�������������λ���ϡ���
��������������һ��char~ָ�룬������һ�����ͳ�Աnum������������ġ�
    ���򷽷���Ҫ�Զ��塣������TestNode�ṹ���е�num��Ա��Ϊ�������ݣ�ʵ��
compTestNode������Ϊ���������������Ԫ�ؼ�ıȽϷ�����
    ngx_int_t compTeBtNode(const ngx_queue_t*a,  const ngx_queue_t*b)
    {
        //����ʹ��ngx_queue_data������ngx queue��t������ȡԪ�ؽṹ��TestNode�ĵ�ַ�棯
        TestNodet aNode=ngx_queue_data(a,  TestNode,  qEle)j
        TestNode��bNode=ngx_queue_data (b,  TestNode,  qEle)j
        
       //����num��Ա�ıȽϽ��
        return  aNode- >num>bNode- >num;
    )
    ����ȽϷ������ngx_queue_sort�������԰������е�Ԫ�ذ���num�Ĵ�С��������
�С��ڴ����У����Կ���ngx_queue_data���÷��������Ը�������Ԫ�ؽṹ��TestNode��
��qEle��Ա��ַ�����TestNode�ṹ������ĵ�ַ������������̵�C���Ա�д��ngx_
queue_t����֮�����ܹ�ͨ�û��Ĺؼ�����������һ��ngx_queue_data�Ķ��壺
#define  ngx_queue_data (q, type, link)   \
(type  *) ��u char  *)  q  -  offsetof (type,  link��
    ��4.2.2���������ᵽ��offsetof���������ʵ�ֵģ������᷵��link��Ա��type��
�����е�ƫ���������磬�������У�����ͨ��ngx_queue_t���͵�ָ���ȥqEle�����
TestNode�ĵ�ַƫ�������õ�TestNode�ṹ��ĵ�ַ��
    ���濪ʼ����˫����������queueContainer���������ʼ��Ϊ������������ʾ��
ngx_queue_t queueContainer;
ngx_queue_init ( &queueContainer )  ;
    ����������ngx_queue_t���弴�ɡ�ע�⣬���ڱ�ʾ����������ngx_queue_t�ṹ�壬
�������ngx_queue_init���г�ʼ����
    ngx_queue_t˫����������ȫ�����������ڴ�ģ�ÿһ������Ԫ�ر����Լ������Լ���
ռ�õ��ڴ档��ˣ������ڽ���ջ�ж�����5��TestNode�ṹ����Ϊ����Ԫ�أ���������
��num��Ա��ʼ��Ϊ0��1��2��3��4��������ʾ��
�������5��TestNode�ṹ����ӵ�queueContainer�����У�ע�⣬����ͬʱʹ����
ngx_queue��insert tail��ngx_queue��insert head��ngx_queue��insert after 3����ӷ���������
����˼��һ��������Ԫ�ص�˳����ʲô���ġ�
ngx_queue_insert_tail ( &queueContainer ,    &node [ O ]  . qEle )  ;
ngx_queue_insert_head ( &queueContainer ,    &node [l]  . qEle )  ;
ngx_queue_insert_tail ( &queueContainer ,    &node [2 ]  . qEle)  ;
ngx_queue_insert_af ter ( &queueContainer ,   &node [ 3 ]  . qEle)  ;
ngx_queue_insert_tail ( &queueContainer,   &node [4 ]  . qEle) ;
    ���ݱ�7-1�н��ܵķ������Եó��������ʱ������Ԫ��˳����num��Ա��ʶ����ôӦ
���������ģ�3��1��0��2��4����������ʣ�����д����������ĳ������һ��˳���Ƿ���ˡ�
����͸��ݱ�7-1�еķ���˵����дһ�μ򵥵ı�������ĳ���
ngx_queue_t*qf
for  (q=ngx_queue_head(&queueContainer);
    q  !=  ngx_queue_sentinel( &queueContainer);
    q= ngx_queue_next (q))
{
    TeetNodek eleNode;ngx_queue_data(q, TestNode, qEle),
    ��������ǰ������Ԫ��eleNode
)
    ������γ��򽫻����δ�����ͷ��������β�����������Ҳ�ܼ򵥡����߿��Գ���ʹ��
ngx_queue_last��ngx_queue_prev������д��ش��롣
    ���濪ʼִ�����򣬴���������ʾ��
    ngx_queue_sort4( &queueContainer,    compTestNode);
    �����������е�Ԫ�ؾͻ���0��1��2��3��4��num��Ա��ֵ�������������ˡ�

*/

#include <ngx_config.h>
#include <ngx_core.h>


/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */
//��������Ԫ��ngx_queue_middle
/*
�����õ��ļ�����ÿ��middle����ƶ�һ����next����ƶ�����������nextָ����β��ʱ��middle��ָ�����м䣬ʱ�临�ӶȾ���O(N)
*/
ngx_queue_t *
ngx_queue_middle(ngx_queue_t *queue)
{
    ngx_queue_t  *middle, *next;

    middle = ngx_queue_head(queue);

    if (middle == ngx_queue_last(queue)) {
        return middle;
    }

    next = ngx_queue_head(queue);

    for ( ;; ) {
        middle = ngx_queue_next(middle);

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }
    }
}

/*
˫����еĸ����������ܼ򵥣���������������ͼ��
1. ngx_queue_init(q)��ʼ���ڱ���㣬��prev�ֶκ�next�ֶξ�ָ��������
2. ngx_queue_empty(q)����ڱ�����prev�ֶ��Ƿ�ָ�����������ж϶����Ƿ�Ϊ�գ�
3. ngx_queue_insert_head(h, x)���ڱ����͵�һ�����֮������½��x��
4. ngx_queue_insert_after(h, x)��ngx_queue_insert_head�ı�����
5. ngx_queue_insert_tail(h, x)�����һ�������ڱ����֮������½�㣻
6. ngx_queue_head(h)��ȡ��һ����㣻
7. ngx_queue_last(h)��ȡ���һ����㣻
8. ngx_queue_sentinel(h)��ȡ�ڱ���㣨������h����
9. ngx_queue_next(q)��ȡ��һ����㣻
10. ngx_queue_prev(q)��ȡ��һ����㣻
11. ngx_queue_remove(x)�����x�Ӷ������Ƴ���
12. ngx_queue_split(h, q, n)��hΪ�ڱ����Ķ�����q��㿪ʼ����β������������֡����ӵ��յ�n�����У�h�����е�ʣ��������¶��У�
13. ngx_queue_add(h, n)��n�����е����н�㰴˳�����ӵ�h����ĩβ��n������գ�
14. ngx_queue_middle(queue)ʹ��˫�������㷨Ѱ��queue���е��м��㣻
15. ngx_queue_sort(queue, cmd)ʹ�ò��������㷨��queue���н���������ɺ���next������Ϊ����prev����Ϊ���� 
*/

/* the stable insertion sort 
ngx_queue_sort(queue, cmd)ʹ�ò��������㷨��queue���н���������ɺ���next������Ϊ����prev����Ϊ���� 
*/
//��locationqueue�������򣬼�ngx_queue_sort������Ĺ�����Զ�һ��ngx_http_cmp_locations������
//�ú������ܾ��ǰ���cmp�Ƚϣ�Ȼ�������queue���д�������ngx_queueʹ�����ӿ��Բο�http://blog.chinaunix.net/uid-26284395-id-3134435.html
//���Կ�����������õ��ǲ��������㷨��ʱ�临�Ӷ�ΪO(n)����������ǳ���ࡣ
void
ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
    ngx_queue_t  *q, *prev, *next;

    q = ngx_queue_head(queue);

    if (q == ngx_queue_last(queue)) {
        return;
    }

    for (q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {

        prev = ngx_queue_prev(q);
        next = ngx_queue_next(q);

        ngx_queue_remove(q); //���������������ǰ�����ҳ���������Ҫ�����Ԫ��

        do {
            if (cmp(prev, q) <= 0) {//�Ӵ�������
                break;
            }

            prev = ngx_queue_prev(prev);

        } while (prev != ngx_queue_sentinel(queue)); //�������Ԫ����Ҫ���뵽ǰ�������ĺ���Ķ��е��Ǹ��ط�

        ngx_queue_insert_after(prev, q);
    }
}

