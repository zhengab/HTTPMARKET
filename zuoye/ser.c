#include <myhead.h>
#include <sqlite3.h>

#define IP_PORT 50001
#define IP_ADDR "127.0.0.1"
#include <semaphore.h>
sem_t SEMA,SEMB;

FILE *fd = 0;

int callbackuser(void*arg,int col ,char** result ,char**title);
int callbackgoods(void*arg,int col ,char** result ,char**title);
int callbackdetails(void*arg,int col ,char** result ,char**title);

/*  字符转码 */
unsigned char FromHex(unsigned char x)
{
	if(0 == x ) return -1;
	unsigned char y;
	if(x>='A' &&x<='Z') y = x-'A'+10;
	else if(x>='a' &&x <='z') y = x-'a'+10;
	else if(x>='0' && x<='9') y = x-'0';

	return y;
}
int  urlDecode(  char* dest, const char* src)
{
	if(NULL ==src || NULL == dest)
	{
		return -1;

	}
	int len = strlen(src);
	int i =0 ;
	for(i = 0 ;i<len;i++)
	{

		if('+' == src[i]) strcat(dest,"");
		else if('%'==src[i])
		{
			//if(i+2len)return -1;
			unsigned char high = FromHex((unsigned char)src[++i]);
			unsigned char low = FromHex((unsigned char)src[++i]);
			unsigned char temp = high*16 +low;
			char temp2[5]={0};
			sprintf(temp2,"%c",temp);
			strcat(dest , temp2);
		}
	}
	return 0;
}

void *th(void *arg)
{	
	pthread_detach(pthread_self());
	sem_wait(&SEMB);
	int alloc_socket ;//= *(int *)arg;
	memcpy(&alloc_socket,arg,4);
	sem_post(&SEMA);
	sqlite3* db;
	sqlite3* db2;
	char *p = NULL;
	char *q = NULL;
	char tmp[4096] = {0};
	char * errmsg;
	char * errmsg2;
	char * errmsg3;
	int flag = 0 ;
	int flag2 = 0 ;
	int flag3 = 0 ;

	int ret = 0;
	char sql_cmd[512]= {0};


	memset(tmp, 0, sizeof(tmp));
	ret = recv(alloc_socket, tmp, sizeof(tmp), 0);

	if(ret<=0)
	{
		close(alloc_socket);
		pthread_exit(NULL);
	}
	printf("%s\n", tmp);
	//打开数据库
	ret = sqlite3_open("./login.db",&db);

	if(SQLITE_OK != ret)
	{
		fprintf(stderr,"can't open db %s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
	}

	ret = sqlite3_open("./123.db",&db2);

	if(SQLITE_OK != ret)
	{
		fprintf(stderr,"can't open db2 %s\n",sqlite3_errmsg(db2));
		sqlite3_close(db2);
	}

	char *method = strtok(tmp, " ");
	char *url = strtok(NULL, " ");
	char *ver = strtok(NULL, "\r\n");
	printf("strtok url %s \n", url);
	if(!strcmp(url, "/"))
	{	
		send_head("/home/linux/1127/zuoye/login.html", alloc_socket);
		send_file("/home/linux/1127/zuoye/login.html", alloc_socket);
	}

	if(strstr(url,"goods="))
	{
		url = url + 14; //指针定位至商品名		
		
		if(*url == '%')
		{
			char oldname[32] = {0};
			char newname[32] = {0};
			memcpy(oldname, url, strlen(url));
			printf("%s\n", oldname);
			urlDecode(newname, oldname);
			printf("----%s\n", newname);
			sprintf(sql_cmd, "select * from goods where goods_name like '%%%s%%';" ,newname);
		}
		else
		{
			printf("非汉字\n");
			sprintf(sql_cmd, "select * from goods where goods_name like '%%%s%%';" ,url);
		}

		printf("%s\n", sql_cmd);	
		/* 拼接网页文件 */
		fd = fopen("./list.html", "w");
		
		char htmlbuf[4096] = {0};
	
		sprintf(htmlbuf, "<!DOCTYPE html>\n");
		sprintf(htmlbuf, "%s<html>\n", htmlbuf);
		sprintf(htmlbuf, "%s<meta charset=\"utf-8\">\n", htmlbuf);
		sprintf(htmlbuf, "%s<head>\n", htmlbuf);
		sprintf(htmlbuf, "%s<title>想屁吃--搜索</title>\n", htmlbuf);
		sprintf(htmlbuf, "%s</head>\n", htmlbuf);
		sprintf(htmlbuf, "%s<body>\n", htmlbuf);
		sprintf(htmlbuf, "%s<form action='login'>\n", htmlbuf);
		sprintf(htmlbuf, "%s<h3><p align='center'>搜索结果</p></h3>\n", htmlbuf);

		fputs(htmlbuf, fd);

		ret = sqlite3_exec(db2,sql_cmd,callbackgoods,&flag2,&errmsg2);

		if(SQLITE_OK != ret)
		{
			fprintf(stderr,"exec %s errmsg:%s\n",sql_cmd,errmsg2);
			sqlite3_free(errmsg2);
			sqlite3_close(db2);
		}
		
		memset(htmlbuf, 0, sizeof(htmlbuf));
		sprintf(htmlbuf, "</body>\n");
		sprintf(htmlbuf, "%s</form>\n", htmlbuf);
		sprintf(htmlbuf, "%s</html>\n\0", htmlbuf);
		fputs(htmlbuf, fd);
	
		fclose(fd);

		if(flag2 == 1)
		{
			printf("有这个东西\n");

			send_head("/home/linux/1127/zuoye/list.html", alloc_socket);
			send_file("/home/linux/1127/zuoye/list.html", alloc_socket);
		}
		else
		{
			send_head("/home/linux/1127/zuoye/NULL.html", alloc_socket);
			send_file("/home/linux/1127/zuoye/NULL.html", alloc_socket);

		}

		flag2 = 0;
	}


	printf("send ok\n");
	printf("==========图片=============\n");
	printf("%s\n", url);

	if(strstr(url,"login"))
	{
		printf("%s\n", url);
		char * name=NULL;
		char * pass=NULL;
		name=strstr(url,"=");
		name+=1;
		pass = strstr(name,"&");
		*pass = '\0';
		pass = strstr(pass+1,"=");
		pass+=1;


		sprintf(sql_cmd, "select * from usr where usrname='%s' and password='%s';", name, pass);

		ret = sqlite3_exec(db,sql_cmd,callbackuser,&flag,&errmsg);
		if(SQLITE_OK != ret)
		{
			fprintf(stderr,"exec %s errmsg:%s\n",sql_cmd,errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
		}
		if(1 == flag)
		{
			send_head("/home/linux/1127/zuoye/search.html", alloc_socket);
			send_file("/home/linux/1127/zuoye/search.html", alloc_socket);

		}

		else
		{
			send_head("/home/linux/1127/zuoye/login.html", alloc_socket);
			send_file("/home/linux/1127/zuoye/login.html", alloc_socket);
		}

	}
	if(strstr(url,"png") || strstr(url, "ico") || strstr(url, "jpg") || strstr(url, "gif"))
	{
		if(strstr(url, "imghead") || strstr(url, "fav"))
		{
			char image[512] = {0};
			sprintf(image, "/home/linux/1127/zuoye/%s", url + 1);
			printf("头图路径%s\n", image);
			send_head(image, alloc_socket);
			send_file(image, alloc_socket);
		}
		else
		{			
			char image[512] = {0};
			sprintf(image, "/home/linux/1127/zuoye%s", url + 22);
			printf("%s\n", image);
			send_head(image, alloc_socket);
			send_file(image, alloc_socket);
		}
	}
	if(strstr(url,"details"))
	{	
		printf("详情网页开始制作\n");

		fd = fopen("./details.html", "w");
		printf("%s\n", url);	
		char html2buf[4096] = {0};
		sprintf(sql_cmd, "select * from goods where goods_id='%s';" ,url + 9);
		printf("^^^^^^^^^^^^^^^^^^^^%s\n", url+9);

		sprintf(html2buf, "<!DOCTYPE html>\n");
		sprintf(html2buf, "%s<html>\n", html2buf);
		sprintf(html2buf, "%s<meta charset=\"utf-8\">\n", html2buf);
		sprintf(html2buf, "%s<head>\n", html2buf);
		sprintf(html2buf, "%s<title>想屁吃--详情</title>\n", html2buf);
		sprintf(html2buf, "%s</head>\n", html2buf);
		sprintf(html2buf, "%s<body>\n", html2buf);
		sprintf(html2buf, "%s<h3><p align='center'>商品详情</p></h3>\n", html2buf);

		fputs(html2buf, fd);

		ret = sqlite3_exec(db2,sql_cmd,callbackdetails,&flag3,&errmsg2);
		if(SQLITE_OK != ret)
		{
			fprintf(stderr,"exec %s errmsg:%s\n",sql_cmd,errmsg3);
			sqlite3_free(errmsg3);
		}
		printf("详情网页末尾制作\n");
		memset(html2buf, 0, sizeof(html2buf));
		sprintf(html2buf, "</body>\n");
		sprintf(html2buf, "%s</html>\n\0", html2buf);
		fputs(html2buf, fd);

		printf("详情网页制作完成\n");
		fclose(fd);

		send_head("/home/linux/1127/zuoye/details.html", alloc_socket);
		send_file("/home/linux/1127/zuoye/details.html", alloc_socket);

	}

	printf("end\n");

	close(alloc_socket);
	memset(tmp, 0, sizeof(tmp));

	sqlite3_close(db);
	sqlite3_close(db2);
}

/* 用户账户回调函数 */
int callbackuser(void*arg,int col ,char** result ,char**title)
{
	*(int*)arg = 1;

	return 0;
}
/* 商品信息回调函数 */
int callbackgoods(void*arg,int col ,char** result ,char**title)
{
	printf("进入callbackgoods回调函数\n");
	printf("====%s====\n", result[0]);
	char tmpbuf[4096] = {0};
	
	sprintf(tmpbuf, "<a href=details/%s><p align='center'><img src=/home/linux/1127/zuoye/%s></p></a>\n",result[0], result[20]);	fputs(tmpbuf, fd);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "<p align='center'>  %s 原价:%s 现价:%s </p>\n", result[3], result[10], result[11]);	fputs(tmpbuf, fd);

	
	*(int*)arg = 1;

	return 0;
}

/* 商品详情回调函数 */
int callbackdetails(void*arg,int col ,char** result ,char**title)
{
	printf("进入callbackdetails回调函数\n");
	printf("====%s====\n", result[0]);
	char tmpbuf[4096] = {0};
	printf("%s\n", result[20]);
	sprintf(tmpbuf, "<p align='center'><img src=/home/linux/1127/zuoye/%s></p>\n", result[20]);	fputs(tmpbuf, fd);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "<p align='center'>  %s 原价:%s 现价:%s %s %s</p>\n", result[3], result[10], result[11], result[16], result[18]);	fputs(tmpbuf, fd);
	printf("回调结束\n");

	return 0;
}

/* 报文发送函数 */
int send_head(char *pfilename, int sockfd)
{
	int i = 0;
	int ret = 0;
	char tmp[32] = {0};
	char buf[4096] = {0};

	struct stat st;
	ret = stat(pfilename, &st);
	if(ret == -1)
	{
		perror("fail to stat");
		return -1;
	}
	sprintf(tmp, "content-length: %ld\r\n\r\n",st.st_size); sprintf(buf, "%s", tmp);

	char *head[5];
	head[0]="HTTP/1.1 200 OK\r\n";
	head[1]="Connection: Keep-alive\r\n";
	head[2]="Content-Type: text/html;charset=utf-8\r\n";
	head[3]="Server: BOOM/1.1\r\n";
	head[4]=tmp;

	for(i = 0 ;i<5;i++)
	{
		send(sockfd,head[i],strlen(head[i]),0);
		printf("%s\n", head[i]);

		if(ret == -1)
		{
			perror("fail to send");
			return -1;
		}
	}

	return 0;
}

/* 文件发送函数  */
int send_file(char *pfilename, int sockfd)
{
	int fd = 0;
	int ret = 0;
	char tmp[4096] = {0};

	fd = open(pfilename, O_RDONLY);
	if(fd == -1)
	{
		perror("fail to open");
		return -1;
	}
	while(1)
	{
		ret = read(fd, tmp, sizeof(tmp));
		if(ret <= 0)
		{
			break;
		}
		send(sockfd, tmp, ret, 0);
	}

	close(fd);

	return 0;
}

int main(int argc, const char *argv[])
{	
	int ret = 0;
	int alloc_socket = 0;
	struct sockaddr_in ser;
	int ser_len = 0;
	int sockfd = 0;
	pthread_t tid;	

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("fail to sockfd");
		return -1;
	}

	ser.sin_family = AF_INET;
	ser.sin_port = htons(IP_PORT);
	ser.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(sockfd, (struct sockaddr *)&ser, sizeof(ser));
	if(ret == -1)
	{
		perror("fail to bind");
		return -1;
	}

	ser_len = sizeof(ser);
	listen(sockfd, 3);
	sem_init(&SEMA,0,1);
	sem_init(&SEMB,0,0);
	while(1)
	{
		alloc_socket = accept(sockfd, NULL, NULL);
		if(alloc_socket == -1)
		{
			perror("fail to accept");
			return -1;
		}
		sem_wait(&SEMA);
		pthread_create(&tid, NULL, th, &alloc_socket);	
		sem_post(&SEMB);
		printf("链接成功, %d\n", alloc_socket);
		sleep(1);

	}
	close(sockfd);

	return 0;
}
