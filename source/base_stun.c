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

extern int stun_create_node(CStunNode **head0);
extern int stun_add_node(CStunNode *head, CStunNode **pnew);
extern void stun_delete_node(CStunNode *head);
extern void stun_free_node(CStunNode *head);

static unsigned int glob_session_id = 1;

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
#if 0
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
//数据包包含了源端口号和目的端口号，客户端socket向服务端发起连接时，系统会给socket随机分配一个源端口号，
//我们可以通过getsocketname来获取连接成功的socket的原端口信息
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
        char *p_local_ip = inet_ntoa(localaddr.sin_addr);
        printf("get_local_port: p_local_ip: %s\n", p_local_ip);
        printf("get_local_port: port: %d\n", ntohs(localaddr.sin_port));
    }
    return ret;
}
#endif


int socket_close(SocketObj *obj)
{
    int ret = 0;
    if(obj->recv_pid > 0)
    {
        char *p0;
        if (pthread_join(obj->recv_pid, (void**)&p0))
        {
            MYPRINT2("socket_close: xxx_recv_run thread is not exit...\n");
        }
        else{
            MYPRINT2("socket_close: p0=%s \n", p0);
            free(p0);
        }
    }
    if(obj->hb_pid > 0)
    {
        char *p0;
        if (pthread_join(obj->hb_pid, (void**)&p0))
        {
            MYPRINT2("socket_close: xxx_hb_run thread is not exit...\n");
        }
        else{
            MYPRINT2("socket_close: p0=%s \n", p0);
            free(p0);
        }
    }
    if(obj->ping_pid0 > 0)
    {
        char *p0;
        if (pthread_join(obj->ping_pid0, (void**)&p0))
        {
            MYPRINT2("socket_close: xxx_ping_run thread is not exit...\n");
        }
        else{
            MYPRINT2("socket_close: p0=%s \n", p0);
            free(p0);
        }
    }
    if(obj->ping_pid1 > 0)
    {
        char *p0;
        if (pthread_join(obj->ping_pid1, (void**)&p0))
        {
            MYPRINT2("socket_close: xxx_ping_run thread is not exit...\n");
        }
        else{
            MYPRINT2("socket_close: p0=%s \n", p0);
            free(p0);
        }
    }


#if defined (WIN32)
    closesocket(obj->sock_fd);
#elif defined(__linux__)
    close(obj->sock_fd);
#endif
    pthread_mutex_destroy(&obj->lock);
    return ret;
}
int send_data(SocketObj *obj, char *data, int size, struct sockaddr_in addr_client, int64_t now_time)
{
    int ret = 0;
    int len = sizeof(addr_client);
    pthread_mutex_lock(&obj->lock);
    ret = sendto(obj->sock_fd, data, size, 0, (struct sockaddr *)&addr_client, len);
    obj->last_send_time = now_time;
    pthread_mutex_unlock(&obj->lock);
    return ret;
}
int heart_beat_run(SocketObj *obj)
{
    int ret = 0;
    int interval = HEARTBEAT_TIME;
    while(obj->status)
    {
        int64_t now_time = (int64_t)api_get_sys_time(0);
        pthread_mutex_lock(&obj->lock);
        int64_t last_time = obj->last_send_time;//  # 避免意外包
        pthread_mutex_unlock(&obj->lock);
        if(last_time)
        {
            int difftime = (int)(now_time - last_time);
            //interval = HEARTBEAT_TIME
            int wait_time = (interval - difftime);// #199, 200, 300
            wait_time = wait_time < 1000 ? 1000 : wait_time;
            //# 周期内无网络传输；
            //# 周期内有网络传输，且在100秒前;
            //# 100秒内已发生过网络传输，则取消当下发送；
            //# 心跳包最大间隔为为300s
            if(difftime > interval)
            {
                char data[300] = "";
                StunInfo *p = (StunInfo *)data;
                memcpy(p, &obj->stunInfo, sizeof(StunInfo));
                //
	            p->time_stamp0 = now_time & 0xFFFFFFFF;
	            p->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
	            p->cmdtype = kHeartBeat;
                ret = send_data(obj, data, sizeof(StunInfo), obj->addr_serv, now_time);
                ret = 1;
            }
            else{
                usleep(wait_time * 1000);
            }
        }
        else{
            usleep(1000 * 1000);//1s
        }
    }
    //
    char *p = malloc(32);
    strcpy(p,"heart_beat_run exit");
    pthread_exit((void*)p);
    return ret;
}
int send_stun_list(SocketObj *obj, CStunNode *head, CStunNode *pnew, struct sockaddr_in addr_client, int64_t now_time)
{
    int ret = 0;
    if(head->num > 1)
    {
        MYPRINT2("send_stun_list: head->num=%d \n", head->num);
        char send_buf[MAX_MTU_SIZE] = "";
        int data_len = MAX_MTU_SIZE;
        int offset = 0;
        int info_size = sizeof(StunInfo);
        uint32_t session_id = pnew->data->session_id;
        uint32_t self_session_id = pnew->data->self_session_id;
        //send the new client to others
        CStunNode *p;
        p = head;
        do{
            p = p->next;
            CStunNode *info = (CStunNode *)p;
            if(p)
            {
                p->data->cmdtype = kPeers;
                if(p->data->self_session_id != self_session_id)
                {
                    ret = send_data(obj, pnew->data, info_size, p->addr_client, now_time);//send the new client to others
                }
                memcpy(&send_buf[offset], p->data, info_size);
                offset += info_size;
                if((offset + info_size) > MAX_MTU_SIZE)
                {
                    ret = send_data(obj, send_buf, offset, addr_client, now_time);//send others to the new client
                    offset = 0;
                }
            }
        }while(p->next);
        if(offset > 0)
        {
            ret = send_data(obj, send_buf, offset, addr_client, now_time);//send others to the new client
            offset = 0;
        }
    }
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
        char recv_buf[MAX_MTU_SIZE] = "";
        int data_len = MAX_MTU_SIZE;
        int recv_num = recvfrom(obj->sock_fd, recv_buf, data_len, 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);

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
        else{
            MYPRINT2("server_recv_run: recv_num=%d \n", recv_num);
            int remote_port = addr_client.sin_port;
            char *p_remote_ip = inet_ntoa(addr_client.sin_addr);
            {
                int64_t now_time = (int64_t)api_get_sys_time(0);
                char *data = recv_buf;
                StunInfo *p = (StunInfo *)data;
                CMDType cmdtype = p->cmdtype;
                uint32_t this_session_id = p->session_id;
                MYPRINT2("server_recv_run: addr0= %s:%d \n", p->local_ip, p->local_port);
                //
                uint64_t time_stamp0 = p->time_stamp0;
	            uint64_t time_stamp1 = p->time_stamp1;
	            int64_t packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	            int delay_time = (int)(now_time - packet_time_stamp);
	            MYPRINT2("server_recv_run: addr1= %s:%d, delay_time=%d \n", p_remote_ip, remote_port, delay_time);

                strcpy(p->remote_ip, p_remote_ip);
                p->remote_port = (uint16_t)remote_port;
                p->session_id = this_session_id;
                if(!this_session_id && (cmdtype == kReg))
                {
                    MYPRINT2("server_recv_run: this_session_id=%u, glob_session_id=%u \n", this_session_id, glob_session_id);
                    glob_session_id++;
                    this_session_id = glob_session_id;
                    p->session_id = this_session_id;
                    p->self_session_id = this_session_id;
                    //
                    ClientInfo *thisClientInfo = &obj->pClientInfo[this_session_id % MAX_ONLINE_NUM];
                    stun_create_node(&thisClientInfo->head);
                    thisClientInfo->head->session_id = this_session_id;
                    CStunNode *pnew = (CStunNode *)calloc(1, sizeof(CStunNode));
                    pnew->data = (StunInfo *)calloc(1, sizeof(StunInfo));
                    memcpy(pnew->data, p, sizeof(StunInfo));
                    pnew->addr_client = addr_client;
                    int client_id = stun_add_node(thisClientInfo->head, &pnew);
                    p->client_id = client_id;
                    ret = send_data(obj, (char *)p, sizeof(StunInfo), addr_client, now_time);//获得session_id
                }
                else if(this_session_id > 0 && (this_session_id <= glob_session_id))
                {
                    ClientInfo *thisClientInfo = &obj->pClientInfo[this_session_id % MAX_ONLINE_NUM];
                    if(thisClientInfo->head)
                    {
                        switch (cmdtype)
		                {
		                	case kPeers:
		                	{
		                	    if(thisClientInfo->head)
		                	    {
		                	        CStunNode *pnew = (CStunNode *)calloc(1, sizeof(CStunNode));
                                    pnew->data = (StunInfo *)calloc(1, sizeof(StunInfo));
                                    memcpy(pnew->data, p, sizeof(StunInfo));
                                    pnew->addr_client = addr_client;
                                    int client_id = stun_add_node(thisClientInfo->head, &pnew);
                                    p->client_id = client_id;
                                    //ret = send_data(obj, data, sizeof(StunInfo), addr_client, now_time);//获得client_id
                                    ret = send_stun_list(obj, thisClientInfo->head, pnew, addr_client, now_time);
		                	    }
		                	    break;
		                	}
		                	case kHeartBeat:
		                	{
		                	    MYPRINT2("server_recv_run: kHeartBeat: cmdtype=%d \n", cmdtype);
		                	    ret = send_data(obj, (char *)p, sizeof(StunInfo), addr_client, now_time);
		                	    break;
		                	}
		                	case kBye:
		                	{
		                	    MYPRINT2("server_recv_run: kBye: cmdtype=%d ############################ \n", cmdtype);
		                	    ret = send_data(obj, (char *)p, sizeof(StunInfo), addr_client, now_time);
		                	    break;
		                	}
		                	case kExit:
		                	{
		                	    ClientInfo *thisClientInfo = &obj->pClientInfo[this_session_id % MAX_ONLINE_NUM];
                                stun_free_node(thisClientInfo->head);
                                ret = send_data(obj, (char *)p, sizeof(StunInfo), addr_client, now_time);
		                	    break;
		                	}
		                	default:
		                	{
		                	    MYPRINT2("server_recv_run: cmdtype=%d \n", cmdtype);
		                		break;
		                	}
		                }
                    }
                    else{
                        MYPRINT2("server_recv_run: cmdtype=%d, thisClientInfo->head=%x \n", cmdtype, thisClientInfo->head);
                    }
                }
                else{
                    MYPRINT2("warning: server_recv_run: this_session_id=%u, glob_session_id=%u \n", this_session_id, glob_session_id);
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
    obj->pClientInfo = (ClientInfo *)calloc(1, MAX_ONLINE_NUM * sizeof(ClientInfo));
    if(pthread_create(&obj->recv_pid, NULL, server_recv_run, obj) < 0)
    {
        MYPRINT2("server_init: Create server_recv_run failed!\n");
    }

    return ret;
}
int ping_loop(SocketObj *obj, ClientInfo *thisClientInfo)
{
    int ret = 0;
    unsigned int self_session_id = obj->stunInfo.self_session_id;
    MYPRINT2("ping_loop: self_session_id=%u \n", self_session_id);
    printf("ping_loop: addr0= %s:%d \n", obj->local_ip, (int)obj->local_port);
    while(obj->status)
    {
        int64_t now_time = (int64_t)api_get_sys_time(0);
        int info_size = sizeof(StunInfo);
        StunInfo stunInfo;
        memcpy(&stunInfo, &obj->stunInfo, info_size);
        stunInfo.cmdtype = kPing;
        stunInfo.time_stamp0 = now_time & 0xFFFFFFFF;
        stunInfo.time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
        CStunNode *p;
        pthread_mutex_lock(&obj->lock);
        CStunNode *head = thisClientInfo->head;
        p = head;
        do{
            p = p->next;
            if(p && (p->data->self_session_id != self_session_id))
            {
                if(!(p->cnn_status & 1) && p->ping_times < 10)
                {
                    MYPRINT2("ping_loop: p->ping_times=%d, p->data->self_session_id=%d \n", \
                    p->ping_times, p->data->self_session_id);
                    struct sockaddr_in addr_client;
                    memset(&addr_client, 0, sizeof(addr_client));
                    addr_client.sin_family = AF_INET;
                    //外网通了，内网没通，则继续stun指导１０次
                    if(!(p->cnn_status & 1))
                    {
                        addr_client.sin_port = htons(p->data->local_port);
                        addr_client.sin_addr.s_addr = inet_addr(p->data->local_ip);
                        p->last_send_time = now_time;
                        pthread_mutex_unlock(&obj->lock);
                        MYPRINT2("ping_loop: addr0: %s:%d \n", p->data->local_ip, p->data->local_port);
                        ret = send_data(obj, (char *)&stunInfo, info_size, addr_client, now_time);
                        pthread_mutex_lock(&obj->lock);
                    }
                    //
                    if(!(p->cnn_status & 2))
                    {
                        addr_client.sin_port = htons(p->data->remote_port);
                        addr_client.sin_addr.s_addr = inet_addr(p->data->remote_ip);
                        p->last_send_time = now_time;
                        pthread_mutex_unlock(&obj->lock);
                        MYPRINT2("ping_loop: addr1: %s:%d \n", p->data->remote_ip, p->data->remote_port);
                        ret = send_data(obj, (char *)&stunInfo, info_size, addr_client, now_time);
                        pthread_mutex_lock(&obj->lock);
                    }
                    //
                    p->ping_times++;
                }
                else if(p->cnn_status)
                {
                    int64_t last_time = p->last_send_time;
                    int difftime = (int)(now_time - last_time);
                    int interval = HEARTBEAT_TIME;
                    //int wait_time = (interval - difftime);// #199, 200, 300
                    //wait_time = wait_time < 1000 ? 1000 : wait_time;
                    //# 周期内无网络传输；
                    //# 周期内有网络传输，且在100秒前;
                    //# 100秒内已发生过网络传输，则取消当下发送；
                    //# 心跳包最大间隔为为300s
                    if(difftime > interval)
                    {
                        stunInfo.cmdtype = kHeartBeat;
                        struct sockaddr_in addr_client;
                        if(!(p->cnn_status & 1))
                        {
                            addr_client.sin_port = htons(p->data->local_port);
                            addr_client.sin_addr.s_addr = inet_addr(p->data->local_ip);
                            pthread_mutex_unlock(&obj->lock);
                            ret = send_data(obj, (char *)&stunInfo, info_size, addr_client, now_time);
                            p->last_send_time = now_time;
                            pthread_mutex_lock(&obj->lock);
                        }
                        else if(!(p->cnn_status & 2))
                        {
                            addr_client.sin_port = htons(p->data->remote_port);
                            addr_client.sin_addr.s_addr = inet_addr(p->data->remote_ip);
                            pthread_mutex_unlock(&obj->lock);
                            ret = send_data(obj, (char *)&stunInfo, info_size, addr_client, now_time);
                            p->last_send_time = now_time;
                            pthread_mutex_lock(&obj->lock);
                        }
                    }
                }
            }
        }while(p->next);
        pthread_mutex_unlock(&obj->lock);
        usleep(100 * 1000);
    }
    return ret;
}
int ping0_run(SocketObj *obj)
{
    int ret = 0;
    MYPRINT2("ping0_run \n");
    ClientInfo *thisClientInfo = &obj->pClientInfo[0];//
    ret = ping_loop(obj, thisClientInfo);
    //
    char *p = malloc(32);
    strcpy(p,"ping0_run exit");
    pthread_exit((void*)p);
    return ret;
}
int ping1_run(SocketObj *obj)
{
    int ret = 0;
    MYPRINT2("ping1_run \n");
    ClientInfo *thisClientInfo = &obj->pClientInfo[1];//
    ret = ping_loop(obj, thisClientInfo);
    //
    char *p = malloc(32);
    strcpy(p,"ping1_run exit");
    pthread_exit((void*)p);
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
        int data_len = MAX_MTU_SIZE;
        int info_size = sizeof(StunInfo);
        char recv_buf[MAX_MTU_SIZE] = "";
        int recv_num = recvfrom(obj->sock_fd, recv_buf, data_len, 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);

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
        else{
            MYPRINT2("client_recv_run: recv_num=%d \n", recv_num);
            int64_t now_time = (int64_t)api_get_sys_time(0);
            int remote_port = addr_client.sin_port;
            char *p_remote_ip = inet_ntoa(addr_client.sin_addr);
            //
            char *data = recv_buf;
            StunInfo *p = (StunInfo *)data;
            uint32_t this_session_id = p->session_id;
            uint32_t self_session_id = p->self_session_id;
            int actor = p->actor;
            int cmdtype = p->cmdtype;
            uint64_t time_stamp0 = p->time_stamp0;
	        uint64_t time_stamp1 = p->time_stamp1;
	        int64_t packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	        int delay_time = (int)(now_time - packet_time_stamp);
	        MYPRINT2("client_recv_run: p_remote_ip=%s, remote_port=%d, delay_time=%d \n", p_remote_ip, remote_port, delay_time);
            MYPRINT2("client_recv_run: session_id %d addr1= %s:%d \n", \
            this_session_id, p->remote_ip, p->remote_port);
            if(this_session_id > 0)
            {
                if(!obj->stunInfo.session_id && (cmdtype == kReg))
                {
                    //
                    pthread_mutex_lock(&obj->lock);
                    ClientInfo *thisClientInfo = &obj->pClientInfo[0];//
                    stun_create_node(&thisClientInfo->head);
                    thisClientInfo->head->session_id = this_session_id;
                    CStunNode *pnew = (CStunNode *)calloc(1, sizeof(CStunNode));
                    pnew->data = (StunInfo *)calloc(1, sizeof(StunInfo));
                    memcpy(pnew->data, p, sizeof(StunInfo));
                    pnew->addr_client = addr_client;
                    stun_add_node(thisClientInfo->head, &pnew);
                    pthread_mutex_unlock(&obj->lock);
                    //
                    memcpy(&obj->stunInfo, p, sizeof(StunInfo));
                    obj->stunInfo.self_session_id = this_session_id;
                    //
                    //p->cmdtype = kBye;
                    //ret = send_data(obj, data, sizeof(StunInfo), addr_client, now_time);//test
                }
                else{
                    switch (cmdtype)
		            {
		            	case kPeers:
		            	{
		            	    int offset = 0;
		            	    int info_size = sizeof(StunInfo);
		            	    MYPRINT2("client_recv_run: kPeers \n" );
		            	    //
		            	    pthread_mutex_lock(&obj->lock);
		            	    ClientInfo *thisClientInfo = &obj->pClientInfo[0];//

		            	    if(this_session_id != obj->stunInfo.self_session_id)
		            	    {
		            	        thisClientInfo = &obj->pClientInfo[1];//
		            	        stun_create_node(&thisClientInfo->head);
		            	        thisClientInfo->head->session_id = this_session_id;
		            	    }
		            	    pthread_mutex_unlock(&obj->lock);
		            	    //
		            	    while(offset < recv_num)
		            	    {
		            	        p = (StunInfo *)&data[offset];
		            	        //
		            	        pthread_mutex_lock(&obj->lock);
		            	        CStunNode *pnew = (CStunNode *)calloc(1, sizeof(CStunNode));
                                pnew->data = (StunInfo *)calloc(1, sizeof(StunInfo));
                                memcpy(pnew->data, p, sizeof(StunInfo));
                                //pnew->addr_client = addr_client;
                                if(p->self_session_id != thisClientInfo->head->session_id)
                                {
                                    stun_add_node(thisClientInfo->head, &pnew);
                                }
                                MYPRINT("client_init: thisClientInfo->head->num=%d \n", thisClientInfo->head->num);
                                pthread_mutex_unlock(&obj->lock);
                                //
		            	        offset += info_size;
		            	    }
		            	    MYPRINT2("client_init: obj->ping_pid0=%d, obj->ping_pid1=%d \n", obj->ping_pid0, obj->ping_pid1);
		            	    if(this_session_id != obj->stunInfo.self_session_id)
		            	    {
		            	        MYPRINT2("client_init: 1: thisClientInfo->head->num=%d \n", thisClientInfo->head->num);
                                if(!obj->ping_pid1 && thisClientInfo->head->num > 1)
		            	        {
		            	            if(pthread_create(&obj->ping_pid1, NULL, ping1_run, obj) < 0)
                                    {
                                        MYPRINT2("client_init: Create heart_beat_run failed!\n");
                                    }
		            	        }
		            	    }
		            	    else
		            	    {
		            	        MYPRINT2("client_init: 0: thisClientInfo->head->num=%d \n", thisClientInfo->head->num);
		            	        if(!obj->ping_pid0 && thisClientInfo->head->num > 1)
		            	        {
		            	            if(pthread_create(&obj->ping_pid0, NULL, ping0_run, obj) < 0)
                                    {
                                        MYPRINT2("client_init: Create heart_beat_run failed!\n");
                                    }
		            	        }
		            	    }
		            	    break;
		            	}
		            	case kPing:
		            	{
		            	    int ack = p->ack;
		            	    MYPRINT2("client_recv_run: kPing \n" );
		            	    if(!ack)
		            	    {
		            	        p->ack = 1;
		            	        ret = send_data(obj, data, sizeof(StunInfo), addr_client, now_time);
		            	    }
		            	    else{
		            	        int ip_type = p->ip_type;
		            	        //
		            	        pthread_mutex_lock(&obj->lock);
		            	        ClientInfo *thisClientInfo = &obj->pClientInfo[0];//
		            	        if(this_session_id != obj->stunInfo.self_session_id)
		            	        {
		            	            thisClientInfo = &obj->pClientInfo[1];//
		            	        }
		            	        CStunNode *q = thisClientInfo->head;
                                do{
                                    q = q->next;
                                    if(q && q->data->self_session_id == self_session_id)
                                    {
                                        q->cnn_status |= 1 << ip_type;
                                        break;
                                    }
                                }while(q->next);
		            	        pthread_mutex_unlock(&obj->lock);
		            	        //
		            	        MYPRINT2("client_recv_run: kPing: sucess \n" );
		            	    }

		            	    break;
		            	}
		            	case kHeartBeat:
		            	{
		            	    MYPRINT2("client_recv_run: kHeartBeat \n" );
		            	    break;
		            	}
		            	default:
		            	{
		            	    MYPRINT2("warning: client_recv_run: this_session_id=%d, obj->stunInfo.session_id=%d, cmdtype=%d \n", \
                            this_session_id, obj->stunInfo.session_id, cmdtype);
		            	    break;
		            	}
		            }
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
    obj->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(obj->sock_fd < 0)
    {
        perror("client_init: socket");
        return -1;
    }
    struct sockaddr_in localaddr = {};
    socklen_t slen;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localaddr.sin_port = 0;

    bind(obj->sock_fd, (struct sockaddr *)&localaddr, sizeof(sin));
    //connect(obj->sock_fd, sizeof(sin));
    /* Now bound,get the address */
    if(false)
    {
        slen = sizeof(localaddr);
        getsockname(obj->sock_fd, (struct sockaddr*)&localaddr, &slen);
        int local_port = (int)ntohs(localaddr.sin_port);
        char *p_local_ip = inet_ntoa(localaddr.sin_addr);
        printf("client_init: p_local_ip: %s\n", p_local_ip);
        printf("client_init: local_port: %d\n", local_port);
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
    connect(obj->sock_fd, (struct sockaddr*)&obj->addr_serv, sizeof(obj->addr_serv));
    if(true)
    {
        slen = sizeof(localaddr);
        getsockname(obj->sock_fd, (struct sockaddr*)&localaddr, &slen);
        int local_port = (int)ntohs(localaddr.sin_port);
        char *p_local_ip = inet_ntoa(localaddr.sin_addr);
        printf("client_init: p_local_ip: %s\n", p_local_ip);
        printf("client_init: local_port: %d\n", local_port);
        obj->local_port = (uint16_t)local_port;
        strcpy(obj->local_ip, p_local_ip);
        obj->stunInfo.local_port = (uint16_t)local_port;
        strcpy(obj->stunInfo.local_ip, p_local_ip);
    }
    //get_local_port(obj->sock_fd);//test

    pthread_mutex_init(&obj->lock,NULL);
    obj->pClientInfo = (ClientInfo *)calloc(1, 2 * sizeof(ClientInfo));
    if(pthread_create(&obj->recv_pid, NULL, client_recv_run, obj) < 0)
    {
        MYPRINT2("client_init: Create client_recv_run failed!\n");
    }
    if(pthread_create(&obj->hb_pid, NULL, heart_beat_run, obj) < 0)
    {
        MYPRINT2("client_init: Create heart_beat_run failed!\n");
    }

    return ret;
}
FQT_API
int api_socket_start(char *handle, char *server_ip, int port, int type)
{
    int ret = 0;
    int64_t *testp = (int64_t *)handle;
    SocketObj *obj = (SocketObj *)testp[0];
    if(!obj)
    {
        obj = (SocketObj *)calloc(1, sizeof(SocketObj));
        obj->type = type;
        obj->port = port;//10080;
        strcpy(obj->server_ip, server_ip);
        if(!type)
        {
            ret = server_init(obj);
        }
        else{
            ret = client_init(obj);
        }
        if(ret < 0)
        {
            return ret;
        }
        int64_t tmp = (int64_t)obj;
        memcpy(handle, &tmp, sizeof(int64_t));
    }
    return ret;
}
FQT_API
int api_socket_stop(char *handle)
{
    int ret = 0;
    int64_t *testp = (int64_t *)handle;
    SocketObj *obj = (SocketObj *)testp[0];
    if(obj)
    {
        obj->status = 0;
        if(obj->pClientInfo)
        {
            if(!obj->type)
            {
                for(int i = 0; i < MAX_ONLINE_NUM; i++)
                {
                    ClientInfo *thisClientInfo = &obj->pClientInfo[i % MAX_ONLINE_NUM];
                    stun_free_node(thisClientInfo->head);
                }
            }
            else{
                for(int i = 0; i < 2; i++)
                {
                    ClientInfo *thisClientInfo = &obj->pClientInfo[i % MAX_ONLINE_NUM];
                    stun_free_node(thisClientInfo->head);
                }
            }

            free(obj->pClientInfo);
            obj->pClientInfo = NULL;
        }
        socket_close(obj);
    }
    return ret;
}


FQT_API
int api_socket_test2(char *handle, int cmdtype, int session_id)
{
    int ret = 0;
    int64_t *testp = (int64_t *)handle;
    SocketObj *obj = (SocketObj *)testp[0];
    if(obj)
    {
        char send_buf[1500] = "";
        int send_size = 300;
        int delay_time0 = 500 * 1000;
        int i = 0;
        {
            char *data = send_buf;
            StunInfo *p = (StunInfo *)data;
            obj->stunInfo.local_port = obj->local_port;
            strcpy(obj->stunInfo.local_ip, obj->local_ip);

            memcpy(p, &obj->stunInfo, sizeof(StunInfo));
            if(session_id > 0)
            {
                p->session_id = session_id;
                MYPRINT2("api_socket_test2: p->self_session_id=%d \n", p->self_session_id);
            }
            else{
                p->session_id = obj->stunInfo.self_session_id;
            }
            p->cmdtype = cmdtype;


            int64_t now_time = (int64_t)api_get_sys_time(0);
			p->time_stamp0 = now_time & 0xFFFFFFFF;
			p->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            ret = send_data(obj, (char *)p, sizeof(StunInfo), obj->addr_serv, now_time);
            if(false)
            {
                struct sockaddr_in localaddr = {};
                socklen_t slen;
                slen = sizeof(localaddr);
                getsockname(obj->sock_fd, (struct sockaddr*)&localaddr, &slen);
                int local_port = (int)ntohs(localaddr.sin_port);
                char *p_local_ip = inet_ntoa(localaddr.sin_addr);
                printf("api_socket_test2: addr0= %s:%d \n", p_local_ip, local_port);
                obj->local_port = (uint16_t)local_port;
                strcpy(obj->local_ip, p_local_ip);
                obj->stunInfo.local_port = obj->local_port;
                strcpy(obj->stunInfo.local_ip, obj->local_ip);
            }
        }
    }
    return ret;
}

FQT_API
int api_socket_test(char *handle, int session_id)
{
    int ret = 0;
    int64_t *testp = (int64_t *)handle;
    SocketObj *obj = (SocketObj *)testp[0];
    if(obj)
    {
        char send_buf[1500] = "";
        int send_size = 300;
        int delay_time0 = 500 * 1000;
        int i = 0;
        obj->stunInfo.session_id = session_id;
        do{
            char *data = send_buf;
            StunInfo *p = (StunInfo *)data;
            obj->stunInfo.local_port = obj->local_port;
            strcpy(obj->stunInfo.local_ip, obj->local_ip);
            obj->stunInfo.cmdtype = kPeers;
            obj->stunInfo.actor = kFather;

            if(!obj->stunInfo.session_id)
            {
                obj->stunInfo.cmdtype = kReg;
            }
            else{
                if(i == 9)
                {
                    obj->stunInfo.cmdtype = kExit;
                }
            }
            memcpy(p, &obj->stunInfo, sizeof(StunInfo));
            int64_t now_time = (int64_t)api_get_sys_time(0);
			p->time_stamp0 = now_time & 0xFFFFFFFF;
			p->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            ret = send_data(obj, data, sizeof(StunInfo), obj->addr_serv, now_time);
            if(true)
            {
                struct sockaddr_in localaddr = {};
                socklen_t slen;
                slen = sizeof(localaddr);
                getsockname(obj->sock_fd, (struct sockaddr*)&localaddr, &slen);
                int local_port = (int)ntohs(localaddr.sin_port);
                char *p_local_ip = inet_ntoa(localaddr.sin_addr);
                printf("api_socket_test: addr0= %s:%d \n", p_local_ip, local_port);
                obj->local_port = (uint16_t)local_port;
                strcpy(obj->local_ip, p_local_ip);
                obj->stunInfo.local_port = obj->local_port;
                strcpy(obj->stunInfo.local_ip, obj->local_ip);
            }
            //send_size += 10;
            i++;
            int delay_time = delay_time0 * i;
            usleep(delay_time);//
            printf("api_sock_test: i=%d \n", i);
        }while(i < 10);
        ret = 1;
    }
    return ret;
}
#if 0
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
    int send_size = 300;
    int delay_time0 = 500 * 1000;
    int i = 0;
    do{
        int64_t now_time = (int64_t)api_get_sys_time(0);
        ret = send_data(client, send_buf, send_size, client->addr_serv, now_time);
        send_size += 10;
        i++;
        int delay_time = delay_time0 * i;
        usleep(delay_time);//
        printf("api_sock_test: i=%d \n", i);
    }while(i < 10);

    //get_local_port(client->sock_fd);
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

    //get_local_mac(test_eth, mac);
    //printf("local %s mac: %s\n", test_eth, mac);

    //get_local_ip(test_eth, ip);
    //printf("local %s ip: %s\n", test_eth, ip);

    return 0;
}
#endif