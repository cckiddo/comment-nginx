/* 
FastCGI�Ƕ�CGI�Ŀ��ŵ���չ����Ϊ����������Ӧ���ṩ�����ܣ���û��Web������API��ȱ�㣨penalty���� 

���淶�������޵ģ�narrow��Ŀ�꣺��Ӧ�õ��ӽǹ涨FastCGIӦ�ú�֧��FastCGI��Web������֮��Ľӿڡ�Web�������ĺܶ������漰FastCGI��
������˵��Ӧ�ù�����ʩ��Ӧ�õ�Web�������Ľӿ��޹أ���˲������������ 

���淶������Unix����ȷ�е�˵��������֧�ֲ�����socket��POSIXϵͳ�������淶����Ǽ򵥵�ͨ��Э�飬���ֽ����޹أ����ҽ���չ������ϵͳ�� 

���ǽ�ͨ����CGI/1.1�ĳ���Unixʵ�ֵıȽ�������FastCGI��FastCGI���������֧�ֳ�פ��long-lived��Ӧ�ý��̣�Ҳ����Ӧ�÷�����������
��CGI/1.1�ĳ���Unixʵ�ֵ���Ҫ���𣬺��߹���Ӧ�ý��̣�������Ӧһ�������Լ������˳��� 

FastCGI���̵ĳ�ʼ״̬��CGI/1.1���̵ĳ�ʼ״̬����࣬��ΪFastCGI���̿�ʼ���������κζ�������û�г���Ĵ򿪵��ļ�stdin��stdout��stderr��
����������ͨ�������������մ�������Ϣ��FastCGI���̵ĳ�ʼ״̬�Ĺؼ������Ǹ����ڼ�����socket��ͨ��������������Web�����������ӡ� 

FastCGI�����������ڼ�����socket���յ�һ������֮�󣬽���ִ�м򵥵�Э�������պͷ������ݡ�Э�����������Ŀ�ġ����ȣ�Э���ڶ��������
FastCGI������·���õ���������·�����֧���ܹ������¼���������̱߳�̼��������������Ӧ�á��ڶ�����ÿ�������ڲ���Э����ÿ������
���ṩ���ɶ����������������ַ�ʽ�����磬stdout��stderr����ͨ����Ӧ�õ�Web�������ĵ���������·���ݣ���������CGI/1.1������Ҫ�����Ĺܵ��� 

һ��FastCGIӦ�ð��ݼ�����ȷ����Ľ�ɫ�е�һ������õ�����Ӧ����Responder����ɫ������Ӧ�ý���������HTTP������ص���Ϣ��������һ��
HTTP��Ӧ������CGI/1.1������ݵĽ�ɫ���ڶ�����ɫ����֤����Authorizer��������Ӧ�ý���������HTTP������ص���Ϣ��������һ���Ͽ�/δ����
�ɵ��ж�����������ɫ�ǹ�������Filter��������Ӧ�ý���������HTTP������ص���Ϣ���Լ���������Դ洢��Web�������ϵ��ļ�����������������
"�ѹ���"�����������ΪHTTP��Ӧ�����������չ�ģ�����Ժ�ɶ�������FastCGI�� 

�ڱ��淶�����ಿ�֣�ֻҪ�����������������"FastCGIӦ��"��"Ӧ�ý���"��"Ӧ�÷�����"��дΪ"Ӧ��"�� 

2. ��ʼ����״̬ 2.1 ������ 
Web������ȱʡ����һ�����е���Ԫ�صĲ�������Ԫ����Ӧ�õ����֣�������ִ��·���������һ���֡�Web���������ṩĳ�ַ�ʽ��ָ����ͬ��
Ӧ�����������ϸ�Ĳ����� 

ע�⣬��Web������ִ�е��ļ������ǽ��ͳ����ļ������ַ�#!��ͷ���ı��ļ������������е�Ӧ�ò�����Ĺ�����execve manҳ�������� 

2.2 �ļ������� 
��Ӧ�ÿ�ʼִ��ʱ��Web����������һ���򿪵��ļ���������FCGI_LISTENSOCK_FILENO��������������Web������������һ�����ڼ�����socket�� 

FCGI_LISTENSOCK_FILENO����STDIN_FILENO����Ӧ�ÿ�ʼִ��ʱ����׼��������STDOUT_FILENO��STDERR_FILENO���رա�һ������Ӧ��ȷ��������
CGI���õĻ�����FastCGI���õĿɿ������ǵ���getpeername(FCGI_LISTENSOCK_FILENO)������FastCGIӦ�ã�������-1��������errnoΪENOTCONN�� 

Web���������ڿɿ������ѡ��Unix��ʽ�ܵ���AF_UNIX����TCP/IP��AF_INET�������ں���FCGI_LISTENSOCK_FILENO socket���ڲ�״̬�еġ� 

2.3 �������� 
Web���������û���������Ӧ�ô����������淶������һ�������ı�����FCGI_WEB_SERVER_ADDRS�������������Ź淶�ķ�չ������ࡣWeb������
���ṩĳ�ַ�ʽ��������������������PATH������ 

2.4 ����״̬ 
Web���������ṩĳ�ַ�ʽָ��Ӧ�õĳ�ʼ����״̬�����������������̵����ȼ����û�ID����ID����Ŀ¼�͹���Ŀ¼�� 

3. Э����� 3.1 ���ţ�Notation�� 
������C���Է���������Э����Ϣ��ʽ�����еĽṹԪ�ذ���unsigned char���Ͷ�������У�����ISO C����������ȷ�ķ�ʽ������չ����������
�䡣�ṹ�ж���ĵ�һ�ֽڵ�һ�������ͣ��ڶ��ֽ��ŵڶ������������ơ� 

����������Լ���������ǵĶ��塣 

���ȣ����������ڵĽṹ������˺�׺��B1���͡�B0��֮��������ͬʱ������ʾ�������������Ϊ��ֵΪB1<<8 + B0�ĵ������֡��õ������ֵ�
��������Щ�����ȥ��׺�����֡����Լ��������һ���ɳ��������ֽڱ�ʾ�����ֵĴ���ʽ�� 

�ڶ���������չC�ṹ��struct����������ʽ 

struct {
unsigned char mumbleLengthB1;
unsigned char mumbleLengthB0;
... /* �������� * /
unsigned char mumbleData[mumbleLength];
};

��ʾһ���䳤�ṹ���˴�����ĳ����ɽ����һ���������ָʾ��ֵȷ���� 

3.2 ���ܴ�����· 
FastCGIӦ�����ļ�������FCGI_LISTENSOCK_FILENO���õ�socket�ϵ���accept()�������µĴ�����·�����accept()�ɹ�������Ҳ����FCGI_WEB_SERVER_ADDRS
������������Ӧ������ִ���������⴦�� 


FCGI_WEB_SERVER_ADDRS��ֵ��һ����Ч������Web��������IP��ַ�� 
�������FCGI_WEB_SERVER_ADDRS��Ӧ��У������·��ͬ��IP��ַ�Ƿ��б��еĳ�Ա�����У��ʧ�ܣ�������·������TCP/IP����Ŀ����ԣ���Ӧ
�ùر���·��Ϊ��Ӧ�� 

FCGI_WEB_SERVER_ADDRS����ʾ�ɶ��ŷָ���IP��ַ�б�ÿ��IP��ַд���ĸ���С����ָ���������[0..255]�е�ʮ�����������Ըñ�����һ��
�Ϸ�����FCGI_WEB_SERVER_ADDRS=199.170.183.28,199.170.183.71�� 



Ӧ�ÿɽ������ɸ����д�����·�������Ǳ���ġ� 

3.3 ��¼ 
Ӧ�����ü򵥵�Э��ִ������Web������������Э��ϸ������Ӧ�õĽ�ɫ�����Ǵ���˵����Web���������ȷ��Ͳ������������ݵ�Ӧ�ã�Ȼ��Ӧ
�÷��ͽ�����ݵ�Web�����������Ӧ����Web����������һ��������ɵ�ָʾ�� 

ͨ��������·����������������FastCGI��¼�����ء�FastCGI��¼ʵ�������¡����ȣ���¼�ڶ��������FastCGI������·���ô�����·���ö�
·���ü���֧���ܹ������¼���������̱߳�̼��������������Ӧ�á��ڶ����ڵ��������ڲ�����¼��ÿ���������ṩ���ɶ���������������
�ַ�ʽ�����磬stdout��stderr������ͨ����Ӧ�õ�Web�������ĵ���������·���ݣ�������Ҫ�����Ĺܵ��� 

typedef struct {
unsigned char version;
unsigned char type;
unsigned char requestIdB1;
unsigned char requestIdB0;
unsigned char contentLengthB1;
unsigned char contentLengthB0;
unsigned char paddingLength;
unsigned char reserved;
unsigned char contentData[contentLength];
unsigned char paddingData[paddingLength];
} FCGI_Record;

FastCGI��¼��һ������ǰ׺����ɱ����������ݺ�����ֽ���ɡ���¼�����߸������ 


version: ��ʶFastCGIЭ��汾�����淶������document��FCGI_VERSION_1�� 
type: ��ʶFastCGI��¼���ͣ�Ҳ���Ǽ�¼ִ�е�һ��ְ�ܡ��ض���¼���ͺ����ǵĹ����ں��沿����ϸ˵���� 
requestId: ��ʶ��¼������FastCGI���� 
contentLength: ��¼��contentData������ֽ����� 
paddingLength: ��¼��paddingData������ֽ����� 
contentData: ��0��65535�ֽ�֮������ݣ����ݼ�¼���ͽ��н��͡� 
paddingData: ��0��255�ֽ�֮������ݣ������ԡ�

�����ò��ϸ��C�ṹ��ʼ���﷨��ָ������FastCGI��¼������ʡ��version�����������䣨Padding�������Ұ�requestId��Ϊ���֡����
{FCGI_END_REQUEST, 1, {FCGI_REQUEST_COMPLETE,0}}�Ǹ�type == FCGI_END_REQUEST��requestId == 1��contentData == {FCGI_REQUEST_COMPLETE,0}�ļ�¼�� 

��䣨Padding�� 
Э����������������Ƿ��͵ļ�¼������Ҫ������߽���paddingLength������paddingData�������������Ϊ����Ч�ش����ֶ�������ݡ�
X����ϵͳЭ���ϵľ�����ʾ�����ֶ��뷽ʽ���������ơ� 

���ǽ����¼�������ڰ��ֽڱ����ı߽��ϡ�FCGI_Record�Ķ��������ǰ��ֽڡ� 

��������ID 
Web����������FastCGI����ID��Ӧ�����˸���������·�ϵ�ÿ������ID�ĵ�ǰ״̬����Ӧ���յ�һ����¼{FCGI_BEGIN_REQUEST, R, ...}ʱ��
����ID R�����Ч�ģ����ҵ�Ӧ����Web���������ͼ�¼{FCGI_END_REQUEST, R, ...}ʱ�����Ч�ġ� 

������ID R��Чʱ��Ӧ�û����requestId == R�ļ�¼�����˸ղ�������FCGI_BEGIN_REQUEST��¼�� 

Web���������Ա���С��FastCGI����ID�����ַ�ʽ��Ӧ�������ö���������ǳ�������ϣ������������ID��״̬��Ӧ��Ҳ��ÿ�ν���һ������
��ѡ����������£�Ӧ��ֻ����Ե�ǰ������ID��������requestIdֵ�� 

��¼���͵����� 
���������õķ���FastCGI��¼���͵ķ�ʽ��

��һ�������ڹ���management����¼��Ӧ�ã�application����¼֮�䡣�����¼�������ض����κ�Web�������������Ϣ���������Ӧ�õ�Э��
��������Ϣ��Ӧ�ü�¼���������ض��������Ϣ����requestId�����ʶ�� 

�����¼��0ֵ��requestId��Ҳ��Ϊnull����ID��Ӧ�ü�¼�з�0��requestId�� 

�ڶ�����������ɢ��������¼֮�䡣һ����ɢ��¼����һ���Լ����������ݵ�������ĵ�Ԫ��һ������¼��stream�Ĳ��֣�Ҳ����һ����������
��0�����ǿռ�¼��length != 0�������һ�������͵Ŀռ�¼��length == 0��������������¼�Ķ��contentData���ʱ���γ�һ���ֽ����У�
���ֽ�����������ֵ���������ֵ���������������ٸ���¼�������ֽ�����ڷǿռ�¼����䡣 

�����ַ����Ƕ����ġ��ڱ����FastCGIЭ�鶨��ļ�¼�����У����й����¼����Ҳ����ɢ��¼���ͣ����Ҽ�������Ӧ�ü�¼���Ͷ�������¼���͡�
��������Ӧ�ü�¼��������ɢ�ģ�����û��ʲô�ܷ�ֹ��ĳЩ�Ժ��Э��汾�ж���һ����ʽ�Ĺ����¼���͡� 

3.4 ��-ֵ�� 
FastCGIӦ�õĺܶ��ɫ��Ҫ��д�ɱ������Ŀɱ䳤�ȵ�ֵ������Ϊ������-ֵ���ṩ��׼��ʽ�����á� 

FastCGI�����ֳ��ȣ����ֵ�ĳ��ȣ�������֣����ֵ����ʽ������-ֵ�ԡ�127�ֽڻ���ٵĳ�������һ�ֽ��б��룬�������ĳ������������ֽ��б��룺 

typedef struct {
unsigned char nameLengthB0; /* nameLengthB0 >> 7 == 0 * /
unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 * /
unsigned char nameData[nameLength];
unsigned char valueData[valueLength];
} FCGI_NameValuePair11;
typedef struct {
unsigned char nameLengthB0; /* nameLengthB0 >> 7 == 0 * /
unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 * /
unsigned char valueLengthB2;
unsigned char valueLengthB1;
unsigned char valueLengthB0;
unsigned char nameData[nameLength];
unsigned char valueData[valueLength
((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
} FCGI_NameValuePair14;
typedef struct {
unsigned char nameLengthB3; /* nameLengthB3 >> 7 == 1 * /
unsigned char nameLengthB2;
unsigned char nameLengthB1;
unsigned char nameLengthB0;
unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 * /
unsigned char nameData[nameLength
((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
unsigned char valueData[valueLength];
} FCGI_NameValuePair41;
typedef struct {
unsigned char nameLengthB3; /* nameLengthB3 >> 7 == 1 * /
unsigned char nameLengthB2;
unsigned char nameLengthB1;
unsigned char nameLengthB0;
unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 * /
unsigned char valueLengthB2;
unsigned char valueLengthB1;
unsigned char valueLengthB0;
unsigned char nameData[nameLength
((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
unsigned char valueData[valueLength
((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
} FCGI_NameValuePair44;

���ȵĵ�һ�ֽڵĸ�λָʾ���ȵı��뷽ʽ����λΪ0��ζ��һ���ֽڵı��뷽ʽ��1��ζ�����ֽڵı��뷽ʽ�� 

��-ֵ�Ը�ʽ�������߲��ö���ı��뷽ʽ���ܴ��������ֵ������������������̷�����ȷ�������ڴ棬��ʹ���ھ޴��ֵ�� 

3.5 �رմ�����· 
Web���������ƴ�����·�������ڡ���û�л������ʱWeb�������ܹر���·������Web������Ҳ�ܰѹرյ�ְȨί�и�Ӧ�ã���FCGI_BEGIN_REQUEST����
�������£�Ӧ����ָ�����������ʱ�ر���·�� 

����������ṩ�˶���Ӧ�÷�񡣼򵥵�Ӧ�û�һ�δ���һ�����󣬲���Ϊÿ���������һ���µĴ�����·�������ӵ�Ӧ�û�ͨ��һ����������
��·�����������󣬶��һ᳤�ڱ��ִ�����·Ϊ��״̬�� 

�򵥵�Ӧ��ͨ����д����Ӧ������رմ�����·�ɵõ��ش������������Web��������Ҫ���Ƴ�פ��·�������ڡ� 

��Ӧ�ùر�һ����·����һ����·�ر��ˣ����ͳ�ʼ��һ������·�� 

4. ����Management����¼���� 4.1 FCGI_GET_VALUES, FCGI_GET_VALUES_RESULT 
Web�������ܲ�ѯӦ���ڲ��ľ���ı��������͵أ�����������Ӧ��������ִ�в�ѯ��ʹϵͳ���õ�ĳЩ�����Զ����� 

Ӧ�ð��յ��Ĳ�ѯ��Ϊ��¼{FCGI_GET_VALUES, 0, ...}��FCGI_GET_VALUES��¼��contentData���ְ���һϵ��ֵΪ�յ���-ֵ�ԡ� 

Ӧ��ͨ�����Ͳ�����ֵ��{FCGI_GET_VALUES_RESULT, 0, ...}��¼����Ӧ�����Ӧ�ò�����ѯ�а�����һ����������������Ӧ�к����Ǹ����֡� 

FCGI_GET_VALUES�����Ϊ���������ı���������ʼ���ṩ��Ϣ������������ִ��Ӧ�ú���·�Ĺ��� 


FCGI_MAX_CONNS����Ӧ�ý����ܵĲ���������·�����ֵ������"1"��"10"�� 
FCGI_MAX_REQS����Ӧ�ý����ܵĲ�����������ֵ������"1"��"50"�� 
FCGI_MPXS_CONNS�����Ӧ�ò���·������·��Ҳ����ͨ��ÿ����·������������Ϊ "0"��������Ϊ"1"��

Ӧ�ÿ����κ�ʱ���յ�FCGI_GET_VALUES��¼������FastCGI�⣬Ӧ�õ���Ӧ�����漰Ӧ�ù��еĿ⡣ 

4.2 FCGI_UNKNOWN_TYPE 
�ڱ�Э���δ���汾�У������¼���ͼ����ܻ�������Ϊ�������ݱ���׼����Э�����FCGI_UNKNOWN_TYPE�����¼����Ӧ���յ��޷���������
ΪT�Ĺ����¼ʱ������{FCGI_UNKNOWN_TYPE, 0, {T}}��Ӧ�� 

FCGI_UNKNOWN_TYPE��¼��contentData���������ʽ�� 

typedef struct {
unsigned char type; 
unsigned char reserved[7];
} FCGI_UnknownTypeBody;

type������޷�ʶ��Ĺ����¼�����͡� 

5. Ӧ�ã�Application����¼���� 5.1 FCGI_BEGIN_REQUEST 
Web����������FCGI_BEGIN_REQUEST��¼��ʼһ������ 

FCGI_BEGIN_REQUEST��¼��contentData���������ʽ�� 

typedef struct {
unsigned char roleB1;
unsigned char roleB0;
unsigned char flags;
unsigned char reserved[5];
} FCGI_BeginRequestBody;

role�������Web����������Ӧ�ð��ݵĽ�ɫ����ǰ����Ľ�ɫ�У� 


FCGI_RESPONDER 
FCGI_AUTHORIZER 
FCGI_FILTER 

��ɫ������ĵ�6����������ϸ�������� 

flags�������һ��������·�رյ�λ�� 


flags & FCGI_KEEP_CONN�����Ϊ0����Ӧ���ڶԱ���������Ӧ��ر���·�������0��Ӧ���ڶԱ���������Ӧ�󲻻�ر���·��Web������Ϊ��·������Ӧ�ԡ�
5.2 ��-ֵ������FCGI_PARAMS FCGI_PARAMS 
������¼���ͣ����ڴ�Web��������Ӧ�÷�����-ֵ�ԡ���-ֵ�Ա���̵����������ͣ�û���ض�˳�� 

5.3 �ֽ�����FCGI_STDIN, FCGI_DATA, FCGI_STDOUT, FCGI_STDERR FCGI_STDIN 
������¼���ͣ����ڴ�Web��������Ӧ�÷����������ݡ�FCGI_DATA����һ������¼���ͣ�������Ӧ�÷��Ͷ������ݡ� 

FCGI_STDOUT��FCGI_STDERR��������¼���ͣ��ֱ����ڴ�Ӧ����Web�����������������ݺʹ������ݡ� 

5.4 FCGI_ABORT_REQUEST 
Web����������FCGI_ABORT_REQUEST��¼����ֹ�����յ�{FCGI_ABORT_REQUEST, R}��Ӧ�þ�����{FCGI_END_REQUEST, R, {FCGI_REQUEST_COMPLETE, 
appStatus}}��Ӧ��������ʵ������Ӧ�õ���Ӧ������������FastCGI��ĵͼ�ȷ�ϡ� 

��HTTP�ͻ��˹ر������Ĵ�����·�������ܿͻ���ί�е�FastCGI������������ʱ��Web��������ֹ��FastCGI��������������Ʋ�̫���ܣ�����FastCGI
������к̵ܶ���Ӧʱ�䣬ͬʱ����ͻ��˺�����Web�������ṩ������塣����FastCGIӦ��������ϵͳ��ͨ�Ż�ִ�з������˽�ջ���ܱ����ڡ� 

������ͨ��һ��������·��·��������ʱ��Web��������ͨ���ر�����Ĵ�����·����ֹ���󡣵�ʹ�ö�·��������ʱ���رմ�����·���в��ҵĽ����
��ֹ��·�ϵ��������� 

5.5 FCGI_END_REQUEST 
�����Ѿ����������󣬻����Ѿ��ܾ�������Ӧ�÷���FCGI_END_REQUEST��¼����ֹ���� 

FCGI_END_REQUEST��¼��contentData���������ʽ�� 

typedef struct {
unsigned char appStatusB3;
unsigned char appStatusB2;
unsigned char appStatusB1;
unsigned char appStatusB0;
unsigned char protocolStatus;
unsigned char reserved[3];
} FCGI_EndRequestBody;

appStatus�����Ӧ�ü����״̬�롣ÿ�ֽ�ɫ˵����appStatus���÷��� 

protocolStatus�����Э�鼶���״̬�룻���ܵ�protocolStatusֵ�ǣ� 


FCGI_REQUEST_COMPLETE����������������� 
FCGI_CANT_MPX_CONN���ܾ��������ⷢ����Web������ͨ��һ����·��Ӧ�÷��Ͳ���������ʱ�����߱����Ϊÿ����·ÿ�δ���һ������ 
FCGI_OVERLOADED���ܾ��������ⷢ����Ӧ������ĳЩ��Դʱ���������ݿ����ӡ� 
FCGI_UNKNOWN_ROLE���ܾ��������ⷢ����Web������ָ����һ��Ӧ�ò���ʶ��Ľ�ɫʱ��
6. ��ɫ 6.1 ��ɫЭ�� 
��ɫЭ��ֻ������Ӧ�ü�¼���͵ļ�¼�����Ǳ����������������������ݡ� 

Ϊ����Э��ɿ��Լ���Ӧ�ñ�̣���ɫЭ�鱻���ʹ�ý���˳����飨nearly sequential marshalling�������ϸ�˳������Э���У�Ӧ�ý���
���һ�����룬Ȼ���ǵڶ������������ơ�ֱ���յ�ȫ����ͬ���أ�Ӧ�÷������һ�������Ȼ���ǵڶ������������ơ�ֱ������ȫ�������벻��
�໥����ģ����Ҳ���ǡ� 

����ĳЩFastCGI��ɫ��˳����������̫�����ƣ���ΪCGI�����ܲ���ʱ�޵أ�timing restriction��д��stdout��stderr�������õ���FCGI_STDOUT��
FCGI_STDERR�Ľ�ɫЭ������������������ 

���н�ɫЭ��ʹ��FCGI_STDERR���ķ�ʽǡ��stderr�ڴ�ͳ��Ӧ�ñ���е�ʹ�÷�ʽ���������ķ�ʽ����Ӧ�ü�����FCGI_STDERR����ʹ�����ǿ�ѡ
�ġ����û�д���Ҫ���棬Ӧ��Ҫô������FCGI_STDERR��¼��Ҫô����һ��0���ȵ�FCGI_STDERR��¼�� 

����ɫЭ��Ҫ���䲻ͬ��FCGI_STDERR����ʱ���������ٴ���һ�������͵ļ�¼����ʹ���ǿյġ� 

�ٴι�ע�ɿ���Э��ͼ򻯵�Ӧ�ñ�̼�������ɫЭ�鱻���Ϊ��������-��Ӧ��������������-��ӦЭ���У�Ӧ���ڷ����������¼ǰ���������е�
�����¼������-��ӦЭ�鲻������ˮ�߼�����pipelining���� 

����ĳЩFastCGI��ɫ��������Ӧ����Լ��̫ǿ���Ͼ���CGI���������ڿ�ʼдstdoutǰ��ȡȫ��stdin������ĳЩ��ɫЭ�������ض��Ŀ����ԡ���
�ȣ����˽�β�������룬Ӧ�ý������������롣����ʼ���ս�β��������ʱ��Ӧ�ÿ�ʼд������� 

����ɫЭ����FCGI_PARAMS�����ı�ֵʱ������CGI����ӻ��������õ���ֵ���䳤�Ȳ�������β��null�ֽڣ���������������null�ֽڡ���Ҫ�ṩ
environ(7)��ʽ����-ֵ�Ե�Ӧ�ñ���������ֵ�����Ⱥţ�����ֵ�����null�ֽڡ� 

��ɫЭ�鲻֧��CGI��δ�����ģ�non-parsed����ͷ���ԡ�FastCGIӦ��ʹ��CGI��ͷStatus��Location������Ӧ״̬�� 

6.2 ��Ӧ����Responder�� 
��Ϊ��Ӧ����FastCGIӦ�þ���ͬCGI/1.1һ����Ŀ�ģ���������HTTP���������������Ϣ������HTTP��Ӧ�� 

�����Խ�����������Ӧ��ģ��CGI/1.1��ÿ��Ԫ�أ� 


��Ӧ��Ӧ��ͨ��FCGI_PARAMS��������Web��������CGI/1.1���������� 
��������Ӧ��Ӧ��ͨ��FCGI_STDIN��������Web��������CGI/1.1 stdin���ݡ����յ���βָʾǰ��Ӧ�ôӸ����������CONTENT_LENGTH�ֽڡ�
��ֻ��HTTP�ͻ���δ���ṩʱ��������Ϊ�ͻ��˱����ˣ�Ӧ�ò��յ�����CONTENT_LENGTH���ֽڡ��� 
��Ӧ��Ӧ��ͨ��FCGI_STDOUT��Web����������CGI/1.1 stdout���ݣ��Լ�ͨ��FCGI_STDERR����CGI/1.1 stderr���ݡ�Ӧ��ͬʱ������Щ����
��һ����һ�����ڿ�ʼдFCGI_STDOUT��FCGI_STDERRǰ��Ӧ�ñ���ȴ���ȡFCGI_PARAMS��ɣ����ǲ���Ҫ�ڿ�ʼд��������ǰ��ɴ�FCGI_STDIN��ȡ�� 
�ڷ���������stdout��stderr���ݺ���Ӧ��Ӧ�÷���FCGI_END_REQUEST��¼��Ӧ������protocolStatus���ΪFCGI_REQUEST_COMPLETE������
��appStatus���ΪCGI����ͨ��exitϵͳ���÷��ص�״̬�롣 

��Ӧ��ִ�и��£�����ʵ��POST������Ӧ�ñȽ���FCGI_STDIN���յ����ֽ�����CONTENT_LENGTH���������������������ֹ���¡� 

6.3 ��֤����Authorizer�� 
��Ϊ��֤����FastCGIӦ�ý���������HTTP������ص���Ϣ��������һ���Ͽ�/δ���Ͽɵ��ж��������Ͽɵ��ж�����֤��Ҳ�ܰ���-ֵ��ͬHTTP
�����������������δ���Ͽɵ��ж�ʱ����֤����HTTP�ͻ��˷��ͽ�����Ӧ�� 

����CGI/1.1��������HTTP�������������Ϣ�ļ��õı�ʾ��ʽ����֤��ʹ��ͬ���ı�ʾ���� 


��֤��Ӧ����FCGI_PARAMS���Ͻ�������Web��������HTTP��Ϣ����ʽͬ��Ӧ��һ����Web���������ᷢ�ͱ�ͷCONTENT_LENGTH��PATH_INFO��
PATH_TRANSLATED��SCRIPT_NAME�� 
��֤��Ӧ����ͬ��Ӧ��һ���ķ�ʽ����stdout��stderr���ݡ�CGI/1.1��Ӧ״ָ̬���Խ���Ĵ������Ӧ�÷���״̬200��OK����Web��������
����ʡ� �����������ã�Web�������ɼ������������ķ��ʼ�飬������������֤�������� 
��֤��Ӧ�õ�200��Ӧ�ɰ�����Variable-Ϊ����ǰ׺�ı�ͷ����Щ��ͷ��Ӧ����Web������������-ֵ�ԡ����磬��Ӧ��ͷ 

Variable-AUTH_METHOD: database lookup
������ΪAUTH-METHOD��ֵ"database lookup"������������������-ֵ��ͬHTTP��������������Ұ����ǰ����ں�����CGI��FastCGI�����У���
Щ�����ڴ���HTTP����Ĺ�����ִ�С���Ӧ�ø���200��Ӧʱ���������������ֲ���Variable-Ϊǰ׺����Ӧ��ͷ�����Һ����κ���Ӧ���ݡ� 
���ڡ�200����OK���������֤����Ӧ״ֵ̬��Web�������ܾ����ʲ�����Ӧ״̬����ͷ�����ݷ���HTTP�ͻ��ˡ� 


6.4 ��������Filter�� 
��Ϊ��������FastCGIӦ�ý���������HTTP�������������Ϣ���Լ���������Դ洢��Web�������ϵ��ļ������������������������ġ��ѹ��ˡ�
�汾��ΪHTTP��Ӧ�� 

�������ڹ�����������Ӧ��������һ�������ļ���Ϊ�����������ǣ�������ʹ�������ļ��͹�������������Web�������ķ��ʿ��ƻ��ƽ��з�
�ʿ��ƣ�����Ӧ�����������ļ�����Ϊ�����������������ļ���ִ���Լ��ķ��ʿ��Ƽ�顣 

��������ȡ�Ĳ�������Ӧ�������ơ������������ṩ����������Ȼ���Ǳ�׼���루������ʽ��POST���ݣ�������������ļ����룺 


��ͬ��Ӧ����������Ӧ��ͨ��FCGI_PARAMS��������Web����������-ֵ�ԡ�������Ӧ�ý��������������ض��ı�����FCGI_DATA_LAST_MOD��FCGI_DATA_LENGTH�� 
��������������Ӧ��ͨ��FCGI_STDIN��������Web��������CGI/1.1 stdin���ݡ����յ���βָʾ��ǰ��Ӧ�ôӸ����������CONTENT_LENGTH�ֽڡ�
��ֻ��HTTP�ͻ���δ���ṩʱ��Ӧ���յ��Ĳ�����CONTENT_LENGTH�ֽڣ�������Ϊ�ͻ��˱����ˡ��� 
��һ����������Ӧ��ͨ��FCGI_DATA��������Web���������ļ����ݡ����ļ�������޸�ʱ�䣨��ʾ����UTC 1970��1��1������������������FCGI_DATA_LAST_MOD��
Ӧ�ÿ��ܲ��ĸñ������ӻ���������Ӧ��������ȡ�ļ����ݡ����յ���βָʾ��ǰ��Ӧ�ôӸ����������FCGI_DATA_LENGTH�ֽڡ� 
������Ӧ��ͨ��FCGI_STDOUT��Web����������CGI/1.1 stdout���ݣ��Լ�ͨ��FCGI_STDERR��CGI/1.1 stderr���ݡ�Ӧ��ͬʱ������Щ��������̵ء��ڿ�
ʼд��FCGI_STDOUT��FCGI_STDERR��ǰ��Ӧ�ñ���ȴ���ȡFCGI_STDIN��ɣ����ǲ���Ҫ�ڿ�ʼд������������ǰ��ɴ�FCGI_DATA�Ķ�ȡ�� 
�ڷ��������е�stdout��stderr����֮��Ӧ�÷���FCGI_END_REQUEST��¼��Ӧ���趨protocolStatus���ΪFCGI_REQUEST_COMPLETE���Լ�appStatus��
��Ϊ���Ƶ�CGI����ͨ��exitϵͳ���÷��ص�״̬���롣

������Ӧ������FCGI_STDIN���յ����ֽ���ͬCONTENT_LENGTH�Ƚϣ��Լ���FCGI_DATA�ϵ�ͬFCGI_DATA_LENGTH�Ƚϡ�������ֲ�ƥ���ҹ������Ǹ���ѯ��
��������ӦӦ���ṩ���ݶ�ʧ��ָʾ��������ֲ�ƥ���ҹ������Ǹ����£�������Ӧ����ֹ���¡� 

7. ���� 
FastCGIӦ����0״̬�˳���ָ������������ˣ����磬Ϊ��ִ��ԭʼ��ʽ�������ռ���FastCGIӦ���Է�0״̬�˳����ٶ�Ϊ�����ˡ���0���0״̬�˳���
Web��������������Ӧ�ù����������ӦӦ�ó����˱��淶�ķ�Χ�� 

Web��������ͨ����FastCGIӦ�÷���SIGTERM��Ҫ�����˳������Ӧ�ú���SIGTERM��Web�������ܲ���SIGKILL�� 

FastCGIӦ��ʹ��FCGI_STDERR����FCGI_END_REQUEST��¼��appStatus�������Ӧ�ü�������ںܶ������У������ͨ��FCGI_STDOUT��ֱ�ӱ�����û��� 

��Unix�ϣ�Ӧ����syslog����ͼ����󣬰���FastCGIЭ������FastCGI���������е��﷨���������ڴ���������ԣ�Ӧ�ÿ��ܼ������Է�0״̬�˳��� 

8. ���ͺͳ��� /*
* ���ڼ�����socket�ļ����
* /
#define FCGI_LISTENSOCK_FILENO 0
typedef struct {
unsigned char version;
unsigned char type;
unsigned char requestIdB1;
unsigned char requestIdB0;
unsigned char contentLengthB1;
unsigned char contentLengthB0;
unsigned char paddingLength;
unsigned char reserved;
} FCGI_Header;
/*
* FCGI_Header�е��ֽ�����Э���δ���汾������ٸ�����
* /
#define FCGI_HEADER_LEN 8
/*
* ������FCGI_Header��version�����ֵ
* /
#define FCGI_VERSION_1 1
/*
* ������FCGI_Header��type�����ֵ
* /
#define FCGI_BEGIN_REQUEST 1
#define FCGI_ABORT_REQUEST 2
#define FCGI_END_REQUEST 3
#define FCGI_PARAMS 4
#define FCGI_STDIN 5
#define FCGI_STDOUT 6
#define FCGI_STDERR 7
#define FCGI_DATA 8
#define FCGI_GET_VALUES 9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE 11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)
/*
* ������FCGI_Header��requestId�����ֵ
* /
#define FCGI_NULL_REQUEST_ID 0
typedef struct {
unsigned char roleB1;
unsigned char roleB0;
unsigned char flags;
unsigned char reserved[5];
} FCGI_BeginRequestBody;
typedef struct {
FCGI_Header header;
FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;
/*
* ������FCGI_BeginRequestBody��flags���������
* /
#define FCGI_KEEP_CONN 1
/*
* ������FCGI_BeginRequestBody��role�����ֵ
* /
#define FCGI_RESPONDER 1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER 3
typedef struct {
unsigned char appStatusB3;
unsigned char appStatusB2;
unsigned char appStatusB1;
unsigned char appStatusB0;
unsigned char protocolStatus;
unsigned char reserved[3];
} FCGI_EndRequestBody;
typedef struct {
FCGI_Header header;
FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;
/*
* ������FCGI_EndRequestBody��protocolStatus�����ֵ
* /
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN 1
#define FCGI_OVERLOADED 2
#define FCGI_UNKNOWN_ROLE 3
/*
* ������FCGI_GET_VALUES/FCGI_GET_VALUES_RESULT��¼�ı�����
* /
#define FCGI_MAX_CONNS "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"
typedef struct {
unsigned char type; 
unsigned char reserved[7];
} FCGI_UnknownTypeBody;
typedef struct {
FCGI_Header header;
FCGI_UnknownTypeBody body;
} FCGI_UnknownTypeRecord;
9. �ο� 
National Center for Supercomputer Applications, The Common Gateway Interface, version CGI/1.1. 

D.R.T. Robinson, The WWW Common Gateway Interface Version 1.1, Internet-Draft, 15 February 1996. 

A. ����¼���͵����� 
�����ͼ���г������м�¼���ͣ���ָ�����Ե���Щ���ԣ� 


WS->App�������͵ļ�¼ֻ����Web���������͵�Ӧ�á��������͵ļ�¼ֻ����Ӧ�÷��͵�Web�������� 
management�������͵ļ�¼���з��ض���ĳ��Web�������������Ϣ������ʹ��null����ID���������͵ļ�¼���������ض�����Ϣ�����Ҳ���ʹ��null����ID�� 
stream�������͵ļ�¼���һ���ɴ��п�contentData�ļ�¼�����������������͵ļ�¼����ɢ�ģ�����Я��һ������������ݵ�Ԫ��
WS->App management stream
FCGI_GET_VALUES x x
FCGI_GET_VALUES_RESULT x
FCGI_UNKNOWN_TYPE x
FCGI_BEGIN_REQUEST x
FCGI_ABORT_REQUEST x
FCGI_END_REQUEST
FCGI_PARAMS x x
FCGI_STDIN x x
FCGI_DATA x x
FCGI_STDOUT x 
FCGI_STDERR x 
B. ���͵�Э����Ϣ���� 
����ʾ���Ĳ������Լ���� 


����¼��contentData��FCGI_PARAMS��FCGI_STDIN��FCGI_STDOUT��FCGI_STDERR����������һ���ַ�������" ... "�������ַ�����̫�����޷���ʾ�ģ�����ֻ��ʾǰ׺�� 
���͵�Web����������Ϣ���������Web����������Ϣ�����Ű档 
��Ϣ��Ӧ�þ�����ʱ��˳����ʾ�� 

1. ��stdin�ϲ������ݵļ������Լ��ɹ�����Ӧ�� 

{FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
{FCGI_PARAMS, 1, "\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42 ... "}
{FCGI_PARAMS, 1, ""}
{FCGI_STDIN, 1, ""}
{FCGI_STDOUT, 1, "Content-type: text/html\r\n\r\n<html>\n<head> ... "}
{FCGI_STDOUT, 1, ""}
{FCGI_END_REQUEST, 1, {0, FCGI_REQUEST_COMPLETE}}

2. ������1���������stdin�����ݡ�Web������ѡ���ñ�֮ǰ�����FCGI_PARAMS��¼���Ͳ����� 

{FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
{FCGI_PARAMS, 1, "\013\002SERVER_PORT80\013\016SER"}
{FCGI_PARAMS, 1, "VER_ADDR199.170.183.42 ... "}
{FCGI_PARAMS, 1, ""}
{FCGI_STDIN, 1, "quantity=100&item=3047936"}
{FCGI_STDIN, 1, ""}
{FCGI_STDOUT, 1, "Content-type: text/html\r\n\r\n<html>\n<head> ... "}
{FCGI_STDOUT, 1, ""}
{FCGI_END_REQUEST, 1, {0, FCGI_REQUEST_COMPLETE}}

3. ������1�������Ӧ�÷����˴���Ӧ�ð�һ����Ϣ��¼��stderr����ͻ��˷���һ��ҳ�棬������Web���������ط�0�˳�״̬��Ӧ��ѡ���ø���FCGI_STDOUT��¼����ҳ�棺 

{FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
{FCGI_PARAMS, 1, "\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42 ... "}
{FCGI_PARAMS, 1, ""}
{FCGI_STDIN, 1, ""}
{FCGI_STDOUT, 1, "Content-type: text/html\r\n\r\n<ht"}
{FCGI_STDERR, 1, "config error: missing SI_UID\n"}
{FCGI_STDOUT, 1, "ml>\n<head> ... "}
{FCGI_STDOUT, 1, ""}
{FCGI_STDERR, 1, ""}
{FCGI_END_REQUEST, 1, {938, FCGI_REQUEST_COMPLETE}}

4. �ڵ�����·�϶�·���õ�������1ʵ������һ������ȵڶ����ѣ�����Ӧ�õߵ����������Щ���� 

{FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, FCGI_KEEP_CONN}}
{FCGI_PARAMS, 1, "\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42 ... "}
{FCGI_PARAMS, 1, ""}
{FCGI_BEGIN_REQUEST, 2, {FCGI_RESPONDER, FCGI_KEEP_CONN}}
{FCGI_PARAMS, 2, "\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42 ... "}
{FCGI_STDIN, 1, ""}
{FCGI_STDOUT, 1, "Content-type: text/html\r\n\r\n"}
{FCGI_PARAMS, 2, ""}
{FCGI_STDIN, 2, ""}
{FCGI_STDOUT, 2, "Content-type: text/html\r\n\r\n<html>\n<head> ... "}
{FCGI_STDOUT, 2, ""}
{FCGI_END_REQUEST, 2, {0, FCGI_REQUEST_COMPLETE}}
{FCGI_STDOUT, 1, "<html>\n<head> ... "}
{FCGI_STDOUT, 1, ""}
{FCGI_END_REQUEST, 1, {0, FCGI_REQUEST_COMPLETE}}




FastcgiЭ�鶨�������˵�� 
http://wangnow.com/article/28-fastcgi-protocol-specification

 

���Ƚ�����Ӧ�����ݣ��Ƚϼ򵥣��������ǶԷ��ص����ݱȽ����С���
1 ��Ӧ��ʽ
�磨ʮ�����Ʒ�ʽ��ʾ��

���� 0  1  2  3  4  5  6  7 ...
��ֵ 01 06 00 01 01 1D 03 00...

����0��ֵ01��Ϊversion���̶�ȡ1����
����1��ֵ06��Ϊtype������FCGI_STDOUT����ʾӦ�õ����
����2 3��00 01������2�ֽڵ�����id��Ĭ��ȡ1���ɣ�׼ȷ˵Ӧ���Ǻ�����Ӧ��ʱ���͵�idһ�£���������������Ӧ��id����1��
����4 5��01 1D������2�ֽڵ�������ȣ����Ϊ65535�����統ǰ���ݳ���Ϊ(0x01 << 8) + 0x1D = 285
����6��03���������padding�ֽ��������Ϊ��8�ֽڵ��������������統ǰ��䣨��0��䣩����Ϊ8 - 285 % 8 = 3������ȡ������ȣ�285�������ݺ�Ҫ�������ֽ�������Ȼ���Ϊ8�����������
����7��00��Ϊ�����ֽ�
8�ֽڣ�����7��֮��Ϊ�������ݣ�contentData����������ݣ�paddingData��

���Ϊ֪ͨweb�����������������¼��������������

���� 0  1  2  3  4  5  6  7 ...
��ֵ 01 03 00 01 00 08 00 00...

��������1��03��type����FCGI_END_REQUEST�������������8�ֽ�֮��ΪcontentData��EndRequestBody����paddingData
EndRequestBody������Ҳ�Ƚϸ��ԣ��ǵ��������

typedef struct {
     unsigned char appStatusB3;
     unsigned char appStatusB2;
     unsigned char appStatusB1;
     unsigned char appStatusB0;
     unsigned char protocolStatus;
     unsigned char reserved[3];
} FCGI_EndRequestBody;


appStatusռ���ĸ��ֽڣ�����Ϊcgiͨ������ϵͳ�˳����ص�״̬�루The application sets the protocolStatus component to FCGI_REQUEST_COMPLETE and the appStatus component to the status code that the CGI program would have returned via the exit system call.��Linux�����ĳ����˳�Ĭ���Ƿ���0��Ӧ���ǰɣ��Ҽ����ǡ�����

protocolStatus��ֵ������

#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN 1
#define FCGI_OVERLOADED 2
#define FCGI_UNKNOWN_ROLE 3


������FCGI_END_REQUEST��contentDataΪ

���� 0  1  2  3  4  5  6  7
��ֵ 00 00 00 00 00 00 00 00

0-3����ΪappStatus
4����protocolStatusΪ0��FCGI_REQUEST_COMPLETE��
5-7����Ϊ������3�ֽ�reserved[3]

2 �����ʽ

���� 0  1  2  3  4  5  6  7 ...
��ֵ 01 01 00 01 00 08 00 00...

����0��ֵ01��Ϊversion
����1��ֵ01��Ϊtype������FCGI_BEGIN_REQUES����ʾ��ʼ�������� 
����2 3��00 01������2�ֽڵ�����id��Ĭ��ȡ1����
����ʼ�ļ�¼��΢���⣬���͵����ݣ�contentData�����¸�ʽ

typedef struct {
     unsigned char roleB1;
     unsigned char roleB0;
     unsigned char flags;
     unsigned char reserved[5];
} FCGI_BeginRequestBody;
 

#role�Ŀ���ȡ��������ֵ
#define FCGI_RESPONDER 1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER 3

����ȡ1��FCGI_RESPONDER��Ϊɶ��˵�Ǻ;����CGI/1.1����һ����http��Щ������
flagsȡ0��ʾ����������Ϻ󼴹ر����ӡ�

���� 0  1  2  3  4  5  6  7
��ֵ 00 01 00 00 00 00 00 00

0��1���д���roleΪ1��FCGI_RESPONDER��
2����Ϊflags 0
3-7����Ϊreserved[5]

��˵��Э����FCGI_PARAMS�е�Name-Value Pairs��Ŀ�����ṩӦ�ò�һЩ��Ҫ�ı���������http�е�header��headerName-headerValue����Ȼ����Ϊ���������ϸ�����http://www.fastcgi.com/devkit/doc/fcgi-spec.html#S3.4
����һ�ֶ����ʽ���£�

typedef struct {
     unsigned char nameLengthB0; /* nameLengthB0 >> 7 == 0 * /
     unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 * /
     unsigned char valueLengthB2;
     unsigned char valueLengthB1;
     unsigned char valueLengthB0;
     unsigned char nameData[nameLength];
     unsigned char valueData[valueLength
     ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
} FCGI_NameValuePair14;

���ʵ��˵����

���� 0  1  2  3  4  5  6  7 ...
��ֵ 00 04 00 01 04 EB 05 00...

����1��04������FCGI_PARAMS
����7֮���Ϊ��Ӧ�����֣�Name�����ȣ�nameLength����ֵ��Value�����ȣ�valueLength�������֣�nameData����ֵ��valueData��
���й涨���ֻ���ֵ�ĳ����������127�ֽڣ���Ҫ��4�ֽڴ洢������

���� 0  1  2  3  4  5  6  7 ............
��ֵ 0F 80 00 00 91 S  C  R IPT_FILENAME/data/www/......

����0��0F��ʮ���Ƶ�15��SCRIPT_FILENAME�ĳ��ȣ���������127����ռһ���ֽ�
����1��80��ʮ���Ƶ�128������127��˵��Ҫռ��4�ֽڣ�80 00 00 91��������Ϊ

((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0
������ڶ����أ������λ�ơ���Ȳ������Ų���Ϥ�Ļ�������ϸ�Ľ��ܼ�֮ǰ������

3 ����˵��
����ֵ����ϸ����μ�http://www.fastcgi.com/devkit/doc/fcgi-spec.html#S8
������һЩ��Ҫ˵��
��¼��Records������˳���ͻ��߽��ܶ����¼���ĸ�ʽ���嶨������

typedef struct {
     unsigned char version;
     unsigned char type;
     unsigned char requestIdB1;
     unsigned char requestIdB0;
     unsigned char contentLengthB1;
     unsigned char contentLengthB0;
     unsigned char paddingLength;
     unsigned char reserved;
     unsigned char contentData[contentLength];
     unsigned char paddingData[paddingLength];
} FCGI_Record;

#ǰ���ֽڶ���ΪHeader��������ô��⣬ͷ��Ϣ+��Ӧ���ݣ�����htppЭ���е�header+body�������ˣ�
#Э��˵���а��ⲿ�ֶ���ΪFCGI_Header�����Ϻ�ɫ���岿�֣�������

typedef struct {
     unsigned char version;
     unsigned char type;
     unsigned char requestIdB1;
     unsigned char requestIdB0;
     unsigned char contentLengthB1;
     unsigned char contentLengthB0;
     unsigned char paddingLength;
     unsigned char reserved;
} FCGI_Header;
 

#version����Ϊ1
#define FCGI_VERSION_1 1


#type����ֵ���壬��Ҫ��עFCGI_BEGIN_REQUEST������ʼ�� FCGI_END_REQUEST����������� FCGI_PARAMS��fastcgi��������һЩ��������������HTTP_USER_AGENT�� FCGI_STDOUT��fastcgi��׼�����������󷵻ص����ݣ�

#define FCGI_BEGIN_REQUEST 1
#define FCGI_ABORT_REQUEST 2
#define FCGI_END_REQUEST 3
#define FCGI_PARAMS 4
#define FCGI_STDIN 5
#define FCGI_STDOUT 6
#define FCGI_STDERR 7
#define FCGI_DATA 8
#define FCGI_GET_VALUES 9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE 11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)











fastcgi_param ��� .
���ࣺ nginx 2012-12-28 11:18 14537���Ķ� ����(0) �ղ� �ٱ� 
[php] view plaincopyprint?
01.fastcgi_param  SCRIPT_FILENAME    $document_root$fastcgi_script_name;#�ű��ļ������·��   �����ȡ�ĺ��PHP���������ļ�Ŀ¼��ͨ���ļ�Ŀ¼+uri��ȡָ���ļ�
02.fastcgi_param  QUERY_STRING       $query_string; #����Ĳ���;��?app=123  
03.fastcgi_param  REQUEST_METHOD     $request_method; #����Ķ���(GET,POST)  
04.fastcgi_param  CONTENT_TYPE       $content_type; #����ͷ�е�Content-Type�ֶ�  
05.fastcgi_param  CONTENT_LENGTH     $content_length; #����ͷ�е�Content-length�ֶΡ�  
06.  
07.fastcgi_param  SCRIPT_NAME        $fastcgi_script_name; #�ű�����   
08.fastcgi_param  REQUEST_URI        $request_uri; #����ĵ�ַ��������  
09.fastcgi_param  DOCUMENT_URI       $document_uri; #��$uri��ͬ��   
10.fastcgi_param  DOCUMENT_ROOT      $document_root; #��վ�ĸ�Ŀ¼����server������rootָ����ָ����ֵ   
11.fastcgi_param  SERVER_PROTOCOL    $server_protocol; #����ʹ�õ�Э�飬ͨ����HTTP/1.0��HTTP/1.1��    
12.  
13.fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;#cgi �汾  
14.fastcgi_param  SERVER_SOFTWARE    nginx/$nginx_version;#nginx �汾�ţ����޸ġ�����  
15.  
16.fastcgi_param  REMOTE_ADDR        $remote_addr; #�ͻ���IP  
17.fastcgi_param  REMOTE_PORT        $remote_port; #�ͻ��˶˿�  
18.fastcgi_param  SERVER_ADDR        $server_addr; #������IP��ַ  
19.fastcgi_param  SERVER_PORT        $server_port; #�������˿�  
20.fastcgi_param  SERVER_NAME        $server_name; #����������������server������ָ����server_name  
21.  
22.#fastcgi_param  PATH_INFO           $path_info;#���Զ������  
23.  
24.# PHP only, required if PHP was built with --enable-force-cgi-redirect  
25.#fastcgi_param  REDIRECT_STATUS    200;  











FastCGIЭ�鱨�ĵķ��� 
http://xiaoxia.org/?p=1891

��֪��ʲôʱ�򣬾Ϳ�ʼ������HomeServer֧��PHP����ͷ�����Ƿ�������FastCGIЭ�顣FastCGI����WebServer��WebApplication֮���ͨѶ������Apache��PHP����

FastCGIЭ�����ݰ���8�ֽڶ���ģ��ɰ�ͷ(Header)�Ͱ���(Body)��ɡ�����Ҫ����һ��index.php��ҳ�棬WebServer������WebApp����һ��Request���ݰ�����ͷ�и�����ID���ڲ��й���ʱ������ͬ������

��ͷ

[�汾:1][����:1][����ID:2][���ݳ���:2][����ֽ���:1][����:1]

����

[��ɫ:2][����:1][����:5]

���ţ��ٷ���һ��Params���ݰ������ڴ���ִ��ҳ������Ҫ�Ĳ����ͻ���������

��ͷ

[�汾:1][����:1][����ID:2][���ݳ���:2][����ֽ���:1][����:1]

����

[���Ƴ���:1��4][ֵ����:1��4][����:�䳤][ֵ:�䳤] ...

���У����ƺ�ֵ�ĳ���ռ�õ��ֽ����ǿɱ䣬ȡ���ڵ�һ���ֽڣ���λ�������λ�Ƿ�Ϊ1��Ϊ1�򳤶���4���ֽڣ�����Ϊ1���ֽڡ���������Ȳ�����128�ֽڣ�����һ���ֽ������泤���㹻�ˡ�

�������ͺ�Ҫ����һ��û�а��壬ֻ�а�ͷ�Ŀյ�Params���ݰ���������ʾ�������ͽ�����

�������ҳ��ʱPOST��ʽ�����ᷢ�ͱ����ݡ����Ҫ�õ�Stdin���ݰ��ˡ�

��ͷ

[�汾:1][����:1][����ID:2][���ݳ���:2][����ֽ���:1][����:1]

����

[��������:�����ڰ�ͷ�����ã�8�ֽڶ���]

��ʱ��POST�����ݴ��ڻ����64KB���Ͳ���ʹ��һ��Stdin���ݰ���������ˣ���Ҫʹ�ö��Stdin���ݰ�������������ݵĴ��䡣��Params���ݰ�һ������βҪ����һ��û�а��壬ֻ�а�ͷ�Ŀյ�Stdin���ݰ���������ʾ�������ͽ�����

���ˣ�WebServerҪ�ṩ��WebApplication�������Ѿ�������ϡ����žͽ�������WebApplication�������ˡ�

���ݽ��հ�Stdout��Stdin�ǲ��ģ����ﲻ���������������յ���������HTTPͷ����ҳ������������ɣ�WebServerҪ������һ���Ĵ������ܷ��͵��������ͬStdin���ݰ�һ����WebServer����յ�һ������WebApplication��Stdout�Ŀ����ݰ�����ʾ���յ�Stdout�����Ѿ���ϡ�

���WebApplication�ᷢ��һ������״̬��EndRequest���ݰ������ˣ�һ��ҳ����������ϡ�

�������һЩ��ؽṹ�ο���

ͨ�ð�ͷ��

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
}FCGI_Header;
 
typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;
 
typedef struct {
    FCGI_Header header;
    FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;
 
typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FCGI_EndRequestBody;
ÿ������ҳ��ʱ�����ݸ�PHP����Ĳ�����

SCRIPT_FILENAME,

QUERY_STRING,

REQUEST_METHOD,

CONTENT_TYPE,

CONTENT_LENGTH,

SCRIPT_NAME,

REQUEST_URI,

DOCUMENT_URI,

DOCUMENT_ROOT,

SERVER_PROTOCOL,

GATEWAY_INTERFACE,

SERVER_SOFTWARE,

REMOTE_ADDR,

REMOTE_PORT,

SERVER_ADDR,

SERVER_PORT,

SERVER_NAME,

REDIRECT_STATUS,

HTTP_ACCEPT,

HTTP_ACCEPT_LANGUAGE,

HTTP_ACCEPT_ENCODING,

HTTP_USER_AGENT,

HTTP_HOST,

HTTP_CONNECTION,

HTTP_CONTENT_TYPE,

HTTP_CONTENT_LENGTH,

HTTP_CACHE_CONTROL,

HTTP_COOKIE,

HTTP_FCGI_PARAMS_MAX











�����ܻ�˵����HTTP�ӿڵĿ���Ч�ʲ����ǵģ�����HTTPЭ��Ŀ���Ч�ʺܸߣ��������ʺϸ������绷������������HTTPЭ����Ҫ���ʹ�����ͷ����
���Ե������ܲ��Ǻ����롣��ô��û��һ�ֱ�HTTPЭ�����ܺò��ұȻ���TCP�ӿڵĿ���Ч�ʸߵĽ�������أ����ǿ϶��ģ����Ǳ��Ľ�����Ҫ���ܵĻ���FastCGI�Ľӿڿ�����


��ôCGI������������������أ�PHP������ÿ�ζ������php.ini�ļ�����ʼ��ִ�л�������׼��CGI��ÿ�����󶼻�ִ����Щ���裬���Դ���ÿ��ʱ���ʱ���Ƚϳ���
��ôFastCGI����ô�����أ����ȣ�FastCGI������һ��master�����������ļ�����ʼ��ִ�л�����Ȼ�����������worker�����������ʱ��master�ᴫ�ݸ�һ��worker��
Ȼ���������Խ�����һ�����������ͱ������ظ����Ͷ���Ч����Ȼ�Ǹߡ����ҵ�worker������ʱ��master���Ը�������Ԥ����������worker���ţ���Ȼ����worker̫��ʱ��
Ҳ��ͣ��һЩ����������������ܣ�Ҳ��Լ����Դ�������FastCGI�ĶԽ��̵Ĺ���











Fastcgi�ٷ��ĵ���http://www.fastcgi.com/devkit/doc/fcgi-spec.html

fastcgi�������:http://fuzhong1983.blog.163.com/blog/static/1684705201051002951763/
http://www.cppblog.com/woaidongmao/archive/2011/06/21/149097.html

http://my.oschina.net/goal/blog/196599  ������ͼ��˵��������


nginx��php��װ����:http://www.cnblogs.com/jsckdao/archive/2011/05/05/2038265.html  
http://www.nginx.cn/231.html      nginx php-fpm��װ����
http://www.cnblogs.com/jsckdao/archive/2011/05/05/2038265.html nginx+php������ 

php-cgi���������˺����ԭ��:http://bbs.csdn.net/topics/380138192  �޸�Դ��php-5.3.8\sapi\cgi\cgi_main.c�� int max_requests = 500;
�Ĵ�㣬����ֱ���޸Ļ�������PHP_FCGI_MAX_REQUESTS

/*


export PATH=/usr/local/php/bin:$PATH
export PATH=/usr/local/php/sbin:$PATH

ע���޸��ļ���Ҫ��������Ч��Ҫ����# source /etc/profile��Ȼֻ�����´��ؽ����û�ʱ��Ч��
php -m �������Щģ�����
mem-phpҪ��ִ�� phpize Ȼ����config make make install
PHP Warning:  PHP Startup: memcache: Unable to initialize module  �������һ����PHP��mem�İ汾��һ�����𣬿���ͨ����ʱ��õ�

PHP-FPM����:http://www.4wei.cn/archives/1002061
�Ҳ���php-fpm.conf�����ļ�:http://blog.sina.com.cn/s/blog_ac08ce040101j2vi.html
����mem-php�Ƿ�װ�ɹ�:http://blog.csdn.net/poechant/article/details/6802312

*/


*/
