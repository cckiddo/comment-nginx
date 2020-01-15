/*
 HTTP ͷ������

1. Accept������WEB�������Լ�����ʲô�������ͣ�* /* ��ʾ�κ����ͣ�type/ * ��ʾ�������µ����������ͣ�type/sub-type��
 
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
                    ��WEB��������֤������Ч�Ժ󣬲���������Ӧ�ͻ�����
                    �� HTTP/1.0 ��ͷ����
                    ���磺Expires��Sat, 23 May 2009 10:02:12 GMT
 
11. Host���ͻ���ָ���Լ�����ʵ�WEB������������/IP ��ַ�Ͷ˿ںš�
                ���磺Host��rss.sina.com.cn
 
12. If-Match���������� ETag û�иı䣬��ʵҲ����ζ������û�иı䣬��ִ������Ķ�����
    If-None-Match���������� ETag �ı��ˣ���ʵҲ����ζ������Ҳ�ı��ˣ���ִ������Ķ�����
 
13. If-Modified-Since���������Ķ����ڸ�ͷ��ָ����ʱ��֮���޸��ˣ���ִ������
                                 �Ķ��������緵�ض��󣩣����򷵻ش���304������������ö���
                                 û���޸ġ�
                                 ���磺If-Modified-Since��Thu, 10 Apr 2008 09:14:42 GMT
    If-Unmodified-Since���������Ķ����ڸ�ͷ��ָ����ʱ��֮��û�޸Ĺ�����ִ��
                                   ����Ķ��������緵�ض��󣩡�
 
14. If-Range����������� WEB �����������������Ķ���û�иı䣬�Ͱ���ȱ�ٵĲ���
                       ���ң��������ı��ˣ��Ͱ�����������ҡ� �����ͨ�������������� 
                       ETag ���� �Լ���֪��������޸�ʱ��� WEB �������������ж϶����Ƿ�
                       �ı��ˡ�
                       ���Ǹ� Range ͷ��һ��ʹ�á�
 
15. Last-Modified��WEB ��������Ϊ���������޸�ʱ�䣬�����ļ�������޸�ʱ�䣬
                                 ��̬ҳ���������ʱ��ȵȡ�
                                 ���磺Last-Modified��Tue, 06 May 2008 02:42:43 GMT
 
16. Location��WEB �������������������ͼ���ʵĶ����Ѿ����Ƶ����λ���ˣ�
                        ����ͷ��ָ����λ��ȥȡ��
                        ���磺Location��
                                  http://i0.sinaimg.cn/dy/deco/2008/0528/sinahome_0803_ws_005_text_0.gif
 
17. Pramga����Ҫʹ�� Pramga: no-cache���൱�� Cache-Control�� no-cache��
                      ���磺Pragma��no-cache
 
18. Proxy-Authenticate�� �����������Ӧ�������Ҫ�����ṩ���������֤��Ϣ��
      Proxy-Authorization���������Ӧ����������������֤�����ṩ�Լ��������Ϣ��
 
19. Range������������� Flashget ���߳�����ʱ������ WEB �������Լ���ȡ������Ĳ��֡�
                    ���磺Range: bytes=1173546-
 
20. Referer��������� WEB �����������Լ��Ǵ��ĸ� ��ҳ/URL ���/��� ��ǰ�����е���ַ/URL��
                   ���磺Referer��http://www.sina.com/
 
21. Server: WEB �����������Լ���ʲô������汾����Ϣ��
                  ���磺Server��Apache/2.0.61 (Unix)
 
22. User-Agent: ����������Լ�����ݣ����������������
                        ���磺User-Agent��Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN;   
                                  rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14
 
23. Transfer-Encoding: WEB �����������Լ��Ա���Ӧ��Ϣ�壨������Ϣ������Ķ���
                                  ���������ı��룬�����Ƿ�ֿ飨chunked����
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
                ʲôЭ�飨�Ͱ汾�����͵�����
                ���ͻ������󵽴��һ�����������ʱ���÷����������Լ���������������
                ��� Via ͷ�����������Լ��������Ϣ������һ����������� �յ���һ������
                ������������ʱ�������Լ��������������渴��ǰһ������������������Via 
                ͷ���������Լ��������Ϣ�ӵ����棬 �Դ����ƣ��� OCS �յ����һ�������
                ����������ʱ����� Via ͷ������֪����������������·�ɡ�
                ���磺Via��1.0 236-81.D07071953.sina.com.cn:80 (squid/2.6.STABLE13)
Cookie: ̹�׵�˵��һ��cookie���Ǵ洢���û�����������е�һС���ı��ļ���Cookies�Ǵ��ı���ʽ�����ǲ������κο�ִ�д��롣һ��
        Webҳ����������֮�����������Щ��Ϣ�洢���һ���һϵ�й�����֮���ÿ�������ж�������Ϣ��������������Web������֮���������
        ��Щ��Ϣ����ʶ�û���������Ҫ��¼��վ��ͨ�����������֤��Ϣͨ����������һ��cookie��֮��ֻҪ���cookie���ڲ��ҺϷ�����Ϳ���
        ���ɵ�������վ������в��֡��ٴΣ�cookieֻ�ǰ��������ݣ����䱾����Բ����к���

Date: ͷ���ʾ��Ϣ���͵�ʱ�䣬ʱ���������ʽ��rfc822���塣���磬Date:Mon,31Dec200104:25:57GMT��Date������ʱ���ʾ�����׼ʱ������ɱ���ʱ�䣬��Ҫ֪���û����ڵ�ʱ����








http://kb.cnblogs.com/page/92320/

HTTP Header ��ⷢ��ʱ��: 2011-02-27 21:07  �Ķ�: 71790 ��  �Ƽ�: 23   ԭ������   [�ղ�]   
HTTP��HyperTextTransferProtocol�������ı�����Э�飬Ŀǰ��ҳ����ĵ�ͨ��Э�顣HTTPЭ�����������/��Ӧģ�ͣ�������������ͻ��˷������󣬷�����������Ӧ��������������Դ������ԣ�����message-header��message-body�����֡����ȴ���message- header����http header��Ϣ ��http header ��Ϣͨ������Ϊ4�����֣�general  header, request header, response header, entity header���������ַַ��������ԣ��о����޲�̫��ȷ������ά���ٿƶ�http header���ݵ���֯��ʽ�������ΪRequest��Response�����֡�

Requests����
Header ���� ʾ�� 
Accept ָ���ͻ����ܹ����յ��������� Accept: text/plain, text/html 
Accept-Charset ��������Խ��ܵ��ַ����뼯�� Accept-Charset: iso-8859-5 
Accept-Encoding ָ�����������֧�ֵ�web��������������ѹ���������͡� Accept-Encoding: compress, gzip 
Accept-Language ������ɽ��ܵ����� Accept-Language: en,zh 
Accept-Ranges ����������ҳʵ���һ�����߶���ӷ�Χ�ֶ� Accept-Ranges: bytes 
Authorization HTTP��Ȩ����Ȩ֤�� Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ== 
Cache-Control ָ���������Ӧ��ѭ�Ļ������ Cache-Control: no-cache 
Connection ��ʾ�Ƿ���Ҫ�־����ӡ���HTTP 1.1Ĭ�Ͻ��г־����ӣ� Connection: close 
Cookie HTTP������ʱ����ѱ����ڸ����������µ�����cookieֵһ���͸�web�������� Cookie: $Version=1; Skin=new; 
Content-Length ��������ݳ��� Content-Length: 348 
Content-Type �������ʵ���Ӧ��MIME��Ϣ Content-Type: application/x-www-form-urlencoded 
Date �����͵����ں�ʱ�� Date: Tue, 15 Nov 2010 08:12:31 GMT 
Expect ������ض��ķ�������Ϊ Expect: 100-continue 
From ����������û���Email From: user@email.com 
Host ָ������ķ������������Ͷ˿ں� Host: www.zcmhi.com 
If-Match ֻ������������ʵ����ƥ�����Ч If-Match: ��737060cd8c284d8af7ad3082f209582d�� 
If-Modified-Since �������Ĳ�����ָ��ʱ��֮���޸�������ɹ���δ���޸��򷵻�304���� If-Modified-Since: Sat, 29 Oct 2010 19:43:31 GMT 
If-None-Match �������δ�ı䷵��304���룬����Ϊ��������ǰ���͵�Etag�����������Ӧ��Etag�Ƚ��ж��Ƿ�ı� If-None-Match: ��737060cd8c284d8af7ad3082f209582d�� 
If-Range ���ʵ��δ�ı䣬���������Ϳͻ��˶�ʧ�Ĳ��֣�����������ʵ�塣����ҲΪEtag If-Range: ��737060cd8c284d8af7ad3082f209582d�� 
If-Unmodified-Since ֻ��ʵ����ָ��ʱ��֮��δ���޸Ĳ�����ɹ� If-Unmodified-Since: Sat, 29 Oct 2010 19:43:31 GMT 
Max-Forwards ������Ϣͨ����������ش��͵�ʱ�� Max-Forwards: 10 
Pragma ��������ʵ���ض���ָ�� Pragma: no-cache 
Proxy-Authorization ���ӵ��������Ȩ֤�� Proxy-Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ== 
Range ֻ����ʵ���һ���֣�ָ����Χ Range: bytes=500-999 
Referer ��ǰ��ҳ�ĵ�ַ����ǰ������ҳ�������,����· Referer: http://www.zcmhi.com/archives/71.html 
TE �ͻ���Ը����ܵĴ�����룬��֪ͨ���������ܽ���β��ͷ��Ϣ TE: trailers,deflate;q=0.5 
Upgrade �������ָ��ĳ�ִ���Э���Ա����������ת�������֧�֣� Upgrade: HTTP/2.0, SHTTP/1.3, IRC/6.9, RTA/x11 
User-Agent User-Agent�����ݰ�������������û���Ϣ User-Agent: Mozilla/5.0 (Linux; X11) 
Via ֪ͨ�м����ػ�����������ַ��ͨ��Э�� Via: 1.0 fred, 1.1 nowhere.com (Apache/1.1) 
Warning ������Ϣʵ��ľ�����Ϣ Warn: 199 Miscellaneous warning 

Responses ���� 

Header ���� ʾ�� 
Accept-Ranges �����������Ƿ�֧��ָ����Χ�����������͵ķֶ����� Accept-Ranges: bytes 
Age ��ԭʼ���������������γɵĹ���ʱ�䣨����ƣ��Ǹ��� Age: 12 
Allow ��ĳ������Դ����Ч��������Ϊ���������򷵻�405 Allow: GET, HEAD 
Cache-Control �������еĻ�������Ƿ���Ի��漰�������� Cache-Control: no-cache 
Content-Encoding web������֧�ֵķ�������ѹ���������͡� Content-Encoding: gzip 
Content-Language ��Ӧ������� Content-Language: en,zh 
Content-Length ��Ӧ��ĳ��� Content-Length: 348 
Content-Location ������Դ������ı��õ���һ��ַ Content-Location: /index.htm 
Content-MD5 ������Դ��MD5У��ֵ Content-MD5: Q2hlY2sgSW50ZWdyaXR5IQ== 
Content-Range �������������б����ֵ��ֽ�λ�� Content-Range: bytes 21010-47021/47022 
Content-Type �������ݵ�MIME���� Content-Type: text/html; charset=utf-8 
Date ԭʼ��������Ϣ������ʱ�� Date: Tue, 15 Nov 2010 08:12:31 GMT 
ETag ���������ʵ���ǩ�ĵ�ǰֵ ETag: ��737060cd8c284d8af7ad3082f209582d�� 
Expires ��Ӧ���ڵ����ں�ʱ�� Expires: Thu, 01 Dec 2010 16:00:00 GMT 
Last-Modified ������Դ������޸�ʱ�� Last-Modified: Tue, 15 Nov 2010 12:45:26 GMT 
Location �����ض�����շ���������URL��λ�������������ʶ�µ���Դ Location: http://www.zcmhi.com/archives/94.html 
Pragma ����ʵ���ض���ָ�����Ӧ�õ���Ӧ���ϵ��κν��շ� Pragma: no-cache 
Proxy-Authenticate ��ָ����֤�����Ϳ�Ӧ�õ�����ĸ�URL�ϵĲ��� Proxy-Authenticate: Basic 
refresh Ӧ�����ض����һ���µ���Դ�����죬��5��֮���ض�����������������󲿷������֧�֣�  
Refresh: 5; url=http://www.zcmhi.com/archives/94.html 
Retry-After ���ʵ����ʱ����ȡ��֪ͨ�ͻ�����ָ��ʱ��֮���ٴγ��� Retry-After: 120 
Server web������������� Server: Apache/1.3.27 (Unix) (Red-Hat/Linux) 
Set-Cookie ����Http Cookie Set-Cookie: UserID=JohnDoe; Max-Age=3600; Version=1 
Trailer ָ��ͷ���ڷֿ鴫������β������ Trailer: Max-Forwards 
Transfer-Encoding �ļ�������� Transfer-Encoding:chunked 
Vary �������δ�����ʹ�û�����Ӧ���Ǵ�ԭʼ���������� Vary: * 
Via ��֪����ͻ�����Ӧ��ͨ�����﷢�͵� Via: 1.0 fred, 1.1 nowhere.com (Apache/1.1) 
Warning ����ʵ����ܴ��ڵ����� Warning: 199 Miscellaneous warning 
WWW-Authenticate �����ͻ�������ʵ��Ӧ��ʹ�õ���Ȩ���� WWW-Authenticate: Basic 


����linux�汾:http://mirrors.163.com/

�Ա����ĵ�ַ:http://tengine.taobao.org/nginx_docs/cn/docs/http/ngx_http_core_module.html   http://shouce.jb51.net/nginx/left.html
openresty���ʵ����ҳ���߿�:http://xuewb.com/openresty/safe_sql.html
#define NGX_CONFIGURE " --add-module=./src/mytest_config --add-module=./src/my_test_module --add-module=./src/mytest_subrequest --add-module=./src/mytest_upstream --add-module=./src/ngx_http_myfilter_module --with-debug --with-file-aio --add-module=./src/sendfile_test --with-threads --add-module=/var/yyz/nginx-1.9.2/src/echo-nginx-module-master --add-module=./src/nginx-requestkey-module-master/


"Address already in use"����ͨ�������׽���SO_REUSEADDR�����

http://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt


�򻯵�nginx�����ģ��demo:http://www.cnblogs.com/liuyidiao/p/multi-process-demo.html

#define NGX_CONFIGURE " --add-module=./src/mytest_config --add-module=./src/my_test_module --add-module=./src/mytest_subrequest --add-module=./src/mytest_upstream --add-module=./src/ngx_http_myfilter_module --with-debug --with-file-aio --add-module=./src/sendfile_test --with-threads"
#define NGX_CONFIGURE " --add-module=./src/mytest_config --add-module=./src/my_test_module --add-module=./src/mytest_subrequest --add-module=./src/mytest_upstream --add-module=./src/ngx_http_myfilter_module --with-debug --with-file-aio --add-module=./src/sendfile_test --with-threads --add-module=/var/yyz/nginx-1.9.2/src/echo-nginx-module-master"

I/O���Թ��ߣ�iostat  ����ӿ��������Թ���ifstat

3865.81
������ ������� sendfile  ��־ģ��
�̳߳�

    buffering��ʽ�ͷ�buffering��ʽ���������ݵ�������ϸ������
    buffering��ʽpipe����������ϸ����
    keepaliveģ�������⣬������ӻ���ԭ�������
    ������˷�����ʧЧ�жϷ���,�Ѿ��ٴλָ�ʹ�ü�ⷽ������
    �������proxy-module��ϸ������proxy_pass��ؽ��շ�ʽ��ʽ�������������proxyģ����������ͽ���ԭ�����
    chunk���뷽ʽ��������������chunk��ʽ���Ͱ��嵽����������Լ�������̷���
    keepalive-module��˷��������ӻ����һ������
    ������subrequest������Ӧ��postpone����
    �༶subrequest��α�֤���ݰ���ָ���Ⱥ�˳���͵Ŀͻ�����������������
    ��ʱ�ļ�������������̷������Լ����������ϸ������
    ָ���Ļ��治��������£��������д����ʱ�ļ����̷���
    proxy_cacheԭ����������������ڴ洴������������������
    slabԭ��������Լ�slab�������ڴ���̷���

    http://blog.csdn.net/weiyuefei/article/details/35782523  http://www.tuicool.com/articles/QnMNr23

    --add-module=./src/mytest_config --add-module=./src/my_test_module --add-module=./src/mytest_subrequest --add-module=./src/mytest_upstream --add-module=./src/ngx_http_myfilter_module --with-debug --with-file-aio --add-module=./src/sendfile_test --with-threads --add-module=/var/yyz/nginx-1.9.2/src/echo-nginx-module-master --add-module=./src/nginx-requestkey-module-master/ --with-http_secure_link_module
#define NGX_CONFIGURE " --add-module=./src/mytest_config --add-module=./src/my_test_module --add-module=./src/mytest_subrequest --add-module=./src/mytest_upstream --add-module=./src/ngx_http_myfilter_module --with-debug --with-file-aio --add-module=./src/sendfile_test --with-threads --add-module=/var/yyz/nginx-1.9.2/src/echo-nginx-module-master --add-module=./src/nginx-requestkey-module-master/ --with-http_secure_link_module --add-module=./src/redis2-nginx-module-master/ --add-module=./src/lua-nginx-module-master/"



    ���÷������������ݵ�ʱ��Ϊʲô��Ҫ��ʱ�ļ���������ϸ��������ϸ���̷���
    loader���̣�cache manager���̴�����̷���,��Ӧhandler��ʱָ����Ʒ���
    �����ļ����з��͹��̷���
    �����ļ�stat״̬��Ϣ������洢���ϻ����̷���  
    �����ļ�������洢���̼��ϻ�����
    �����ļ�loader���ع��̷���
    aio�첽�ļ���ȡ���ͷ���
    sendfile���̷���
    �̳߳�������
    aio sendfile �̳߳طֽ����ϸ����

    ����Ϊʲôÿ���ڷ��ͺ��������󶼻����ngx_http_send_special��ԭ�����
    ��һ������aio�첽�¼���������
    �ؼ���������ӡ�����÷�����־��
    ������ngx_http_write_filter��ʱ�������Ӻ�˽��յ����ݵ��˻����ļ������������ϳ�ȥ������ʱin file�ģ���ʵ������out��ʱ��ȴ��in mem����in file
    sendfile����ͨngx_writev�ֽ���һ������
    ��������µ�copy filterִ�����̺ͷǻ�������µ�copy filterִ�����̽�һ������ע�͡�
    filter������ݵ��ͻ��˵�ʱ���Ƿ���Ҫ���¿����ڴ�ռ�ֽ����ϸ������
    ����ֱ��direct I/O����Ч�������Լ����������Ϸ���˵��ʱ��ʹ��direct I/O
    direct I/O���첽AIO�������Ч�ʴ����������ѹ����
    directio������Ч���̴������
    ͬʱ����sendfile on;  aio on; dirction xxx;�������sendfile��aioѡ���ж�������ϸ����
    �̳߳�thread_pool_moduleԴ����ϸ����ע�� 
    aio threadִ��������ϸ����
    ��һ��������ȡ�����ļ�ͷ�����ֺ��ļ���벿���岿�ֵķ������̡�
    secure_linkȨ�޷���ģ��ԭ��������Լ�������������������Է���
    �����ߵ�index��staticģ�飬���secure_link���ԡ�
    ��ӵ�����ģ��-������ģ�飬�˽�ԭ����߶��������

    ����uri��Я����arg_xxx�����Ľ����γɹ��̣����secure link���ʹ�ã��𵽰�ȫ��������,�Ը�ģ�������ϸ����ע��
    aio thread_poolʹ�����̷������̳߳ض�ȡ�ļ����̷�����
    ��ͨ����ʽ�´��ļ�(10M����)���ع������̷������Լ���С�ļ�(С��32768)���ع��̲�ͬ���̱ȽϷ���
    AIO on��ʽ�´��ļ�(10M����)���ع������̷������Լ���С�ļ�(С��32768)���ع��̲�ͬ���̱ȽϷ���
    AIO thread_pool��ʽ�´��ļ�(10M����)���ع������̷������Լ���С�ļ�(С��32768)���ع��̲�ͬ���̱ȽϷ���
    ����С�ļ�(С��65535/2)����ȫ���̺ʹ��ļ�����ȫ������־������ֱ�۵õ������������̣�������־
    ���refer������ģ�����°�hash�߶����ע��һ�飬�������
    �������ǰ��ͨ�������ͨ���hash�洢����
    ���lua���lua-module �Ѷ�ʱ�� �¼����ƺ�������޸���ӵ�luaģ���redisģ��

    �����ļ��ȴ浽��ʱ�ļ����и�ԭ���Ƿ�ֹֻ����һ�룬��˷��������ˣ������ļ�����������




    ����ע��errlog_moduleģ�飬�о�errlog_module��Ngx_http_core_module��error_log���õ�����ͬʱ������������ģ��������Чǰ����־��¼����
    ����ngx_log_debugall���ܣ������Լ���ӵ��Դ��룬����Ӱ��ԭ�д��룬������ϣ������������ܴ�������Ų顣
    ͬʱ���ö���error_log������Ϣ��ÿ���ļ����͵ȼ���һ����Ҳ���ǲ�ͬ�ȼ���־�������ͬ��־�ļ��Ĺ��̷�����ע�͡�
    ngx_http_log_module��ϸ�������Լ�access_log����ǳ���·���ͱ���·�����Ż�������̡�
    NGX_HTTP_LOG_PHASE�׶�ngx_http_log_module��Ч���̼���־��¼����ϸ������ע��,ͬʱ��ȷngx_errlog_module��ngx_http_log_module������
    ��һ��ϸ������ngx_http_index_module���룬�Լ����rewrite-phase�����ڲ��ض�������
    �о�����ngx_http_autoindex_moduleĿ¼���ģ��Դ�����ע�ͣ�ͬʱ����ֻ��ngx_http_index_moduleģ�鲻���뵽Դ���е�ʱ��ngx_http_autoindex_module�Ż���Ч�����index-module
        ģ������Դ�룬��Ҫô�����ڲ��ض���Ҫôֱ�ӹر����ӣ���Զ����ִ��ngx_http_autoindex_module������͹���������һ�£�TODO��������ȷ��
    autoindex�Ŀ¼����������Դ���������
    ��ϴ������If-None-Match��ETag , If-Modified-Since��Last-Modified�⼸��ͷ������������������Ч����,Ҳ���Ǿ����Ƿ�(304 NOT modified)��
        ���ҷ���ע��ngx_http_not_modified_filter_moduleʵ�ֹ���
    Cache-Control expire ͷ���д���ʵ�����̣���ϸ����expires��add_header���ã�����ע��ngx_http_headers_filter_moduleģ��
    ���½�ϴ���Աȷ������ܽἸ�ָ�ծ�����㷨(hash  ip_hash least_conn rr)
    ���error_pages���ã���ngx_http_special_response_handler���з�����ͬʱ���ڲ��ض����ܺ�@new_location���з���
   

    ��һ������internal��location{}����Ȩ�޿���
    ����Ϊnginxȫ�ֺ���ģ�飬��׼HTTPģ�����©���������������ע��
    ���types{}���ã���content_typeͷ���е��γɹ��̽�����ϸ����
    ���·������ý������̣�δ��




����㼰��������:
        �ͺ�˷�����ͨ������׽�������״̬���ж��Ƿ�ʧЧ�����ʧЧ��������һ�������������ִ���ȱ�ݣ����������˷�����ֱ�Ӱε����߻��ߺ�˷�������
    ���ˣ������׽������жϲ������ģ�Э��ջ��Ҫ��ʱ���������жϳ�������رյ�Э��ջ��keepalive������Զ��ⲻ�������Ǹ�����bug��
*/

