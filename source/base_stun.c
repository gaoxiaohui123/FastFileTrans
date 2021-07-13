/*
 * Copyright (c) 2020
 * Created by gxh_20200602
 */

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif


#ifdef _WIN32
#include <direct.h>
#endif

#include <math.h>
#include <inttypes.h>
//#include <map>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
//#include <vld.h>
#include <Mmsystem.h>
#else
#include <sys/time.h>
#endif

#include "file_rtp.h"

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#define MAC_SIZE    18
#define IP_SIZE     16

// function declare
//int get_ip_by_domain(const char *domain, char *ip); // 根据域名获取ip
//int get_local_mac(const char *eth_inf, char *mac); // 获取本机mac
//int get_local_ip(const char *eth_inf, char *ip); // 获取本机ip




// 根据域名获取ip
int get_ip_by_domain(const char *domain, char *ip)
{
    char **pptr;
    struct hostent *hptr;

    hptr = gethostbyname(domain);
    if(NULL == hptr)
    {
        printf("gethostbyname error for host:%s/n", domain);
        return -1;
    }

    for(pptr = hptr->h_addr_list ; *pptr != NULL; pptr++)
    {
        if (NULL != inet_ntop(hptr->h_addrtype, *pptr, ip, IP_SIZE) )
        {
            return 0; // 只获取第一个 ip
        }
    }

    return -1;
}
#if 1
// 获取本机mac
int get_local_mac(const char *eth_inf, char *mac)
{
    struct ifreq ifr;
    int sd;

    bzero(&ifr, sizeof(struct ifreq));
    if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("get %s mac address socket creat error\n", eth_inf);
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, sizeof(ifr.ifr_name) - 1);

    if(ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
    {
        printf("get %s mac address error\n", eth_inf);
        close(sd);
        return -1;
    }

    snprintf(mac, MAC_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned char)ifr.ifr_hwaddr.sa_data[0],
        (unsigned char)ifr.ifr_hwaddr.sa_data[1],
        (unsigned char)ifr.ifr_hwaddr.sa_data[2],
        (unsigned char)ifr.ifr_hwaddr.sa_data[3],
        (unsigned char)ifr.ifr_hwaddr.sa_data[4],
        (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    close(sd);

    return 0;
}
#endif

#ifdef WIN32
void get_local_ip(char *ip, int size)
{
    WORD v = MAKEWORD(1, 1);
    WSADATA wsaData;
    WSAStartup(v, &wsaData); // 加载套接字库

    struct hostent *phostinfo = gethostbyname("");
    char *p = inet_ntoa (* ((struct in_addr *)(*phostinfo->h_addr_list)) );
    strncpy(ip, p, size - 1);
    ip[size - 1] = '\0';
    WSACleanup( );
}
#elif(0)
// 获取本机ip
int get_local_ip(const char *eth_inf, char *ip)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd)
    {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
    {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}
#else
int get_local_ip(char *ip, int size)
{
    int fd, intrface, retn = 0;
    struct ifreq buf[INET_ADDRSTRLEN];
    struct ifconf ifc;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        //caddr_t,Linux内核源码里定义的：typedef void *caddr_t；
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
        {
            intrface = ifc.ifc_len/sizeof(struct ifreq);
            while (intrface-- > 0)
            {
                if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
                {
                    ip = (inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
                    printf("get_local_ip: IP=%s \n", ip);
                }
            }
        }
        close(fd);
    }
    else{
        MYPRINT2("get_local_ip: error \n");
    }
    return 0;
}
int get_local_port(SOCKFD fd)
{
    int ret = 0;
    struct sockaddr_in localaddr;
    ///一定要给出结构体大小，要不然获取到的端口号可能是0
    socklen_t len = sizeof(localaddr); ///fd是创建的套接字
    ret = getsockname(fd, (struct sockaddr*)&localaddr, &len);
    if(ret != 0)
    {
        perror("get_local_port: 1: getsockname");
    }
    else
    {
        perror("get_local_port: 2: getsockname");
        printf("get_local_port: port: %d\n", ntohs(localaddr.sin_port));
    }
    return ret;
}
#endif

int socket_close(SocketObj *obj)
{
    int ret = 0;
    char *p0;
    if (pthread_join(obj->recv_pid, (void**)&p0))
    {
        MYPRINT2("socket_close: xxx_recv_run thread is not exit...\n");
    }
    else{
        MYPRINT2("socket_close: p0=%s \n", p0);
        free(p0);
    }

#if defined (WIN32)
    closesocket(obj->sock_fd);
#elif defined(__linux__)
    close(obj->sock_fd);
#endif
    pthread_mutex_destroy(&obj->lock);
    return ret;
}
int send_data(SocketObj *obj, char *data, int size, struct sockaddr_in addr_client)
{
    int ret = 0;
    int len = sizeof(obj->addr_serv);
    ret = sendto(obj->sock_fd, data, size, 0, (struct sockaddr *)&addr_client, len);
    return ret;
}
int server_recv_run(SocketObj *obj)
{
    int ret = 0;
    //
    struct sockaddr_in addr_client;
    int len = sizeof(obj->addr_serv);
    obj->status = 1;
    while(obj->status)
    {
        char recv_buf[1500] = "";
        int data_len = 1500;
        int recv_num = recvfrom(obj->sock_fd, recv_buf, data_len, 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        MYPRINT2("server_recv_run: recv_num=%d \n", recv_num);
        int remote_port = addr_client.sin_port;
        //int remote_ip = addr_client.sin_addr.s_addr;
        char *p_remote_ip = inet_ntoa(addr_client.sin_addr);
        MYPRINT2("server_recv_run: p_remote_ip=%s, remote_port=%d \n", p_remote_ip, remote_port);
        if(recv_num <= 0)
        {
            if(errno != EAGAIN && !recv_num)
            {
                MYPRINT2("server_recv_run: recv_num=%d \n", recv_num);
                //free(recv_buf);
                perror("server_recv_run: recvfrom error:");
                MYPRINT2("server_recv_run: errno=%d \n", errno);
                if(!recv_num)
                {
                    ret = -1;
                    break;
                }
            }
        }
    }
    //
    char *p = malloc(32);
    strcpy(p,"server_recv_run exit");
    pthread_exit((void*)p);
    return ret;
}
int server_init(SocketObj *obj)
{
    int ret = 0;
    /* sock_fd --- socket文件描述符 创建udp套接字*/
    obj->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(obj->sock_fd < 0)
    {
        perror("server_init: socket");
        return -1;
        //exit(1);
    }

    /* 将套接字和IP、端口绑定 */
    //struct sockaddr_in addr_serv;
    //int len;
    memset(&obj->addr_serv, 0, sizeof(struct sockaddr_in));  //每个字节都用0填充
    obj->addr_serv.sin_family = AF_INET;                       //使用IPV4地址
    obj->addr_serv.sin_port = htons(obj->port);                //端口
    /* INADDR_ANY表示不管是哪个网卡接收到数据，只要目的端口是obj->port，就会被该应用程序接收到 */
    obj->addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);  //自动获取IP地址
        //超时时间
#ifdef _WIN32
    int timeout = 500;//ms
#else
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;//500ms
#endif
	socklen_t len = sizeof(timeout);
	ret = setsockopt(obj->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
	if (ret == -1) {
		return -1;
	}
	ret = setsockopt(obj->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
	if (ret == -1) {
	    return -1;
	}
    //
    len = sizeof(obj->addr_serv);
    /* 绑定socket */
    if(bind(obj->sock_fd, (struct sockaddr *)&obj->addr_serv, sizeof(obj->addr_serv)) < 0)
    {
        perror("server_init: bind error:");
        return -1;
    }
    pthread_mutex_init(&obj->lock,NULL);

    if(pthread_create(&obj->recv_pid, NULL, server_recv_run, obj) < 0)
    {
        MYPRINT2("server_init: Create server_recv_run failed!\n");
    }

    return ret;
}

int client_recv_run(SocketObj *obj)
{
    int ret = 0;
    //
    struct sockaddr_in addr_client;
    int len = sizeof(obj->addr_serv);
    obj->status = 1;
    while(obj->status)
    {
        char recv_buf[1500] = "";
        int data_len = 1500;
        int recv_num = recvfrom(obj->sock_fd, recv_buf, data_len, 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        MYPRINT2("client_recv_run: recv_num=%d \n", recv_num);
        if(recv_num <= 0)
        {
            if(errno != EAGAIN && !recv_num)
            {
                MYPRINT2("client_recv_run: recv_num=%d \n", recv_num);
                //free(recv_buf);
                perror("client_recv_run: recvfrom error:");
                MYPRINT2("client_recv_run: errno=%d \n", errno);
                if(!recv_num)
                {
                    ret = -1;
                    break;
                }
            }
        }
    }
    //
    char *p = malloc(32);
    strcpy(p,"client_recv_run exit");
    pthread_exit((void*)p);
    return ret;
}
int client_init(SocketObj *obj)
{
    int ret = 0;
    //
    char ip[1024] = "";
    get_local_ip(ip, 1024);//test

    /* 建立udp socket */
    obj->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(obj->sock_fd < 0)
    {
        perror("client_init: socket");
        return -1;
    }
    //超时时间
#ifdef _WIN32
    int timeout = 500;//ms
#else
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;//500ms
#endif
	socklen_t len = sizeof(timeout);
	ret = setsockopt(obj->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
	if (ret == -1) {
		perror("client_init error:");
		return -1;
	}
	ret = setsockopt(obj->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
	if (ret == -1) {
	    perror("client_init error:");
	    return -1;
	}
    /* 设置address */
    //struct sockaddr_in addr_serv;
    memset(&obj->addr_serv, 0, sizeof(obj->addr_serv));
    obj->addr_serv.sin_family = AF_INET;
    obj->addr_serv.sin_addr.s_addr = inet_addr(obj->server_ip);
    obj->addr_serv.sin_port = htons(obj->port);//换为网络字节序

    get_local_port(obj->sock_fd);//test

    pthread_mutex_init(&obj->lock,NULL);

    if(pthread_create(&obj->recv_pid, NULL, client_recv_run, obj) < 0)
    {
        MYPRINT2("client_init: Create client_recv_run failed!\n");
    }

    return ret;
}
FQT_API
int api_sock_test(char *server_ip, int port)
{
    int ret = 0;
    SocketObj *server = (SocketObj *)calloc(1, sizeof(SocketObj));
    SocketObj *client = (SocketObj *)calloc(1, sizeof(SocketObj));
    server->port = port;//10080;
    strcpy(server->server_ip, server_ip);
    strcpy(client->server_ip, server_ip);
    client->port = port;
    ret = server_init(server);
    if(ret < 0)
    {
        return ret;
    }
    ret = client_init(client);
    if(ret < 0)
    {
        return ret;
    }
    usleep(1000 * 1000);//
    char send_buf[1500];
    ret = send_data(client, send_buf, 300, client->addr_serv);
    usleep(1000 * 1000);//
    client->status = 0;
    server->status = 0;
    socket_close(client);
    socket_close(server);
    return ret;
}
/****** main test **********/
int main(void)
{
    char ip[IP_SIZE];
    char mac[MAC_SIZE];
    const char *test_domain = "www.baidu.com";
    const char *test_eth = "eth0";

    get_ip_by_domain(test_domain, ip);
    printf("%s ip: %s\n", test_domain, ip);

    get_local_mac(test_eth, mac);
    printf("local %s mac: %s\n", test_eth, mac);

    //get_local_ip(test_eth, ip);
    //printf("local %s ip: %s\n", test_eth, ip);

    return 0;
}
#if 0
struct sockaddr_in localaddr;
    ///一定要给出结构体大小，要不然获取到的端口号可能是0
    socklen_t len = sizeof(localaddr);　　　　///fd是创建的套接字
    int ret = getsockname(fd, (struct sockaddr*)&localaddr, &len);

    if(ret != 0)
    {
        perror("getsockname");
    }
    else
    {
        perror("getsockname");
        printf("port: %d\n", ntohs(localaddr.sin_port));
    }
#endif