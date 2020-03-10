#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include "FTPAPI.h"
#include "FTPResponseCode.h"
#pragma comment(lib,"WS2_32.LIB") 

#define BUFSIZE 1024

/**
 * 作用: 创建一个Socket并返回.
 * 参数: IP或域名, 端口
 * 返回值: Socket套接字
 * */
SOCKET socket_connect(char *host, int port)
{
	int i = 0;
	//初始化 Socket dll
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 0);
	if (WSAStartup(socketVersion, &wsaData))
	{
		printf("Init socket dll error!");
		exit(1);
	}

	struct hostent * server = gethostbyname(host);
	if (!server)
		return -1;
	unsigned char ch[4];
	char ip[20];
	//一个hostname 可以对应多个ip
	while (server->h_addr_list[i] != NULL)
	{
		memcpy(&ch, server->h_addr_list[i], 4);
		sprintf(ip, "%d.%d.%d.%d", ch[0], ch[1], ch[2], ch[3]);
		//printf("%s\n",ip);
		i++;
	}

	//创建Socket
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
	if (SOCKET_ERROR == s)
	{
		printf("Create Socket Error!");
		exit(1);
	}
	//设置超时连接
	int timeout = 3000; //复杂的网络环境要设置超时判断
	int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));//发送时限
	ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));//接受时限
	//指定服务器地址
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.S_un.S_addr = inet_addr(ip);
	address.sin_port = htons((unsigned short)port);
	//连接
	if (SOCKET_ERROR == connect(s, (LPSOCKADDR)&address, sizeof(address)))
	{
		printf("Can Not Connect To Server IP!\n");
		exit(1);
	}
	return s;
}

/**
 * 作用: 连接到一个FTP服务器，返回socket
 * 参数: IP或域名, 端口
 * 返回值: Socket套接字
 * */
SOCKET connect_server(char *host, int port)
{
	SOCKET ctrl_sock;
	char buf[BUFSIZE];
	int result;
	size_t len;

	ctrl_sock = socket_connect(host, port);
	if (-1 == ctrl_sock)
	{
		return -1;
	}
	if ((len = recv(ctrl_sock, buf, BUFSIZE, 0)) > 0)
	{
		//len = recv(ctrl_sock, buf, BUFSIZE, 0);
		buf[len] = 0;
		printf("%s\n", buf); //220-FileZilla Server version 0.9.43 beta
	}
	sscanf(buf, "%d", &result);

	if (FTP_SERVICE_READY != result)
	{
		printf("FTP Not ready, Close the socet.");
		closesocket(ctrl_sock); //关闭Socket
		return -1;
	}
	return ctrl_sock;
}

/**
 * 作用: send发送命令，并返回recv结果
 * 参数: SOCKET，命令，命令返回码-命令返回描述，命令返回字节数
 * 返回值: 0 表示发送成功  -1表示发送失败
 * */
int ftp_sendcmd_re(SOCKET sock, char *cmd, char *re_buf, size_t *len)
{
	char buf[BUFSIZE];
	size_t r_len;
	if (send(sock, cmd, strlen(cmd), 0) == -1)
	{
		return -1;
	}
	r_len = recv(sock, buf, BUFSIZE, 0);
	if (r_len < 1)
		return -1;
	buf[r_len] = 0;
	if (NULL != len)
		*len = r_len;
	if (NULL != re_buf)
		sprintf(re_buf, "%s", buf);
	return 0;
}

/**
 * 作用: send发送命令
 * 参数: SOCKET,命令
 * 返回值: FTP响应码
 * */
int ftp_sendcmd(SOCKET sock, char *cmd)
{
	char buf[BUFSIZE];
	int result;
	size_t len;
	printf("FTP Client: %s", cmd);
	result = ftp_sendcmd_re(sock, cmd, buf, &len);
	printf("FTP Server: %s", buf);
	if (0 == result)
	{
		sscanf(buf, "%d", &result);
	}
	return result;
}

/**
 * 作用: 登录FTP服务器
 * 参数: SOCKET套接字，明文用户名，明文密码
 * 返回值: 0 表示登录成功   -1 表示登录失败
 * */
int login_server(SOCKET sock, char *user, char *pwd)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "USER %s\r\n", user);
	//这里要对socket进行阻塞
	int timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	result = ftp_sendcmd(sock, buf);
	if (FTP_LOGIN_SUCCESS == result) //直接登录
		return 0;
	else if (FTP_PASSWORD_REQUIREd == result) //需要密码
	{
		sprintf(buf, "PASS %s\r\n", pwd);
		result = ftp_sendcmd(sock, buf);
		if (FTP_LOGIN_SUCCESS == result)
		{
			return 0;
		}
		else //530 密码错误
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

/**
 * 作用: winsock使用后，要调用WSACleanup函数关闭网络设备，以便释放其占用的资源
 * 参数: SOCKET
 * 返回值: 无
 * */
void socket_close(int c_sock)
{
	WSACleanup();
}

/**
 * 作用: 连接到FTP服务器
 * 参数: hostname或IP，端口，用户名，密码
 * 返回值: 已连接到FTP服务器的SOCKET   -1 表示登录失败
 * */
SOCKET ftp_connect(char *host, int port, char *user, char *pwd)
{
	SOCKET sock;
	sock = connect_server(host, port);
	if (-1 == sock)
	{
		return -1;
	}
	if (-1 == login_server(sock, user, pwd))
	{
		closesocket(sock);
		return -1;
	}
	return sock;
}

/**
 * 作用: 断开FTP服务器
 * 参数: SOCKET
 * 返回值: 成功断开状态码
 * */
int ftp_quit(SOCKET sock)
{
	int result = 0;
	result = ftp_sendcmd(sock, "QUIT\r\n");
	closesocket(sock);
	socket_close(sock);
	return result;
}

/**
 * 作用: 设置FTP传输类型 A:ascii I:Binary
 * 参数: SOCkET，类型
 * 返回值: 0 表示成功   -1 表示失败
 * */
int ftp_type(SOCKET sock, char mode)
{
	char buf[BUFSIZ];
	sprintf(buf, "TYPE %c\r\n", mode);
	if (FTP_SUCCESS != ftp_sendcmd(sock, buf))
		return -1;
	else
		return 0;
}

/**
 * 作用: 更改工作目录
 * 参数: SOCKET，工作目录
 * 返回值: 0 表示成功  -1 表示失败
 * */
int ftp_cwd(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "CWD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)  //250 文件行为完成
		return -1;
	else
		return 0;
}

/**
 * 作用: 回到上级目录
 * 参数: SOCKET
 * 返回值: 0 正常操作返回  result 服务器返回响应码
 * */
int ftp_cdup(SOCKET sock)
{
	int result;
	result = ftp_sendcmd(sock, "CDUP\r\n");
	if (FTP_FILE_ACTION_COMPLETE == result || FTP_SUCCESS == result)
		return 0;
	else
		return result;
}

/**
 * 作用: 创建目录
 * 参数: SOCKET，文件目录路径(可相对路径，绝对路径)
 * 返回值: 0 正常操作返回  result 服务器返回响应码
 * */
int ftp_mkd(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "MKD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_CREATED != result) //257 路径名建立
		return result; //550 目录已存在
	else
		return 0;
}

/**
 * 作用: 连接到PASV接口
 *       PASV（被动）方式的连接过程是：
 *       客户端向服务器的FTP端口（默认是21）发送连接请求，
 *       服务器接受连接，建立一条命令链路。
 * 参数: 命令链路SOCKET cmd-socket
 * 返回值: 数据链路SOCKET raw-socket  -1 表示创建失败
 * */
SOCKET ftp_pasv_connect(SOCKET c_sock)
{
	SOCKET r_sock;
	int send_result;
	size_t len;
	int addr[6]; //IP*4+Port*2
	char buf[BUFSIZE];
	char result_buf[BUFSIZE];

	//设置PASV被动模式
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "PASV\r\n");
	send_result = ftp_sendcmd_re(c_sock, buf, result_buf, &len);
	if (send_result == 0)
	{
		sscanf(result_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",
			&addr[0], &addr[1], &addr[2], &addr[3],
			&addr[4], &addr[5]);
	}

	//连接PASV端口
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
	r_sock = socket_connect(buf, addr[4] * 256 + addr[5]);
	if (-1 == r_sock)
		return -1;
	return r_sock;
}

/**
 * 作用: 列出FTP工作空间的所有目录
 * 参数: 命令链路SOCKET，工作空间，列表信息，列表信息大小
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码  -1 表示创建pasv错误
 * */
int ftp_list(SOCKET c_sock, char *path, char **data, int *data_len)
{
	SOCKET r_sock;
	char buf[BUFSIZE];
	int send_re;
	int result;
	size_t len, buf_len, total_len;

	//连接到PASV接口
	r_sock = ftp_pasv_connect(c_sock);
	if (-1 == r_sock)
	{
		return -1;
	}
	//发送LIST命令
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "LIST %s\r\n", path);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
		return send_re;
	len = total_len = 0;
	buf_len = BUFSIZE;
	char *re_buf = (char *)malloc(buf_len);
	while ((len = recv(r_sock, buf, BUFSIZE, 0)) > 0)
	{
		if (total_len + len > buf_len)
		{
			buf_len *= 2;
			char *re_buf_n = (char *)malloc(buf_len);
			memcpy(re_buf_n, re_buf, total_len);
			free(re_buf);
			re_buf = re_buf_n;
		}
		memcpy(re_buf + total_len, buf, len);
		total_len += len;
	}
	closesocket(r_sock);

	//向服务器接收返回值
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result != 226)
	{
		free(re_buf);
		return result;
	}
	*data = re_buf;
	*data_len = total_len;
	return 0;
}

/**
 * 作用: 删除目录
 * 参数: 命令链路SOCKET，路径目录
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码
 * */
int ftp_deletefolder(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "RMD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)
	{
		//550 Directory not empty.
		//550 Directory not found.
		return result;
	}
	return 0;
}

/**
 * 作用: 删除文件
 * 参数: 命令链路SOCKET，路径文件(相对/绝对)
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码
 * */
int ftp_deletefile(SOCKET sock, char *filename)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "DELE %s\r\n", filename);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != 250) //250 File deleted successfully
	{
		//550 File not found.
		return result;
	}
	return 0;
}

/**
 * 作用: 修改文件名&移动目录
 * 参数: 命令链路SOCKET，源地址，目的地址
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码
 * */
int ftp_renamefile(SOCKET sock, char *s, char *d)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "RNFR %s\r\n", s);
	result = ftp_sendcmd(sock, buf);
	if (350 != result) //350 文件行为暂停，因为要进行移动操作
		return result;
	sprintf(buf, "RNTO %s\r\n", d);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)
	{
		return result;
	}
	return 0;
}

/**
 * 作用: 从服务器复制文件到本地 RETR
 * 参数: SOCKET，源地址，目的地址，文件大小
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码
 *         -1:文件创建失败  -2 pasv接口错误
 * */
int ftp_server2local(SOCKET c_sock, char *s, char *d, int * size)
{
	SOCKET d_sock;
	size_t len, write_len;
	char buf[BUFSIZ];
	int result;
	*size = 0;
	//打开本地文件
	FILE * fp = fopen(d, "wb");
	if (NULL == fp)
	{
		printf("Can't Open the file.\n");
		return -1;
	}
	//设置传输模式
	ftp_type(c_sock, 'I');

	//连接到PASV接口 用于传输文件
	d_sock = ftp_pasv_connect(c_sock);
	if (-1 == d_sock)
	{
		fclose(fp); //关闭文件
		return -2;
	}

	//发送RETR命令
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "RETR %s\r\n", s);
	result = ftp_sendcmd(c_sock, buf);
	// 150 Opening data channel for file download from server of "xxxx"
	if (result >= 300 || result == 0) //失败可能是没有权限什么的，具体看响应码
	{
		fclose(fp);
		return result;
	}

	//开始向PASV读取数据(下载)
	memset(buf, sizeof(buf), 0);
	while ((len = recv(d_sock, buf, BUFSIZE, 0)) > 0)
	{
		write_len = fwrite(&buf, len, 1, fp);
		if (write_len != 1) //写入文件不完整
		{
			closesocket(d_sock); //关闭套接字
			fclose(fp); //关闭文件
			return -1;
		}
		if (NULL != size)
		{
			*size += write_len;
		}
	}
	//下载完成
	closesocket(d_sock);
	fclose(fp);

	//向服务器接收返回值
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	printf("%s\n", buf);
	sscanf(buf, "%d", &result);
	if (result >= 300)
	{
		return result;
	}
	//226 Successfully transferred "xxxx"
	return 0;
}

/**
 * 作用: 从本地复制文件到服务器 STOR
 * 参数: SOCKET，源地址，目的地址，文件大小
 * 返回值: 0 表示列表成功  result>0 表示其他错误响应码
 *         -1:文件创建失败  -2 pasv接口错误
 * */
int ftp_local2server(SOCKET c_sock, char *s, char *d, int * size)
{
	SOCKET d_sock;
	size_t len, send_len;
	char buf[BUFSIZE];
	FILE * fp;
	int send_re;
	int result;
	//打开本地文件
	fp = fopen(s, "rb");
	if (NULL == fp)
	{
		printf("Can't Not Open the file.\n");
		return -1;
	}
	//设置传输模式
	ftp_type(c_sock, 'I');
	//连接到PASV接口
	d_sock = ftp_pasv_connect(c_sock);
	if (d_sock == -1)
	{
		fclose(fp);
		return -1;
	}

	//发送STOR命令
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "STOR %s\r\n", d);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
	{
		fclose(fp);
		return send_re;
	}

	//开始向PASV通道写数据
	memset(buf, sizeof(buf), 0);
	while ((len = fread(buf, 1, BUFSIZE, fp)) > 0)
	{
		send_len = send(d_sock, buf, len, 0);
		if (send_len != len)
		{
			closesocket(d_sock);
			fclose(fp);
			return -1;
		}
		if (NULL != size)
		{
			*size += send_len;
		}
	}
	//完成上传
	closesocket(d_sock);
	fclose(fp);

	//向服务器接收响应码
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result >= 300)
	{
		return result;
	}
	return 0;
}

/**
 * 作用: 获取一行响应码
 * 参数:
 * 返回值:
 * */
int ftp_recv(SOCKET sock, char *re_buf, size_t *len)
{
	char buf[BUFSIZE];
	size_t r_len;
	int timeout = 3000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	r_len = recv(sock, buf, BUFSIZE, 0);
	timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if (r_len < 1)
		return -1;
	buf[r_len] = 0;
	if (NULL != len)
		*len = r_len;
	if (NULL != re_buf)
		sprintf(re_buf, "%s", buf);
	return 0;
}

int main() {
	int size = 144;
	SOCKET s = ftp_connect("127.0.0.1", 21, "user", "user"); //登录到FTP服务器
	int ret = ftp_local2server(s, "D:\cat.jpg", "cat.jpg", &size); //从本地上传文件到服务器
//	ftp_quit(s); //退出FTP服务器
}