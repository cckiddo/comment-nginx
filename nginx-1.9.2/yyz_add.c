/*

�ֲ�ʽ��һ��ҵ��ֲ�����ҵ�񣬲����ڲ�ͬ�ķ�������
��Ⱥ��ͬһ��ҵ�񣬲����ڶ����������

�û��������ͼƬʱ������CDN�����ṩ����DNS������������ģ���ȻҲ���������������ӵĲ��ԣ����縺�����������״̬�ȣ�����ڵ��ַ���ظ��û���
�û����󵽴�ָ���ķ������ڵ��ϣ��ýڵ����ṩ������Squid/Vanish�Ĵ������������ǵ�һ�������·��������Դվ��ȡͼƬ��Դ���ؿͻ����������
��������д��ڣ���ֱ�Ӵӻ����л�ȡ�����ظ��ͻ�����������������/��Ӧ���̡�





HTTP�����ʽ

���������Web��������������ʱ�����������������һ�����ݿ飬Ҳ����������Ϣ��HTTP������Ϣ��3������ɣ�

l ������:���󷽷� URIЭ�� �汾

l ����ͷ(Request Header)

l ��������

������һ��HTTP��������ӣ�

GET /sample.jsp HTTP/1.1

 

Accept:image/gif.image/jpeg,**

Accept-Language:zh-cn

Connection:Keep-Alive

Host:localhost

User-Agent:Mozila/4.0(compatible:MSIE5.01:Windows NT5.0)

Accept-Encoding:gzip,deflate.

 

��3����������

����ͷ����������֮����һ�����У�����зǳ���Ҫ������ʾ����ͷ�Ѿ������������������������ġ����������п��԰����ͻ��ύ�Ĳ�ѯ�ַ�����Ϣ��

username=jinqiao&password=1234

�����ϵ����ӵ�HTTP�����У����������ֻ��һ�����ݡ���Ȼ����ʵ��Ӧ���У�HTTP�������Ŀ��԰�����������ݡ�

HTTP���󷽷�������ֻ����GET������POST����

l GET����

GET������Ĭ�ϵ�HTTP���󷽷��������ճ���GET�������ύ�����ݣ�Ȼ����GET�����ύ�ı�����ֻ�����˼򵥵ı��룬ͬʱ������ΪURL��һ������Web���������ͣ���ˣ����ʹ��GET�������ύ�����ݾʹ����Ű�ȫ�����ϡ�����

Http://127.0.0.1/login.jsp?Name=zhangshi&Age=30&Submit=?%E+??

�������URL�����У������׾Ϳ��Ա��ϳ����ύ�����ݡ�����֮������ݣ���������GET�����ύ����������ΪURL�����һ���������ύ������������̫��

 

l POST����

POST������GET������һ���������������Ҫ����Web�������ύ�����ݣ������Ǵ����������ݡ�POST�����˷���GET������һЩȱ�㡣ͨ��POST�����ύ������ʱ�����ݲ�����ΪURL�����һ���ֶ�����Ϊ��׼���ݴ��͸�Web����������Ϳ˷���GET�����е���Ϣ�޷����ܺ�������̫С��ȱ�㡣��ˣ����ڰ�ȫ�Ŀ����Լ����û���˽�����أ�ͨ�����ύʱ����POST������

�����ӱ�̵ĽǶ�����������û�ͨ��GET�����ύ���ݣ������ݴ����QUERY��STRING���������У���POST�����ύ����������Դӱ�׼�������л�ȡ��

 

http��Ӧ��ʽ

HTTPӦ����HTTP�������ƣ�HTTP��ӦҲ��3�����ֹ��ɣ��ֱ��ǣ�

l ��״̬��

l ����Ӧͷ(Response Header)

l ����Ӧ����

�ڽ��պͽ���������Ϣ�󣬷������᷵��һ��HTTP��Ӧ��Ϣ��

״̬����Э��汾��������ʽ��״̬���롢����Ӧ��״̬��������Ԫ��֮���Կո�ָ���

��ʽ: HTTP-Version Status-Code Reason-Phrase CRLF

����: HTTP/1.1 200 OK \r\n

 

״̬���룺

״̬������3λ������ɣ���ʾ�����Ƿ��������㡣

״̬������

״̬���������˹���״̬����ļ�̵�����������

״̬����ĵ�һ�����ֶ�������Ӧ����𣬺�����λû�о���ķ��ࡣ

��һ�����������ֿ��ܵ�ȡֵ��

- 1xx: ָʾ��Ϣ����ʾ�����ѽ��գ���������

- 2xx: �ɹ�����ʾ�����Ѿ����ɹ����ա���⡢���ܡ�

- 3xx: �ض���Ҫ������������и���һ���Ĳ�����

- 4xx: �ͻ��˴����������﷨����������޷�ʵ�֡�

- 5xx: �������˴��󡪷�����δ��ʵ�ֺϷ�������

״̬����״̬���� ˵��

200 OK �ͻ�������ɹ�

400 Bad Request ���ڿͻ����������﷨���󣬲��ܱ�����������⡣

401 Unauthonzed ����δ����Ȩ�����״̬��������WWW-Authenticate��ͷ��һ��ʹ��

403 Forbidden �������յ����󣬵��Ǿܾ��ṩ���񡣷�����ͨ��������Ӧ�����и������ṩ�����ԭ��

404 Not Found �������Դ�����ڣ����磬�����˴����URL��

500 Internal Server Error ��������������Ԥ�ڵĴ��󣬵����޷���ɿͻ��˵�����

503 Service Unavailable ��������ǰ���ܹ�����ͻ��˵�������һ��ʱ��֮�󣬷��������ܻ�ָ�������

 

��Ӧͷ

��Ӧͷ���ܰ�����

Location��

Location��Ӧ��ͷ�������ض�������ߵ�һ���µ�λ�á����磺�ͻ����������ҳ���Ѳ�����ԭ�ȵ�λ�ã�Ϊ���ÿͻ����ض������ҳ���µ�λ�ã��������˿��Է���Location��Ӧ��ͷ��ʹ���ض�����䣬�ÿͻ���ȥ�����µ���������Ӧ�ķ������ϵ���Դ����������JSP��ʹ���ض�������ʱ�򣬷���������ͻ��˷��ص���Ӧ��ͷ�У��ͻ���Location��Ӧ��ͷ��

Server��

Server��Ӧ��ͷ������˷�����������������������Ϣ������User-Agent����ͷ�������Ӧ�ģ�ǰ�߷��ͷ��������������Ϣ�����߷��Ϳͻ������(�����)�Ͳ���ϵͳ����Ϣ��������Server��Ӧ��ͷ���һ�����ӣ�Server: Apache-Coyote/1.1

WWW-Authenticate��

WWW-Authenticate��Ӧ��ͷ����뱻������401(δ��Ȩ��)��Ӧ��Ϣ�У������ͷ���ǰ�潲����Authorization����ͷ������صģ����ͻ����յ�401��Ӧ��Ϣ����Ҫ�����Ƿ�������������������֤�����Ҫ����������������֤���Ϳ��Է���һ�������� Authorization��ͷ�������������WWW-Authenticate��Ӧ��ͷ���һ�����ӣ�WWW-Authenticate: Basic realm="Basic Auth Test!"

�������Ӧ��ͷ�򣬿���֪���������˶��������������Դ���õ��ǻ�����֤���ơ�

Content-Encoding��

Content-Encodingʵ�屨ͷ��ʹ����ý�����͵����η�������ֵָʾ���Ѿ���Ӧ�õ�ʵ�����ĵĸ������ݱ��룬���Ҫ���Content- Type��ͷ���������õ�ý�����ͣ����������Ӧ�Ľ�����ơ�Content-Encoding��Ҫ�����¼�ĵ���ѹ������������������һ�����ӣ� Content-Encoding: gzip�����һ��ʵ�����Ĳ����˱��뷽ʽ�洢����ʹ��֮ǰ�ͱ�����н��롣

Content-Language��

Content-Languageʵ�屨ͷ����������Դ���õ���Ȼ���ԡ�Content-Language�����û������������ѡ������ʶ�������ʵ�塣������ʵ�����ݽ��������ṩ��������Ķ��ߣ���ô���԰������µķ�ʽ�������ʵ�屨ͷ��Content-Language: da��

���û��ָ��Content-Language��ͷ����ôʵ�����ݽ��ṩ���������Ե��Ķ��ߡ�

Content-Length��

Content-Lengthʵ�屨ͷ������ָ�����ĵĳ��ȣ����ֽڷ�ʽ�洢��ʮ������������ʾ��Ҳ����һ�������ַ�ռһ���ֽڣ������Ӧ��ASCII��洢���䡣

Ҫע����ǣ�������Ƚ����Ǳ�ʾʵ�����ĵĳ��ȣ�û�а���ʵ�屨ͷ�ĳ��ȡ�

Content-Type

Content-Typeʵ�屨ͷ������ָ�����͸������ߵ�ʵ�����ĵ�ý�����͡����磺

Content-Type: text/html;charset=ISO-8859-1

Content-Type: text/html;charset=GB2312

Last-Modified

Last-Modifiedʵ�屨ͷ������ָʾ��Դ�����޸����ڼ�ʱ�䡣

Expires

Expiresʵ�屨ͷ�������Ӧ���ڵ����ں�ʱ�䡣ͨ���������������������Ỻ��һЩҳ�档���û��ٴη�����Щҳ��ʱ��ֱ�Ӵӻ����м��ز���ʾ���û���������������Ӧ��ʱ�䣬���ٷ������ĸ��ء�Ϊ���ô�����������������һ��ʱ������ҳ�棬���ǿ���ʹ��Expiresʵ�屨ͷ��ָ��ҳ����ڵ�ʱ�䡣���û���һ�η���ҳ��ʱ�����Expires��ͷ����������ں�ʱ���Date��ͨ��ͷ����������ں�ʱ��Ҫ��(����ͬ)����ô�����������������Ͳ�����ʹ�û����ҳ����Ǵӷ�������������µ�ҳ�档����Ҫע�⣬��ʹҳ������ˣ�Ҳ������ζ�ŷ������ϵ�ԭʼ��Դ�ڴ�ʱ��֮ǰ��֮�����˸ı䡣

Expiresʵ�屨ͷ��ʹ�õ����ں�ʱ�������RFC 1123�е����ڸ�ʽ�����磺

Expires: Thu, 15 Sep 2005 16:00:00 GMT

HTTP1.1�Ŀͻ��˺ͻ�����뽫�����Ƿ������ڸ�ʽ(Ҳ����0)�����ѹ��ڡ����磬Ϊ�����������Ҫ����ҳ�棬����Ҳ��������Expiresʵ�屨ͷ����������ֵΪ0������(JSP)��response.setDateHeader("Expires",0);

������һ��HTTP��Ӧ�����ӣ�

HTTP/1.1 200 OK

Server:Apache Tomcat/5.0.12
Date:Mon,6Oct2003 13:23:42 GMT




�����ĸ�ע�� 
    HTTP������������֣�������(Request Line)��ͷ��(Headers)��������(Body)�����У������������󷽷�(method)��������ַRequest-URI��Э�� (Protocol)���ɣ�������ͷ����������ԣ�����������Ա���Ϊ�Ǹ���������֮����ı���������ļ��� 
    �������������ʾ��һ��HTTP�����Header���ݣ���Щ����������������HTTPЭ���IE��������ݵ�Apache�������ϵġ� 
GET /qingdao.html HTTP/1.1 
Accept:text/html, * / * 
Accept-Language:zh-cn 
Accept-Encoding:gzip,deflate 
User-Agent:Mozilla/4.0(compatible;MSIE 5.01;Windows NT 5.0;DigExt) 
Host: www.6book.net 
Referer: http://www.6book.net/beijing.html 
Connection:Keep-Alive 

    ��γ���ʹ����6��Header������һЩHeaderû�г��֡����ǲο�������Ӿ������HTTP�����ʽ�� 
1.HTTP�����У������и�ʽΪMethod Request-URI Protocol����������������"GET / HTTP/1.1"�������С� 
2.Accept:ָ������������ͻ����ԽӰ���MIME�ļ���ʽ�����Ը������жϲ������ʵ����ļ���ʽ�� 
3.Accept-Charset��ָ����������Խ��ܵ��ַ����롣Ӣ���������Ĭ��ֵ��ISO-8859-1. 
4.Accept-Language��ָ����������Խ��ܵ��������࣬��en��en-us��ָӢ� 
5.Accept-Encoding��ָ����������Խ��ܵı��뷽ʽ�����뷽ʽ��ͬ���ļ���ʽ������Ϊ��ѹ���ļ��������ļ������ٶȡ�������ڽ��յ�Web��Ӧ֮���Ƚ��룬Ȼ���ټ���ļ���ʽ�� 
6.Authorization����ʹ���������ʱ������ʶ������� 
7.Cache-Control�����ù������󱻴���������洢�����ѡ�һ���ò����� 
8.Connection���������߷������Ƿ����ά�̶ֹ���HTTP���ӡ�HTTP/1.1ʹ��Keep-AliveΪĬ��ֵ�����������������Ҫ����ļ�ʱ(����һ��HTML�ļ�����ص�ͼ���ļ�)������Ҫÿ�ζ��������ӡ� 
9.Content-Type����������request���������͡�������HttpServletRequest�� getContentType()����ȡ�á� 
10.Cookie���������������������������Cookie��Cookie����������мĴ��С�������壬�����Լ��غͷ�������ص��û���Ϣ��Ҳ��������ʵ�ֻỰ���ܡ� 
11.Expect����ʱ�ͻ�Ԥ�ڵ���Ӧ״̬�� 
12.From�������ͻ���HTTP�������˵�email��ַ�� 
13.Host����Ӧ��ַURL�е�Web���ƺͶ˿ںš� 
14.If-Match����PUT����ʹ�á� 
15.If-Modified-Since���ͻ�ʹ��������Ա�����ֻ��Ҫ��ָ������֮����Ĺ�����ҳ����Ϊ���������ʹ����洢���ļ������شӷ���������������ʡ��Web��Դ������Servlet�Ƕ�̬���ɵ���ҳ��һ�㲻��Ҫʹ��������ԡ� 
16.If-None-Match����If-Match�෴�Ĳ�������PUT����ʹ�á� 
17.If-Unmodified-Since����If-Match-Since�෴�� 
18.Pragma���������ֻ��һ��ֵ����Pragma��no-cache,�������servlet�䵱�������������ʹ�����Ѿ��洢����ҳ��ҲҪ�����󴫵ݸ�Ŀ�ķ������� 
19.Proxy-Authorization�����������ʹ��������ԣ�һ���ò����� 
20.Range������ͻ��в�����ҳ��������Կ�������ʣ�ಿ�֡� 
21.Referer�����������������ҳURL�� 
�������ҳ/beijing.html�е��һ�����ӵ���ҳ/qingdao.html,������������͵�GET /beijing.html�е������У�Referer��http://www.6book.net/qingdao.html ��������Կ�����������Web�����Ǵ�ʲô��վ���ġ� 
22.Upgrage���ͻ�ͨ����������趨����ʹ����HTTP/1.1��ͬ��Э�顣 
23.User-Agent���ǿͻ���������ơ� 
24.Via��������¼Web���󾭹��Ĵ����������Webͨ���� 
25.Warning�������ɿͻ��������ݻ�洢(cache)���� 





HTTP��Ӧ���롢httpͷ����ϸ������һ��| �������ࣺhttp ��Ӧ����    һ��HTTP��Ӧ����Ӧ������λʮ����������ɣ����ǳ�������HTTP���������͵���Ӧ�ĵ�һ�С� 
     ��Ӧ����������ͣ������ǵĵ�һλ���ֱ�ʾ�� 
1xx����Ϣ�������յ����������� 
2xx���ɹ�����Ϊ���ɹ��ؽ��ܡ����Ͳ��� 
3xx���ض���Ϊ��������󣬱����һ��ִ�еĶ��� 
4xx���ͻ��˴�����������﷨������������޷�ʵ�� 
5xx�����������󣬷���������ʵ��һ��������Ч������ 
�±���ʾÿ����Ӧ�뼰�京�壺 
100 ����101 ���齻��Э200 OK201 ������202 ������203 ����Ȩ��Ϣ204 ������205 ��������206 ��������300 ��ѡ��301 ���õش���302 �ҵ�303 �μ�����304 δ�Ķ�305 ʹ�ô���307 ��ʱ�ض���400 ��������401 δ��Ȩ402 Ҫ�󸶷�403 ��ֹ404 δ�ҵ�405 ������ķ���406 ��������407 Ҫ�������Ȩ408 ����ʱ409 ��ͻ410 ���ڵ�411 Ҫ��ĳ���412 ǰ�᲻����413 ����ʵ��̫��414 ����URI̫��415 ��֧�ֵ�ý������416 �޷����������Χ417 ʧ�ܵ�Ԥ��500 �ڲ�����������501 δ��ʹ��502 ���ش���503 �����õķ���504 ���س�ʱ505 HTTP�汾δ��֧�� 
    ����HTTPͷ��ͷ��������/ֵ����ɡ����������ͻ��˻��߷����������ԡ����������Դ�Լ�Ӧ��ʵ�����ӡ� 
     ���ֲ�ͬ���͵�ͷ�꣺ 
1.ͨ��ͷ�꣺������������Ҳ��������Ӧ������Ϊһ������������ض���Դ������������� 
2.����ͷ�꣺����ͻ��˴��ݹ����������Ϣ��ϣ������Ӧ��ʽ�� 
3.��Ӧͷ�꣺���������ڴ���������Ϣ����Ӧ�� 
4.ʵ��ͷ�꣺���屻������Դ����Ϣ��������������Ҳ��������Ӧ�� 
ͷ���ʽ��<name>:<value><CRLF> 
�±�������HTTP/1.1���õ���ͷ�� 
Accept ����ͻ��˿��Դ����ý�����ͣ������ȼ�������һ���Զ���Ϊ�ָ����б��У����Զ���������ͺ�ʹ��ͨ��������磺Accept: image/jpeg,image/png,* / *Accept-Charset ����ͻ��˿��Դ�����ַ����������ȼ�������һ���Զ���Ϊ�ָ����б��У����Զ���������ͺ�ʹ��ͨ��������磺Accept-Charset: iso-8859-1,*,utf-8 
Accept-Encoding ����ͻ��˿������ı�����ơ����磺Accept-Encoding:gzip,compress 
Accept-Language ����ͻ������ڽ��ܵ���Ȼ�����б����磺Accept-Language: en,de 
Accept-Ranges һ����Ӧͷ�꣬�����������ָ�������ڸ�����ƫ�ƺͳ��ȴ���Ϊ��Դ��ɲ��ֵĽ������󡣸�ͷ���ֵ�����Ϊ����Χ�Ķ�����λ������Accept-Ranges: bytes��Accept-Ranges: none 
Age ����������涨�Է��������ɸ���Ӧ������������ʱ�䳤�ȣ�����Ϊ��λ����ͷ����Ҫ���ڻ�����Ӧ�����磺Age: 30 
Allow һ����Ӧͷ�꣬������һ����λ������URI�еĴ�Դ��֧�ֵ�HTTP�����б����磺Allow: GET,PUT 
aUTHORIZATION һ����Ӧͷ�꣬���ڶ������һ����Դ���������Ȩ����ͱ�������û�ID���������磺Authorization: Basic YXV0aG9yOnBoaWw= 
Cache-Control һ�����ڶ��建��ָ���ͨ��ͷ�ꡣ���磺Cache-Control: max-age=30 
Connection һ�����ڱ����Ƿ񱣴�socket����Ϊ���ŵ�ͨ��ͷ�ꡣ���磺Connection: close��Connection: keep-alive 
Content-Base һ�ֶ������URI��ʵ��ͷ�꣬Ϊ����ʵ�巶Χ�ڽ������URLs�����û�ж���Content-Baseͷ��������URLs��ʹ��Content- Location URI�������Ҿ��ԣ���ʹ��URI�������磺Content-Base:   http://www.myweb.com 
Content-Encoding һ�ֽ����������η�������һ��ʵ������α���ġ����磺Content-Encoding: zipContent-Language ����ָ���������������ݵ���Ȼ�������͡����磺Content-Language: en 
Content-Length ָ���������������Ӧ�����ݵ��ֽڳ��ȡ����磺Content-Length:382 
Content-Location ָ���������������Ӧ�е���Դ��λ��URI���������һ������URL��Ҳ��Ϊ������ʵ������URL�ĳ����㡣���磺Content-Location:   http://www.myweb.com/news 
Content-MD5 ʵ���һ��MD5ժҪ������У��͡����ͷ��ͽ��ܷ�������MD5ժҪ�����ܷ���������ֵ���ͷ���д��ݵ�ֵ���бȽϡ����磺Content-MD5: <base64 of 128 MD5 digest> 
Content-Range �沿��ʵ��һͬ���ͣ������������ֽڵĵ�λ���λ�ֽ�ƫ�ƣ�Ҳ������ʵ����ܳ��ȡ����磺Content-Range: 1001-2000/5000 
Contern-Type �������ͻ��߽��յ�ʵ���MIME���͡����磺Content-Type: text/html 
Date ����HTTP��Ϣ�����ڡ����磺Date: Mon,10PR 18:42:51 GMT 
ETag һ��ʵ��ͷ�꣬���򱻷��͵���Դ����һ��Ψһ�ı�ʶ�������ڿ���ʹ�ö���URL�������Դ��ETag��������ȷ��ʵ�ʱ����͵���Դ�Ƿ�Ϊͬһ��Դ�����磺ETag: '208f-419e-30f8dc99' 
Expires ָ��ʵ�����Ч�ڡ����磺Expires: Mon,05 Dec 2008 12:00:00 GMT 
Form һ������ͷ�꣬���������û�������˹��û��ĵ����ʼ���ַ�����磺From:   webmaster@myweb.com 
Host ��������Դ��������������ʹ��HTTP/1.1��������ԣ�������ǿ���Եġ����磺Host:   www.myweb.com 
If-Modified-Since ���������GET���󣬵��¸����������Ե���������Դ�ϴ��޸����ڡ���������˴�ͷ�꣬������ָ����������������Դ�ѱ��޸ģ�Ӧ�÷���һ��304��Ӧ�� �롣���磺If-Modified-Since: Mon,10PR 18:42:51 GMT 
If-Match ���������һ������ָ��һ�����߶��ʵ���ǡ�ֻ������ETag���б��б���������Դ�����磺If-Match: '208f-419e-308dc99' 
If-None-Match �������һ������ָ��һ�����߶��ʵ���ǡ���Դ��ETag�����б��е��κ�һ������ƥ�䣬������ִ�С����磺If-None-Match: '208f-419e-308dc99' 
If-Range ָ����Դ��һ��ʵ���ǣ��ͻ����Ѿ�ӵ�д���Դ��һ��������������Rangeͷ��һͬʹ�á������ʵ�����ϴα��ͻ��˼����������������޸Ĺ�����ô������ ֻ����ָ���ķ�Χ��������������������Դ�����磺Range: byte=0-499<CRLF>If-Range:'208f-419e-30f8dc99' 
If-Unmodified-Since ֻ����ָ���������������������ʵ�廹�������޸Ĺ����Ż᷵�ش�ʵ�塣���磺If-Unmodified-Since:Mon,10PR 18:42:51 GMT 
Last-Modified ָ����������Դ�ϴα��޸ĵ����ں�ʱ�䡣���磺Last-Modified: Mon,10PR 18:42:51 GMT 
Location ����һ���Ѿ��ƶ�����Դ�������ض�������������һ��λ�á���״̬����302����ʱ�ƶ�������301���������ƶ������ʹ�á����磺Location:   http://www2.myweb.com/index.jsp 
Max-Forwards һ������TRACE����������ͷ�꣬��ָ����������ص������Ŀ��������ͨ�����زŵ���·�ɡ���ͨ�����󴫵�֮ǰ�����������Ӧ�ü��ٴ���Ŀ�����磺Max-Forwards: 3 
Pragma һ��ͨ��ͷ�꣬������ʵ����ص���Ϣ�����磺Pragma: no-cache 
Proxy-Authenticate ������WWW-Authenticate��������������ֻ��������������������һ������������֤�����磺Proxy-Authenticate: Basic realm-admin 
Proxy-Proxy-Authorization ��������Ȩ�����������⴫���κα��ڼ�ʱ���������и���һ�������ݡ����磺Proxy-Proxy-Authorization: Basic YXV0aG9yOnBoaWw= 
Public �б���ʾ��������֧�ֵķ����������磺Public: OPTIONS,MGET,MHEAD,GET,HEAD 
Range ָ��һ�ֶ�����λ��һ�����ֱ�������Դ��ƫ�Ʒ�Χ�����磺Range: bytes=206-5513 
Refener һ������ͷ���򣬱�����������ĳ�ʼ��Դ������HTML�����������˱���Webҳ��ĵ�ַ�����磺Refener:   http://www.myweb.com/news/search.html 
Retry-After һ����Ӧͷ�����ɷ�������״̬����503���޷��ṩ������Ϸ��ͣ��Ա����ٴ�����֮ǰӦ�õȴ��೤ʱ�䡣��ʱ�伴������һ�����ڣ�Ҳ������һ���뵥λ�����磺Retry-After: 18 
Server һ�ֱ���Web�������������汾�ŵ�ͷ�ꡣ���磺Server: Apache/2.0.46(Win32) 
Transfer-Encoding һ��ͨ��ͷ�꣬������Ӧ�����ܷ��������Ϣ��ʵʩ�任�����͡����磺Transfer-Encoding: chunked 
Upgrade ���������ָ��һ���µ�Э������µ�Э��汾������Ӧ����101���л�Э�飩���ʹ�á����磺Upgrade: HTTP/2.0 
User-Agent �������ڲ��������������ͣ����͵���Web������������磺User-Agent: Mozilla/4.0(compatible; MSIE 5.5; Windows NT; DigExt) 
Vary һ����Ӧͷ�꣬���ڱ�ʾʹ�÷�����������Э�̴ӿ��õ���Ӧ��ʾ��ѡ����Ӧʵ�塣���磺Vary: *Via һ�����������м�������Э���ͨ��ͷ�꣬���������������磺Via: 1.0 fred.com, 1.1 wilma.com 
Warning �����ṩ������Ӧ״̬������Ϣ����Ӧͷ�ꡣ���磺Warning: 99   www.myweb.com   Piano needs tuning 
www-Authenticate һ����ʾ�û������ṩ�û����Ϳ������Ӧͷ�꣬��״̬����401��δ��Ȩ�����ʹ�á���Ӧһ����Ȩͷ�ꡣ���磺www-Authenticate: Basic realm=zxm.mgmt 








��ȫ��HTTPͷ����Ϣ���� 
2011-11-03 �����мң�ξ����
    
HTTPͷ������1.Accept������WEB�������Լ�����ʲô�������ͣ�* /*��ʾ�κ����ͣ�type/*��ʾ�������µ����������ͣ�type/sub-type��2.Accept-Charset������������Լ����յ��ַ���Accept-Encoding������������Լ����յı��뷽����ͨ��ָ��ѹ���������Ƿ�֧��ѹ����֧��ʲôѹ��������gzip��deflate��Accept-Language�������������

 HTTP ͷ������

1. Accept������WEB�������Լ�����ʲô�������ͣ�* /* ��ʾ�κ����ͣ�type/* ��ʾ�������µ����������ͣ�type/sub-type��
 
2. Accept-Charset��   ����������Լ����յ��ַ���
   Accept-Encoding��  ����������Լ����յı��뷽����ͨ��ָ��ѹ���������Ƿ�֧��ѹ����֧��ʲôѹ������  ��gzip��deflate��
   Accept-Language��������������Լ����յ��������Ը��ַ������������������ԣ������ж����ַ���������big5��gb2312��gbk�ȵȡ�
 
3. Accept-Ranges��WEB�����������Լ��Ƿ���ܻ�ȡ��ĳ��ʵ���һ���֣������ļ���һ���֣�������bytes����ʾ���ܣ�none����ʾ�����ܡ�
 
4. Age����������������Լ������ʵ��ȥ��Ӧ����ʱ���ø�ͷ��������ʵ��Ӳ��������ھ����೤ʱ���ˡ�
 
5. Authorization�����ͻ��˽��յ�����WEB�������� WWW-Authenticate ��Ӧʱ����ͷ������Ӧ�Լ��������֤��Ϣ��WEB��������
 
6. Cache-Control������no-cache����Ҫ�����ʵ�壬Ҫ�����ڴ�WEB������ȥȡ��
                                     max-age����ֻ���� Age ֵС�� max-age ֵ������û�й��ڵĶ���
                                     max-stale�������Խ��ܹ�ȥ�Ķ��󣬵��ǹ���ʱ�����С�� 
                                                        max-stale ֵ��
                                     min-fresh�������������������ڴ����䵱ǰ Age �� min-fresh ֵ֮�͵�
                                                        �������
                            ��Ӧ��public(������ Cached ���ݻ�Ӧ�κ��û�)
                                      private��ֻ���û������ݻ�Ӧ��ǰ��������ݵ��Ǹ��û���
                                      no-cache�����Ի��棬����ֻ���ڸ�WEB��������֤������Ч�󣬲��ܷ��ظ��ͻ��ˣ�
                                      max-age��������Ӧ�����Ķ���Ĺ���ʱ�䣩
                                      ALL:  no-store���������棩
 
7. Connection������close������WEB���������ߴ��������������ɱ����������Ӧ
                                                  �󣬶Ͽ����ӣ���Ҫ�ȴ��������ӵĺ��������ˣ���
                                 keepalive������WEB���������ߴ��������������ɱ��������
                                                         ��Ӧ�󣬱������ӣ��ȴ��������ӵĺ������󣩡�
                       ��Ӧ��close�������Ѿ��رգ���
                                 keepalive�����ӱ����ţ��ڵȴ��������ӵĺ������󣩡�
   Keep-Alive�������������󱣳����ӣ����ͷ������ϣ�� WEB ����������
                      ���Ӷ೤ʱ�䣨�룩��
                      ���磺Keep-Alive��300
 
8. Content-Encoding��WEB�����������Լ�ʹ����ʲôѹ��������gzip��deflate��ѹ����Ӧ�еĶ��� 
                                 ���磺Content-Encoding��gzip                   
   Content-Language��WEB ����������������Լ���Ӧ�Ķ�������ԡ�
   Content-Length��    WEB ����������������Լ���Ӧ�Ķ���ĳ��ȡ�
                                ���磺Content-Length: 26012
   Content-Range��    WEB ��������������Ӧ�����Ĳ��ֶ���Ϊ����������ĸ����֡�
                                ���磺Content-Range: bytes 21010-47021/47022
   Content-Type��      WEB ����������������Լ���Ӧ�Ķ�������͡�
                                ���磺Content-Type��application/xml
 
9. ETag������һ�����󣨱���URL���ı�־ֵ����һ��������ԣ�����һ�� html �ļ���
              ������޸��ˣ��� Etag Ҳ����޸ģ� ���ԣ�ETag �����ø� Last-Modified ��
              ���ò�࣬��Ҫ�� WEB ������ �ж�һ�������Ƿ�ı��ˡ�
              ����ǰһ������ĳ�� html �ļ�ʱ��������� ETag�����������������ļ�ʱ�� 
              ������ͻ����ǰ��õ� ETag ֵ���͸�  WEB ��������Ȼ�� WEB ������
              ������ ETag �����ļ��ĵ�ǰ ETag ���жԱȣ�Ȼ���֪������ļ�
              ��û�иı��ˡ�
         
10. Expired��WEB������������ʵ�彫��ʲôʱ����ڣ����ڹ����˵Ķ���ֻ����
             ��WEB��������֤������Ч�Ժ󣬲���������Ӧ�ͻ������� HTTP/1.0 ��ͷ����
             ���磺Expires��Sat, 23 May 2009 10:02:12 GMT
 
11. Host���ͻ���ָ���Լ�����ʵ�WEB������������/IP ��ַ�Ͷ˿ںš�
                ���磺Host��rss.sina.com.cn
 
12. If-Match���������� ETag û�иı䣬��ʵҲ����ζ������û�иı䣬��ִ������Ķ�����
    If-None-Match���������� ETag �ı��ˣ���ʵҲ����ζ������Ҳ�ı��ˣ���ִ������Ķ�����
 
13. If-Modified-Since���������Ķ����ڸ�ͷ��ָ����ʱ��֮���޸��ˣ���ִ������
                       �Ķ��������緵�ض��󣩣����򷵻ش���304������������ö���û���޸ġ�
                       ���磺If-Modified-Since��Thu, 10 Apr 2008 09:14:42 GMT
    If-Unmodified-Since���������Ķ����ڸ�ͷ��ָ����ʱ��֮��û�޸Ĺ�����ִ������Ķ��������緵�ض��󣩡�
 
14. If-Range����������� WEB �����������������Ķ���û�иı䣬�Ͱ���ȱ�ٵĲ���
               ���ң��������ı��ˣ��Ͱ�����������ҡ� �����ͨ�������������� 
               ETag ���� �Լ���֪��������޸�ʱ��� WEB �������������ж϶����Ƿ�
               �ı��ˡ����Ǹ� Range ͷ��һ��ʹ�á�
 
15. Last-Modified��WEB ��������Ϊ���������޸�ʱ�䣬�����ļ�������޸�ʱ�䣬
                   ��̬ҳ���������ʱ��ȵȡ����磺Last-Modified��Tue, 06 May 2008 02:42:43 GMT
 
16. Location��WEB �������������������ͼ���ʵĶ����Ѿ����Ƶ����λ���ˣ�����ͷ��ָ����λ��ȥȡ��
                        ���磺Location��http://i0.sinaimg.cn/dy/deco/2008/0528/sinahome_0803_ws_005_text_0.gif
 
17. Pramga����Ҫʹ�� Pramga: no-cache���൱�� Cache-Control�� no-cache�����磺Pragma��no-cache
 
18. Proxy-Authenticate�� �����������Ӧ�������Ҫ�����ṩ���������֤��Ϣ��
      Proxy-Authorization���������Ӧ����������������֤�����ṩ�Լ��������Ϣ��
 
19. Range������������� Flashget ���߳�����ʱ������ WEB �������Լ���ȡ������Ĳ��֡�
                    ���磺Range: bytes=1173546-
 
20. Referer��������� WEB �����������Լ��Ǵ��ĸ� ��ҳ/URL ���/��� ��ǰ�����е���ַ/URL��
                   ���磺Referer��http://www.sina.com/
 
21. Server: WEB �����������Լ���ʲô������汾����Ϣ�����磺Server��Apache/2.0.61 (Unix)
 
22. User-Agent: ����������Լ�����ݣ����������������
                        ���磺User-Agent��Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN;   
                                  rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14
 
23. Transfer-Encoding: WEB �����������Լ��Ա���Ӧ��Ϣ�壨������Ϣ������Ķ������������ı��룬�����Ƿ�ֿ飨chunked����
                       ���磺Transfer-Encoding: chunked
 
24. Vary: WEB�������ø�ͷ�������ݸ��� Cache ����������ʲô�����²����ñ���Ӧ
                 �����صĶ�����Ӧ����������
                 ����ԴWEB�������ڽӵ���һ��������Ϣʱ������Ӧ��Ϣ��ͷ��Ϊ��
                 Content-Encoding: gzip; Vary: Content-Encoding  ��ô Cache ���������������
                 ������Ϣ��ͷ��������� Accept-Encoding���Ƿ����ǰ��Ӧ�� Vary ͷ��ֵ
                 һ�£����Ƿ�ʹ����ͬ�����ݱ��뷽���������Ϳ��Է�ֹ Cache ���������Լ�
                 Cache ����ѹ�����ʵ����Ӧ�����߱���ѹ�������������
                 ���磺Vary��Accept-Encoding
 
25. Via�� �г��ӿͻ��˵� OCS �����෴�������Ӧ��������Щ�����������������
        ʲôЭ�飨�Ͱ汾�����͵����󡣵��ͻ������󵽴��һ�����������ʱ���÷����������Լ���������������
        ��� Via ͷ�����������Լ��������Ϣ������һ����������� �յ���һ�����������������ʱ�������Լ���
        �����������渴��ǰһ������������������Viaͷ���������Լ��������Ϣ�ӵ����棬 �Դ����ƣ��� OCS 
        �յ����һ�����������������ʱ����� Via ͷ������֪����������������·�ɡ�
        ���磺Via��1.0 236-81.D07071953.sina.com.cn:80 (squid/2.6.STABLE13)










���HTTP��Ϣͷ��������������Ҳ�ܷḻ�� .
���ࣺ ��վ 2014-09-08 00:16 154���Ķ� ����(0) �ղ� �ٱ� 
Ŀ¼(?)[+]

���HTTP��Ϣͷ ��������HTTP������403 Access Forbidden404 Object not found401 Access Denied302 Object Moved500 Internal Server Error���HTTP��Ϣͷ �����HTTP��Ϣͷ �ķ��������ص���ϢContent-TypeContent-DispositionContent-Type��Content-DispositionCache��һ����ʶHTTP��Ϣͷ

������WEB�������˶��벻��HTTP�����ı�����Э�飩����Ҫ�˽�HTTP������HTML�������⣬����һ���ֲ��ɺ��ӵľ���HTTP��Ϣͷ��
����Socket��̵��˶�֪�������������һ��ͨ��Э��ʱ������Ϣͷ/��Ϣ�塱�ķָʽ�Ǻܳ��õģ���Ϣͷ���߶Է������Ϣ�Ǹ�ʲô�ģ���Ϣ����߶Է���ô�ɡ�HTTP�������ϢҲ�������涨�ģ�ÿһ��HTTP������ΪHTTPͷ��HTTP�������֣������ǿ�ѡ�ģ���ǰ���Ǳ���ġ�ÿ�����Ǵ�һ����ҳ�����������Ҽ���ѡ�񡰲鿴Դ�ļ�������ʱ������HTML�������HTTP����Ϣ�壬��ô��Ϣͷ�������أ�IE������������ǿ����ⲿ�֣������ǿ���ͨ����ȡ���ݰ��ȷ�����������
���������һ���򵥵����ӣ�
��������һ���ǳ��򵥵���ҳ����������ֻ��һ�У�
<html><body>hello world</body></html>
�����ŵ�WEB�������ϣ�����IIS��Ȼ����IE������������ҳ�棨http://localhost:8080/simple.htm�����������������ҳ��ʱ�������ʵ�����������������
1 ������������ĵ�ַ�����зֽ��Э���������������˿ڡ�����·���Ȳ��֣��������ǵ������ַ�������õ��Ľ�����£�
Э������http
��������localhost
�˿ڣ�8080
����·����/simple.htm
2 �����ϲ��ֽ�ϱ����Լ�����Ϣ����װ��һ��HTTP�������ݰ�
3 ʹ��TCPЭ�����ӵ�������ָ���˿ڣ�localhost, 8080�����������ѷ�װ�õ����ݰ�
4 �ȴ��������������ݣ��������������ݣ������ʾ����
�ɽ�ȡ�������ݰ����ǲ��ѷ�����������ɵ�HTTP���ݰ����������£�
GET /simple.htm HTTP/1.1<CR>
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, * / *<CR>
Accept-Language: zh-cn<CR>
Accept-Encoding: gzip, deflate<CR>
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)<CR>
Host: localhost:8080<CR>
Connection: Keep-Alive<CR>
<CR>
Ϊ����ʾ����Ұ����еĻس��ĵط��������ˡ�<CR>����ע�������һ�����м�һ���س��������������HTTP�涨����Ϣͷ����Ϣ��ķֽ��ߣ���һ���������µ����ݾ�����Ϣ�壬����������ݰ���û����Ϣ��ġ�
��Ϣ�ĵ�һ�С�GET����ʾ������ʹ�õ�HTTP�������������ܵĻ��С�POST���ȣ�GET����Ϣû����Ϣ�壬��POST��Ϣ������Ϣ��ģ���Ϣ������ݾ���ҪPOST�����ݡ�����/simple.htm��������Ҫ����Ķ���֮��HTTP1.1��ʾʹ�õ���HTTP1.1Э�顣
�ڶ��б�ʾ�������õ�������ܽ��ܵ�Content-type�����������������Ժͱ�����Ϣ����������ʾ�����������ϵ��Ϣ��������������͡�����ϵͳ��Ϣ�ȣ��ܶ���վ������ʾ������ʹ�õ�������Ͳ���ϵͳ�汾��������Ϊ���Դ������ȡ����Щ��Ϣ��
�����б�ʾ����������������Ͷ˿ڣ������б�ʾʹ��Keep-Alive��ʽ�������ݴ����겢�������ر����ӡ�
���������յ����������ݰ��Ժ���������������Ӧ�Ĵ������������û�С�/simple.htm�������������У����ݷ�������������������δ��������HTM������Ҫʲô���ӵĴ���ֱ�ӷ��������ݼ��ɡ�����ֱ�ӷ���֮ǰ������Ҫ����HTTP��Ϣͷ��
���������ص�����HTTP��Ϣ���£�
HTTP/1.1 200 OK<CR>
Server: Microsoft-IIS/5.1<CR>
X-Powered-By: ASP.NET<CR>
Date: Fri, 03 Mar 2006 06:34:03 GMT<CR>
Content-Type: text/html<CR>
Accept-Ranges: bytes<CR>
Last-Modified: Fri, 03 Mar 2006 06:33:18 GMT<CR>
ETag: "5ca4f75b8c3ec61:9ee"<CR>
Content-Length: 37<CR>
<CR>
<html><body>hello world</body></html>
ͬ�������á�<CR>������ʾ�س������Կ����������ϢҲ���ÿ����зֳ���Ϣͷ����Ϣ�������֣���Ϣ��Ĳ�����������ǰ��д�õ�HTML���롣
��Ϣͷ��һ�С�HTTP/1.1��Ҳ�Ǳ�ʾ��ʹ�õ�Э�飬����ġ�200 OK����HTTP���ش��룬200�ͱ�ʾ�����ɹ�������������������404��ʾ����δ�ҵ���500��ʾ����������403��ʾ�������Ŀ¼�ȵȡ�
�ڶ��б�ʾ���������ʹ�õ�WEB�����������������IIS 5.1����������ASP.Net��һ��������ʾ��ûʲôʵ���ô����������Ǵ���������ʱ�䡣�����о��������ص���Ϣ��content-type����������������������δ�����Ϣ����������ݣ�����������text/html����ô������ͻ�����HTML���������������������image/jpeg����ô�ͻ�ʹ��JPEG�Ľ�����������
��Ϣͷ���һ�С�Content-Length����ʾ��Ϣ��ĳ��ȣ��ӿ����Ժ�������������ֽ�Ϊ��λ����������յ�����ָ�����ֽ����������Ժ�ͻ���Ϊ�����Ϣ�Ѿ������������ˡ�
 
 
 
 
���HTTP��Ϣͷ ������
������HTTP������
��һƪ�������Ҽ�Ҫ��˵��˵HTTP��Ϣͷ�ĸ�ʽ��ע�⵽�ڷ��������ص�HTTP��Ϣͷ����һ����HTTP/1.1 200 OK���������200��HTTP�涨�ķ��ش��룬��ʾ�����Ѿ�������������ɡ������ͨ��������ش���Ϳ���֪������������������Ĵ��������ʲô��ÿһ�ַ��ش��붼���Լ��ĺ��塣�����оټ��ֳ����ķ����롣
1 403 Access Forbidden
���������ͼ�����������һ���ļ��У�����WEB������������ļ��в�û�����������ļ�����Ŀ¼�Ļ����ͻ᷵��������롣һ��������403�ظ������������ģ���IIS5.1��
HTTP/1.1 403 Access Forbidden
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 08:57:39 GMT
Connection: close
Content-Type: text/html
Content-Length: 172
 
<html><head><title>Directory Listing Denied</title></head>
<body><h1>Directory Listing Denied</h1>This Virtual Directory does not allow contents to be listed.</body></html>
2 404 Object not found
����������Ķ����ڷ������ϲ�������ʱ���ͻ����������ش��룬�����Ҳ������Ĵ�������ˡ�IIS������404��Ϣ���ݺܳ���������Ϣͷ���⻹��һ��������˵����Ϊʲô������������ҳ��APACHE��������404��Ϣ�Ƚϼ�̣����£�
HTTP/1.1 404 Not Found
Date: Mon, 06 Mar 2006 09:03:14 GMT
Server: Apache/2.0.55 (Unix) PHP/5.0.5
Content-Length: 291
Keep-Alive: timeout=15, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=iso-8859-1
 
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>404 Not Found</title>
</head><body>
<h1>Not Found</h1>
<p>The requested URL /notexist was not found on this server.</p>
<hr>
<address>Apache/2.0.55 (Unix) PHP/5.0.5 Server at localhost Port 8080</address>
</body></html>
Ҳ������ʣ�������404����200����������Ϣ���ڸ���һ��˵����ҳ����ô���ڿͻ�����˵������ʲô�����أ�һ���Ƚ����Ե���������200�ǳɹ�������������¼�������ַ���Ա��´��ٷ���ʱ�����Զ���ʾ�õ�ַ����404��ʧ�����������ֻ����ʾ�����ص�ҳ�����ݣ��������¼�˵�ַ��Ҫ�ٴη���ʱ����Ҫ���������ĵ�ַ��
3 401 Access Denied
��WEB�������������������ʣ���������û���ṩ��ȷ���û���/����ʱ���������ͻ����������ش��롣��IIS�У�����IIS�İ�ȫ����Ϊ�������������ʣ�����ͼ������ʱֱ�ӷ��ʵĻ��ͻ�õ����·��ؽ����

HTTP/1.1 401 Access Denied
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 09:15:55 GMT
WWW-Authenticate: Negotiate
WWW-Authenticate: NTLM
Connection: close
Content-Length: 3964
Content-Type: text/html
 
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html dir=ltr>
����
��ʱ������ϸ�������ʾ����ͼ�������������û��������룺

�򷵻���Ϣ����Ϣ��ϳ���ֻȡǰ���������ݡ�ע�⣬�������localhost�����ʱ�����IIS����IE����ֱ��ȡ�õ�ǰ�û�����ݣ�����ͷ�������ֱ�ӽ���Э�̣����Բ��ῴ��401��ʾ��
���������������û����������Ժ󣬷�������ͻ��˻��ٽ������ζԻ������ȿͻ������������ȡһ����Կ���������˻᷵��һ����Կ�����߶���BASE64���룬��Ӧ����Ϣ���£����벿���Ѿ����˴�����
GET / HTTP/1.1
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, * / *
Accept-Language: zh-cn
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: 192.168.0.55:8080
Connection: Keep-Alive
Authorization: Negotiate ABCDEFG����
 
HTTP/1.1 401 Access Denied
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 09:20:53 GMT
WWW-Authenticate: Negotiate HIJKLMN����
Content-Length: 3715
Content-Type: text/html
 
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html dir=ltr>
����
�ͻ����õ���Կ֮��ʹ�ù�Կ���û�����������м����룬Ȼ��Ѽ����Ժ�Ľ�����·�����������
GET / HTTP/1.1
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, * / *
Accept-Language: zh-cn
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: 192.168.0.55:8080
Connection: Keep-Alive
Authorization: Negotiate OPQRST����
�����������֤ͨ�����������˾ͻ����������ݷ��͹����ˣ�Ҳ����˵��ֹ�������ʵ���վ�ᾭ����������ſ��Կ���ҳ�档����Ϊ�ͻ���������Ѿ������˹�Կ����ͬһ������������ٴ����������վ�ϵ�����ҳ��ʱ�Ϳ���ֱ�ӷ�����֤��Ϣ���Ӷ�һ�ν����Ϳ�������ˡ�
4 302 Object Moved
�ù�ASP���˶�֪��ASP��ҳ���ض���������Redirect��Transfer���ַ�����������������Redirect�ǿͻ����ض��򣬶�Transfer�Ƿ��������ض�����ô���Ǿ��������ͨ��HTTP��Ϣͷʵ�ֵ��أ�
������һ��Transfer�����ӣ�
����ASP�ļ�1.aspֻ��һ��
<% Server.Transfer "1.htm" %>
HTML�ļ�1.htmҲֻ��һ�У�
<p>this is 1.htm</p>
������Ǵ������������1.asp�����͵������ǣ�
GET /1.asp HTTP/1.1
Accept: * / *
Accept-Language: zh-cn
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: localhost:8080
Connection: Keep-Alive
Cookie: ASPSESSIONIDACCTRTTT=PKKDJOPBAKMAMBNANIPIFDAP
ע��������ļ�ȷʵ��1.asp�����õ��Ļ�Ӧ���ǣ�
HTTP/1.1 200 OK
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 12:52:44 GMT
X-Powered-By: ASP.NET
Content-Length: 20
Content-Type: text/html
Cache-control: private
 
<p>this is 1.htm</p>
���ѿ�����ͨ��Server.Transfer�����������Ѿ�����ҳ���ض��򣬶��ͻ��˶Դ�һ����֪�������Ͽ���ȥ�õ��ľ���1.asp�Ľ����
�����1.asp�����ݸ�Ϊ��
<% Response.Redirect "1.htm" %>
�ٴ�����1.asp�����͵�����û�б仯���õ��Ļ�Ӧȴ����ˣ�
HTTP/1.1 302 Object moved
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 12:55:57 GMT
X-Powered-By: ASP.NET
Location: 1.htm
Content-Length: 121
Content-Type: text/html
Cache-control: private
 
<head><title>Object moved</title></head>
<body><h1>Object Moved</h1>This object may be found <a HREF="">here</a>.</body>
ע��HTTP�ķ��ش�����200�����302����ʾ����һ���ض�����Ϣ���ͻ�����Ҫ������Ϣͷ��Location�ֶε�ֵ���·����������Ǿ���������һ��Ի���
GET /1.htm HTTP/1.1
Accept: * / *
Accept-Language: zh-cn
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: localhost:8080
Connection: Keep-Alive
If-Modified-Since: Thu, 02 Mar 2006 06:50:13 GMT
If-None-Match: "b224758ec53dc61:9f0"
Cookie: ASPSESSIONIDACCTRTTT=PKKDJOPBAKMAMBNANIPIFDAP

HTTP/1.1 200 OK
Server: Microsoft-IIS/5.1
X-Powered-By: ASP.NET
Date: Mon, 06 Mar 2006 12:55:57 GMT
Content-Type: text/html
Accept-Ranges: bytes
Last-Modified: Mon, 06 Mar 2006 12:52:32 GMT
ETag: "76d85bd51c41c61:9f0"
Content-Length: 20
 
<p>this is 1.htm</p>
�����ԣ������ض���ʽ��Ȼ����ȥ������񣬵���ʵ��ԭ�����кܴ�Ĳ�ͬ��
5 500 Internal Server Error
500�Ŵ������ڷ����������д����ʱ�����磬ASP����Ϊ
<% if %>
��Ȼ������򲢲����������ǵõ��Ľ��Ϊ��
HTTP/1.1 500 Internal Server Error
Server: Microsoft-IIS/5.1
Date: Mon, 06 Mar 2006 12:58:55 GMT
X-Powered-By: ASP.NET
Content-Length: 4301
Content-Type: text/html
Expires: Mon, 06 Mar 2006 12:58:55 GMT
Set-Cookie: ASPSESSIONIDACCTRTTT=ALKDJOPBPPKNPCNOEPCNOOPD; path=/
Cache-control: private

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html dir=ltr>
����
������������500�Ŵ��󣬲��Һ���ͨ��HTML�ķ�ʽ˵���˴����ԭ��

 
 
���HTTP��Ϣͷ ������

������ �ͻ��˷��͵�����
��һ����Ҫ���۲�HTTP��Ϣͷ�пͻ��˵����󣬴����ҵ�һЩ����˼�����ݡ�
 
1 HTTP_REFERER
д�����򵥵���ҳ��
a.htm��
<a href=b.htm>to page b</a>
b.htm��
haha
���ݺܼ򵥣�������ҳA����һ����B�����ӡ������Ƿŵ�IIS�ϣ���������ҳA�������ٵ����B�����ӣ����ǿ�����Bҳ�ġ�haha������ô������������ʲô��ͬ�𣿹۲����������͵�HTTP��Ϣͷ�������Ե�������Ƿ���Bҳʱ�ȷ���Aҳʱ����һ�У�
Referer: http://localhost/a.htm
��һ�оͱ�ʾ���û�Ҫ���ʵ�Bҳ�Ǵ�Aҳ���ӹ����ġ�
��������Ҫ��ȡ�����ֵҲ�Ǻ����׵ģ���ASPΪ����ֻ��Ҫдһ��
<% =Request.ServerVariables("HTTP_REFERER") %>
�Ϳ����ˡ�
һЩ��վͨ��HTTP_REFERER������ȫ��֤���ж��û��ǲ��Ǵ������ҳ���������ģ�������ֱ�Ӵ�������ϴ�URL�������ҳ�����ӹ������������Դ�һ���̶��Ϸ�ֹ��ҳ�����Ƿ�ʹ�á���������ԭ����������Ҫƭ��������Ҳ�������ѣ�ֻҪ�ֹ����������HTTP��Ϣͷ�Ϳ����ˣ��������õ��ֶλ���ͨ��HOSTS�ļ�α�������ȡ�
���˳��������⣬�����������ַ�ʽ�ᵼ��HTTP_REFERER��Ϣ�����ͣ��磺
������ܣ�<iframe src=b.asp></iframe>
��ܼ���<frameset><frame src=b.asp></frameset>
���ύ��<form action=b.asp><input type=submit></form>
SCRIPT���ã�<script src=b.asp></script>
CSS���ã�<link rel=stylesheet type=text/css href=b.asp>
XML���ݵ���<xml src=b.asp></xml>
��������ʽ���ᷢ��HTTP_REFERER��
scriptת��<script>location.href="b.asp"</script>
script���´��ڣ�<script>window.open("b.asp");</script>
METAת��<meta http-equiv="refresh" content="0;URL=b.asp">
����ͼƬ��<img src=b.asp>
 
2 COOKIE
COOKIE�Ǵ�Ҷ��ǳ���Ϥ���ˣ�ͨ���������ڿͻ��˱����û�״̬����ʹ�û��ر������Ҳ�ܼ������档��ô�ͻ����������������ν���COOKIE��Ϣ���أ�û��Ҳ��ͨ��HTTP��Ϣͷ��
����дһ���򵥵�ASP��ҳ��
<%
Dim i
i =  Request.Cookies("key")
Response.Write i
Response.Cookies("key") = "haha"
Response.Cookies("key").Expires = #2007-1-1#
%>
��һ�η��ʴ���ҳʱ����Ļ��һƬ�ף��ڶ��η���ʱ�������ʾ����haha����ͨ���Ķ������ѷ��֣���Ļ����ʾ������ʵ������COOKIE�����ݣ�����һ�η���ʱ��û������COOKIE��ֵ�����Բ�������ʾ���ڶ�����ʾ���ǵ�һ�����õ�ֵ����ô��Ӧ��HTTP��ϢͷӦ����ʲô�����أ�
��һ������ʱûʲô��ͬ���Թ�
��һ�η���ʱ��Ϣ���ݶ���������һ�У�
Set-Cookie: key=haha; expires=Sun, 31-Dec-2006 16:00:00 GMT; path=/
�����ԣ�key=haha��ʾ����Ϊ��key����COOKIE��ֵΪ��haha��������������COOKIE�Ĺ���ʱ�䣬��Ϊ���õ����Ĳ���ϵͳ��ʱ���Ƕ�������2007��1��1��0���Ӧ��GMTʱ�����2006��12��31��16�㡣
�ڶ����ٷ��ʴ���ҳʱ�����͵����ݶ�������һ�У�
Cookie: key=haha
�������ݾ��Ǹղ����COOKIE�����ݡ��ɼ����ͻ����ڴӷ������˵õ�COOKIEֵ�Ժ�ͱ�����Ӳ���ϣ��ٴη���ʱ�ͻ�������͵�������������ʱ��û�з��͹���ʱ�䣬��Ϊ�������Թ���ʱ�䲢�����ģ���COOKIE���ں�������Ͳ����ٷ������ˡ�
���ʹ��IE6.0��������ҽ���COOKIE���ܣ����Է��ַ������˵�set-cookie�����еģ����ͻ��˲������������Ҳ���ᷢ��������Щ��վ���ر�������ͶƱ��վͨ����¼COOKIE��ֹ�û��ظ�ͶƱ���ƽ�ܼ򵥣�ֻҪ��IE6�����������COOKIE�Ϳ����ˡ�Ҳ�е���վͨ��COOKIEֵΪĳֵ���ж��û��Ƿ�Ϸ��������ж�Ҳ�ǳ�����ͨ���ֹ�����HTTP��Ϣͷ����ƭ����Ȼ��HOSTS�ķ�ʽҲ�ǿ�����ƭ�ġ�
 
3 SESSION
HTTPЭ�鱾������״̬�ģ��������Ϳͻ��˶�����֤�û������ڼ����ӻ�һֱ���֣���ʵ�ϱ���������HTTP1.1���е������ݣ����ͻ��˷��͵���Ϣͷ���С�Connection: Keep-Alive��ʱ��ʾ�ͻ��������֧�ֱ������ӵĹ�����ʽ�����������Ҳ����һ��ʱ��û��������Զ��Ͽ����Խ�ʡ��������Դ��Ϊ���ڷ�������ά���û�״̬��SESSION�ͱ����������ˣ����ڸ������Ķ�̬��ҳ�������߶�֧��SESSION����֧�ֵķ�ʽ����ȫ��ͬ�����½���ASPΪ����
���û�����һ��ASP��ҳʱ���ڷ��ص�HTTP��Ϣͷ�л���һ�У�
Set-Cookie: ASPSESSIONIDCSQCRTBS=KOIPGIMBCOCBFMOBENDCAKDP; path=/
������ͨ��COOKIE�ķ�ʽ���߿ͻ������SESSIONID�Ƕ��٣��������ǡ�KOIPGIMBCOCBFMOBENDCAKDP�������ҷ������ϱ����˺ʹ�SESSIONID��ص����ݣ���ͬһ�û��ٴη�������ʱ����������COOKIE�ٷ��ͻ�ȥ���������˸��ݴ�ID�ҵ����û������ݣ�Ҳ��ʵ���˷��������û�״̬�ı��档����������ASP���ʱ����ʹ�á�session("name")=user�������ķ�ʽ�����û���Ϣ��ע���COOKIE�����ﲢû�й���ʱ�䣬���ʾ����һ�����ر������ʱ�������ڵ�COOKIE�������ᱻ���浽Ӳ���ϡ����ֹ�����ʽ�ȵ�����COOKIE�ķ�ʽҪ��ȫ�ܶ࣬��Ϊ�ڿͻ��˲�û��ʲô���������޸ĺ���ƭ��ֵ��Ψһ����Ϣ����SESSIONID�������ID��������ر�ʱ������ʧЧ�����Ǳ��������������վ�ڼ��ر��������ܶ�ʱ����֪����ID��ֵ��������һЩ��ƭ�����Ϊ���������ж�SESSION���ڵķ�ʽ�����ǶϿ����ӻ�ر������������ͨ���û��ֹ�����SESSION��ȴ���ʱ�����û��ر���������һ��ʱ����SESSION��û�г�ʱ��������ʱ���֪���˸ղŵ�SESSIONID�����ǿ�����ƭ�ġ�����ȫ�İ취�������뿪��վ֮ǰ�ֹ�����SESSION���ܶ���վ���ṩ��Logout�����ܣ�����ͨ������SESSION�е�ֵΪ���˳�״̬����SESSION�������ڴӶ��𵽰�ȫ��Ŀ�ġ�
SESSION��COOKIE�ķ�ʽ������ȱ�㡣SESSION���ŵ��ǱȽϰ�ȫ�������ױ���ƭ��ȱ���ǹ���ʱ��̣�����ù��ڳ�������ʱ����û��������������κ���Ϣ���ͻᱻ��Ϊ���������ˣ�COOKIE���෴�����ݷ����������õĳ�ʱʱ�䣬���Գ�ʱ�䱣����Ϣ����ʹ�ػ��ٿ���Ҳ���ܱ���״̬������ȫ����Ȼ����ۿۡ��ܶ���վ���ṩ������֤��ʽ���ϣ�����û���ʱ����̨���Է��ʴ˷�������Ҫ�����û��������룬������COOKIE������û�ʹ�õ����Լ��ĸ��˵��ԣ����������վ���Լ�Ӳ���ϱ���COOKIE���Ժ����ʱ�Ͳ���Ҫ���������û����������ˡ�
 
4 POST
��������ʷ��������õķ�ʽ��GET��POST���֣�GET��ʽֻ����HTTP��Ϣͷ��û����Ϣ�壬Ҳ���ǳ���ҪGET�Ļ�����Ϣ֮�ⲻ��������ṩ������Ϣ����ҳ����FROM����Ĭ���ύ��ʽ������GET��ʽ�������������������ύ����Ϣ����ΪURL����Ĳ�������a.asp?a=1&b=2�����ķ�ʽ������Ҫ�ύ���������ܴ󣬻������ύ���ݲ�ϣ������ֱ�ӿ���ʱ��Ӧ��ʹ��POST��ʽ��POST��ʽ�ύ����������ΪHTTP��Ϣ����ڵģ����磬дһ����ҳ����
<form method=post>
<input type=text name=text1>
<input type=submit>
</form>
���ʴ���ҳ�����ڱ�������һ����haha����Ȼ���ύ�����Կ����˴��ύ�����͵���Ϣ���£�
POST /form.asp HTTP/1.1
Accept:  * / *
Referer: http://localhost:8080/form.asp
Accept-Language: zh-cn
Content-Type: application/x-www-form-urlencoded
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: localhost:8080
Content-Length: 10
Connection: Keep-Alive
Cache-Control: no-cache
Cookie: key=haha; ASPSESSIONIDCSQCRTBS=LOIPGIMBLMNOGCOBOMPJBOKP
text1=haha
ǰ��ؼ��ִӡ�GET����Ϊ�ˡ�POST����Content-Type����ˡ�application/x-www-form-urlencoded�����������ݲ��޴�仯��ֻ�Ƕ���һ�У�Content-Length: 10����ʾ�ύ�����ݵĳ��ȡ����к�������Ϣ�壬���ݾ��Ǳ�����������ݡ�ע���ʱ���͵�����ֻ�ǡ�Name=Value������ʽ��������������Ϣ���ᱻ���ͣ�������ֱ�Ӵӷ�������ȡ��list box�����е�list item�ǰ첻���ģ��������ύǰ��һ��script�����е�item���ݶ�����һ��ŵ�һ�����������С�
������ñ��ϴ��ļ��������Ҫ����һЩ�ˣ������Ǳ�������Ҫ����һ�仰��enctype='multipart/form-data'����ʾ��������ύ������ݣ�����HTML��input type=file������һ���ļ��ύ��
���������£�
<form method=post enctype='multipart/form-data'>
<input type=text name=text1>
<input type=file name=file1>
<input type=submit>
</form>
����Ϊtext1�������֣�hehe��Ϊfile1ѡ���ļ�haha.txt��������Ϊ��ABCDEFG����Ȼ���ύ�˱����ύ����ȫ��ϢΪ��
POST /form.asp HTTP/1.1
Accept: * / *
Referer: http://localhost:8080/form.asp
Accept-Language: zh-cn
Content-Type: multipart/form-data; boundary=---------------------------7d62bf2f9066c
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
Host: localhost:8080
Content-Length: 337
Connection: Keep-Alive
Cache-Control: no-cache
Cookie: key=haha; ASPSESSIONIDCSQCRTBS=LOIPGIMBLMNOGCOBOMPJBOKP
-----------------------------7d62bf2f9066c
Content-Disposition: form-data; name="text1"
hehe
-----------------------------7d62bf2f9066c
Content-Disposition: form-data; name="file1"; filename="H:\Documents and Settings\Administrator\����\haha.txt"
Content-Type: text/plain
ABCDEFG
-----------------------------7d62bf2f9066c--
 
��Ȼ����ύ����ϢҪ��ǰ���ĸ��ӺܶࡣContent-Type����ˡ�multipart/form-data�������滹����һ��boundary����ֵ��Ϊ������POST�����ݵ������õģ�ֻҪ�������������˴�ֵ���ͱ�ʾ����Ҫ��ʼһ���µ������ˣ�ÿ�����ε�������Զ���������������Ǵ�ֵ���������������ţ����ʾȫ�����ݵ��˽�����ÿ����Ҳ��Ϊ��ͷ�Ͷ��������֣��ÿ��и�����ÿ�ζ����Լ������ͺ������Ϣ�����һ������text1��ֵ�����������ǡ�text1����ֵΪ��hehe�����ڶ������ļ����ݣ�����������˴��ļ�������ơ�file1���ʹ��ļ����û������ϵ�λ�ã���������ļ������ݡ�
���������Ҫ�Լ�дһ���ϴ��ļ����������HTML�����͵��ļ����ݣ���ô����ĵ�������ǽ��������ݰ�������ȡ����Ҫ����Ϣ��
 


 
 
���HTTP��Ϣͷ ���ģ�
���������ص���Ϣ
���������ص�HTTP��ϢҲ��Ϊ��Ϣͷ����Ϣ�������֡�ǰ�����صĵڶ�ƪ���Ѿ������˷�����Ϣ�г������ش���ĺ��塣���ڷ������ķ��ش���Ĵ���Ƚϼ򵥣�ֻҪ����Ҫ��ȥ���ͺ��ˣ������������ķ��ش��루200�����䴦��ʽ�Ͷ��ֶ����ˡ�

1 Content-Type
Content-Type�Ƿ�����Ϣ�зǳ���Ҫ�����ݣ�����ʶ������������ݵ����ͣ���ֵΪ��������/�����͡��ĸ�ʽ����������ľ���text/html��������˼��˵���ص��������ı����ͣ�����ı�����HTML��ʽ�ġ�ԭ��������������Content-Type�����������ʾ���ص���Ϣ�����ݡ����������������У�
text/html HTML�ı�
image/jpeg JPGͼƬ
image/gif GIFͼƬ
application/xml XML�ĵ�
audio/x-mpegurl MP3�ļ��б������װ��Winamp�������ֱ�Ӱ�������M3U�ļ�����
������������Ϳ�����ע���HKCR\MIME\Database\Content Type���¿���
����IE6�������˵�����Content-Type�е����ͺ�ʵ�ʵ���Ϣ�����Ͳ�һ�£���ô������������е�����������ʵ��Ӧ����ʲô���ͣ�����JPG��GIF�ȳ���ͼƬ��ʽ��������ȷ��ʶ�������������Content-Type��д����ʲô��
���Content-Type��ָ���������������ֱ�Ӵ򿪵����ͣ���ô������ͻ�ֱ�Ӵ���������ʾ����������Ǳ�����������Ӧ�ó�������ͣ���ʱ��Ҫ����ע����й����������͵�ע����������������ֱ�Ӵ򿪶�����Ҫѯ�ʵģ��ͻ�ֱ�ӵ������������Ӧ�ó�����������ļ���������ǲ�����ֱ�Ӵ򿪵ģ��ͻ�ѯ���Ƿ�򿪡�����û�й������κ�Ӧ�ó�������ͣ�IE�������֪��������δ򿪣���ʱIE6�ͻ��������XML�����Դ򿪡�
2 Content-Disposition
�����AddHeader�ķ�����HTTP��Ϣͷ�м���Content-Disposition�Σ���ָ����ֵΪ��attachment������ô��������ļ��Ǻ����ͣ������������ʾ�������ش��ļ�����Ϊ��ʱ����Ϊ�������Ϣ����һ����������������Ҫ��������������ˡ����磬��ASP.Net��д��������䣺
Response.AddHeader("Content-Disposition: attachment");
�����ҳ���ǵõ��Ľ���磺
HTTP/1.1 200 OK
Server: Microsoft-IIS/5.1
Date: Thu, 23 Mar 2006 07:54:53 GMT
Content-Disposition: attachment
Cache-Control: private
Content-Type: text/html; charset=utf-8
����
Ҳ����˵��ͨ��AddHeader��������ΪHTTP��Ϣͷ���������Զ�������ݡ�ʹ�����ַ�������ǿ�����������ʾ�����ļ�����ʹ����ļ���������֪�����ͣ�������HTML��ҳ�������Ҫ���û�����ʱ��ʾһ��Ĭ�ϵ��ļ�����ֻ��Ҫ��ǰ��һ�仰����ϡ�filename=�ļ��������ɡ����磺
Response.AddHeader("Content-Disposition: attachment; filename=mypage.htm");
3 Content-Type��Content-Disposition
�����Content-Type��Content-Disposition�����һ��ʹ�û���ô���أ�
��һ����ҳʱ������������ȿ��Ƿ���Content-Disposition: attachment��һ�����У�����Content-Type��ֵ��ʲô��������ʾ�ļ����ء�
���ָ����filename���ͻ���ʾĬ�ϵ��ļ���Ϊ���ļ�����ע�⵽��IE6�г��ˡ����桱��Ť�⻹�С��򿪡���Ť����ʱ���ļ�������������filename��ָ�����ļ���չ�������ģ�������filename=mypic.jpg��������ͻ����Ĭ�ϵ�ͼƬ�鿴�����򿪴��ļ���
���û��ָ��filename����ô������͸���Content-Type�е������������ļ������ͣ�����Content-Type����Ϊimage/gif����ô�ͻ�ȥ����Ĭ�ϵĿ�GIFͼƬ�Ĺ��ߣ��������ô��ļ�������Ϊ���������ҳ��������������չ�������϶�Ӧ�ڴ��ļ���Ū��չ�������������mypage.aspx���ͻ��Զ����mypage.gif�������û��ָ��Content-Typeֵ����ô��Ĭ����Ϊ��text/html�������ұ�����ļ����������������ҳ�ļ�����
�����û��ָ��Content-Disposition����ô�ͺ�ǰ�����Content-Type�������۵������һ�����ˡ�

4 Cache
������Ϣ�е�Cache����ָ����ҳ���档���Ǿ������Կ����������������һ����ҳʱ�ٶȲ��죬���ٴδ�ʱ�ͻ��ܶ࣬ԭ����������Ѿ��Դ�ҳ������˻��棬��ô��ͬһ������������ٴδ򿪴�ҳʱ�������´ӷ������˻�ȡ����ҳ�Ļ�������HTTP��Ϣͷ�еġ�Cache-control�������Ƶģ�������ȡֵ��private��no-cache��max-age��must-revalidate�ȣ�Ĭ��Ϊprivate�������ø��ݲ�ͬ�����������ʽ��Ϊ���¼��������
��1�� ���´���
���ָ��cache-control��ֵΪprivate��no-cache��must-revalidate����ô���´��ڷ���ʱ�������·��ʷ������������ָ����max-ageֵ����ô�ڴ�ֵ�ڵ�ʱ����Ͳ������·��ʷ����������磺
Cache-control: max-age=5
��ʾ�����ʴ���ҳ���5�����ٴη��ʲ���ȥ������
��2�� �ڵ�ַ���س�
���ֵΪprivate��must-revalidate��������˵�Ĳ�һ��������ֻ�е�һ�η���ʱ����ʷ��������Ժ�Ͳ��ٷ��ʡ����ֵΪno-cache����ôÿ�ζ�����ʡ����ֵΪmax-age�����ڹ���֮ǰ�����ظ����ʡ�
��3�� �����˰�Ť
���ֵΪprivate��must-revalidate��max-age���򲻻��ط��ʣ������Ϊno-cache����ÿ�ζ��ظ�����
��4�� ��ˢ�°�Ť
����Ϊ��ֵ�������ظ�����

��ָ��Cache-controlֵΪ��no-cache��ʱ�����ʴ�ҳ�治����Internet��ʱ���¼�����ҳ�汸�ݡ�
���⣬ͨ��ָ����Expires��ֵҲ��Ӱ�쵽���档���磬ָ��ExpiresֵΪһ�����ѹ�ȥ��ʱ�䣬��ô���ʴ���ʱ���ظ��ڵ�ַ�����س�����ôÿ�ζ����ظ����ʣ�
Expires: Fri, 31 Dec 1999 16:00:00 GMT

��ASP�У�����ͨ��Response�����Expires��ExpiresAbsolute���Կ���Expiresֵ��ͨ��Response�����CacheControl���Կ���Cache-control��ֵ�����磺
Response.ExpiresAbsolute = #2000-1-1# ' ָ�����ԵĹ���ʱ�䣬���ʱ���õ��Ƿ���������ʱ�䣬�ᱻ�Զ�ת��ΪGMTʱ��
Response.Expires = 20  ' ָ����ԵĹ���ʱ�䣬�Է���Ϊ��λ����ʾ�ӵ�ǰʱ��������ٷ��ӹ��ڡ�
Response.CacheControl = "no-cache" 

















HTTPЭ��:
����
HTTP ��һ������Ӧ�ò����������Э�飬�������ݡ����ٵķ�ʽ�������ڷֲ�ʽ��ý����Ϣϵͳ������1990����������������ʹ���뷢
չ���õ����ϵ����ƺ� ��չ��Ŀǰ��WWW��ʹ�õ���HTTP/1.0�ĵ����棬HTTP/1.1�Ĺ淶���������ڽ���֮�У�����HTTP-NG(Next Generation
of HTTP)�Ľ����Ѿ������
HTTPЭ�����Ҫ�ص�ɸ������£�
1.֧�ֿͻ�/������ģʽ��
2.�򵥿��٣��ͻ���������������ʱ��ֻ�贫�����󷽷���·�������󷽷����õ���GET��HEAD��POST��ÿ�ַ����涨�˿ͻ����������ϵ�����Ͳ�ͬ������HTTPЭ��򵥣�ʹ��HTTP�������ĳ����ģС�����ͨ���ٶȺܿ졣
3.��HTTP�������������͵����ݶ������ڴ����������Content-Type���Ա�ǡ�
4.�����ӣ������ӵĺ���������ÿ������ֻ����һ�����󡣷�����������ͻ������󣬲��յ��ͻ���Ӧ��󣬼��Ͽ����ӡ��������ַ�ʽ���Խ�ʡ����ʱ�䡣
5.��״̬��HTTPЭ������״̬Э�顣��״̬��ָЭ�����������û�м���������ȱ��״̬��ζ���������������Ҫǰ�����Ϣ�����������ش����������ܵ���ÿ�����Ӵ��͵�������������һ���棬�ڷ���������Ҫ��ǰ��Ϣʱ����Ӧ��ͽϿ졣
 
һ��HTTPЭ�����֮URLƪ
http�����ı�����Э�飩��һ��������������Ӧģʽ�ġ���״̬�ġ�Ӧ�ò��Э�飬������TCP�����ӷ�ʽ��HTTP1.1�汾�и���һ�ֳ������ӵĻ��ƣ����������Web���������ǹ�����HTTPЭ��֮�ϵ�WebӦ�á�
HTTP URL (URL��һ���������͵�URI�����������ڲ���ĳ����Դ���㹻����Ϣ)�ĸ�ʽ���£�
http://host[":"port][abs_path]
http��ʾҪͨ��HTTPЭ������λ������Դ��host��ʾ�Ϸ���Internet������������IP��ַ��portָ��һ���˿ںţ�Ϊ����ʹ��ȱʡ�˿� 80��abs_pathָ��������Դ��URI�����URL��û�и���abs_path����ô������Ϊ����URIʱ��������"/"����ʽ������ͨ��������� ������Զ���������ɡ�
eg:
1�����룺www.guet.edu.cn
������Զ�ת���ɣ�http://www.guet.edu.cn/
2��http:192.168.0.116:8080/index.jsp 
 
����HTTPЭ�����֮����ƪ
    http��������������ɣ��ֱ��ǣ������С���Ϣ��ͷ(�ֽ�����ͷ)����������
1����������һ���������ſ�ͷ���Կո�ֿ���������������URI��Э��İ汾����ʽ���£�Method Request-URI HTTP-Version CRLF  
���� Method��ʾ���󷽷���Request-URI��һ��ͳһ��Դ��ʶ����HTTP-Version��ʾ�����HTTPЭ��汾��CRLF��ʾ�س��ͻ��У�������Ϊ��
β��CRLF�⣬��������ֵ�����CR��LF�ַ�����

���󷽷������з���ȫΪ��д���ж��֣����������Ľ������£�
GET     �����ȡRequest-URI����ʶ����Դ
POST    ��Request-URI����ʶ����Դ�󸽼��µ�����
HEAD    �����ȡ��Request-URI����ʶ����Դ����Ӧ��Ϣ��ͷ
PUT     ����������洢һ����Դ������Request-URI��Ϊ���ʶ
DELETE  ���������ɾ��Request-URI����ʶ����Դ
TRACE   ��������������յ���������Ϣ����Ҫ���ڲ��Ի����
CONNECT ��������ʹ��
OPTIONS �����ѯ�����������ܣ����߲�ѯ����Դ��ص�ѡ�������
Ӧ�þ�����
GET��������������ĵ�ַ����������ַ�ķ�ʽ������ҳʱ�����������GET�������������ȡ��Դ��eg:GET /form.html HTTP/1.1 (CRLF)
POST����Ҫ��������������ܸ��������������ݣ��������ύ����
eg��POST /reg.jsp HTTP/ (CRLF)                                       ������
Accept:image/gif,image/x-xbit,... (CRLF)                             ���浽CRLF�е�Ϊ����ͷ
...
HOST:www.guet.edu.cn (CRLF)
Content-Length:22 (CRLF)
Connection:Keep-Alive (CRLF)
Cache-Control:no-cache (CRLF)
(CRLF)         //��CRLF��ʾ��Ϣ��ͷ�Ѿ��������ڴ�֮ǰΪ��Ϣ��ͷ
user=jeffrey&pwd=1234  //��������Ϊ�ύ������                        ��������(POST���������ģ�GETû��)
HEAD������GET����������һ���ģ�����HEAD����Ļ�Ӧ������ ˵������HTTPͷ���а�������Ϣ��ͨ��GET�������õ�����Ϣ����ͬ�ġ�����������������ش���������Դ���ݣ��Ϳ��Եõ�Request-URI����ʶ ����Դ����Ϣ���÷��������ڲ��Գ����ӵ���Ч�ԣ��Ƿ���Է��ʣ��Լ�����Ƿ���¡�
2������ͷ����
3����������(��) 
 
����HTTPЭ�����֮��Ӧƪ
    �ڽ��պͽ���������Ϣ�󣬷���������һ��HTTP��Ӧ��Ϣ��
HTTP��ӦҲ��������������ɣ��ֱ��ǣ�״̬��(��Ӧ��)����Ϣ��ͷ(��Ӧͷ��)����Ӧ����
1��״̬�и�ʽ���£�
HTTP-Version Status-Code Reason-Phrase CRLF
���У�HTTP-Version��ʾ������HTTPЭ��İ汾��Status-Code��ʾ���������ص���Ӧ״̬���룻Reason-Phrase��ʾ״̬������ı�������
״̬��������λ������ɣ���һ�����ֶ�������Ӧ������������ֿ���ȡֵ��
1xx��ָʾ��Ϣ--��ʾ�����ѽ��գ���������
2xx���ɹ�--��ʾ�����ѱ��ɹ����ա���⡢����
3xx���ض���--Ҫ������������и���һ���Ĳ���
4xx���ͻ��˴���--�������﷨����������޷�ʵ��
5xx���������˴���--������δ��ʵ�ֺϷ�������
����״̬���롢״̬������˵����
200 OK      //�ͻ�������ɹ�
400 Bad Request  //�ͻ����������﷨���󣬲��ܱ������������
401 Unauthorized //����δ����Ȩ�����״̬��������WWW-Authenticate��ͷ��һ��ʹ�� 
403 Forbidden  //�������յ����󣬵��Ǿܾ��ṩ����
404 Not Found  //������Դ�����ڣ�eg�������˴����URL
500 Internal Server Error //��������������Ԥ�ڵĴ���
503 Server Unavailable  //��������ǰ���ܴ���ͻ��˵�����һ��ʱ�����ָܻ�����
eg��HTTP/1.1 200 OK ��CRLF��
2����Ӧ��ͷ����
3����Ӧ���ľ��Ƿ��������ص���Դ������ 
 
�ġ�HTTPЭ�����֮��Ϣ��ͷƪ
    HTTP��Ϣ�ɿͻ��˵�������������ͷ��������ͻ��˵���Ӧ��ɡ�������Ϣ����Ӧ��Ϣ�����ɿ�ʼ�У�����������Ϣ����ʼ�о��������У�������Ӧ��Ϣ����ʼ�о���״̬�У�����Ϣ��ͷ����ѡ�������У�ֻ��CRLF���У�����Ϣ���ģ���ѡ����ɡ�
HTTP��Ϣ��ͷ������ͨ��ͷ������ͷ����Ӧ��ͷ��ʵ�屨ͷ��
ÿһ����ͷ����������+"��"+�ո�+ֵ ��ɣ���Ϣ��ͷ��������Ǵ�Сд�޹صġ�
1����ͨ��ͷ
����ͨ��ͷ�У���������ͷ���������е��������Ӧ��Ϣ�����������ڱ������ʵ�壬ֻ���ڴ������Ϣ��
eg��
Cache-Control   ����ָ������ָ�����ָ���ǵ���ģ���Ӧ�г��ֵĻ���ָ����������δ�ػ���֣������Ƕ����ģ�һ����Ϣ�Ļ���ָ���Ӱ����һ����Ϣ����Ļ�����ƣ���HTTP1.0ʹ�õ����Ƶı�ͷ��ΪPragma��
����ʱ�Ļ���ָ�������no-cache������ָʾ�������Ӧ��Ϣ���ܻ��棩��no-store��max-age��max-stale��min-fresh��only-if-cached;
��Ӧʱ�Ļ���ָ�������public��private��no-cache��no-store��no-transform��must-revalidate��proxy-revalidate��max-age��s-maxage.
eg��Ϊ��ָʾIE��������ͻ��ˣ���Ҫ����ҳ�棬�������˵�JSP������Ա�д���£�response.sehHeader("Cache-Control","no-cache");
//response.setHeader("Pragma","no-cache");�����൱���������룬ͨ������//����
�����뽫�ڷ��͵���Ӧ��Ϣ��������ͨ��ͷ��Cache-Control:no-cache

Date��ͨ��ͷ���ʾ��Ϣ���������ں�ʱ��
Connection��ͨ��ͷ��������ָ�����ӵ�ѡ�����ָ������������������ָ��"close"ѡ�֪ͨ������������Ӧ��ɺ󣬹ر�����
2������ͷ
����ͷ����ͻ�����������˴�������ĸ�����Ϣ�Լ��ͻ����������Ϣ��
���õ�����ͷ
Accept
Accept����ͷ������ָ���ͻ��˽�����Щ���͵���Ϣ��eg��Accept��image/gif�������ͻ���ϣ������GIFͼ���ʽ����Դ��Accept��text/html�������ͻ���ϣ������html�ı���
Accept-Charset
Accept-Charset����ͷ������ָ���ͻ��˽��ܵ��ַ�����eg��Accept-Charset:iso-8859-1,gb2312.�����������Ϣ��û�����������ȱʡ���κ��ַ��������Խ��ܡ�
Accept-Encoding
Accept-Encoding����ͷ��������Accept��������������ָ���ɽ��ܵ����ݱ��롣eg��Accept-Encoding:gzip.deflate.���������Ϣ��û�����������������ٶ��ͻ��˶Ը������ݱ��붼���Խ��ܡ�
Accept-Language
Accept-Language����ͷ��������Accept��������������ָ��һ����Ȼ���ԡ�eg��Accept-Language:zh-cn.���������Ϣ��û�����������ͷ�򣬷������ٶ��ͻ��˶Ը������Զ����Խ��ܡ�
Authorization
Authorization����ͷ����Ҫ����֤���ͻ�����Ȩ�鿴ĳ����Դ�������������һ��ҳ��ʱ������յ�����������Ӧ����Ϊ401��δ��Ȩ�������Է���һ������Authorization����ͷ�������Ҫ����������������֤��
Host����������ʱ���ñ�ͷ���Ǳ���ģ�
Host����ͷ����Ҫ����ָ����������Դ��Internet�����Ͷ˿ںţ���ͨ����HTTP URL����ȡ�����ģ�eg��
����������������룺http://www.guet.edu.cn/index.html
��������͵�������Ϣ�У��ͻ����Host����ͷ�����£�
Host��www.guet.edu.cn
�˴�ʹ��ȱʡ�˿ں�80����ָ���˶˿ںţ����ɣ�Host��www.guet.edu.cn:ָ���˿ں�
User-Agent
����������½��̳��ʱ�������ῴ��һЩ��ӭ��Ϣ�������г�����Ĳ���ϵͳ�����ƺͰ汾������ʹ�õ�����������ƺͰ汾���������úܶ��˸е������棬ʵ�� �ϣ�������Ӧ�ó�����Ǵ�User-Agent�������ͷ���л�ȡ����Щ��Ϣ��User-Agent����ͷ������ͻ��˽����Ĳ���ϵͳ������������� ���Ը��߷������������������ͷ���Ǳ���ģ���������Լ���дһ�����������ʹ��User-Agent����ͷ����ô�������˾��޷���֪���ǵ���Ϣ �ˡ�
����ͷ������
GET /form.html HTTP/1.1 (CRLF)
Accept:image/gif,image/x-xbitmap,image/jpeg,application/x-shockwave-flash,application/vnd.ms-excel,application/vnd.ms-powerpoint,application/msword,* / * (CRLF)
Accept-Language:zh-cn (CRLF)
Accept-Encoding:gzip,deflate (CRLF)
If-Modified-Since:Wed,05 Jan 2007 11:21:25 GMT (CRLF)
If-None-Match:W/"80b1a4c018f3c41:8317" (CRLF)
User-Agent:Mozilla/4.0(compatible;MSIE6.0;Windows NT 5.0) (CRLF)
Host:www.guet.edu.cn (CRLF)
Connection:Keep-Alive (CRLF)
(CRLF)
3����Ӧ��ͷ
��Ӧ��ͷ������������ݲ��ܷ���״̬���еĸ�����Ӧ��Ϣ���Լ����ڷ���������Ϣ�Ͷ�Request-URI����ʶ����Դ������һ�����ʵ���Ϣ��
���õ���Ӧ��ͷ
Location
Location��Ӧ��ͷ�������ض�������ߵ�һ���µ�λ�á�Location��Ӧ��ͷ�����ڸ���������ʱ��
Server
Server��Ӧ��ͷ������˷�����������������������Ϣ����User-Agent����ͷ�������Ӧ�ġ�������
Server��Ӧ��ͷ���һ�����ӣ�
Server��Apache-Coyote/1.1
WWW-Authenticate
WWW-Authenticate��Ӧ��ͷ����뱻������401��δ��Ȩ�ģ���Ӧ��Ϣ�У��ͻ����յ�401��Ӧ��Ϣʱ�򣬲�����Authorization��ͷ��������������������֤ʱ���������Ӧ��ͷ�Ͱ����ñ�ͷ��
eg��WWW-Authenticate:Basic realm="Basic Auth Test!"  //���Կ�����������������Դ���õ��ǻ�����֤���ơ�

4��ʵ�屨ͷ
�������Ӧ��Ϣ�����Դ���һ��ʵ�塣һ��ʵ����ʵ�屨ͷ���ʵ��������ɣ���������˵ʵ�屨ͷ���ʵ������Ҫ��һ���ͣ�����ֻ����ʵ�屨ͷ��ʵ�屨ͷ�����˹���ʵ�����ģ�eg������ʵ�����ģ�����������ʶ����Դ��Ԫ��Ϣ��
���õ�ʵ�屨ͷ
Content-Encoding
Content-Encodingʵ�屨ͷ������ý�����͵����η�������ֵָʾ���Ѿ���Ӧ�õ�ʵ�����ĵĸ������ݵı��룬���Ҫ���Content- Type��ͷ���������õ�ý�����ͣ����������Ӧ�Ľ�����ơ�Content-Encoding�������ڼ�¼�ĵ���ѹ��������eg��Content- Encoding��gzip
Content-Language
Content-Languageʵ�屨ͷ����������Դ���õ���Ȼ���ԡ�û�����ø�������Ϊʵ�����ݽ��ṩ�����е������Ķ�
�ߡ�eg��Content-Language:da
Content-Length
Content-Lengthʵ�屨ͷ������ָ��ʵ�����ĵĳ��ȣ����ֽڷ�ʽ�洢��ʮ������������ʾ��
Content-Type
Content-Typeʵ�屨ͷ������ָ�����͸������ߵ�ʵ�����ĵ�ý�����͡�eg��
Content-Type:text/html;charset=ISO-8859-1
Content-Type:text/html;charset=GB2312
Last-Modified
Last-Modifiedʵ�屨ͷ������ָʾ��Դ������޸����ں�ʱ�䡣
Expires
Expiresʵ�屨ͷ�������Ӧ���ڵ����ں�ʱ�䡣Ϊ���ô�����������������һ��ʱ���Ժ���»�����(�ٴη��������ʹ���ҳ��ʱ��ֱ�Ӵӻ����м��أ� ������Ӧʱ��ͽ��ͷ���������)��ҳ�棬���ǿ���ʹ��Expiresʵ�屨ͷ��ָ��ҳ����ڵ�ʱ�䡣eg��Expires��Thu��15 Sep 2006 16:23:12 GMT
HTTP1.1�Ŀͻ��˺ͻ�����뽫�����Ƿ������ڸ�ʽ������0�������Ѿ����ڡ�eg��Ϊ�����������Ҫ����ҳ�棬����Ҳ��������Expiresʵ�屨ͷ������Ϊ0��jsp�г������£�response.setDateHeader("Expires","0");
 
�塢����telnet�۲�httpЭ���ͨѶ����
    ʵ��Ŀ�ļ�ԭ��
    ����MS��telnet���ߣ�ͨ���ֶ�����http������Ϣ�ķ�ʽ����������������󣬷��������ա����ͺͽ�������󣬻᷵��һ����Ӧ������Ӧ����telnet��������ʾ�������Ӷ��Ӹ����ϼ����httpЭ���ͨѶ���̵���ʶ��
    ʵ�鲽�裺
1����telnet
1.1 ��telnet
����-->cmd-->telnet
1.2 ��telnet���Թ���
set localecho
2�����ӷ���������������
2.1 open www.guet.edu.cn 80  //ע��˿ںŲ���ʡ��
    HEAD /index.asp HTTP/1.0
    Host:www.guet.edu.cn
    
   /*���ǿ��Ա任���󷽷�,������ֵ�����ҳ����,������Ϣ���� 
    open www.guet.edu.cn 80 
   
    GET /index.asp HTTP/1.0  //������Դ������
    Host:www.guet.edu.cn  
2.2 open www.sina.com.cn 80  //��������ʾ������ֱ������telnet www.sina.com.cn 80
    HEAD /index.asp HTTP/1.0
    Host:www.sina.com.cn
 
3 ʵ������
3.1 ������Ϣ2.1�õ�����Ӧ��:
HTTP/1.1 200 OK                                              //����ɹ�
Server: Microsoft-IIS/5.0                                    //web������
Date: Thu,08 Mar 200707:17:51 GMT
Connection: Keep-Alive                                 
Content-Length: 23330
Content-Type: text/html
Expries: Thu,08 Mar 2007 07:16:51 GMT
Set-Cookie: ASPSESSIONIDQAQBQQQB=BEJCDGKADEDJKLKKAJEOIMMH; path=/
Cache-control: private
//��Դ����ʡ��
3.2 ������Ϣ2.2�õ�����Ӧ��:
HTTP/1.0 404 Not Found       //����ʧ��
Date: Thu, 08 Mar 2007 07:50:50 GMT
Server: Apache/2.0.54 <Unix>
Last-Modified: Thu, 30 Nov 2006 11:35:41 GMT
ETag: "6277a-415-e7c76980"
Accept-Ranges: bytes
X-Powered-By: mod_xlayout_jh/0.0.1vhs.markII.remix
Vary: Accept-Encoding
Content-Type: text/html
X-Cache: MISS from zjm152-78.sina.com.cn
Via: 1.0 zjm152-78.sina.com.cn:80<squid/2.6.STABLES-20061207>
X-Cache: MISS from th-143.sina.com.cn
Connection: close

ʧȥ�˸�����������
�����������...
4 .ע�����1������������������󲻻�ɹ���
          2����ͷ�򲻷ִ�Сд��
          3������һ���˽�HTTPЭ�飬���Բ鿴RFC2616����http://www.letf.org/rfc���ҵ����ļ���
          4��������̨�����������httpЭ��
����HTTPЭ����ؼ�������
    1��������
    �߲�Э���У��ļ�����Э��FTP�������ʼ�����Э��SMTP������ϵͳ����DNS���������Ŵ���Э��NNTP��HTTPЭ���
�н������֣�����(Proxy)������(Gateway)��ͨ��(Tunnel)��һ���������URI�ľ��Ը�ʽ������������дȫ���򲿷���Ϣ��ͨ�� URI�ı�ʶ���Ѹ�ʽ�����������͵���������������һ�����մ�����ΪһЩ�������������ϲ㣬�����������Ļ������԰���������²�ķ�����Э�顣һ ��ͨ����Ϊ���ı���Ϣ����������֮����м̵㡣��ͨѶ��Ҫͨ��һ���н�(���磺����ǽ��)�������н鲻��ʶ����Ϣ������ʱ��ͨ��������ʹ�á�
     ����(Proxy)��һ���м���������Գ䵱һ����������Ҳ���Գ䵱һ���ͻ�����Ϊ�����ͻ�����������������ͨ�����ܵķ������ڲ��򾭹����ݵ������� �������С�һ�������ڷ���������Ϣ֮ǰ��������Ͳ������������д������������Ϊͨ������ǽ�Ŀͻ����˵��Ż�������������Ϊһ������Ӧ����ͨ��Э�鴦 ��û�б��û�������ɵ�����
����(Gateway)��һ����Ϊ�����������м�ý��ķ������������ͬ���ǣ����ؽ�������ͺ���Ա��������Դ��˵������Դ����������������Ŀͻ�����û����ʶ������ͬ���ش򽻵���
���ؾ�����Ϊͨ������ǽ�ķ������˵��Ż������ػ�������Ϊһ��Э�鷭�����Ա��ȡ��Щ�洢�ڷ�HTTPϵͳ�е���Դ��
    ͨ��(Tunnel)������Ϊ���������м̵��н����һ�����ͨ���㱻��Ϊ������HTTPͨѶ������ͨ�������Ǳ�һ��HTTP�����ʼ���ġ������м� ���������˹ر�ʱ��ͨ������ʧ����һ���Ż�(Portal)������ڻ��н�(Intermediary)���ܽ����м̵�ͨѶʱͨ��������ʹ�á�

2��Э�����������-HTTP������������繥��
��ģ�黯�ķ�ʽ�Ը߲�Э����з�����������δ�����ּ��ķ���
HTTP�������ĳ��ö˿�80��3128��8080��network������port��ǩ�����˹涨

3��HTTPЭ��Content Lenth����©�����¾ܾ����񹥻�
ʹ��POST����ʱ����������ContentLenth��������Ҫ���͵����ݳ��ȣ�����ContentLenth:999999999���ڴ������ǰ���� �治���ͷţ������߿����������ȱ�ݣ�������WEB������������������ֱ��WEB�������ڴ�ľ������ֹ������������������ºۼ���
http://www.cnpaf.net/Class/HTTP/0532918532667330.html

4������HTTPЭ������Խ��оܾ����񹥻���һЩ��˼
��������æ�ڴ�������α���TCP�����������Ͼ��ǿͻ����������󣨱Ͼ��ͻ��˵�����������ʷǳ�֮С������ʱ�������ͻ��ĽǶȿ�����������ʧȥ��Ӧ������������ǳ��������������ܵ���SYNFlood������SYN��ˮ��������
��Smurf��TearDrop��������ICMP������Flood��IP��Ƭ�����ġ�������"��������"�ķ����������ܾ����񹥻���
19�˿��������Ѿ�����������Chargen�����ˣ���Chargen_Denial_of_Service�����ǣ������õķ���������̨Chargen ������֮�����UDP���ӣ��÷��������������Ϣ��DOWN������ô���ɵ�һ̨WEB�������������ͱ�����2����1.��Chargen����2.��HTTP ����
������������α��ԴIP��N̨Chargen������������Connect����Chargen���յ����Ӻ�ͻ᷵��ÿ��72�ֽڵ��ַ�����ʵ���ϸ�������ʵ�����������ٶȸ��죩����������

5��Httpָ��ʶ����
   Httpָ��ʶ���ԭ�������Ҳ����ͬ�ģ���¼��ͬ��������HttpЭ��ִ���е�΢С������ʶ��.Httpָ��ʶ���TCP/IP��ջָ��ʶ������ ��,�����Ƕ���Http�������������ļ������Ӳ�������ʹ�ø���Http����Ӧ��Ϣ��ĺ�����,����ʹ��ʶ�������ѣ�Ȼ������TCP/IP��ջ����Ϊ ��Ҫ�Ժ��Ĳ�����޸�,���Ծ�����ʶ��.
      Ҫ�÷��������ز�ͬ��Banner��Ϣ�������Ǻܼ򵥵�,��Apache�����Ŀ���Դ�����Http������,�û�������Դ�������޸�Banner��Ϣ,Ȼ ������Http�������Ч�ˣ�����û�й���Դ�����Http����������΢���IIS������Netscape,�����ڴ��Banner��Ϣ��Dll�ļ����� ��,��ص����������۵�,���ﲻ��׸��,��Ȼ�������޸ĵ�Ч�����ǲ����.����һ��ģ��Banner��Ϣ�ķ�����ʹ�ò����
���ò�������
1��HEAD/Http/1.0���ͻ�����Http����
2��DELETE/Http/1.0������Щ�������������,����Delete����
3��GET/Http/3.0����һ���Ƿ��汾��HttpЭ������
4��GET/JUNK/1.0����һ������ȷ����HttpЭ������
Httpָ��ʶ�𹤾�Httprint,��ͨ������ͳ��ѧԭ��,���ģ�����߼�ѧ����,�ܺ���Ч��ȷ��Http������������.�����Ա������ռ��ͷ�����ͬHttp������������ǩ����

6��������Ϊ������û�ʹ�������ʱ�����ܣ��ִ��������֧�ֲ����ķ��ʷ�ʽ�����һ����ҳʱͬʱ����������ӣ���Ѹ�ٻ��һ����ҳ�ϵĶ��ͼ�꣬�����ܸ��������������ҳ�Ĵ��䡣
HTTP1.1���ṩ�����ֳ������ӵķ�ʽ������һ��HTTPЭ�飺HTTP-NG���������йػỰ���ơ��ḻ������Э�̵ȷ�ʽ��֧�֣����ṩ
����Ч�ʵ����ӡ�
 
��л�롤ŵ��������.���������������ϵĵ�һ̨�����,��ʹ��������Щ������ǹ����,��"�������ݺ�"��"ѧ������"����Ϊ"���Ӽ�����"��"ѧ������".����
      ��л���������ʦ.����Ҳ��������дΪ����������,����ֻ��������������--��֪��д��ʲô��֪�����ο����׶�����ô������.֮�������ͨ���˴��.��������˺Ͱ����׵���ʦ,����������ƽ��������ʦ,���������ƽ�׽��˶���ΰ�����ʦ. 








ΪʲôҪ��URI���б���:

����LOFTER�ͻ��� 
Ϊʲô��ҪUrl���룬ͨ�����һ��������Ҫ���룬˵���������������ʺϴ��䡣ԭ����ֶ�������Size���󣬰�����˽���ݣ�����Url��˵��
֮�� ��Ҫ���б��룬����ΪUrl����Щ�ַ����������塣 ����Url�����ַ�����ʹ��key=value��ֵ����������ʽ�����Σ���ֵ��֮����&���ŷָ���
��/s?q=abc&ie=utf- 8��������value�ַ����а�����=����&����ô�Ʊػ���ɽ���Url�ķ���������������˱��뽫���������&��=���Ž���ת�壬 
Ҳ���Ƕ�����б��롣 ���磬Url�ı����ʽ���õ���ASCII�룬������Unicode����Ҳ����˵�㲻����Url�а����κη�ASCII�ַ����������ġ�
��������ͻ���� �����ͷ���������֧�ֵ��ַ�����ͬ������£����Ŀ��ܻ�������⡣

Url�����ԭ�����ʹ�ð�ȫ���ַ���û��������;������������Ŀɴ�ӡ�ַ���ȥ��ʾ��Щ����ȫ���ַ��� ��Щ�ַ���Ҫ���� RFC3986�ĵ��涨��
Url��ֻ�������Ӣ����ĸ��a-zA-Z�������֣�0-9����-_.~4�������ַ��Լ����б����ַ��� RFC3986�ĵ���Url�ı����������������ϸ�Ľ��飬
ָ������Щ�ַ���Ҫ������Ų�������Url�����ת�䣬�Լ���Ϊʲô��Щ�ַ���Ҫ������������ Ӧ�Ľ��͡� US-ASCII�ַ�����û�ж�Ӧ�Ŀɴ�ӡ
�ַ� Url��ֻ����ʹ�ÿɴ�ӡ�ַ���US-ASCII���е�10-7F�ֽ�ȫ����ʾ�����ַ�����Щ�ַ�������ֱ�ӳ�����Url�С�ͬʱ������80-FF�ֽ�
��ISO-8859-1���������Ѿ�������US-ACII������ֽڷ�Χ�����Ҳ�����Է���Url�С�

�����ַ� Url���Ի��ֳ����ɸ������Э�顢������·���ȡ���һЩ�ַ���:/?#[]@���������ָ���ͬ����ġ�����:ð�����ڷָ�Э���������
/���ڷָ������� ·����?���ڷָ�·���Ͳ�ѯ�������ȵȡ�����һЩ�ַ���!$&��()*+,;=��������ÿ��������𵽷ָ����õģ���=���ڱ�ʾ
��ѯ�����еļ�ֵ �ԣ�&�������ڷָ���ѯ�����ֵ�ԡ�������е���ͨ���ݰ�����Щ�����ַ�ʱ����Ҫ������б��롣

RFC3986��ָ���������ַ�Ϊ�����ַ��� ! * �� ( ) ; : @ & = + $ , / ? # [ ] ����ȫ�ַ� ����һЩ�ַ���������ֱ�ӷ���Url�е�ʱ��
���ܻ����������������塣��Щ�ַ�����Ϊ����ȫ�ַ���ԭ���кܶࡣ �ո� Url�ڴ���Ĺ��̣������û����Ű�Ĺ��̣������ı��������
�ڴ���Url�Ĺ��̣����п��������޹ؽ�Ҫ�Ŀո񣬻��߽���Щ������Ŀո��ȥ�� �����Լ�<> ���źͼ�����ͨ����������ͨ�ı����𵽷ָ�
Url������ # ͨ�����ڱ�ʾ��ǩ����ê�� % �ٷֺű��������Բ���ȫ�ַ����б���ʱʹ�õ������ַ�����˱�����Ҫ���� {}|\^[]`~ ĳһЩ
���ػ��ߴ�������۸���Щ�ַ� 

��Ҫע����ǣ�����Url�еĺϷ��ַ�������Ͳ������ǵȼ۵ģ����Ƕ��������ᵽ����Щ�ַ���������������룬��ô�����п��ܻ����Url����
�Ĳ�ͬ���� �˶���Url���ԣ�ֻ����ͨӢ���ַ������֣������ַ�$-_.+!*��()���б����ַ������ܳ�����δ�������Url֮�С������ַ�����Ҫ
��������֮��� �ܳ�����Url�С�

Javascript���ṩ��3�Ժ���������Url�����Եõ��Ϸ���Url�����Ƿֱ���escape / unescape,encodeURI / decodeURI��encodeURIComponent / 
decodeURIComponent�����ڽ���ͱ���Ĺ����ǿ���ģ��������ֻ���ͱ���Ĺ��̡� ����������ĺ����D�Descape��encodeURI��encodeURIComponent�D�D
�������ڽ�����ȫ���Ϸ���Url�ַ�ת��Ϊ�Ϸ��� Url�ַ���ʾ�����������¼�����ͬ�㡣 

��ȫ�ַ���ͬ ����ı���г��������������İ�ȫ�ַ����������������Щ�ַ����б��룩 ��ȫ�ַ� escape��69���� * /@+-._0-9a-zA-Z 
encodeURI��82���� !#$&��()*+,/:;=?@-._~0-9a-zA-Z encodeURIComponent��71���� !��()*-._~0-9a-zA-Z �����Բ�ͬ escape������
��Javascript1.0��ʱ��ʹ����ˣ�����������������Javascript1.5������ġ��������� Javascript1.5�Ѿ��ǳ��ռ��ˣ�����ʵ����
ʹ��encodeURI��encodeURIComponent��������ʲô���������⡣

��Unicode�ַ��ı��뷽ʽ��ͬ ��������������ASCII�ַ��ı��뷽ʽ��ͬ������ʹ�ðٷֺ�+��λʮ�������ַ�����ʾ�����Ƕ���Unicode�ַ���
escape�ı��뷽ʽ ��%uxxxx�����е�xxxx��������ʾunicode�ַ���4λʮ�������ַ������ַ�ʽ�Ѿ���W3C�����ˡ�������ECMA-262��׼����
Ȼ������ escape�����ֱ����﷨��encodeURI��encodeURIComponent��ʹ��UTF-8�Է�ASCII�ַ����б��룬Ȼ���ٽ��аٷֺ� ���롣����RFC
�Ƽ��ġ���˽��龡���ܵ�ʹ���������������escape���б��롣

���ó��ϲ�ͬ encodeURI��������һ��������URI���б��룬��encodeURIComponent��������URI��һ��������б��롣 �������ᵽ�İ�ȫ�ַ�
��Χ������������ǻᷢ�֣�encodeURIComponent������ַ���ΧҪ��encodeURI�Ĵ����������ᵽ�������� �ַ�һ���������ָ�URI���
��һ��URI���Ա��и�ɶ��������ο�Ԥ��֪ʶһ�ڣ��������������URI�в�ѯ�����ķָ���������:�����ڷָ� scheme��������?������
�ָ�������·��������encodeURI���ݵĶ�����һ�������ĵ�URI����Щ�ַ���URI�б�������������;�������Щ�� ���ַ����ᱻencodeURI
���룬��������ͱ��ˡ� ����ڲ����Լ������ݱ�ʾ��ʽ��������Щ�����ڲ����ܰ����зָ�����ı����ַ�������ͻᵼ������URI�����
�ķָ����ҡ���˶��ڵ������ʹ�� encodeURIComponent����Ҫ������ַ��͸����ˡ� 










��Щ��Ϣ�ڴ洢ʱ��������Ҫռ��һ���������ֽڣ���ֻ��ռ������һ��������λ�������ڴ��һ��������ʱ��ֻ��0��1 ����״̬����һλ����λ���ɡ�Ϊ�˽� ʡ�洢�ռ䣬��ʹ�����㣬C�������ṩ��һ�����ݽṹ����Ϊ��λ�򡱻�λ�Ρ�����ν��λ���ǰ�һ���ֽ��еĶ���λ����Ϊ������ͬ�����򣬲�˵��ÿ�� �����λ����ÿ������һ�������������ڳ����а��������в����������Ϳ��԰Ѽ�����ͬ�Ķ�����һ���ֽڵĶ�����λ������ʾ��һ��λ��Ķ����λ�������˵�� λ������ṹ������£�����ʽΪ��

struct λ��ṹ��

{ λ���б� };

����λ���б����ʽΪ�� ����˵���� λ������λ�򳤶�
���磺
struct bs
{
int a:8;
int b:2;
int c:6;
};

λ�������˵����ṹ����˵���ķ�ʽ��ͬ�� �ɲ����ȶ����˵����ͬʱ����˵������ֱ��˵�������ַ�ʽ�����磺

struct bs
{
int a:8;
int b:2;
int c:6;
}data;

˵��dataΪbs��������ռ�����ֽڡ�����λ��aռ8λ��λ��bռ2λ��λ��cռ6λ������λ��Ķ����������¼���˵����

1. һ��λ�����洢��ͬһ���ֽ��У����ܿ������ֽڡ���һ���ֽ���ʣ�ռ䲻�������һλ��ʱ��Ӧ����һ��Ԫ���Ÿ�λ��Ҳ��������ʹĳλ�����һ��Ԫ��ʼ�����磺

struct bs
{
unsigned a:4
unsigned :0 ����
unsigned b:4 ����һ��Ԫ��ʼ���
unsigned c:4
}

�����λ�����У�aռ��һ�ֽڵ�4λ����4λ��0��ʾ��ʹ�ã�b�ӵڶ��ֽڿ�ʼ��ռ��4λ��cռ��4λ��

2. ����λ������������ֽڣ����λ��ĳ��Ȳ��ܴ���һ���ֽڵĳ��ȣ�Ҳ����˵���ܳ���8λ����λ��

3. λ�������λ��������ʱ��ֻ�������������λ�á�������λ���ǲ���ʹ�õġ����磺

struct k
{
int a:1
int :2 ��2λ����ʹ��
int b:3
int c:2
};

�����Ϸ������Կ�����λ���ڱ����Ͼ���һ�ֽṹ���ͣ� �������Ա�ǰ�����λ����ġ�

����λ���ʹ��

λ���ʹ�úͽṹ��Ա��ʹ����ͬ����һ����ʽΪ�� λ���������λ���� λ�������ø��ָ�ʽ�����

main(){
struct bs
{
unsigned a:1;
unsigned b:3;
unsigned c:4;
} bit,*PBit;
bit.a=1;
bit.b=7;
bit.c=15;
printf("%d,%d,%d/n",bit.a,bit.b,bit.c);
PBit=&bit;
PBit->a=0;
PBit->b&=3;
PBit->c|=1;
printf("%d,%d,%d/n",PBit->a,PBit->b,PBit->c);
}

���������ж�����λ��ṹbs������λ��Ϊa,b,c��˵����bs���͵ı���bit��ָ��bs���͵�ָ�����PBit�����ʾλ��Ҳ�ǿ���ʹ��ָ��ġ�

�����9��10��11���зֱ������λ��ֵ��( Ӧע�⸳ֵ���ܳ�����λ�������Χ)�����12������������ʽ�������������ݡ���13�а�λ�����bit�ĵ�ַ�͸�ָ�����PBit����14����ָ�뷽ʽ��λ��a���¸�ֵ����Ϊ0����15��ʹ���˸��ϵ�λ�����"&="�� �����൱�ڣ� PBit->b=PBit->b&3λ��b��ԭ��ֵΪ7����3����λ������Ľ��Ϊ3(111&011=011,ʮ����ֵΪ 3)��ͬ���������16����ʹ���˸���λ����"|="�� �൱�ڣ� PBit->c=PBit->c|1����Ϊ15�������17����ָ�뷽ʽ��������������ֵ��





configure���

���Կ�����configure����������Ҫ�����Ľ���ϸ�������ʹ��configure���������configure��������ι����ģ���������Ҳ���Կ���Nginx��һЩ���˼�롣

1.5.1��configure���������

ʹ��help������Բ鿴configure�����Ĳ�����

./configure --help

���ﲻһһ�г�help�Ľ����ֻ�ǰ����Ĳ�����Ϊ���Ĵ����ͣ����潫�����������������в������÷������塣

1. ·����صĲ���

��1-2�г���Nginx�ڱ����ڡ�����������·����صĸ��ֲ�����

��1-2��configure֧�ֵ�·����ز���
�������� �⡡�� Ĭ �� ֵ
--prefix=PATH Nginx��װ�����ĸ�Ŀ¼ Ĭ��Ϊ/usr/local/nginxĿ¼��ע�⣺���Ŀ������û�Ӱ�����������е����Ŀ¼�����磬���������--sbin-path=sbin/nginx����ôʵ���Ͽ�ִ���ļ��ᱻ�ŵ�/usr/local/nginx/sbin/nginx��
--sbin-path=PATH ��ִ���ļ��ķ���·�� <prefix>/sbin/nginx
--conf-path=PATH �����ļ��ķ���·�� <prefix>/conf/nginx.conf
--error-log-path=PATH error��־�ļ��ķ���·����error��־���ڶ�λ���⣬��������ּ��𣨰���debug���Լ��𣩵���־���������÷ǳ���������nginx.conf������Ϊ��ͬ�������־���������ͬ��log�ļ��С�������Ĭ�ϵ�Nginx������־·�� <prefix>/logs/error.log
--pid-path=PATH pid�ļ��Ĵ��·��������ļ������ASC II������Nginx master�Ľ���ID�������������ID����ʹ�������У�����nginx -s reload��ͨ����ȡmaster����ID��master���̷����ź�ʱ�����ܶ������е�Nginx����������� <prefix>/logs/nginx.pid
--lock-path=PATH lock�ļ��ķ���·�� <prefix>/logs/nginx.lock
--builddir=DIR configureִ��ʱ������ڼ��������ʱ�ļ����õ�Ŀ¼������������Makefile��CԴ�ļ���Ŀ���ļ�����ִ���ļ��� <nginx source path>/objs
--with-perl_modules_path=PATH perl module���õ�·����ֻ��ʹ���˵�������perl module������Ҫ�������·�� ��
--with-perl=PATH perl binary���õ�·����������õ�Nginx��ִ��Perl�ű�����ô�ͱ���Ҫ���ô�·�� ��
--http-log-path=PATH access��־���õ�λ�á�ÿһ��HTTP�����ڽ���ʱ�����¼�ķ�����־ <prefix>/logs/access.log
--http-client-body-temp-path=PATH ����HTTP����ʱ�������İ�����Ҫ��ʱ��ŵ���ʱ�����ļ��У������������ʱ�ļ����õ���·���� <prefix>/client_body_temp
--http-proxy-temp-path=PATH Nginx��ΪHTTP������������ʱ�����η�����������HTTP��������Ҫ��ʱ��ŵ������ļ�ʱ�����12.8�ڣ�����������ʱ�ļ����ŵ���·���� <prefix>/proxy_temp
--http-fastcgi-temp-path=PATH Fastcgi��ʹ����ʱ�ļ��ķ���Ŀ¼ <prefix>/fastcgi_temp
--http-uwsgi-temp-path=PATH uWSGI��ʹ����ʱ�ļ��ķ���Ŀ¼ <prefix>/uwsgi_temp
--http-scgi-temp-path=PATH SCGI��ʹ����ʱ�ļ��ķ���Ŀ¼ <prefix>/scgi_temp

2. ������صĲ���

��1-3�г��˱���Nginxʱ���������صĲ�����

��1-3��configure֧�ֵı�����ز���
������� �⡡��
--with-cc=PATH C��������·��
--with-cpp=PATH CԤ��������·��
--with-cc-opt=OPTIONS ���ϣ����Nginx�����ڼ�ָ������һЩ����ѡ���ָ�������ʹ��-I����ĳЩ��Ҫ������Ŀ¼����ʱ����ʹ�øò������Ŀ��
--with-ld-opt=OPTIONS ���յĶ����ƿ�ִ���ļ����ɱ�������ɵ�Ŀ���ļ���һЩ���������������ɵģ���ִ�����Ӳ���ʱ���ܻ���Ҫָ�����Ӳ�����--with-ld-opt�������ڼ�������ʱ�Ĳ��������磬�������ϣ����ĳ�������ӵ�Nginx�����У���Ҫ���������--with-ld-opt=-llibraryName -LlibraryPath������libraryName��Ŀ�������ƣ�libraryPath����Ŀ������ڵ�·��
--with-cpu-opt=CPU ָ��CPU�������ܹ���ֻ�ܴ�����ȡֵ��ѡ��pentium��pentiumpro��pentium3��pentium4��athlon��opteron��sparc32��sparc64��ppc64

3. �����������ز���

��1-4����1-8�г���Nginx�����ĳ������֧�ֵĲ�����

��1-4��PCRE�����ò���
PCRE������ò��� �⡡��
--without-pcre ���ȷ��Nginx���ý���������ʽ��Ҳ����˵��nginx.conf�����ļ��в������������ʽ����ô����ʹ���������
--with-pcre ǿ��ʹ��PCRE��
--with-pcre=DIR ָ��PCRE���Դ��λ�ã��ڱ���Nginxʱ������Ŀ¼����PCREԴ��
--with-pcre-opt=OPTIONS ����PCREԴ��ʱϣ������ı���ѡ��

��1-5��OpenSSL�����ò���
OpenSSL������ò��� �⡡��
--with-openssl=DIR ָ��OpenSSL���Դ��λ�ã��ڱ���Nginxʱ������Ŀ¼����OpenSSLԴ��
ע�⣺���Web������֧��HTTPS��Ҳ����SSLЭ�飬NginxҪ�����ʹ��OpenSSL�����Է���http://www.openssl.org/�������
--with-openssl-opt=OPTIONS ����OpenSSLԴ��ʱϣ������ı���ѡ��

��1-6��ԭ�ӿ�����ò���
atomic��ԭ�ӣ�������ò��� �⡡��
--with-libatomic ǿ��ʹ��atomic�⡣atomic����CPU�ܹ�������һ��ԭ�Ӳ�����ʵ�֡���֧��������ϵ�ܹ���x86������i386��x86_64����PPC64��Sparc64��v9����߰汾�����߰�װ��GCC 4.1.0�����߰汾�ļܹ���14.3�ڽ�����ԭ�Ӳ�����Nginx�е�ʵ��
--with-libatomic=DIR atomic�����ڵ�λ��

��1-7��ɢ�к���������ò���
ɢ�к���������ò��� ����
--with-MD5=DIR ָ��MD5���Դ��λ�ã��ڱ���Nginxʱ������Ŀ¼����MD5Դ��
ע�⣺NginxԴ�����Ѿ�����MD5�㷨��ʵ�֣����û������������ô��ȫ����ʹ��Nginx����ʵ�ֵ�MD5�㷨
--with-MD5-opt=OPTIONS ����MD5Դ��ʱϣ������ı���ѡ��
---with-MD5-asm ʹ��MD5�Ļ��Դ��
--with-SHA1=DIR ָ��SHA1���Դ��λ�ã��ڱ���Nginxʱ������Ŀ¼����SHA1Դ�롣
ע�⣺OpenSSL���Ѿ�����SHA1�㷨��ʵ�֡�����Ѿ���װ��OpenSSL����ô��ȫ����ʹ��OpenSSLʵ�ֵ�SHA1�㷨
--with-SHA1-opt=OPTIONS ����SHA1Դ��ʱϣ������ı���ѡ��
--with-SHA1-asm ʹ��SHA1�Ļ��Դ��

��1-8��zlib������ò���
zlib������ò��� �⡡��
--with-zlib=DIR ָ��zlib���Դ��λ�ã��ڱ���Nginxʱ������Ŀ¼����zlibԴ�롣���ʹ����gzipѹ�����ܣ�����Ҫzlib���֧��
--with-zlib-opt=OPTIONS ����zlibԴ��ʱϣ������ı���ѡ��
--with-zlib-asm=CPU ָ�����ض���CPUʹ��zlib��Ļ���Ż����ܣ�Ŀǰ��֧�����ּܹ���pentium��pentiumpro

4. ģ����صĲ���

�����������Ĵ����⣬Nginx��ȫ���ɸ��ֹ���ģ����ɵġ���Щģ���������ò��������Լ�����Ϊ����ˣ���ȷ��ʹ�ø���ģ��ǳ��ؼ�����configure�Ĳ����У����ǰ����Ƿ�Ϊ����ࡣ

�¼�ģ�顣

Ĭ�ϼ��������Nginx��HTTPģ�顣

Ĭ�ϲ���������Nginx��HTTPģ�顣

�ʼ������������ص�mail ģ�顣

����ģ�顣

��1���¼�ģ��

��1-9���г���Nginx����ѡ����Щ�¼�ģ����뵽��Ʒ�С�

��1-9��configure֧�ֵ��¼�ģ�����
������� �⡡��
--with-rtsig_module ʹ��rtsig module�����¼�����
Ĭ������£�Nginx�ǲ���װrtsig module�ģ��������rtsig module��������յ�Nginx�����Ƴ�����
--with-select_module ʹ��select module�����¼�����
select��Linux�ṩ��һ�ֶ�·���û��ƣ���epoll����û�е���ǰ��������Linux 2.4����֮ǰ���ں��У�select����֧�ַ������ṩ�߲�������
Ĭ������£�Nginx�ǲ���װselect module�ģ������û���ҵ��������õ��¼�ģ�飬��ģ�齫�ᱻ��װ
--without-select_module ����װselect module
--with-poll_module ʹ��poll module�����¼�����
poll��������select���ƣ��ڴ����������������ܶ�Զ����epoll��Ĭ������£�Nginx�ǲ���װpoll module��
--without-poll_module ����װpoll module
--with-aio_module ʹ��AIO��ʽ�����¼�����
ע�⣺�����aio moduleֻ����FreeBSD����ϵͳ�ϵ�kqueue�¼�������ƺ�����Linux���޷�ʹ��
Ĭ��������ǲ���װaio module��

��2��Ĭ�ϼ��������Nginx��HTTPģ��

��1-10�г���Ĭ�Ͼͻ�����Nginx�ĺ���HTTPģ�飬�Լ���ΰ���ЩHTTPģ��Ӳ�Ʒ��ȥ����

��1-10��configure��Ĭ�ϱ��뵽Nginx�е�HTTPģ�����
Ĭ�ϰ�װ��HTTPģ��                        �⡡��
--without-http_charset_module ����װhttp charset module�����ģ����Խ�������������HTTP��Ӧ�ر���
--without-http_gzip_module ����װhttp gzip module���ڷ�����������HTTP��Ӧ���У����ģ����԰��������ļ�ָ����content-type���ض���С��HTTP��Ӧ����ִ��gzipѹ��
--without-http_ssi_module ����װhttp ssi module����ģ����������û����ص�HTTP��Ӧ�����м����ض������ݣ���HTML�ļ��й̶���ҳͷ��ҳβ
--without-http_userid_module ����װhttp userid module�����ģ�����ͨ��HTTP����ͷ����Ϣ���һЩ�ֶ���֤�û���Ϣ����ȷ�������Ƿ�Ϸ�
--without-http_access_module ����װhttp access module�����ģ����Ը���IP��ַ�����ܹ����ʷ������Ŀͻ���
--without-http_auth_basic_module ����װhttp auth basic module�����ģ������ṩ��򵥵��û���/������֤
--without-http_autoindex_module ����װhttp autoindex module����ģ���ṩ�򵥵�Ŀ¼�������
--without-http_geo_module ����װhttp geo module�����ģ����Զ���һЩ��������Щ������ֵ����ͻ���IP��ַ����������Nginx��Բ�ͬ�ĵ����Ŀͻ��ˣ�����IP��ַ�жϣ����ز�һ���Ľ�������粻ͬ������ʾ��ͬ���Ե���ҳ
--without-http_map_module ����װhttp map module�����ģ����Խ���һ��key/valueӳ�����ͬ��key�õ���Ӧ��value������������Բ�ͬ��URL�����⴦�����磬����302�ض�����Ӧʱ����������URL��ͬʱ���ص�Location�ֶ�Ҳ��һ��
--without-http_split_clients_module ����װhttp split client module����ģ�����ݿͻ��˵���Ϣ������IP��ַ��headerͷ��cookie�ȣ������ִ���
--without-http_referer_module ����װhttp referer module����ģ����Ը��������е�referer�ֶ����ܾ�����
--without-http_rewrite_module ����װhttp rewrite module����ģ���ṩHTTP������Nginx�����ڲ����ض����ܣ�����PCRE��
--without-http_proxy_module ����װhttp proxy module����ģ���ṩ������HTTP���������
--without-http_fastcgi_module ����װhttp fastcgi module����ģ���ṩFastCGI����
--without-http_uwsgi_module ����װhttp uwsgi module����ģ���ṩuWSGI����
--without-http_scgi_module ����װhttp scgi module����ģ���ṩSCGI����
--without-http_memcached_module ����װhttp memcached module����ģ�����ʹ��Nginxֱ�������ε�memcached�����ȡ���ݣ����򵥵������HTTP��Ӧ���ظ��ͻ���
--without-http_limit_zone_module ����װhttp limit zone module����ģ�����ĳ��IP��ַ���Ʋ��������������磬ʹNginx��һ��IP��ַ������һ������
--without-http_limit_req_module ����װhttp limit req module����ģ�����ĳ��IP��ַ���Ʋ���������
--without-http_empty_gif_module ����װhttp empty gif module����ģ�����ʹ��Nginx���յ���Ч����ʱ�����̷����ڴ��е�1��1���ص�GIFͼƬ�����ֺô����ڣ��������Ե���Ч���󲻻�ȥ��ͼ�˷ѷ�������Դ
--without-http_browser_module ����װhttp browser module����ģ������HTTP�����е�user-agent�ֶΣ����ֶ�ͨ�����������д����ʶ�������
--without-http_upstream_ip_hash_module ����װhttp upstream ip hash module����ģ���ṩ��Nginx����server��������ʱ�������IP��ɢ������������������̨serverͨ�ţ���������ʵ�ָ��ؾ���

��3��Ĭ�ϲ���������Nginx��HTTPģ��

��1-11�г���Ĭ�ϲ��������Nginx�е�HTTPģ���Լ������Ǽ����Ʒ�еķ�����

��1-11��configure��Ĭ�ϲ�����뵽Nginx�е�HTTPģ�����
��ѡ��HTTP ģ�� �⡡��
--with-http_ssl_module ��װhttp ssl module����ģ��ʹNginx֧��SSLЭ�飬�ṩHTTPS����
ע�⣺��ģ��İ�װ������OpenSSL��Դ�����������Ӧȷ���Ѿ���֮ǰ�Ĳ�����������OpenSSL
--with-http_realip_module ��װhttp realip module����ģ����Դӿͻ����������header��Ϣ����X-Real-IP����X-Forwarded-For���л�ȡ�����Ŀͻ���IP��ַ
--with-http_addition_module ��װhttp addtion module����ģ������ڷ��ؿͻ��˵�HTTP����ͷ������β����������
--with-http_xslt_module ��װhttp xslt module�����ģ�����ʹXML��ʽ�������ڷ����ͻ���ǰ����XSL��Ⱦ
ע�⣺���ģ��������libxml2��libxslt�⣬��װ��ǰ����ȷ��������������Ѿ���װ
--with-http_image_filter_module ��װhttp image_filter module�����ģ�齫�������õ�ͼƬʵʱѹ��Ϊָ����С��width*height��������ͼ�ٷ��͸��û���Ŀǰ֧��JPEG��PNG��GIF��ʽ��
ע�⣺���ģ�������ڿ�Դ��libgd�⣬�ڰ�װǰȷ������ϵͳ�Ѿ���װ��libgd
--with-http_geoip_module ��װhttp geoip module����ģ���������MaxMind GeoIP��IP��ַ���ݿ�Կͻ��˵�IP��ַ�õ�ʵ�ʵĵ���λ����Ϣ
ע�⣺�ÿ�������MaxMind GeoIP�Ŀ��ļ����ɷ���http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz��ȡ
--with-http_sub_module ��װhttp sub module����ģ�������Nginx���ؿͻ��˵�HTTP��Ӧ���н�ָ�����ַ����滻Ϊ�Լ���Ҫ���ַ���
���磬��HTML�ķ����У���</head>�滻Ϊ</head><script language="javascript" src="$script"></script>
--with-http_dav_module ��װhttp dav module�����ģ�������Nginx֧��Webdav��׼����֧��WebdavЭ���е�PUT��DELETE��COPY��MOVE��MKCOL������
--with-http_flv_module ��װhttp flv module�����ģ���������ͻ��˷�����Ӧʱ����FLV��ʽ����Ƶ�ļ���headerͷ��һЩ����ʹ�ÿͻ��˿��Թۿ����϶�FLV��Ƶ
--with-http_mp4_module ��װhttp mp4 module����ģ��ʹ�ͻ��˿��Թۿ����϶�MP4��Ƶ
--with-http_gzip_static_module ��װhttp gzip static module���������gzipģ���һЩ�ĵ�����gzip��ʽѹ�����ٷ��ظ��ͻ��ˣ���ô��ͬһ���ļ�ÿ�ζ�������ѹ�������ǱȽ����ķ�����CPU��Դ�ġ�gzip staticģ���������gzipѹ��ǰ���Ȳ鿴��ͬλ���Ƿ����Ѿ�����gzipѹ����.gz�ļ�������У���ֱ�ӷ��ء������Ϳ���Ԥ���ڷ������������ĵ���ѹ������CPU����
--with-http_random_index_module ��װhttp random index module����ģ���ڿͻ��˷���ĳ��Ŀ¼ʱ��������ظ�Ŀ¼�µ������ļ�
--with-http_secure_link_module ��װhttp secure link module����ģ���ṩһ����֤�����Ƿ���Ч�Ļ��ơ����磬������֤URL����Ҫ�����token�����Ƿ������ض��ͻ��˷����ģ��Լ����ʱ����Ƿ����
--with-http_degradation_module ��װhttp degradation module����ģ�����һЩ�����ϵͳ���ã���sbrk����һЩ�Ż�����ֱ�ӷ���HTTP��Ӧ��Ϊ204����444��Ŀǰ��֧��Linuxϵͳ
--with-http_stub_status_module ��װhttp stub status module����ģ������������е�Nginx�ṩ����ͳ��ҳ�棬��ȡ��صĲ������ӡ��������Ϣ��14.2.1���м򵥽����˸�ģ���ԭ��
--with-google_perftools_module ��װgoogle perftools module����ģ���ṩGoogle�����ܲ��Թ���

��4���ʼ������������ص�mailģ��

��1-12�г��˰��ʼ�ģ����뵽��Ʒ�еĲ�����

��1-12��configure�ṩ���ʼ�ģ�����
��ѡ��mail ģ�� �⡡��
--with-mail ��װ�ʼ��������������ģ�飬ʹNginx���Է������IMAP��POP3��SMTP��Э�顣��ģ��Ĭ�ϲ���װ
--with-mail_ssl_module ��װmail ssl module����ģ�����ʹIMAP��POP3��SMTP��Э�����SSL/TLSЭ��֮��ʹ�á���ģ��Ĭ�ϲ���װ��������OpenSSL��
--without-mail_pop3_module ����װmail pop3 module����ʹ��--with-mail������pop3 module��Ĭ�ϰ�װ�ģ���ʹNginx֧��POP3Э��
--without-mail_imap_module ����װmail imap module����ʹ��--with-mail������imap module��Ĭ�ϰ�װ�ģ���ʹNginx֧��IMAP
--without-mail_smtp_module ����װmail smtp module����ʹ��--with-mail������smtp module��Ĭ�ϰ�װ�ģ���ʹNginx֧��SMTP

5.��������

configure������һЩ������������1-13���г�����ز�����˵����

��1-13��configure�ṩ����������
����һЩ���� �⡡��
--with-debug ��Nginx��Ҫ��ӡdebug���Լ�����־�Ĵ�������Nginx������������Nginx����ʱͨ���޸������ļ���ʹ���ӡ������־��������о�����λNginx����ǳ��а���
--add-module=PATH ����Nginx����������ģ��ʱ��ͨ���������ָ��������ģ���·���������������������ο���HTTPģ��ʱʹ�õ�
--without-http ����HTTP������
--without-http-cache ����HTTP��������Ļ���Cache����
--with-file-aio �����ļ����첽I/O��������������ļ�������ҪLinux�ں�֧��ԭ�����첽I/O
--with-ipv6 ʹNginx֧��IPv6
--user=USER ָ��Nginx worker��������ʱ�������û�
ע�⣺��Ҫ������worker���̵��û���Ϊroot����worker���̳�����ʱmaster����Ҫ�߱�ֹͣ/����worker���̵�����
--group=GROUP ָ��Nginx worker��������ʱ��������




���������TCP_CORKѡ�����Nagle�����䷽ʽ����ͬ TCP_NODELAY�෴��TCP_CORK �� TCP_NODELAY �ǻ����ų�ģ���






Nginx�������п���2013-04-03 11:54:50     ����˵���� �ղ�    ��ҪͶ��    ��������ͼ�� > �������Nginx�����ǰ���Ͱ�����Nginx����ר��Ż����Ѫ֮���������߶���ľ���ᾧ��Ҳ��Ŀǰ�г���Ψһһ��ͨ����ԭNginx���˼�룬����Nginx�ܹ����������߿��ٸ�Ч����HTTPģ���ͼ�顣��������ͨ�����ܹٷ�Nginx�Ļ�����...  ����ȥ������������Linux�У���Ҫʹ��������������Nginx��������������ֹͣ�����������ļ����ع���־�ļ���ƽ����������Ϊ��Ĭ������£�Nginx����װ��Ŀ¼/usr/local/nginx/�У���������ļ�·��Ϊ/usr/local/nginc/sbin/nginx�������ļ�·��Ϊ/usr/local/nginx/conf/nginx.conf����Ȼ����configureִ��ʱ�ǿ���ָ�������ǰ�װ�ڲ�ͬĿ¼�ġ�Ϊ�˼����������ֻ˵��Ĭ�ϰ�װ����µ������е�ʹ�������������߰�װ��Ŀ¼�����˱仯����ô�滻һ�¼��ɡ�

��1��Ĭ�Ϸ�ʽ����

ֱ��ִ��Nginx�����Ƴ������磺

/usr/local/nginx/sbin/nginx

��ʱ�����ȡĬ��·���µ������ļ���/usr/local/nginx/conf/nginx.conf��

ʵ���ϣ���û����ʽָ��nginx.conf�����ļ�·��ʱ��������configure����ִ��ʱʹ��--conf-path=PATHָ����nginx.conf�ļ����μ�1.5.1�ڣ���

��2������ָ�������ļ���������ʽ

ʹ��-c����ָ�������ļ������磺

/usr/local/nginx/sbin/nginx -c /tmp/nginx.conf

��ʱ�����ȡ-c������ָ����nginx.conf�����ļ�������Nginx��

��3������ָ����װĿ¼��������ʽ

ʹ��-p����ָ��Nginx�İ�װĿ¼�����磺

/usr/local/nginx/sbin/nginx -p /usr/local/nginx/

��4������ָ��ȫ���������������ʽ

����ͨ��-g������ʱָ��һЩȫ���������ʹ�µ���������Ч�����磺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;"

��������������ζ�Ż��pid�ļ�д��/var/nginx/test.pid�С�

-g������Լ��������ָ�������������Ĭ��·���µ�nginx.conf�е����������ͻ�������޷������������������������������������pid logs/nginx.pid���ǲ��ܴ�����Ĭ�ϵ�nginx.conf�еġ�

��һ��Լ�������ǣ���-g��ʽ������Nginx����ִ������������ʱ����Ҫ��-g����Ҳ���ϣ�������ܳ��������ƥ������Ρ����磬���ҪֹͣNginx������ô��Ҫִ��������룺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;" -s stop

���������-g "pid /var/nginx/test.pid;"����ô�Ҳ���pid�ļ���Ҳ������޷�ֹͣ����������

��5������������Ϣ�Ƿ��д���

�ڲ�����Nginx������£�ʹ��-t���������������ļ��Ƿ��д������磺

/usr/local/nginx/sbin/nginx -t

ִ�н������ʾ�����Ƿ���ȷ��

��6���ڲ������ý׶β������Ϣ

��������ѡ��ʱ��ʹ��-q�������Բ���error�������µ���Ϣ�������Ļ�����磺

/usr/local/nginx/sbin/nginx -t -q

��7����ʾ�汾��Ϣ

ʹ��-v������ʾNginx�İ汾��Ϣ�����磺

/usr/local/nginx/sbin/nginx -v

��8����ʾ����׶εĲ���

ʹ��-V�������˿�����ʾNginx�İ汾��Ϣ�⣬��������ʾ���ñ���׶ε���Ϣ����GCC�������İ汾������ϵͳ�İ汾��ִ��configureʱ�Ĳ����ȡ����磺

/usr/local/nginx/sbin/nginx -V

��9�����ٵ�ֹͣ����

ʹ��-s stop����ǿ��ֹͣNginx����-s������ʵ�Ǹ���Nginx�������������е�Nginx�������ź�����Nginx����ͨ��nginx.pid�ļ��еõ�master���̵Ľ���ID�����������е�master���̷���TERM�ź������ٵعر�Nginx�������磺

/usr/local/nginx/sbin/nginx -s stop

ʵ���ϣ����ͨ��kill����ֱ����nginx master���̷���TERM����INT�źţ�Ч����һ���ġ����磬��ͨ��ps�������鿴nginx master�Ľ���ID��

:ahf5wapi001:root > ps -ef | grep nginx

root     10800     1  0 02:27 ?        00:00:00 nginx: master process ./nginx

root     10801 10800  0 02:27 ?        00:00:00 nginx: worker process

������ֱ��ͨ��kill�����������źţ�

kill -s SIGTERM 10800

���ߣ�

kill -s SIGINT 10800

�������������Ч����ִ��/usr/local/nginx/sbin/nginx -s stop����ȫһ���ġ�

��10�������š���ֹͣ����

���ϣ��Nginx������������ش����굱ǰ����������ֹͣ������ô����ʹ��-s quit������ֹͣ�������磺

/usr/local/nginx/sbin/nginx -s quit

�����������ֹͣNginx������������ġ�������ֹͣ����ʱ��worker������master�������յ��źź����������ѭ�����˳����̡��������š���ֹͣ����ʱ�����Ȼ�رռ����˿ڣ�ֹͣ�����µ����ӣ�Ȼ��ѵ�ǰ���ڴ��������ȫ�������꣬������˳����̡�

�����ֹͣ�������ƣ�����ֱ�ӷ���QUIT�źŸ�master������ֹͣ������Ч����ִ��-s quit������һ���ġ����磺

kill -s SIGQUIT <nginx master pid>

���ϣ�������š���ֹͣĳ��worker���̣���ô����ͨ����ý��̷���WINCH�ź���ֹͣ�������磺

kill -s SIGWINCH <nginx worker pid>

��11��ʹ�����е�Nginx�ض��������Ч

ʹ��-s reload��������ʹ�����е�Nginx�������¼���nginx.conf�ļ������磺

/usr/local/nginx/sbin/nginx -s reload

��ʵ�ϣ�Nginx���ȼ���µ��������Ƿ��������ȫ����ȷ���ԡ����š��ķ�ʽ�رգ�����������Nginx��ʵ�����Ŀ�ġ����Ƶģ�-s�Ƿ����źţ���Ȼ������kill�����HUP�ź����ﵽ��ͬ��Ч����

kill -s SIGHUP <nginx master pid>

��12����־�ļ��ع�

ʹ��-s reopen�����������´���־�ļ������������Ȱѵ�ǰ��־�ļ�������ת�Ƶ�����Ŀ¼�н��б��ݣ������´�ʱ�ͻ������µ���־�ļ����������ʹ����־�ļ������ڹ������磺

/usr/local/nginx/sbin/nginx -s reopen

��Ȼ������ʹ��kill�����USR1�ź�Ч����ͬ��

kill -s SIGUSR1 <nginx master pid>

��13��ƽ������Nginx

��Nginx�����������µİ汾ʱ������Ҫ���ɵĶ������ļ�Nginx�滻����ͨ�������������Ҫ��������ģ���Nginx֧�ֲ���������������°汾��ƽ��������

����ʱ�������²��裺

1��֪ͨ�������еľɰ汾Nginx׼��������ͨ����master���̷���USR2�źſɴﵽĿ�ġ����磺

kill -s SIGUSR2 <nginx master pid>

��ʱ�������е�Nginx�Ὣpid�ļ����������罫/usr/local/nginx/logs/nginx.pid������Ϊ/usr/local/nginx/logs/nginx.pid.oldbin�������µ�Nginx���п��������ɹ���

2�������°汾��Nginx������ʹ�����Ͻ��ܹ�������һ��������������ʱͨ��ps������Է����¾ɰ汾��Nginx��ͬʱ���С�

3��ͨ��kill������ɰ汾��master���̷���SIGQUIT�źţ��ԡ����š��ķ�ʽ�رվɰ汾��Nginx�����ֻ���°汾��Nginx�������У���ʱƽ��������ϡ�
















nginxԴ�������3��- �Զ��ű�

nginx���Զ��ű�ָ����configure�ű������auto��Ŀ¼����Ľű������Զ��ű�����������飬��һ�Ǽ�黷��������������ļ���
���ɵ��ļ������࣬һ���Ǳ��������Ҫ��Makefile�ļ���һ���Ǹ��ݻ�����������ɵ�c���롣���ɵ�Makefile�ܸɾ���Ҳ�������Ķ���
���ɵ�c�����������ļ���ngx_auto_config.h�Ǹ��ݻ������Ľ��������һЩ�궨�壬���ͷ�ļ���include��ngx_core.h�У����Իᱻ���е�Դ�����õ���
��ȷ����Դ���ǿ���ֲ�ģ�ngx_auto_headers.h��Ҳ��һЩ�궨�壬�����ǹ���ϵͳͷ�ļ������Ե�������ngx_modules.c��Ĭ�ϱ�������ϵͳ�е�ģ���������
�����ȥ��һЩģ�飬ֻҪ�޸�����ļ����ɡ�

configure���Զ��ű�������������ͨ�����autoĿ¼�²�ͬ���ܵĽű�������ɻ������������ļ������񡣻��������Ҫ���������֣��������汾��֧�����ԡ�
����ϵͳ�汾��֧�����ԡ���������֧�֣����Ľű�����ֱ�����auto/cc��auto/os��auto/lib������Ŀ¼�С����ķ�������Ȥ��ͨ���Զ��������ڼ��ĳ
�����ԵĴ���Ƭ�Σ����ݱ��������������ж��Ƿ�֧�ָ������ԡ����ݼ�������������������֧������һ���򵥰汾��nginx���ͻ�����Makefile��c���룬
��Щ�ļ��������´�����objsĿ¼�¡���Ȼ��Ҳ���ܻ�ʧ�ܣ�����ϵͳ��֧��pcre��ssh�����û�����ε���ص�ģ�飬�Զ��ű��ͻ�ʧ�ܡ�

autoĿ¼�µĽű�ְ�ܻ��ַǳ��������м�黷���ģ��м��ģ��ģ����ṩ������Ϣ��(./configure --help)���д���ű������ģ�Ҳ��һЩ�ű�������Ϊ��
ģ�黯�Զ��ű�����Ƴ����ģ�����feature�ű������ڼ�鵥һ���Եģ������Ļ������ű������������ű�ȥ���ĳ�����ԡ�����һЩ�ű������������
Ϣ�������ļ��ģ�����have��nohave��make��install�ȡ�

֮����Ҫ��Դ�������ר��̸���Զ��ű�������Ϊnginx���Զ��ű�������autoconf֮��Ĺ������ɵģ����������ֹ���д�ģ����Ұ���һ������Ƴɷ֣�����
��Ҫ��д�Զ��ű�������˵���кܸߵĲο���ֵ������Ҳ�����Ǵ��ԵĽ���һ�£���Ҫ��ϸ�˽�����Ƕ�һ����Щ�ű�����Щ�ű���û��ʹ�ö�����Ƨ���﷨��
�ɶ����ǲ���ġ�

btw�����濪ʼ����������Դ������׶Σ�nginx��Դ�����зǳ���Ľṹ�壬��Щ�ṹ��֮������Ҳ��Ƶ�������������ֱ������֮��Ĺ�ϵ��������ͼ����
��õķ�ʽ�������Ҫ����һ�ָ�Ч������ͼ��������ѡ�����graphviz������at&t���׵Ŀ�ƽ̨��ͼ�����ɹ��ߣ�ͨ��дһ�ֳ�Ϊ��the dot language��
�Ľű����ԣ�Ȼ����dot����Ϳ���ֱ������ָ����ʽ��ͼ���ܷ��㡣




��ʲô���������Բ鿴������cpu��ʹ���ʣ����������˫�ˣ��߼��˺����ֵģ�:top Ȼ��1


��2�¡�Nginx������
����2.1�������е�Nginx���̼�Ĺ�ϵ2.2��Nginx���õ�ͨ���﷨2.2.1����������2.2.2����������﷨��ʽ2.2.3���������ע��2.2.4��
������ĵ�λ2.2.5����������ʹ�ñ���2.3��Nginx����Ļ�������2.3.1�����ڵ��Խ��̺Ͷ�λ�����������2.3.2���������е�������
2.3.3���Ż����ܵ�������2.3.4���¼���������2.4����HTTP����ģ������һ����̬Web������2.4.1����������������ķַ�2.4.2���ļ�·���Ķ���
2.4.3���ڴ漰������Դ�ķ���2.4.4���������ӵ�����2.4.5��MIME���͵�����2.4.6���Կͻ������������2.4.7���ļ��������Ż�
2.4.8���Կͻ�����������⴦��2.4.9��ngx_http_core_moduleģ���ṩ�ı���2.5����HTTP proxy module����һ��������������
2.5.1�����ؾ���Ļ�������2.5.2���������Ļ�������2.6��С��


nginx  configureִ������
���ǿ���configure����֧�ַǳ���Ĳ��������߿��ܻ��������ִ��ʱ����������Щ���飬���ڽ�ͨ������configureԴ����������һ�����Ե���ʶ��configure��Shell�ű���д���м�����<nginx-source>/auto/Ŀ¼�µĽű������ｫֻ��configure�ű������������������������õ�autoĿ¼�µ��������߽ű���ֻ�������Ե�˵����

configure�ű����������£�
#!/bin/sh

# Copyright (C) Igor Sysoev
# Copyright (C) Nginx, Inc.

#auto/options�ű�����configure����Ĳ��������磬���������--help����ô��ʾ֧�ֵ����в�����ʽ��options�ű��ᶨ�����������Ҫ�õ��ı�����Ȼ����ݱ��β����Լ�Ĭ��ֵ������Щ����
. auto/options

#auto/init�ű���ʼ���������������ļ�·�������磬Makefile��ngx_modules.c���ļ�Ĭ������½�����<nginx-source>/objs/
. auto/init

#auto/sources�ű�������Nginx��Դ��ṹ���������ܹ��������Makefile�ļ�
. auto/sources

#�������������Ŀ���ļ����ɵ�·���ɡ�builddir=DIR����ָ����Ĭ�������Ϊ<nginx-source>/objs����ʱ���Ŀ¼���ᱻ����
test -d $NGX_OBJS || mkdir $NGX_OBJS

#��ʼ׼������ngx_auto_headers.h��autoconf.err�ȱ�Ҫ�ı����ļ�
echo > $NGX_AUTO_HEADERS_H
echo > $NGX_AUTOCONF_ERR

#��objs/ngx_auto_config.hд�������д��Ĳ���
echo "#define NGX_CONFIGURE \"$NGX_CONFIGURE\"" > $NGX_AUTO_CONFIG_H

#�ж�DEBUG��־������У���ô��objs/ngx_auto_config.h�ļ���д��DEBUG��
if [ $NGX_DEBUG = YES ]; then
    have=NGX_DEBUG . auto/have
fi

#���ڿ�ʼ������ϵͳ�����Ƿ�֧�ֺ�������
if test -z "$NGX_PLATFORM"; then
    echo "checking for OS"

    NGX_SYSTEM=`uname -s 2>/dev/null`
    NGX_RELEASE=`uname -r 2>/dev/null`
    NGX_MACHINE=`uname -m 2>/dev/null`

#��Ļ�����OS���ơ��ں˰汾��32λ/64λ�ں�
    echo " + $NGX_SYSTEM $NGX_RELEASE $NGX_MACHINE"

    NGX_PLATFORM="$NGX_SYSTEM:$NGX_RELEASE:$NGX_MACHINE";

    case "$NGX_SYSTEM" in
        MINGW32_*)
            NGX_PLATFORM=win32
        ;;
    esac

else
    echo "building for $NGX_PLATFORM"
    NGX_SYSTEM=$NGX_PLATFORM
fi

#��鲢���ñ���������GCC�Ƿ�װ��GCC�汾�Ƿ�֧�ֺ�������nginx
. auto/cc/conf

#�Է�Windows����ϵͳ����һЩ��Ҫ��ͷ�ļ�����������Ƿ���ڣ��Դ˾���configure���������Ƿ���Գɹ�
if [ "$NGX_PLATFORM" != win32 ]; then
    . auto/headers
fi

#���ڵ�ǰ����ϵͳ������һЩ�ض��Ĳ���ϵͳ��صķ�������鵱ǰ�����Ƿ�֧�֡����磬����Linux��������ʹ��sched_setaffinity���ý������ȼ���ʹ��Linux���е�sendfileϵͳ�����������������з����ļ���
. auto/os/conf

#������UNIX ����ϵͳ��ͨ�õ�ͷ�ļ���ϵͳ���õȣ�����鵱ǰ�����Ƿ�֧��
if [ "$NGX_PLATFORM" != win32 ]; then
    . auto/unix
fi

#����ĵĹ���������modules�Ľű�������������ngx_modules.c�ļ�������ļ��ᱻ�����Nginx�У�������������Ψһ��������Ƕ�����ngx_modules���顣ngx_modulesָ��Nginx�����ڼ�����Щģ�����뵽����Ĵ����У�����HTTP������ܻ�ʹ����Щģ�鴦����ˣ���������Ԫ�ص�˳��ǳ����У�Ҳ����˵�����󲿷�ģ����ngx_modules�����е�˳����ʵ�ǹ̶��ġ����磬һ�����������ִ��ngx_http_gzip_filter_moduleģ�������޸�HTTP��Ӧ�е�ͷ���󣬲���ʹ��ngx_http_header_filterģ�鰴��headers_in�ṹ����ĳ�Ա�������TCP����ʽ���͸��ͻ��˵�HTTP��Ӧͷ����ע�⣬������--add-module=���������ĵ�����ģ��Ҳ�ڴ˲���д�뵽ngx_modules.c�ļ�����
. auto/modules

#conf�ű��������Nginx�������ڼ���Ҫ���ӵĵ�������̬�⡢��̬�����Ŀ���ļ��Ƿ����
. auto/lib/conf

#����Nginx��װ���·��
case ".$NGX_PREFIX" in
    .)
        NGX_PREFIX=${NGX_PREFIX:-/usr/local/nginx}
        have=NGX_PREFIX value="\"$NGX_PREFIX/\"" . auto/define
    ;;

    .!)
        NGX_PREFIX=
    ;;

    *)
        have=NGX_PREFIX value="\"$NGX_PREFIX/\"" . auto/define
    ;;
esac

#����Nginx��װ��conf�ļ���·��
if [ ".$NGX_CONF_PREFIX" != "." ]; then
    have=NGX_CONF_PREFIX value="\"$NGX_CONF_PREFIX/\"" . auto/define
fi

#����Nginx��װ�󣬶������ļ���pid��lock�������ļ���·���ɲμ�configure������·����ѡ���˵��
have=NGX_SBIN_PATH value="\"$NGX_SBIN_PATH\"" . auto/define
have=NGX_CONF_PATH value="\"$NGX_CONF_PATH\"" . auto/define
have=NGX_PID_PATH value="\"$NGX_PID_PATH\"" . auto/define
have=NGX_LOCK_PATH value="\"$NGX_LOCK_PATH\"" . auto/define
have=NGX_ERROR_LOG_PATH value="\"$NGX_ERROR_LOG_PATH\"" . auto/define


have=NGX_HTTP_LOG_PATH value="\"$NGX_HTTP_LOG_PATH\"" . auto/define
have=NGX_HTTP_CLIENT_TEMP_PATH value="\"$NGX_HTTP_CLIENT_TEMP_PATH\"" . auto/define
have=NGX_HTTP_PROXY_TEMP_PATH value="\"$NGX_HTTP_PROXY_TEMP_PATH\"" . auto/define
have=NGX_HTTP_FASTCGI_TEMP_PATH value="\"$NGX_HTTP_FASTCGI_TEMP_PATH\"" . auto/define
have=NGX_HTTP_UWSGI_TEMP_PATH value="\"$NGX_HTTP_UWSGI_TEMP_PATH\"" . auto/define
have=NGX_HTTP_SCGI_TEMP_PATH value="\"$NGX_HTTP_SCGI_TEMP_PATH\"" . auto/define

#��������ʱʹ�õ�objs/Makefile�ļ�
. auto/make

#Ϊobjs/Makefile������Ҫ���ӵĵ�������̬�⡢��̬�����Ŀ���ļ�
. auto/lib/make

#Ϊobjs/Makefile����install���ܣ���ִ��make installʱ���������ɵı�Ҫ�ļ����Ƶ���װ·����������Ҫ��Ŀ¼
. auto/install

# ��ngx_auto_config.h�ļ��м���NGX_SUPPRESS_WARN�ꡢNGX_SMP��
. auto/stubs

#��ngx_auto_config.h�ļ���ָ��NGX_USER��NGX_GROUP�꣬���ִ��configureʱû�в���ָ����Ĭ�����߽�Ϊnobody��Ҳ����Ĭ����nobody�û����н��̣�
have=NGX_USER value="\"$NGX_USER\"" . auto/define
have=NGX_GROUP value="\"$NGX_GROUP\"" . auto/define

#��ʾconfigureִ�еĽ�������ʧ�ܣ������ԭ��
. auto/summary























��α�д�������ܵ�./config:

config�ļ���ʵ��һ����ִ�е�Shell�ű������ֻ�뿪��һ��HTTPģ�飬��ôconfig�ļ�����Ҫ��������3��������
ngx_addon_name������configureִ��ʱʹ�ã�һ������Ϊģ�����ơ�
HTTP_MODULES���������е�HTTPģ�����ƣ�ÿ��HTTPģ����ɿո������������������HTTP_MODULES����ʱ��
��Ҫֱ�Ӹ���������Ϊconfigure���õ��Զ����config�ű�ǰ���Ѿ�������HTTPģ�����õ�HTTP_MODULES �������ˣ���ˣ�Ҫ�������������ã�
"$HTTP_MODULES ngx_http_mytest_module"
NGX_ADDON_SRCS������ָ������ģ���Դ���룬����������Դ������Կո��������ע�⣬������NGX_ADDON_SRCSʱ
����ʹ��$ngx_addon_dir���������ȼ���configureִ��ʱ--add-module=PATH��PATH������
��ˣ�����mytestģ�飬����������дconfig�ļ���

ngx_addon_name=ngx_http_mytest_module
HTTP_MODULES="$HTTP_MODULES ngx_http_mytest_module"
NGX_ADDON_SRCS="$NGX_ADDON_SRCS $ngx_addon_dir/ngx_http_mytest_module.c"

ע�⡡����3������������Ψһ������config�ļ����Զ���Ĳ��֡�������ǲ��ǿ���HTTPģ�飬���ǿ���һ��HTTP����ģ�飬
��ô��Ҫ��HTTP_FILTER_MODULES��������HTTP_MODULES��������ʵ�ϣ�����$CORE_MODULES��$EVENT_MODULES��$HTTP_MODULES�� 
$HTTP_FILTER_MODULES��$HTTP_HEADERS_FILTER_MODULE��ģ������������ض��壬���Ƿֱ��Ӧ��Nginx�ĺ���ģ�顢�¼�ģ�顢
HTTPģ�顢HTTP����ģ�顢HTTPͷ������ģ�顣����NGX_ADDON_SRCS������������һ���������ǻ��õ�����$NGX_ADDON_DEPS������
��ָ����ģ��������·����ͬ��������config�����á�




















����configure�ű������Ƶ�ģ����뵽Nginx��:


��1.6���ᵽ��configureִ�������У����������нű����𽫵�����ģ����뵽Nginx�У�������ʾ��

. auto/modules

. auto/make

���������ؽ���һ��configure�ű��������3.3.1�����ᵽ��config�ļ���������Ѷ��Ƶĵ�����ģ����뵽Nginx�еġ�

��ִ��configure --add-module=PATH����ʱ��PATH(����һ��Ҫ�Ǿ���·�������򱨴�./configure: error: no mytest_config/config was found)���ǵ�����ģ�����ڵ�·������configure�У�ͨ��auto/options�ű�������NGX_ADDONS������
--add-module=*)                  NGX_ADDONS="$NGX_ADDONS $value" ;;

��configure����ִ�е�auto/modules�ű�ʱ���������ɵ�ngx_modules.c�ļ��м��붨�Ƶĵ�����ģ�顣
if test -n "$NGX_ADDONS"; then

    echo configuring additional modules

    for ngx_addon_dir in $NGX_ADDONS
    do
        echo "adding module in $ngx_addon_dir"

        if test -f $ngx_addon_dir/config; then
            #������ִ���Զ����config�ű�
            . $ngx_addon_dir/config

            echo " + $ngx_addon_name was configured"

        else
            echo "$0: error: no $ngx_addon_dir/config was found"
            exit 1
        fi
    done
fi

���Կ�����$NGX_ADDONS���԰������Ŀ¼������ÿ��Ŀ¼��������д���config�ļ��ͻ�ִ�У�Ҳ����˵����config�����¶���ı���������Ч��֮��auto/modules�ű���ʼ����ngx_modules.c�ļ�������ļ��Ĺؼ�����Ƕ�����ngx_module_t *ngx_modules[]���飬�������洢��Nginx�е�����ģ�顣Nginx�ڳ�ʼ������������ʱ������ѭ������ngx_modules���飬ȷ��������һ��ģ����������������һ��auto/modules�������������ģ�����������ʾ��
modules="$CORE_MODULES $EVENT_MODULES"

if [ $USE_OPENSSL = YES ]; then
    modules="$modules $OPENSSL_MODULE"
    CORE_DEPS="$CORE_DEPS $OPENSSL_DEPS"
    CORE_SRCS="$CORE_SRCS $OPENSSL_SRCS"
fi

if [ $HTTP = YES ]; then
    modules="$modules $HTTP_MODULES $HTTP_FILTER_MODULES \
             $HTTP_HEADERS_FILTER_MODULE \
             $HTTP_AUX_FILTER_MODULES \
             $HTTP_COPY_FILTER_MODULE \
             $HTTP_RANGE_BODY_FILTER_MODULE \
             $HTTP_NOT_MODIFIED_FILTER_MODULE"

    NGX_ADDON_DEPS="$NGX_ADDON_DEPS \$(HTTP_DEPS)"
fi

���ȣ�auto/modules�ᰴ˳������modules������ע�⣬�����$HTTP_MODULES���Ѿ���config�ļ����ض����ˡ���ʱ��modules�����ǰ�������ģ��ġ�Ȼ�󣬿�ʼ����ngx_modules.c�ļ���
cat << END                                    > $NGX_MODULES_C

#include <ngx_config.h>
#include <ngx_core.h>

$NGX_PRAGMA

END

for mod in $modules
do
    echo "extern ngx_module_t  $mod;"         >> $NGX_MODULES_C
done

echo                                          >> $NGX_MODULES_C
echo 'ngx_module_t *ngx_modules[] = {'        >> $NGX_MODULES_C

for mod in $modules
do
    #��ngx_modules���������Nginxģ��
    echo "    &$mod,"                         >> $NGX_MODULES_C
done

cat << END                                    >> $NGX_MODULES_C
    NULL
};

END

�������Ѿ�ȷ����Nginx������ʱ������Զ����ģ�飬��auto/make�ű���������ģ������Nginx��

��Makefile�����ɱ��������ģ���Դ�������£�
if test -n "$NGX_ADDON_SRCS"; then

    ngx_cc="\$(CC) $ngx_compile_opt \$(CFLAGS) $ngx_use_pch \$(ALL_INCS)"

    for ngx_src in $NGX_ADDON_SRCS
    do
        ngx_obj="addon/`basename \`dirname $ngx_src\``"

        ngx_obj=`echo $ngx_obj/\`basename $ngx_src\` \
            | sed -e "s/\//$ngx_regex_dirsep/g"`

        ngx_obj=`echo $ngx_obj \
            | sed -e
              "s#^\(.*\.\)cpp\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e
              "s#^\(.*\.\)cc\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e
              "s#^\(.*\.\)c\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e
              "s#^\(.*\.\)S\\$#$ngx_objs_dir\1$ngx_objext#g"`

        ngx_src=`echo $ngx_src | sed -e "s/\//$ngx_regex_dirsep/g"`

        cat << END                                            >> $NGX_MAKEFILE

$ngx_obj: \$(ADDON_DEPS)$ngx_cont$ngx_src
 $ngx_cc$ngx_tab$ngx_objout$ngx_obj$ngx_tab$ngx_src$NGX_AUX

END
     done

fi

������δ������ڽ�����ģ���Ŀ���ļ����õ�ngx_obj�����У������Ż�����Makefile������Ӵ��룬�������е�Ŀ���ļ������ļ����ӳɶ����Ƴ���
for ngx_src in $NGX_ADDON_SRCS
do
    ngx_obj="addon/`basename \`dirname $ngx_src\``"

    test -d $NGX_OBJS/$ngx_obj || mkdir -p $NGX_OBJS/$ngx_obj

    ngx_obj=`echo $ngx_obj/\`basename $ngx_src\` \
        | sed -e "s/\//$ngx_regex_dirsep/g"`

    ngx_all_srcs="$ngx_all_srcs $ngx_obj"
done

��

cat << END                                                    >> $NGX_MAKEFILE

$NGX_OBJS${ngx_dirsep}nginx${ngx_binext}: 
 $ngx_deps$ngx_spacer \$(LINK)
 ${ngx_long_start}${ngx_binout}$NGX_OBJS${ngx_dirsep}nginx$ngx_long_cont$ngx_objs$ngx_libs$ngx_link
 $ngx_rcc
${ngx_long_end}
END

���Ͽ�֪��������ģ���������Ƕ�뵽Nginx�����еġ�




�����Ƕԡ��������Nginx��һ���е�ʵ������ʵսʱ�ļ�¼��

1ģ��Ŀ¼�ṹ
my_test_module/

������ config

������ ngx_http_mytest_module.c
1.1�����ļ�
config�ļ��������£�

ngx_addon_name=ngx_http_mytest_module

HTTP_MODULES="$HTTP_MODULESngx_http_mytest_module"

NGX_ADDON_SRCS="$NGX_ADDON_SRCS$ngx_addon_dir/ngx_http_mytest_module.c"

1.2ģ��Դ��
ngx_http_mytest_module.c�е��������£�

 

#include <ngx_config.h>  
#include <ngx_core.h>  
#include <ngx_http.h>  
  
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)  
{  
    // Only handle GET/HEAD method  
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {  
        return NGX_HTTP_NOT_ALLOWED;  
    }  
  
    // Discard request body  
    ngx_int_t rc = ngx_http_discard_request_body(r);  
    if (rc != NGX_OK) {  
        return rc;  
    }  
  
    // Send response header  
    ngx_str_t type = ngx_string("text/plain");  
    ngx_str_t response = ngx_string("Hello World!!!");  
    r->headers_out.status = NGX_HTTP_OK;  
    r->headers_out.content_length_n = response.len;  
    r->headers_out.content_type = type;  
      
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
      
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);  
    clcf->handler = ngx_http_mytest_handler;  
    return NGX_CONF_OK;  
}  
  
static ngx_command_t ngx_http_mytest_commands[] = {  
    {  
        ngx_string("mytest"),  
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,  
        ngx_http_mytest,  
        NGX_HTTP_LOC_CONF_OFFSET,  
        0,  
        NULL  
    },  
    ngx_null_command      
};  
  
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
 

2��������
2.1����Nginx���ҵ�ģ��
Nginx����GCC, PCRE�ȣ�������ҪԤ�Ȱ�װ��Щ��������֮������nginx-1.4.2.tar.gzԴ�������ѹ��

         tarzxvf nginx-1.4.2.tar.gz

         cdnginx-1.4.2

         ./configure --add-module=/var/yyz/nginx/nginx-1.9.2/src/my_test_module

         make

         makeinstall

 

��make����ʱ����־Ҳ�ܿ������ǵ�ģ�鱻�����Nginx�ں��ˣ�
2.2�����ҵ�ģ��
NginxĬ�ϱ���װ��/usr/local/nginx���ˣ�����/conf/nginx.conf�����ļ������һ��server�������������ǵ�ģ�飺

         server{

                   listen80;

                   server_namemytest.com

                   location/ {

                            mytest;

                   }

         }

2.3����Ч��
������mytest.com��ӵ�Host�У�ָ��Nginx���ڻ�����Ȼ�����http:/ip��ַ/mytest���ܿ���Ч���ˣ�




3.9.1�����뷽ʽ���޸�2013-04-06 09:28:55     ����˵���� �ղ�    ��ҪͶ��    ��������ͼ�� > �������Nginx�����ǰ���Ͱ�����Nginx����ר��Ż����Ѫ֮���������߶���ľ���ᾧ��
Ҳ��Ŀǰ�г���Ψһһ��ͨ����ԭNginx���˼�룬����Nginx�ܹ����������߿��ٸ�Ч����HTTPģ���ͼ�顣��������ͨ�����ܹٷ�Nginx�Ļ�����...  ����ȥ����������

Nginx��configure�ű�û�ж�C++���Ա���ģ���ṩ֧�֣���ˣ��޸ı��뷽ʽ������������˼·��
1���޸�configure��صĽű���
2���޸�configureִ����Ϻ����ɵ�Makefile�ļ���

�����Ƽ�ʹ�õ�2�ַ�������ΪNginx��һ���ŵ��Ǿ߱������ĵ�����ģ�飬��Щģ�鶼�ǻ��ڹٷ���configure�ű���д�ģ������޸�configure�ű��ᵼ�����ǵ�Nginx�޷�ʹ�õ�����ģ�顣

�޸�Makefile��ʵ�Ǻܼ򵥵ġ��������Ǹ���3.3.2�ڽ��ܵķ�ʽ��ִ��configure�ű���֮�������objs/Makefile�ļ�����ʱֻ��Ҫ�޸�����ļ���3������ʵ��C++ģ�顣���ﻹ����mytestģ��Ϊ�����������¡�
CC =   gcc
CXX = g++
CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Wunused-function -Wunused-variable -Wunused-value -Werror -g
CPP =   gcc -E
LINK =  $(CXX)

��
objs/addon/httpmodule/ngx_http_mytest_module.o: $(ADDON_DEPS) \
        ../sample/httpmodule/ngx_http_mytest_module.c
        $(CXX) -c $(CFLAGS)  $(ALL_INCS) \
                -o objs/addon/httpmodule/ngx_http_mytest_module.o \
                ../sample/httpmodule/ngx_http_mytest_module.cpp
��

�������һ�������������޸ĵĵط���

��Makefile�ļ��ײ�������һ��CXX = g++���������C++��������

�����ӷ�ʽLINK =  $(CC)��Ϊ��LINK =  $(CXX)����ʾ��C++���������������ӡ�

��ģ��ı��뷽ʽ�޸�ΪC++���������������ֻ��һ��C++Դ�ļ�����ֻҪ�޸�һ����������ж��C++Դ�ļ�����ÿ���ط�����Ҫ�޸ġ��޸ķ�ʽ�ǰ�$(CC)��Ϊ$(CXX)��

���������뷽ʽ���޸���ϡ��޸�Դ�ļ���Ҫ����ִ��configure�ű�������Ḳ���Ѿ��޸Ĺ���Makefile�����齫�޸Ĺ���Makefile�ļ����б��ݣ�����ÿ��ִ��configure�������޸�Makefile��

ע�⡡ȷ���ڲ���ϵͳ���Ѿ���װ��C++�������������1.3.2���еķ�ʽ��װgcc-c++��������





C������C++�������Ĳ�ͬ���ڱ����ķ����в��C++Ϊ��֧�ֶ�������������ԣ������ء���ȣ������ķ�������C������ȫ��ͬ����
�����ͨ��C++�����ṩ��extern ��C�� {}��ʵ�ַ��ŵĻ���ʶ��Ҳ����˵����C++���Կ�����ģ���У�include������Nginx�ٷ�ͷ�ļ�����Ҫʹ��extern ��C�������������磺
extern "C" {
    #include <ngx_config.h>
    #include <ngx_core.h>
    #include <ngx_http.h>
}

�����Ϳ��������ص���Nginx�ĸ��ַ����ˡ�
���⣬����ϣ��Nginx��ܻص���������ngx_http_mytest_handler�����ķ���Ҳ��Ҫ����extern ��C���С�






��ngx_http_mytest_handler�ķ���ֵ�У������������HTTP�����룬Nginx�ͻᰴ�չ淶����Ϸ�����Ӧ�����͸��û���
���磬�������PUT�����ݲ�֧�֣���ô���ڴ������з��ַ�������PUTʱ������NGX_HTTP_NOT_ALLOWED������NginxҲ�ͻṹ�������������Ӧ�����û���
http/1.1 405 Not Allowed
Server: nginx/1.0.14
Date: Sat, 28 Apr 2012 06:07:17 GMT
Content-Type: text/html
Content-Length: 173
Connection: keep-alive

<html>
<head><title>405 Not Allowed</title></head>
<body bgcolor="white">
<center><h1>405 Not Allowed</h1></center>
<hr><center>nginx/1.0.14</center>
</body>
</html>



���������Nginx����1�濱�󹫲� ���ߣ�taohui | ���ࣺ�������Nginx | ��ǩ�� �������Nginx  ����  ��1��  | �Ķ� 2758 �� | ������2013��12��20�� 7:57 a.m. 
 ����  
����΢����Ѷ΢��ӡ��ʼǶ���������QQ�ռ�E-Mail ���ڱ�д��æ������������ڲ�����©���ҽ������﹫�����е�ȱ�ݣ��Լ�����������¶��������޶����ݡ�

1��98ҳ�ڶ��δ���ע��

������ÿ��ngx_table_elt_t��Ա����RFC1616�淶....�� �����У�RFC1616ӦΪRFC2616

2��109ҳ��һ��Դ���뵹����5��

b->file->name.len = sizeof(filename) - 1;Ӧ��Ϊb->file->name.len = strlen(filename) ;

3��139ҳ��4-5�ĵ����ڶ��б������

ngx_bufs_t���͵ĳ�Ա����ʹ��nginx_conf_merge_str_value�ϲ��꣬Ӧ��Ϊ��ngx_bufs_t���͵ĳ�Ա����ʹ�� nginx_conf_merge_bufs_value�ϲ���

4��97ҳ������5�У��Ǵ������еĵ�����5�ƣ�ʵ���� 3) URL���������£�

arg ָ���û������е�URL������Ӧ��Ϊ��args ָ���û������е�URL����

5.����104ҳ������Դ����

TestHead�� TestValudrnӦ��Ϊ TestHead�� TestValuern

6.97ҳ��5��6������

4���ᵽ��extern����Ӧ��Ϊexten����

7.97ҳ������5��

��http_protocolָ���û�������HTTP����ʼ��ַ����Ӧ��Ϊ��http_protocol��data��Աָ���û�������HTTPЭ��汾�ַ�������ʼ��ַ��len��ԱΪЭ��汾�ַ������ȡ���

8.101ҳ������2��

��NGX_http_SPECIAL_RESPONSE��Ӧ��Ϊ��NGX_HTTP_SPECIAL_RESPONSE��

9.102ҳ������7��

����ngx_http_request_t�ķ���ֵ�Ƕ����ġ�Ӧ��Ϊ����ngx_http_send_header�ķ���ֵ�Ƕ����ġ�

10.122ҳ��4-2��ngx_conf_set_size_slot��ĵ�2��

"Kilobyt"ӦΪ"Kilobyte"

11.119ҳ��һ��������mycf�ṹ���еĳ�Ա��ʼ��ʱ��ǰ׺"test"Ӧ��Ϊ"my"�����£�

mycf->test_flagӦ��Ϊmycf->my_flag
mycf->test_numӦ��Ϊmycf->my_num
mycf->test_str_arrayӦ��Ϊmycf->my_str_array
mycf->test_keyvalӦ��Ϊmycf->my_keyval
mycf->test_offӦ��Ϊmycf->my_off
mycf->test_msecӦ��Ϊmycf->my_msec
mycf->test_secӦ��Ϊmycf->my_sec
mycf->test_sizeӦ��Ϊmycf->my_size
12.554ҳ������7�У���//F_WRLCK��ζ�Ų��ᵼ�½���˯�ߡ�Ӧ��Ϊ��//F_SETLK��ζ�Ų��ᵼ�½���˯�ߡ�
��555ҳ������14�У���//F_WRLCK��ζ�Żᵼ�½���˯�ߡ�Ӧ��Ϊ��//F_SETLKW��ζ�Żᵼ�½���˯�ߡ�

13.77ҳ��9�У�header = part->elts;ӦΪstr = part->elts;��
14��283ҳͼ88�ĵ�1���������У����ngx_noaccept��־λΪ1��Ӧ��Ϊngx_reap��־λΪ1��

15��284ҳͼ8-8�У��������һ���С��������ӽ������˳��򷵻ص�liveΪ1��Ӧ��Ϊ���������ӽ������˳��򷵻ص�liveΪ0����������ڵġ�live��־λΪ1��ͬʱ��Ӧ��Ϊ��live��־λΪ1��ͬʱ����

16��310ҳ��13���У���󲹳�һ��˵������ע�⣬��accept_mutex������ִ����һ��������

17��327ҳ��6�С����û������timer_resolution��һ������£�process_events������timer�������Ǵ���0��С��500�����ֵ������������Ӧ��ɾ����

18��337ҳȫ��8����ngx_process_changes���Լ�338ҳ��2�е�ngx_process_changes����Ӧ��Ϊngx_process_events��

19��356��ͼ10-3�У�ngx_http_core_man+conf_t��Ϊngx_http_core_man_conf_t

20��379ҳ10.6.4���е�һ�ε�����2�䣬�� ��������ɵ������ȻҪ��ngx_http_phase_engine_t�׶εĴ����� �� �У�ngx_http_phase_engine_tӦ��ΪNGX_HTTP_FIND_CONFIG_PHASE��




��Linux�У���Ҫʹ��������������Nginx��������������ֹͣ�����������ļ����ع���־�ļ���ƽ����������Ϊ��Ĭ������£�Nginx����װ��Ŀ¼
/usr/local/nginx/�У���������ļ�·��Ϊ/usr/local/nginc/sbin/nginx�������ļ�·��Ϊ/usr/local/nginx/conf/nginx.conf����Ȼ��
��configureִ��ʱ�ǿ���ָ�������ǰ�װ�ڲ�ͬĿ¼�ġ�Ϊ�˼����������ֻ˵��Ĭ�ϰ�װ����µ������е�ʹ�������������߰�װ��
Ŀ¼�����˱仯����ô�滻һ�¼��ɡ�

��1��Ĭ�Ϸ�ʽ����

ֱ��ִ��Nginx�����Ƴ������磺

/usr/local/nginx/sbin/nginx

��ʱ�����ȡĬ��·���µ������ļ���/usr/local/nginx/conf/nginx.conf��

ʵ���ϣ���û����ʽָ��nginx.conf�����ļ�·��ʱ��������configure����ִ��ʱʹ��--conf-path=PATHָ����nginx.conf�ļ����μ�1.5.1�ڣ���

��2������ָ�������ļ���������ʽ

ʹ��-c����ָ�������ļ������磺

/usr/local/nginx/sbin/nginx -c /tmp/nginx.conf

��ʱ�����ȡ-c������ָ����nginx.conf�����ļ�������Nginx��

��3������ָ����װĿ¼��������ʽ

ʹ��-p����ָ��Nginx�İ�װĿ¼�����磺

/usr/local/nginx/sbin/nginx -p /usr/local/nginx/

��4������ָ��ȫ���������������ʽ

����ͨ��-g������ʱָ��һЩȫ���������ʹ�µ���������Ч�����磺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;"

��������������ζ�Ż��pid�ļ�д��/var/nginx/test.pid�С�

-g������Լ��������ָ�������������Ĭ��·���µ�nginx.conf�е����������ͻ�������޷������������������������������������pid logs/nginx.pid��
�ǲ��ܴ�����Ĭ�ϵ�nginx.conf�еġ�

��һ��Լ�������ǣ���-g��ʽ������Nginx����ִ������������ʱ����Ҫ��-g����Ҳ���ϣ�������ܳ��������ƥ������Ρ����磬���ҪֹͣNginx����
��ô��Ҫִ��������룺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;" -s stop

���������-g "pid /var/nginx/test.pid;"����ô�Ҳ���pid�ļ���Ҳ������޷�ֹͣ����������

��5������������Ϣ�Ƿ��д���

�ڲ�����Nginx������£�ʹ��-t���������������ļ��Ƿ��д������磺

/usr/local/nginx/sbin/nginx -t

ִ�н������ʾ�����Ƿ���ȷ��

��6���ڲ������ý׶β������Ϣ

��������ѡ��ʱ��ʹ��-q�������Բ���error�������µ���Ϣ�������Ļ�����磺

/usr/local/nginx/sbin/nginx -t -q

��7����ʾ�汾��Ϣ

ʹ��-v������ʾNginx�İ汾��Ϣ�����磺

/usr/local/nginx/sbin/nginx -v

��8����ʾ����׶εĲ���

ʹ��-V�������˿�����ʾNginx�İ汾��Ϣ�⣬��������ʾ���ñ���׶ε���Ϣ����GCC�������İ汾������ϵͳ�İ汾��ִ��configureʱ�Ĳ����ȡ����磺

/usr/local/nginx/sbin/nginx -V

��9�����ٵ�ֹͣ����

ʹ��-s stop����ǿ��ֹͣNginx����-s������ʵ�Ǹ���Nginx�������������е�Nginx�������ź�����Nginx����ͨ��nginx.pid�ļ��еõ�master���̵Ľ���ID��
���������е�master���̷���TERM�ź������ٵعر�Nginx�������磺

/usr/local/nginx/sbin/nginx -s stop

ʵ���ϣ����ͨ��kill����ֱ����nginx master���̷���TERM����INT�źţ�Ч����һ���ġ����磬��ͨ��ps�������鿴nginx master�Ľ���ID��

:ahf5wapi001:root > ps -ef | grep nginx

root     10800     1  0 02:27 ?        00:00:00 nginx: master process ./nginx

root     10801 10800  0 02:27 ?        00:00:00 nginx: worker process

������ֱ��ͨ��kill�����������źţ�

kill -s SIGTERM 10800

���ߣ�

kill -s SIGINT 10800

�������������Ч����ִ��/usr/local/nginx/sbin/nginx -s stop����ȫһ���ġ�

��10�������š���ֹͣ����

���ϣ��Nginx������������ش����굱ǰ����������ֹͣ������ô����ʹ��-s quit������ֹͣ�������磺

/usr/local/nginx/sbin/nginx -s quit

�����������ֹͣNginx������������ġ�������ֹͣ����ʱ��worker������master�������յ��źź����������ѭ�����˳����̡��������š���ֹͣ����ʱ��
���Ȼ�رռ����˿ڣ�ֹͣ�����µ����ӣ�Ȼ��ѵ�ǰ���ڴ��������ȫ�������꣬������˳����̡�

�����ֹͣ�������ƣ�����ֱ�ӷ���QUIT�źŸ�master������ֹͣ������Ч����ִ��-s quit������һ���ġ����磺

kill -s SIGQUIT <nginx master pid>

���ϣ�������š���ֹͣĳ��worker���̣���ô����ͨ����ý��̷���WINCH�ź���ֹͣ�������磺

kill -s SIGWINCH <nginx worker pid>

��11��ʹ�����е�Nginx�ض��������Ч

ʹ��-s reload��������ʹ�����е�Nginx�������¼���nginx.conf�ļ������磺

/usr/local/nginx/sbin/nginx -s reload

��ʵ�ϣ�Nginx���ȼ���µ��������Ƿ��������ȫ����ȷ���ԡ����š��ķ�ʽ�رգ�����������Nginx��ʵ�����Ŀ�ġ����Ƶģ�-s�Ƿ����źţ�
��Ȼ������kill�����HUP�ź����ﵽ��ͬ��Ч����

kill -s SIGHUP <nginx master pid>

��12����־�ļ��ع�

ʹ��-s reopen�����������´���־�ļ������������Ȱѵ�ǰ��־�ļ�������ת�Ƶ�����Ŀ¼�н��б��ݣ������´�ʱ�ͻ������µ���־�ļ���
�������ʹ����־�ļ������ڹ������磺

/usr/local/nginx/sbin/nginx -s reopen

��Ȼ������ʹ��kill�����USR1�ź�Ч����ͬ��

kill -s SIGUSR1 <nginx master pid>

��13��ƽ������Nginx

��Nginx�����������µİ汾ʱ������Ҫ���ɵĶ������ļ�Nginx�滻����ͨ�������������Ҫ��������ģ���Nginx֧�ֲ���������������°汾��ƽ��������

����ʱ�������²��裺

1��֪ͨ�������еľɰ汾Nginx׼��������ͨ����master���̷���USR2�źſɴﵽĿ�ġ����磺

kill -s SIGUSR2 <nginx master pid>

��ʱ�������е�Nginx�Ὣpid�ļ����������罫/usr/local/nginx/logs/nginx.pid������Ϊ/usr/local/nginx/logs/nginx.pid.oldbin�������µ�Nginx���п��������ɹ���

2�������°汾��Nginx������ʹ�����Ͻ��ܹ�������һ��������������ʱͨ��ps������Է����¾ɰ汾��Nginx��ͬʱ���С�

3��ͨ��kill������ɰ汾��master���̷���SIGQUIT�źţ��ԡ����š��ķ�ʽ�رվɰ汾��Nginx�����ֻ���°汾��Nginx�������У���ʱƽ��������ϡ�

��14����ʾ�����а���

ʹ��-h����-?��������ʾ֧�ֵ����������в�����

*/

