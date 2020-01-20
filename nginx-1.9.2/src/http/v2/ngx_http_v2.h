/*
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Valentin V. Bartenev
 */


#ifndef _NGX_HTTP_V2_H_INCLUDED_
#define _NGX_HTTP_V2_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/*
һ��֡ͨ�ø�ʽ

��ͼΪHTTP/2֡ͨ�ø�ʽ��֡ͷ+���صı���λͨ�ýṹ��

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  Type (8)     |  Flags (8)    |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (31)                       |
+=+=============================================================+
|                  Frame Payload (0...)                       ...
+---------------------------------------------------------------+
֡ͷΪ�̶���9���ֽڣ�(24+8+8+1+31)/8=9�����֣��仯��Ϊ֡�ĸ���(payload)��������������֡���ͣ�Type�����塣

    ֡����Length���޷��ŵ���Ȼ����24�����ر�ʾ������ʾ֡������ռ���ֽ�����������֡ͷ��ռ�õ�9���ֽڡ�Ĭ�ϴ�
С����ΪΪ0~16,384(2^14)��һ������Ĭ�����ֵ2^14(16384)�����ͷ������������ͣ����ǽ��յ����շ������
SETTINGS_MAX_FRAME_SIZE��һ���ֵ����Ϊ2^14 ~ 2^24��ֵ��֪ͨ��
    ֡����Type��8�����ر�ʾ��������֡���صľ����ʽ��֡�����壬HTTP/2�淶������10��֡���ͣ����ﲻ����ʵ������֡����չ����֡
    ֡�ı�־λFlags��8�����ر�ʾ�������ھ���֡���ͣ�Ĭ��ֵΪ0x0����һ��С������Ҫע�⣬һ��������8�����ؿ�������8����ͬ�ı�
־�����磬PADDEDֵΪ0x8�������Ʊ�ʾΪ00001000��END_HEADERSֵΪ0x4�������Ʊ�ʾΪ00000100��END_STREAMֵΪ0X1��������Ϊ00000001��
����ͬʱ��һ���ֽ��д������ֱ�־λ�������Ʊ�ʾΪ00001101����0x13����ˣ������֡�ṹ�У���־λһ���ʹ��8�����ر�ʾ����ĳλ
��ȷ����ʹ���ʺ�?�������ʾ�˴����ܻᱻ���ñ�־λ
    ֡��������ΪR����HTTP/2�ﾳ��Ϊ�����ı���λ���̶�ֵΪ0X0
    ����ʶ��Stream Identifier���޷��ŵ�31���ر�ʾ�޷�����Ȼ����0x0ֵ��ʾΪ֡�����������ӣ��������ڵ���������
    ����֡���ȣ���Ҫ�Լӹ�ע�� - 0 ~ 2^14��16384��ΪĬ��Լ�����ȣ����ж˵㶼��Ҫ���� - 2^14 (16,384) ~ 2^24-1(16,777,215)
��������ֵ����Ҫ���շ�����SETTINGS_MAX_FRAME_SIZE����������ֵ - һ�˽��յ���֡���ȳ����趨���޻�̫֡С����Ҫ����FRAME_SIZE_ERR
���� - ��֡�������Ӱ�쵽��������״̬ʱ���������Ӵ���Դ�֮������HEADERS��PUSH_PROMISE��CONTINUATION��SETTINGS���Լ�֡��ʶ��
����Ϊ0��֡�ȣ�����Ҫ��˴��� - ��һ�˶�û���������ʹ����һ��֡�����п��ÿռ� - ��֡���ܻᵼ���ӳ٣����ʱ�����е�֡����
��RST_STREAM, WINDOW_UPDATE, PRIORITY����Ҫ���ٷ��ͳ�ȥ�������ӳٵ������ܵ�����

֡�ı�־λ:
Frame Types vs Flags and Stream ID
    Table represent possible combination of frame types and flags.
    Last column -- Stream ID of frame types (x -- sid >= 1, 0 -- sid = 0)


                        +-END_STREAM 0x1
                        |   +-ACK 0x1
                        |   |   +-END_HEADERS 0x4
                        |   |   |   +-PADDED 0x8
                        |   |   |   |   +-PRIORITY 0x20
                        |   |   |   |   |        +-stream id (value)
                        |   |   |   |   |        |
    | frame type\flag | V | V | V | V | V |   |  V  |
    | --------------- |:-:|:-:|:-:|:-:|:-:| - |:---:|
    | DATA            | x |   |   | x |   |   |  x  |
    | HEADERS         | x |   | x | x | x |   |  x  |
    | PRIORITY        |   |   |   |   |   |   |  x  |
    | RST_STREAM      |   |   |   |   |   |   |  x  |
    | SETTINGS        |   | x |   |   |   |   |  0  |
    | PUSH_PROMISE    |   |   | x | x |   |   |  x  |
    | PING            |   | x |   |   |   |   |  0  |
    | GOAWAY          |   |   |   |   |   |   |  0  |
    | WINDOW_UPDATE   |   |   |   |   |   |   | 0/x |
    | CONTINUATION    |   |   | x | x |   |   |  x  |


����HTTP/2�����֡

�淶������10����ʽʹ�õ�֡���ͣ���չʵ�����͵�ALTSVC��BLOCKED�Ȳ��ڽ���֮�С����水������ʹ��˳������������

1. SETTINGS

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|     0x4 (8)   | 0000 000? (8) |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier/0x0 (32)                   |
+=+=============================+===============================+
|       Identifier (16)         |
+-------------------------------+-------------------------------+
|                        Value (32)                             |
+---------------------------------------------------------------+
|       Identifier (16)         |
+-------------------------------+-------------------------------+
|                        Value (32)                             |
+---------------------------------------------------------------+  
����֡��������������ͨ�漺���趨���������������ӳɹ�������һ�����͵�֡��

�ֶ�Identifier���������²����� 
    - SETTINGS_HEADER_TABLE_SIZE (0x1)��֪ͨ�����߱�ͷ����ֽ������ֵ����ͷ�����ʹ�ã���ʼֵΪ4096���ֽڣ�Ĭ�Ͽɲ������� 
    - SETTINGS_ENABLE_PUSH (0x2)��0����ֹ���������ͣ�1���������ͣ�����ֵ�Ƿ���PROTOCOL_ERROR���� 
    - SETTINGS_MAX_CONCURRENT_STREAMS (0x3)������������ɴ��������ֵ������ֵ100��Ĭ�Ͽɲ������ã�0ֵΪ��ֹ�������� 
    - SETTINGS_INITIAL_WINDOW_SIZE (0x4)�����Ͷ����ش��ڴ�С��Ĭ��ֵ2^16-1 (65,535)���ֽڴ�С�����ֵΪ2^31-1���ֽڴ�С��
        �������Ҫ��FLOW_CONTROL_ERROR���� 
    - SETTINGS_MAX_FRAME_SIZE (0x5)����֡�������ֵ��Ĭ��Ϊ2^14��16384�����ֽڣ�����������֡�����յ����趨Ӱ�죻
        ֵ����Ϊ2^14��16384��-2^24-1(16777215) 
    - SETTINGS_MAX_HEADER_LIST_SIZE (0x6)�����Ͷ�ͨ���Լ�׼�����յı�ͷ�������ֵ�����ֽ�������ֵ������δѹ��
        ��ͷ�ֶΣ������ֶ����ơ��ֶ�ֵ�Լ�ÿһ����ͷ�ֶε�32���ֽڵĿ����ȣ��ĵ�������˵Ĭ��ֵ�������ƣ���Ϊ��
        ����ͷ���ϴ�С�����Ƶ�Ӱ�죬������Ϊ��Ҫ����2 SETTINGS_MAX_FRAME_SIZE����2^142=32768���������ͷ̫��������ࡣ

��־λ�� * ACK (0x1)����ʾ�������Ѿ����յ�SETTING֡����Ϊȷ�ϱ������ô˱�־λ����ʱ����Ϊ�գ�������Ҫ��FRAME_SIZE_ERROR����

ע����� 
- �����ӿ�ʼ�׶α���������SETTINGS֡������һ��Ҫ���� 
- �����ӵ����������ڿ���������һ�˵㷢�� 
- �����߲���Ҫά��������״̬��ֻ��Ҫ��¼��ǰֵ���� 
- SETTINGS֡�������ڵ�ǰ���ӣ�����Ե��������������ʶ��Ϊ0x0 
- �������򲻺Ϲ淶��SETTINGS֡��Ҫ�׳�PROTOCOL_ERROR�������Ӵ��� 
- �����ֽ���Ϊ6���ֽڵı�����6*N (N>=0)

�������̣� 
- ���Ͷ˷�����Ҫ���˶���ҪЯ�������ص�SETTINGS����֡�����ܹ�����ACK��־λ 
- ���ն˽��յ���ACK��־λ��SETTINGS֡�����밴��֡���ֶγ���˳��һһ���д����м䲻�ܹ���������֡ 
- ���ն˴���ʱ����Բ���֧�ֵĲ�������� 
- ���ն˴������֮�󣬱�����Ӧһ��������ACKȷ�ϱ�־λ���޸��ص�SETTINGS֡ 
- ���Ͷ˽��յ�ȷ�ϵ�SETTINGS֡����ʾ������������Ч 
- ���Ͷ˵ȴ�ȷ������ʱ����SETTINGS_TIMEOUT�������Ӵ���


2. HEADER

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|    0x1 (8)    | 00?0 ??0? (8) |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (31)                       |
+=+=============+===============================================+
|Pad Length? (8)|
+-+-------------+-----------------------------------------------+
|E|                 Stream Dependency? (31)                     |
+-+-------------+-----------------------------------------------+
|  Weight? (8)  |
+-+-------------+-----------------------------------------------+
|                   Header Block Fragment (*)                 ...
+---------------------------------------------------------------+
|                           Padding (*)                       ...
+---------------------------------------------------------------+
��ͷ��Ҫ���壬����ͷ����Ӧͷ��ͬʱ��Ҳ���ڴ�һ�������������ڴ�"open"����Զ�̰�ر�"half closed (remote)"״̬�����Է��͡�

�ֶ��б� 
- Pad Length��������PADDED��־�����Ƿ���ʾ��8�����ر�ʾ�����ֽ����� 
- E��һ�����ر�ʾ�������Ƿ�ר�ã���ѡ�ֻ�������ȼ�PRIORITY������ʱ��Ч 
- Stream Dependency��31�����ر�ʾ��������ֻ�������ȼ�PRIORITY������ʱ��Ч 
Weight��8�����أ�һ���ֽڣ���ʾ�޷��ŵ���Ȼ�������ȼ���ֵ��Χ��Ȼ��(1~256)�����֮ΪȨ�ء�ֻ�������ȼ�PRIORITY������ʱ��Ч 
- Header Block Fragment����ͷ���Ƭ 
- Padding�������ֽڣ�������PADDED��־�����Ƿ���ʾ��������Pad Length�ֶξ���

�����־λ�� 
END_STREAM (0x1): ��ͷ��Ϊ���һ������ζ�����Ľ����������ɽ�����CONTINUATION֡�ڵ�ǰ�����У���Ҫ��CONTINUATION֡��ΪHEADERS֡��һ���ֶԴ� 
END_HEADERS (0x4): �˱�ͷ֡�����Ƭ��������һ��֡������������ҪCONTINUATION֡��æ���롣��û�д˱�־��HEADER֡������֡��������CONTINUATION֡�����ڵ�ǰ�����У������������Ҫ��ӦPROTOCOL_ERROR���͵����Ӵ��� 
PADDED (0x8): ��Ҫ���ı�־ 
PRIORITY (0x20): ���ȼ���־λ�����ƶ�����־λE��������������Ȩ�ء�

ע����� 
- �为��Ϊ��ͷ���Ƭ�������ݹ�����Ҫ������CONTINUATION֡�������䡣������ʶ��Ϊ0x0����������Ҫ����
    PROTOCOL_ERROR�����쳣��HEADERS֡�������ȼ���Ϣ��Ϊ�˱���Ǳ�ڵĲ�ͬ��֮�����ȼ�˳��ĸ��š� 
- ��ʵһ������������ͷ�����������£�һ��HEADERS�Ϳ�������ˣ������������Cookie�ֶγ���16KiB��С����������


3. CONTINUATION

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  0x9 (8)      |  0x0/0x4  (8) |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (32)                       |
+=+=============================================================+
|                  Header Block Fragment (*)                    |
+---------------------------------------------------------------+
�ֶ��б� - Header Block Fragment������Э��HEADERS/PUSH_PROMISE�ȵ�֡�޷����������ı�ͷʣ�ಿ�����ݡ�

ע����� 
- һ��HEADERS/PUSH_PROMISE֡���������������CONTINUATION��ֻҪ��һ��֡û������END_HEADERS��־λ���Ͳ���һ��֡�������ݵĽ����� 
- ���ն˴������������ӿ�ʼ��HEADERS/PUSH_PROMISE֡�����һ��������END_HEADERS��־λ֡�������ϲ������ݲ�����һ���������ݿ��� 
- ��HEADERS/PUSH_PROMISE��û��END_HEADERS��־λ����CONTINUATION֡�м䣬�ǲ��ܹ���������֡�ģ�������Ҫ��PROTOCOL_ERROR����

��־λ�� * END_HEADERS(0X4)����ʾ��ͷ������һ��֡��������滹�����CONTINUATION֡��


4. DATA

һ������DATA֡��Ϊ������Ӧ�������壬��Ϊ�����Ľṹ���£�

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
| 0x0 (8)       | 0000 ?00? (8) |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (31)                       |
+=+=============+===============================================+
|Pad Length? (8)|
+---------------+-----------------------------------------------+
|                            Data (*)                         ...
+---------------------------------------------------------------+
|                          Padding? (*)                       ...
+---------------------------------------------------------------+
�ֶΣ� 
Pad Length: һ���ֽڱ�ʾ�����ֽڳ��ȡ�ȡ����PADDED��־�Ƿ�����. 
Data: ������Ӧ�����ݣ�������С��Ҫ��ȥ�����ֶΣ�������䳤�Ⱥ�������ݣ����ȡ� * 
Padding: �������Ϊ���ɸ�0x0�ֽڣ���PADDED��־�����Ƿ���ʾ�����ն˴���ʱ�ɺ�����֤������ݡ�
    ����֤�����ԶԷ�0x0��������ӦPROTOCOL_ERROR���������쳣��

��־λ�� 
END_STREAM (0x1): ��־��֡Ϊ��Ӧ��־�����һ��֡���������˰�ر�/�ر�״̬�� 
PADDED (0x8): ������Ҫ��䣬Padding Length + Data + Padding��ɡ�

ע����� 
- ������ʶ��Ϊ0x0����������Ҫ��ӦPROTOCOL_ERROR���Ӵ��� 
- DATAֻ֡����������"open" or "half closed (remote)"״̬ʱ�����ͳ�ȥ��������ն˱�����Ӧһ��STREAM_CLOSED��
    ���Ӵ�������䳤�Ȳ�С�ڸ��س��ȣ����ն˱�����Ӧһ��PROTOCOL_ERROR���Ӵ���


5. PUSH_PROMISE

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  0x5 (8)      | 0000 ??00 (8) |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (32)                       |
+=+=============================================================+
|Pad Length? (8)|
+-+-------------+-----------------------------------------------+
|R|                Promised Stream ID (31)                      |
+-+-------------------------------------------------------------+
|                  Header Block Fragment (*)                . . .
+---------------------------------------------------------------+
|                           Padding (*)                     . . .
+---------------------------------------------------------------+
��������֪ͨ�Զ˳�ʼ��һ���µ�������׼���Ժ��������ݣ� 
- Ҫ��������Ϊ�򿪻�Զ�˰�رգ�half closed (remote)��״̬������PROTOCOL_ERROR���� 
- ��ŵ������һ��Ҫ����������˳�����ʹ�ã��������Ժ�ʹ�� 
- �ܶԶ�������SETTINGS_ENABLE_PUSH��־λ�����Ƿ��ͣ�������ΪPROTOCOL_ERROR����Դ� 
- ���ն�һ���ܾ��������ͣ��ᷢ��RST_STREAM֡��֪�Է�������Ч

�ֶ��б� 
- Promised Stream ID��31�����ر�ʾ�޷��ŵ���Ȼ����Ϊ���ͱ���������ʶ�������������ڷ����������� 
- Header Block Fragment������ͷ���ֶ�ֵ���ɿ����Ƿ�������ģ��ͻ��˷���һ����Դ����

��־λ�� 
END_HEADERS��0x4/00000010������֡���������ı�ͷ�飬���ú������CONTINUATION֡�� 
PADDED��0x8/00000100������俪�أ������������Pad Length��Padding�Ƿ�Ҫ��䣬�����HEADERS֡����һ�£�����˵

6. PING

���ȼ�֡������ֵΪ0x6��8���ֽڱ�ʾ�������߲�����С����ʱ�䣬�����������ڼ����������Ƿ���Ч��

+-----------------------------------------------+
|                0x8 (24)                       |
+---------------+---------------+---------------+
|  0x6 (8)      | 0000 000? (8) |
+-+-------------+---------------+-------------------------------+
|R|                          0x0 (32)                           |
+=+=============================================================+
|                        Opaque Data (64)                       |
+---------------------------------------------------------------+
�ֶ��б�
- Opaque Data��8���ֽڸ��أ�ֵ������д��

��־λ�� * 
ACK(0x1)��һ�����ã���ʾ��PING֡Ϊ��������Ӧ��PING֡���Ƿ����ߡ�

ע����� 
- PING֡���ͷ�ACK��־λΪ0x0�����շ���Ӧ��PING֡ACK��־λΪ0x1������ֱ�Ӷ����������ȼ�Ҫ������������֡�� 
- PING֡���;������������������ʶ��Ϊ0x0�����շ���Ҫ��ӦPROTOCOL_ERROR�������Ӵ��� 
- �������س��ȣ���������Ҫ��ӦFRAME_SIZE_ERROR�������Ӵ���


7. PRIORITY

���ȼ�֡������ֵΪ0x2��5���ֽڱ�ʾ������˷��ͷ��������ȼ�Ȩ�صĽ���ֵ���������κ�״̬�¶����Է��ͣ��������л�رյ�����

+-----------------------------------------------+
|                   0x5 (24)                    |
+---------------+---------------+---------------+
|   0x2 (8)     |    0x0 (8)    |
+-+-------------+---------------+-------------------------------+
|R|                  Stream Identifier (31)                     |
+=+=============================================================+
|E|                  Stream Dependency (31)                     |
+-+-------------+-----------------------------------------------+
| Weight (8)    |
+---------------+
�ֶ��б� 
- E�����Ƿ���� 
- Stream Dependency����������ֵΪ���ı�ʶ������ȻҲ��31�����ر�ʾ�� 
- Weight��Ȩ��/���ȼ���һ���ֽڱ�ʾ��Ȼ������Χ1~256

ע�����
- PRIORITY֡������ʶ��Ϊ0x0�����շ���Ҫ��ӦPROTOCOL_ERROR���͵����Ӵ��� 
- PRIORITY֡���������κ�״̬�·��ͣ��������ǲ��ܹ���һ�������б�ͷ��������֡������֣�
�䷢��ʱ����Ҫ�������Ѿ���������Ȼ���Է��ͣ����Ѿ�û��ʲôЧ���� 

- ����5���ֽ�PRIORITY֡���շ���ӦFRAME_SIZE_ERROR����������

8. WINDOW_UPDATE

+-----------------------------------------------+
|                0x4 (24)                       |
+---------------+---------------+---------------+
|   0x8 (8)     |    0x0 (8)    |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (31)                       |
+=+=============================================================+
|R|              Window Size Increment (31)                     |
+-+-------------------------------------------------------------+
��������֡�������ڵ������Լ��������ӣ���ֻ��Ӱ�������˵�֮�䴫���DATA����֡������ע�⣬�н鲻ת����֡��

�ֶ��б� 
- Window Size Increment��31������λ�޷�����Ȼ������ΧΪ1-2^31-1��2,147,483,647�����ֽ�����
  ���������߿��Է��͵�����ֽ������Լ������߿��Խ��յ�������ֽ�����

ע����� 
- Ŀǰ����ֻ��Ӱ�쵽DATA����֡ 
- ����ʶ��Ϊ0��Ӱ���������ӣ��ǵ����� 
- ����ʶ����Ϊ�գ��������ı�ʶ������ֻ�ܹ�Ӱ�쵽������ 
- WINDOW_UPDATE��ĳ��Я����END_STREAM֡�ĺ��汻���ͣ���ǰ�����ڹرջ�Զ�̹ر�״̬�������ն˿ɺ��ԣ���������Ϊ����Դ� 
- �����߲��ܷ���һ������ֵ���ڽ������ѳ��У����ն��Ѿ�ӵ��һ�����ش���ֵ�����ÿռ��С��WINDOW_UPDATE֡ 
- �����ش��������ÿ��ÿռ��Ѻľ�ʱ���Զ˷���һ���㸺�ش���END_STREAM��־λ��DATA����֡�������������Ϊ 
- �������Ʋ������֡ͷ��ռ�õ�9���ֽڿռ� - ������ֵ�������Ե���������ӦRST_STREAM��������FLOW_CONTROL_ERROR��֡��
  ����������ӵģ���ӦGOAWAY��������FLOW_CONTROL_ERROR��֡ - DATA����֡�Ľ��շ��ڽ��յ�����֮֡����Ҫ����������
  �����ش��ڿ��ÿռ䣬ͬʱҪ�����¿��ô��ڿռ䷢�͸��Զ� - DATA����֡���ͷ����յ�WINDOW_UPDATE֮֡�󣬻�ȡ���¿��ô���ֵ 
- ���շ��첽���·��ͷ�����ֵ��������ͣ��/ʧ�� - Ĭ��������������ƴ���ֵΪ65535�����ǽ��յ�SETTINGS֡SETTINGS_INITIAL_WINDOW_SIZE������
  ����WINDOWS_UPDATE֡Я���Ĵ���ֵ��С�����򲻻�ı� - SETTINGS_INITIAL_WINDOW_SIZEֵ�ĸı�ᵼ�´��ڿ��ÿռ䲻�������׳����⣬
  �����߱���ֹͣ������Ӱ���DATA����֡�ķ���ֱ�����յ�WINDOW_UPDATE֡����µĴ���ֵ���Ż�������͡�

eg���ͻ��������ӽ�����˲��һ����������60KB�����ݣ������Է�����SETTINGS����֡�ĳ�ʼ����ֵΪ16KB���ͻ���ֻ�ܹ���
��WINDOW_UPDATE֡��֪�µĴ���ֵ��Ȼ��������ʹ���ʣ�µ�44KB���� - SETTINGS֡�޷��޸�����������ӵ��������ƴ���ֵ 

- ��һ�˵��ڴ���SETTINGS_INITIAL_WINDOW_SIZEֵʱһ���������ش���ֵ�������ֵ������Ҫ��Ϊһ��FLOW_CONTROL_ERROR�������Ӵ���Դ�


9. RST_STREAWM
���ȼ�֡������ֵΪ0x3��4���ֽڱ�ʾ������˷��ͷ��������ȼ�Ȩ�صĽ���ֵ���κ�ʱ���κ��������Է��ͣ��������л�رյ�����

+-----------------------------------------------+
|                0x4 (24)                       |
+---------------+---------------+---------------+
|  0x3  (8)     |  0x0 (8)      |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (31)                       |
+=+=============================================================+
|                        Error Code (32)                        |
+---------------------------------------------------------------+
�ֶ��б� 
- Error Code��������룬32λ�޷��ŵ���Ȼ����ʾ�����رյĴ���ԭ��

ע����� 
- ���յ�RST_STREAM֡����Ҫ�رն�Ӧ���������ҲҪ���ڹر�״̬�� 
- �����߲��ܹ��ڴ����Ϸ����κ�֡�� 
- ���Ͷ���Ҫ����׼�����ս��ն˽��յ�RST_STREAM֮֡ǰ���͵�֡�������϶��֡��Ҫ���� 
- ������ʶ��Ϊ0x0�����շ���Ҫ��ӦPROTOCOL_ERROR�������Ӵ��� 
- �������ڿ���״̬idle״̬ʱ�ǲ��ܹ�����RST_STREAM֡��������շ��ᱨ��PROOTOCOL_ERROR�������Ӵ���


10. GOAWAY
+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  0x7 (8)      |     0x0 (8)   |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (32)                       |
+=+=============================================================+
|R|                  Last-Stream-ID (31)                        |
+-+-------------------------------------------------------------+
|                      Error Code (32)                          |
+---------------------------------------------------------------+
|                  Additional Debug Data (*)                    |
+---------------------------------------------------------------+
һ��֪ͨ�Զ˽�Ϊ���ŵķ�ʽֹͣ��������ͬʱ��Ҫ���֮ǰ�ѽ�����������

һ�����ͣ������߽����Խ��յ�������ʶ������Last-Stream-ID�κ�֡
�����߲��ܹ��ڵ�ǰ���ϴ��������������������򴴽��µ�����
�����ڷ������Ĺ�����Ϊ���������������ά���׶Σ�����׼�������µ�����
�ֶ�Last-Stream-IDΪ���ͷ�ȡ�����һ�����ڴ�����Ѿ��������ı�ʶ��
��������������ʶ������Last-Stream-ID����֡�����ᱻ����
�ն�Ӧ�������ڹر�����֮ǰ����GOAWAY��ʽ��ʽ��֪�Է�ĳЩ���Ƿ��Ѿ�������
�ն˿���ѡ��ر����ӣ������Ϊ�������ն˲�����GOAWAY֡
GOAWAYӦ���ڵ�ǰ���ӣ��Ǿ�����
û�д����κ���������£�Last-Stream-IDֵ��Ϊ0��Ҳ�ǺϷ�
������ʶ��С�ڻ�������б�ŵı�ʶ���������ӹر�֮ǰû�б���ȫ�رգ���Ҫ�����µ����ӽ�������
���Ͷ��ڷ���GOAWAYʱ����һЩ������û����ɣ�����������Ϊ��״ֱ̬���������
�ն˿����������������ı�ʱ���Ͷ��GOAWAY֡����Last-Stream-ID����������
Additional Debug Dataû�����壬�����������������Ŀ�ġ���Я����½��־û��������ݣ���Ҫ�а�ȫ��֤����δ����Ȩ���ʡ�



�ġ�֡����չ
HTTP/2Э�����չ��������ڵģ������ṩ���������չ������ 
- ������֡����Ҫ����ͨ��֡��ʽ 
- �µ����ò���������������֡������� 
- �µĴ�����룬Լ��֡���ܴ����Ĵ���

������һ����֡����Ҫע�� 
1. �淶�����µ���չ��Ҫ����˫��Э�̺����ʹ�� 
1. ��SETTINGS֡����µĲ����������������ʱ���͸��Զˣ������ʵ����ᷢ�� 
1. ˫��Э�̳ɹ�������ʹ���µ���չ

��֪ALTSVC��BLOCKED������չ֡��

1. ALTSVC
�������ṩ���ͻ��˵�ǰ���õ��������������CNAME���ͻ��˲�֧�ֿ���ѡ�����

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  0xa (8)      |     0x0 (8)   |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier (32)                       |
+=+=============================+===============================+
|         Origin-Len (16)       | Origin? (*)                 ...
+-------------------------------+-------------------------------+
|                   Alt-Svc-Field-Value (*)                   ...
+---------------------------------------------------------------+
�ֶ��б� 
- Origin-Len: 16����λ������˵����Origin�ֶ��ֽ��� 
- Origin: ASCII�ַ�����ʾ������� 
- Alt-Svc-Field-Value: ������Alt-Svc HTTP Header Field������=Length (24) 
- Origin-Len (16)

��Ҫע�⣺ - �н��豸����ת�����ͻ��ˣ�ԭ������н������滻����ת��������ҵ�����ݸ��ͻ��˾���
����ɲο���https://tools.ietf.org/html/draft-ietf-httpbis-alt-svc-06

2. BLOCKED

һ�˸�����һ����Ϊ�ܵ��������Ƶ����������ݵ��޷����͡�

+-----------------------------------------------+
|                Length (24)                    |
+---------------+---------------+---------------+
|  0xb (8)      |     0x0 (8)   |
+-+-------------+---------------+-------------------------------+
|R|                Stream Identifier/0x0 (32)                   |
+=+=============================================================+
Stream Identifier��Ϊ0x0�����ʾ����������ӣ�������Ծ�����
���������ƴ�����Ч֮ǰ���ܷ���BLOCKED
һ�������������⣬˵�����ǵ�ʵ�ֿ�����ȱ�ݣ��޷��õ�����Ĵ�������
ֻ�ܹ���WINDOW_UPDATE֡����֮ǰ��SETTINGS_INITIAL_WINDOW_SIZE��������֮ǰ����

�塣С��

���ϼ�¼��HTTP/2֡�����ṹ��10���ĵ��������ʽ֡���Լ������������չ֡��
*/
#define NGX_HTTP_V2_ALPN_ADVERTISE       "\x02h2"
#define NGX_HTTP_V2_NPN_ADVERTISE        NGX_HTTP_V2_ALPN_ADVERTISE

#define NGX_HTTP_V2_STATE_BUFFER_SIZE    16

#define NGX_HTTP_V2_MAX_FRAME_SIZE       ((1 << 24) - 1) /* http2ͷ�������ֶ����ֵ */

#define NGX_HTTP_V2_INT_OCTETS           4
#define NGX_HTTP_V2_MAX_FIELD            ((1 << NGX_HTTP_V2_INT_OCTETS * 7) - 1)

/* �����������Ǹ�ֵ��V2��ngx_http_v2_stream_t.skip_data */
#define NGX_HTTP_V2_DATA_DISCARD         1
#define NGX_HTTP_V2_DATA_ERROR           2
#define NGX_HTTP_V2_DATA_INTERNAL_ERROR  3

#define NGX_HTTP_V2_FRAME_HEADER_SIZE    9 //HTTP2ͷ������9�ֽ�

/* ���յ���Ӧ֡��ʱ��ص���ngx_http_v2_frame_states */
/* frame types  HTTP2����ͷ����type�ֶ�  ���ʱ�����е�֡������RST_STREAM, WINDOW_UPDATE, PRIORITY����Ҫ���ٷ��ͳ�ȥ */
#define NGX_HTTP_V2_DATA_FRAME           0x0
#define NGX_HTTP_V2_HEADERS_FRAME        0x1
#define NGX_HTTP_V2_PRIORITY_FRAME       0x2
//���յ�RST_STREAM֡����Ҫ�رն�Ӧ���������ҲҪ���ڹر�״̬�� - �����߲��ܹ��ڴ����Ϸ����κ�֡
//������´������õķ��ʹ����������������е�send_bufֵ����������޶���ᷢ��RST֡
#define NGX_HTTP_V2_RST_STREAM_FRAME     0x3 
//setting֡�������յ�HEAD֡�󴴽�����ʱ��ȷ�����ʹ��ڵĴ�С�������ڴ�С��ͬʱ������ͨ��write chain����frame֡��ʱ�򣬾���û�����ݿ�Ĵ�С
//����һ��frame֡Ϊ10K�����ǶԶ�ָ����������ݵ�frame_size=5K�����frame�ᱻ�������frame_size��С�İ��巢��
#define NGX_HTTP_V2_SETTINGS_FRAME       0x4 //����֡�������ȼ����Ƽ�ngx_http_v2_queue_frame
#define NGX_HTTP_V2_PUSH_PROMISE_FRAME   0x5
#define NGX_HTTP_V2_PING_FRAME           0x6
//http2������ɺ���ngx_http_v2_finalize_connection�е��øú����ͶԶ�˵�ݰ�. nginx�յ�goaway֡��ʱ��ɶҲû��
#define NGX_HTTP_V2_GOAWAY_FRAME         0x7
//HTTP/2�����֡���͵�һ�֣���;��֪ͨ�Զ����Ӵ���ֵ��WINDOW_UPDATE��ָ�����ӵĴ�С
#define NGX_HTTP_V2_WINDOW_UPDATE_FRAME  0x8
#define NGX_HTTP_V2_CONTINUATION_FRAME   0x9

/* frame flags HTTP2����ͷ����flag�ֶ� */
#define NGX_HTTP_V2_NO_FLAG              0x00
//ACK (0x1)����ʾ�������Ѿ����յ�SETTING֡����Ϊȷ�ϱ������ô˱�־λ����ʱ����Ϊ��
#define NGX_HTTP_V2_ACK_FLAG             0x01 
/* END_STREAM (0x1) ��λ1������ʾ��ǰ֡��ȷ���������͵����һ֡ */
#define NGX_HTTP_V2_END_STREAM_FLAG      0x01
/* 
�ñ��ֻ���header֡��Ч��λ3��ʾ֡�����������ı�ͷ�飬�Һ���û������֡�� ������END_HEADERS��
�ǵı�ͷ֡��ͬ�����Ϻ�������������֡�����ն˽��յ��κ��������͵�֡�������������ϵ�֡������Ϊ����Ϊ
Э���������Ӵ����� ����ǽ��������һ��header֡��������Ըñ�ǽ�������ngx_http_v2_state_header_complete
*/
#define NGX_HTTP_V2_END_HEADERS_FLAG     0x04
#define NGX_HTTP_V2_PADDED_FLAG          0x08 /* ˵��HTTP2���ݲ��ִ���pad���� */
#define NGX_HTTP_V2_PRIORITY_FLAG        0x20 /* flag���иñ�ʶ����ʾ���ݲ��ִ���Stream Dependency��weight */


typedef struct ngx_http_v2_connection_s   ngx_http_v2_connection_t;
typedef struct ngx_http_v2_node_s         ngx_http_v2_node_t;
typedef struct ngx_http_v2_out_frame_s    ngx_http_v2_out_frame_t;


typedef u_char *(*ngx_http_v2_handler_pt) (ngx_http_v2_connection_t *h2c,
    u_char *pos, u_char *end);

/* ��̬��洢�ڵ㣬���մ���ngx_http_v2_hpack_t.entries�� */
typedef struct {
    ngx_str_t                        name;
    ngx_str_t                        value;
} ngx_http_v2_header_t;


typedef struct {
    ngx_uint_t                       sid; //http2ͷ��sid����ngx_http_v2_state_head
    size_t                           length; //http2ͷ��leng��ÿ����x�ֽڣ����ȥx,��ngx_http_v2_state_head
    size_t                           padding; //HTTP2ͷ��flag����NGX_HTTP_V2_PADDED_FLAG��ʶ����paddingΪpad���ݵĳ���
    unsigned                         flags:8; //http2ͷ��flag����ngx_http_v2_state_head

    //ngx_http_v2_state_save�и�ֵ����ʾname����value�����ݻ�û�ж�ȡ����
    unsigned                         incomplete:1;

    /* HPACK */
    /* ����յ���name����ѹ�����У���Ҫֱ�Ӵӱ��Ķ�ȡ����ngx_http_v2_state_header_block ngx_http_v2_state_process_header */
    unsigned                         parse_name:1;
    /* ����յ���value����ѹ�����У���Ҫֱ�Ӵӱ��Ķ�ȡ����ngx_http_v2_state_header_block ngx_http_v2_state_process_header */
    unsigned                         parse_value:1;
    /* ngx_http_v2_state_header_block����1����ʾ��Ҫ��name:valueͨ��ngx_http_v2_add_header���붯̬���У�Ȼ����0 */
    unsigned                         index:1; //��Ҫ���name:value��ӳ�����
    ngx_http_v2_header_t             header; //��ֵ��ngx_http_v2_get_indexed_header
    /* header�����е�����name+value֮�ͳ������ֵ����Ч��ngx_http_v2_state_process_header 
    ���ƾ���HPACKѹ������������ͷ�����ߴ硣
    */
    size_t                           header_limit;
    //���ƾ���HPACKѹ��������ͷ�е���name:value�ֶε����ߴ硣ͨ��http2_max_field_size����
    size_t                           field_limit;
    u_char                           field_state;

    /*ngx_http_v2_state_field_len��filed_start����ռ����ڴ洢�����������У�ֻ�ܴ�Э���ж�ȡ��name����value��Ӧ���ַ��� */
    u_char                          *field_start;
    u_char                          *field_end;
    /* ��¼��ǰ��Ҫ������name����value�ĳ��ȣ���ֵ��ngx_http_v2_state_field_len */
    size_t                           field_rest;
    /* ��ֵ��ngx_http_v2_state_headers */
    ngx_pool_t                      *pool;

    /* ��ǰ���ڴ����stream����ֵ��ngx_http_v2_state_headers */
    ngx_http_v2_stream_t            *stream;

    /* �������ݲ�������ngx_http_v2_state_save����Ѿ��������ⲿ�ֲ�������������ʱ���浽buffer�У�
    ��ngx_http_v2_read_handler�����жԺ�����ȡ���ݺϲ���һ�� */
    u_char                           buffer[NGX_HTTP_V2_STATE_BUFFER_SIZE];
    size_t                           buffer_used; //buffer�Ѿ�ʹ���˵Ŀռ�

    /* 
    ��ֵ����Ϊ:ngx_http_v2_state_preface  ngx_http_v2_state_header_complete ngx_http_v2_state_skip_headers  ngx_http_v2_state_head
    ִ����ngx_http_v2_read_handler
    ��ȡ�ͻ��˷��͹��������ݺ������handler���н�������
    */
    ngx_http_v2_handler_pt           handler;
} ngx_http_v2_state_t;


/* ngx_http_v2_connection_t.hpack */
typedef struct {
    /* ���ٿռ�͸�ֵ��ngx_http_v2_add_header��Ĭ��64��ָ�����飬ָ��storage�еĶ��ngx_http_v2_header_t�ṹ��
    ͨ��entries[i]ָ��Ϳ���ֱ�ӷ��ʵ�storage�е�ĳ��name:value,����ͨ��entries����ָ��Ϳ��Է��ʵ����е�name:value */
    ngx_http_v2_header_t           **entries;

    /* storage���ܵ�name:value������ngx_http_v2_add_header������ */
    ngx_uint_t                       added;
    /* ���ռ䲻����ʱ�򣬻��ظ�����storage�ռ䣬Ҳ���ǰѲ���name:value����Ϊ��Ч�������µ�name:value��ӵ�storage�У�
       deleted��ʾ��Ϊstorage�ռ䲻��������������µ�name:value�ӽ���������������ϵ�name:value���ڳ��ռ䣬��ngx_http_v2_add_header
    */
    ngx_uint_t                       deleted;
    ngx_uint_t                       reused;
    /* entriesָ��������ָ������� */
    ngx_uint_t                       allocated;

    //Ĭ��NGX_HTTP_V2_TABLE_SIZE
    size_t                           size;
    //Ĭ��NGX_HTTP_V2_TABLE_SIZE   storage�еĿ��ÿռ�
    size_t                           free;
    /* �����Ŀռ������� */
    u_char                          *storage;
    /* ָ��storage�е���Ч�ռ���ʼλ�� */
    u_char                          *pos;
} ngx_http_v2_hpack_t;

/* ngx_http_v2_init�з���ռ� */
struct ngx_http_v2_connection_s {
    ngx_connection_t                *connection;//��Ӧ�Ŀͻ������ӣ���ֵ��ngx_http_v2_init
    ngx_http_connection_t           *http_connection;

    /* ngx_http_v2_create_stream����������ʾ������������������ǰ����ʹ�õ���������ngx_http_v2_close_stream�Լ� */
    ngx_uint_t                       processing;
    
    /* ע��ngx_http_v2_connection_s��send_window��recv_window��init_window��ngx_http_v2_stream_s��send_window��recv_window������ */
    /* ngx_http_v2_send_chain���Ͷ���DATA֡��h2c->send_window�ͻ���ٶ��٣��Զ˽��յ�����h2c->recv_windowҲ����ٶ��٣�
       ���Զ��յ�DATA֡��ʱ��������ָ������ϵ�recv_window��С�Ѿ�С��NGX_HTTP_V2_MAX_WINDOW / 4�ˣ���֪ͨ�����Ͷ�
       ��h2c->send_window����ΪNGX_HTTP_V2_MAX_WINDOW��ͬʱ�Զ�Ҳ�����ӵĽ���recv_window��������MAXֵ���Զ��յ�DATA֡
       ��ʱ��������ָ������ϵ�recv_window��С�Ѿ�С��NGX_HTTP_V2_MAX_WINDOW / 4�ˣ���֪ͨ�����Ͷ˰�h2c->send_window
       ����ΪNGX_HTTP_V2_MAX_WINDOW��ͬʱ�Զ�Ҳ�����ӵĽ���recv_window��������MAXֵ
    */ //���ո����ӱ��ص�send_windowʼ�պͶԶ˵�recv_window����һ�µ�
    //���ķ��ʹ��ڿ���ͨ��setting֡���������ǽ��ܴ��ڶ���Ĭ��ֵNGX_HTTP_V2_MAX_WINDOW
    size_t                           send_window;//Ĭ��NGX_HTTP_V2_DEFAULT_WINDOW 65535
    /* �ڽ��նԶ˷��͹�����DATA֡�󣬻������ֵ��������յ��Զ˷��͹�����len�������ݣ��򱾶˽��մ��ڼ�����ô�࣬
        ��Ч��ngx_http_v2_state_data */
    size_t                           recv_window;//Ĭ��NGX_HTTP_V2_MAX_WINDOW 2^32 -1
    // ���յ��Զ˵�setting֡�󣬻�����������ngx_http_v2_state_settings_params  stream->send_window = h2c->init_window;
    //���ķ��ʹ��ڿ���ͨ��setting֡���������ǽ��ܴ��ڶ���Ĭ��ֵNGX_HTTP_V2_MAX_WINDOW
    size_t                           init_window;//Ĭ��NGX_HTTP_V2_DEFAULT_WINDOW  �������ķ��ʹ��ڴ�С һ��Զ˻ᷢ��SETTING֡���е���

    //���յ��Զ˵�setting֡�󣬻�����������ngx_http_v2_state_settings_params ��������FRAME֡��ʱ��ÿ֡���ݴ�С��������ֵ
    size_t                           frame_size;//Ĭ��NGX_HTTP_V2_DEFAULT_FRAME_SIZE  ���յ��Զ˵�setting֡�󣬻�����������ngx_http_v2_state_settings_params

    ngx_queue_t                      waiting;

    ngx_http_v2_state_t              state;
    /* hpack��̬�������ռ�͸�ֵ��ngx_http_v2_add_header */
    ngx_http_v2_hpack_t              hpack;

    ngx_pool_t                      *pool;
    /* frameͨ����free������ʵ���ظ����ã����Բο�ngx_http_v2_get_frame ngx_http_v2_frame_handler*/
    ngx_http_v2_out_frame_t         *free_frames;
    /* �����ռ�͸�ֵ��ngx_http_v2_create_stream�����ݿͻ�������α���һ������ */
    ngx_connection_t                *free_fake_connections;
    
    /* ngx_http_v2_node_t���͵�����ָ�룬ngx_http_v2_init�д����ռ�͸�ֵ��������ngx_http_v2_node_t��ֵ��ngx_http_v2_get_node_by_id */
    ngx_http_v2_node_t             **streams_index;
    /* ��ngx_http_v2_queue_blocked_frame�а�֡�����������ngx_http_v2_send_output_queue�з��Ͷ����е����� */
    //����ͨ��ͬһ��connect�����������ļ�����2���ļ��������Ϣ�ᱻ���һ��һ�������֡���ص��������Ͻ��н��淢��
    ngx_http_v2_out_frame_t         *last_out; /* http2 header֡��data֡�����ص��������� */
    ngx_queue_t                      posted;
    /*
                         NGX_HTTP_V2_ROOT
                         /|\            /|\
                          /               \
                         /                 \parent          
                        /                   \           
      (����һ������)h2c->dependencies      h2c->dependencies(����һ������)
                      |                          |
                      |                          |
                      |                          |children
                      |                          |
            queue    \|/                        \|/     queue      queue
    nodeB<--------nodeA                         nodeA-------->nodeB------->nodeC (ͬһ�����ڵ��µ��ӽڵ㣬����ĳ��A B C�������ڽڵ�dependencies)

    */
    //�����Ӹ��������������ṹ�ҽ���NGX_HTTP_V2_ROOT���棬 ����ͬһ����������Ķ������Ӧ������node�ڵ㶼ͨ��h2c->dependencies������һ��
    ngx_queue_t                      dependencies;
    /* ���г�ԱΪngx_http_v2_node_t����ֵ��ngx_http_v2_state_priority */
    ngx_queue_t                      closed;
    /* ��ĩβ��һ����ID�� */
    ngx_uint_t                       last_sid;

    unsigned                         closed_nodes:8;
    unsigned                         blocked:1;
};

/*
                     NGX_HTTP_V2_ROOT
                     /|\            /|\
                      /               \
                     /                 \parent          
                    /                   \           
  (����һ������)h2c->dependencies      h2c->dependencies(����һ������)
                  |                          |
                  |                          |
                  |                          |children
                  |                          |
        queue    \|/                        \|/     queue      queue        (A������parent������B������parent������C������parent)
nodeA<--------nodeB                         nodeC-------->nodeB------->nodeA (ͬһ�����ڵ��µ��ӽڵ㣬����ĳ��A B C�������ڽڵ�dependencies)
*/

/* ngx_http_v2_connection_t.streams_indexָ�������ԱΪ�����ͣ�һ����ID��Ӧһ���ýṹ����ngx_http_v2_get_node_by_id */
/* һ��ngx_http_v2_stream_s.node����Ӧһ��ngx_http_v2_node_t */ 
//�������е�node�ڵ㶼ͨ��h2c->dependencies������һ��
struct ngx_http_v2_node_s {  //���յ�header֡��ʱ����ngx_http_v2_get_node_by_id
/* PRIORITY֡ͨ���ĳ����ID��Ӧ��weight��dependecy�������ڸýṹ�У���ngx_http_v2_state_priority */
    ngx_uint_t                       id; /* stream id */
    ngx_http_v2_node_t              *index;
    /* ���û��parent�����ָ��ָ��NGX_HTTP_V2_ROOT */
    ngx_http_v2_node_t              *parent; 
    /* childͨ����queue���뵽parent->children����ôparent�Ϳ���ͨ��children���л�ȡ�����е�child
    �����nodeû��parent��ͨ����queue���뵽&h2c->dependencies����ngx_http_v2_set_dependency,*/
    ngx_queue_t                      queue;
    /* ���е�children�ڵ�ҵ��ö����� */
    //��children(Ҳ���Ǹ��ڵ��children�ڵ�)ָ��node�ڵ㣬����ж��node����parent�ڵ㣬������node�ڵ�ͨ��queue������һ������ͼ:
    /*
        parent
          ^
          |children
          |
          |       queue      queue
          nodeC-------->nodeB------->nodeA (A������parent������B������parent������C������parent)
    */
    ngx_queue_t                      children;
    ngx_queue_t                      reuse;
    //��ʾ��nodeΪNGX_HTTP_V2_ROOT�µĵڼ��㣬rank = parent->rank + 1 //ֻ�����ȼ�֡��headder֡�漰���������������ȼ���
    ngx_uint_t                       rank;
    //�����ȼ�priority֡����headder֡�н�������,��ngx_http_v2_state_priority��ngx_http_v2_state_headers
    //ֻ�����ȼ�֡��headder֡�漰���������������ȼ��� ���ȼ����Ϳ��Ƽ�ngx_http_v2_queue_frame
    /* stream�������ο�http://www.blogjava.net/yongboy/archive/2015/03/19/423611.aspx */
    ngx_uint_t                       weight;
    //��ngx_http_v2_node_s.weightȨ���йأ��ο�ngx_http_v2_set_dependency
    //��nodeȨ��ռ��Ȩ��256�İٷֱ�  ���ȼ����Ϳ��Ƽ�ngx_http_v2_queue_frame
    double                           rel_weight;
    /* ��node��Ӧ������һ��node��Ӧһ��������ֵ��ngx_http_v2_state_headers */
    ngx_http_v2_stream_t            *stream; 
};

/*
�����ռ�͸�ֵ��ngx_http_v2_create_stream
*/
struct ngx_http_v2_stream_s {   
    /*��ʼ��ֵ��ngx_http_v2_create_stream��http2��Ӧ��r����س�Ա��ֵ��ngx_http_v2_pseudo_header*/
    ngx_http_request_t              *request;
    /*��ʼ��ֵ��ngx_http_v2_create_stream*/
    ngx_http_v2_connection_t        *connection;
    /* һ��ngx_http_v2_stream_s.node����Ӧһ��ngx_http_v2_node_t,��id�ȴ��ڸ�node�� */
    /* ��node��Ӧ������һ��node��Ӧһ��������ֵ��ngx_http_v2_state_headers */
    ngx_http_v2_node_t              *node;

    ngx_uint_t                       header_buffers;
    //1��ʾ�����Ѿ���ӣ���û�з��ͣ���ngx_http_v2_finalize_connection��0
    ngx_uint_t                       queued;

    /*
     * A change to SETTINGS_INITIAL_WINDOW_SIZE could cause the
     * send_window to become negative, hence it's signed.
     */
    /* ע��ngx_http_v2_connection_s��send_window��recv_window��init_window��ngx_http_v2_stream_s��send_window��recv_window������ */
    /* ngx_http_v2_state_dataû�ڸ�����ÿ���Ͷ���DATA֡���ݣ�stream->send_window�ͼ��ٶ��٣��Է��յ������ݺ���
       stream->recv_windowҲ��Ӧ�ļ��ٶ��٣����Է�������recv_window����NGX_HTTP_V2_MAX_WINDOW / 4���ְ���recv_window
       ��ԭΪNGX_HTTP_V2_MAX_WINDOW��ͬʱ֪ͨ����������Ӧ��������ԭΪNGX_HTTP_V2_MAX_WINDOW
    ���գ�����stream��send_windowʼ�պͶԶ�strean��recv_window����һ�� */
    //Ĭ��ֵNGX_HTTP_V2_DEFAULT_WINDOW  �������յ��Է���Setting֡����send_window���е���
    ssize_t                          send_window; //Ĭ�ϵ���h2c->init_window��Ҳ����65535
    size_t                           recv_window; //Ĭ��ֵNGX_HTTP_V2_MAX_WINDOW 2^32 - 1

    ngx_http_v2_out_frame_t         *free_frames;
    ngx_chain_t                     *free_data_headers;
    ngx_chain_t                     *free_bufs;

    ngx_queue_t                      queue;
    /* �����ռ�͸�ֵ��ngx_http_v2_cookie,header֡�����������cookie����ֵ���������� */
    ngx_array_t                     *cookies;
    /* header�����е�����name+value֮�ͳ������ֵ����Ч��ngx_http_v2_state_process_header 
        ���ƾ���HPACKѹ������������ͷ�����ߴ硣
    */
    size_t                           header_limit;

    unsigned                         handled:1;
    unsigned                         blocked:1;
    unsigned                         exhausted:1;
    /* ngx_http_v2_state_header_complete�и�ֵ��1��ʾ����header֡�Ѿ����ջ��߷�����ϣ�����û�и�header֡������֡���� */
    unsigned                         end_headers:1;
    unsigned                         in_closed:1;
    unsigned                         out_closed:1;
    /* ���HTTP2����ֵΪNGX_HTTP_V2_DATA_DISCARD�� */
    unsigned                         skip_data:2;
};


/* �����ռ�͸�ֵ��ngx_http_v2_send_settings����Ӧһ��setting��HEADER��֡��ÿ��֡��Ӧһ���ýṹ�����ռ���ngx_http_v2_connection_t->last_out */
//�ýṹstream->free_frames������ظ����ã�ͬһ���Ӷ�Ӧ�Ķ��stream id�ϵ�frame֡��Ϣ����ͨ��ngx_http_v2_queue_frame�������ȼ�����
struct ngx_http_v2_out_frame_s {
    ngx_http_v2_out_frame_t         *next;
    ngx_chain_t                     *first;
    ngx_chain_t                     *last;

    /* 
    ��ֵ:ngx_http_v2_settings_frame_handler ngx_http_v2_data_frame_handler  ngx_http_v2_headers_frame_handler  ngx_http_v2_frame_handler
    ִ����ngx_http_v2_send_output_queue
    */ //ÿ��֡���͵��Զ˳ɹ��󶼻���ö�Ӧ��handler
    ngx_int_t                      (*handler)(ngx_http_v2_connection_t *h2c,
                                        ngx_http_v2_out_frame_t *frame);

    ngx_http_v2_stream_t            *stream;
    size_t                           length;

    /* ˵����֡��ngx_http_v2_send_output_queue���õ�ʱ��û�з��ͳ�ȥ�������ݷ��ͳ�ȥ��ngx_http_v2_out_frame_t�ᱻ
    stream->free_frames���գ���ʱ����Ϊ1���´�get�ظ����õ�ʱ�����0��
    */
    unsigned                         blocked:1;
    unsigned                         fin:1;
};

//ngx_http_v2_queue_frame����Ҫ���͵�֡�������ȼ����ƣ�Ȩ�޸ߵķ�������ײ����͵ķ���β��������������ngx_http_v2_send_output_queue
static ngx_inline void
ngx_http_v2_queue_frame(ngx_http_v2_connection_t *h2c,
    ngx_http_v2_out_frame_t *frame)
{
    ngx_http_v2_out_frame_t  **out;

    /* �������ȼ���ӵ�last_out  */
    for (out = &h2c->last_out; *out; out = &(*out)->next) {

        if ((*out)->blocked || (*out)->stream == NULL) {
            break;
        }

        /* ���νṹ�в�ͬ��rank�㣬��������ȼ������������ȼ��ߣ��ȷ���
           ͬһ������ݣ�rel_weight����ȷ���
        */
        if ((*out)->stream->node->rank < frame->stream->node->rank
            || ((*out)->stream->node->rank == frame->stream->node->rank
                && (*out)->stream->node->rel_weight
                   >= frame->stream->node->rel_weight))
        {
            break;
        }
    }

    frame->next = *out;
    *out = frame;
}

//����ͨ��ͬһ��connect�����������ļ�����2���ļ��������Ϣ(����header֡��DATA֡)�ᱻ���һ��һ�������֡���ص��������Ͻ��н��淢��
/* ���frame��last_out���� */
static ngx_inline void
ngx_http_v2_queue_blocked_frame(ngx_http_v2_connection_t *h2c,
    ngx_http_v2_out_frame_t *frame)
{
    ngx_http_v2_out_frame_t  **out;

    for (out = &h2c->last_out; *out; out = &(*out)->next)
    {
        if ((*out)->blocked || (*out)->stream == NULL) {
            break;
        }
    }

    frame->next = *out;
    *out = frame;
}


void ngx_http_v2_init(ngx_event_t *rev);
void ngx_http_v2_request_headers_init(void);

ngx_int_t ngx_http_v2_read_request_body(ngx_http_request_t *r,
    ngx_http_client_body_handler_pt post_handler);

void ngx_http_v2_close_stream(ngx_http_v2_stream_t *stream, ngx_int_t rc);

ngx_int_t ngx_http_v2_send_output_queue(ngx_http_v2_connection_t *h2c);


ngx_int_t ngx_http_v2_get_indexed_header(ngx_http_v2_connection_t *h2c,
    ngx_uint_t index, ngx_uint_t name_only);
ngx_int_t ngx_http_v2_add_header(ngx_http_v2_connection_t *h2c,
    ngx_http_v2_header_t *header);
ngx_int_t ngx_http_v2_table_size(ngx_http_v2_connection_t *h2c, size_t size);


ngx_int_t ngx_http_v2_huff_decode(u_char *state, u_char *src, size_t len,
    u_char **dst, ngx_uint_t last, ngx_log_t *log);

/* ��bits - 1λȫΪ1  ����bitsΪ4������Ϊbit:1111   ����bitsΪ5������Ϊbit:1111*/
#define ngx_http_v2_prefix(bits)  ((1 << (bits)) - 1)


#if (NGX_HAVE_NONALIGNED)

#define ngx_http_v2_parse_uint16(p)  ntohs(*(uint16_t *) (p))
#define ngx_http_v2_parse_uint32(p)  ntohl(*(uint32_t *) (p))

#else

#define ngx_http_v2_parse_uint16(p)  ((p)[0] << 8 | (p)[1])
#define ngx_http_v2_parse_uint32(p)                                           \
    ((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3])

#endif

#define ngx_http_v2_parse_length(p)  ((p) >> 8)
#define ngx_http_v2_parse_type(p)    ((p) & 0xff)
#define ngx_http_v2_parse_sid(p)     (ngx_http_v2_parse_uint32(p) & 0x7fffffff)
#define ngx_http_v2_parse_window(p)  (ngx_http_v2_parse_uint32(p) & 0x7fffffff)


#define ngx_http_v2_write_uint16_aligned(p, s)                                \
    (*(uint16_t *) (p) = htons((uint16_t) (s)), (p) + sizeof(uint16_t))
#define ngx_http_v2_write_uint32_aligned(p, s)                                \
    (*(uint32_t *) (p) = htonl((uint32_t) (s)), (p) + sizeof(uint32_t))

#if (NGX_HAVE_NONALIGNED)

#define ngx_http_v2_write_uint16  ngx_http_v2_write_uint16_aligned
#define ngx_http_v2_write_uint32  ngx_http_v2_write_uint32_aligned

#else

#define ngx_http_v2_write_uint16(p, s)                                        \
    ((p)[0] = (u_char) ((s) >> 8),                                            \
     (p)[1] = (u_char)  (s),                                                  \
     (p) + sizeof(uint16_t))

#define ngx_http_v2_write_uint32(p, s)                                        \
    ((p)[0] = (u_char) ((s) >> 24),                                           \
     (p)[1] = (u_char) ((s) >> 16),                                           \
     (p)[2] = (u_char) ((s) >> 8),                                            \
     (p)[3] = (u_char)  (s),                                                  \
     (p) + sizeof(uint32_t))

#endif

/* http2ͷ��type��lenһ���պ�4�ֽڣ�http2ͷ��type��len��ֵ */
#define ngx_http_v2_write_len_and_type(p, l, t)                               \
    ngx_http_v2_write_uint32_aligned(p, (l) << 8 | (t))

#define ngx_http_v2_write_sid  ngx_http_v2_write_uint32

#endif /* _NGX_HTTP_V2_H_INCLUDED_ */
