/*****************************************************************************
 * hcsvc.h: hcsvc public header
 *****************************************************************************
 * Copyright (C) 2020-2020 hcsc project
 *
 * Authors: Xiaohui Gao <waky_7635@126.com>
  *****************************************************************************/

#ifndef HCSVC_HCSVC_H
#define HCSVC_HCSVC_H


#ifdef __cplusplus
#define EXTERNC extern "C"
EXTERNC {
#else
#define EXTERNC
#endif
//FQT_API: file quick transmitting

#ifdef _WIN32
#define FQT_API EXTERNC __declspec(dllexport)
#else
#define FQT_API __attribute__ ((__visibility__("default")))
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <pthread.h>

#if defined (WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if defined (WIN32)
#define SOCKFD SOCKET
#else
#define SOCKFD int
#endif

#ifndef false
#define false   (0)
#endif

#ifndef true
#define true    (!false)
#endif

#define MAX_ONLINE_NUM (1 << 20)

#define FILE_PLT 127
#define FIX_MTU_SIZE 1400
#define MTU_SIZE 1100
#define RAW_OFFSET 4//2
#define EXTEND_PROFILE_ID   0xA55A


#define MAX_PKT_BUF_SIZE (1 << 16) //utility_server.c
#define LEFT_SHIFT32 ((int64_t)1 << 32) //注意类型，防止当成负数
#define HALF_UINT ((int64_t)1 << 31)
#define QUART_UINT ((int64_t)1 << 30)
#define HALF_QUART_UINT (HALF_UINT + QUART_UINT) //注意类型，防止当成负数
#define MAX_UINT    (((int64_t)1 << 32) - 1)
#define LEFT_SHIFT16 ((int)1 << 16)
#define MAX_USHORT (((int)1 << 16) - 1)
#define HALF_USHORT ((int)1 << 15)
#define QUART_USHORT ((int)1 << 14)
#define HALF_QUART_USHORT (HALF_USHORT + QUART_USHORT)
#define	VIDIORASHEADSIZE	4
#define RECV_BUF_NUM  1000
#define MAX_PACKET_SIZE  1500
#define MAX_PKT_NUM (1 << 10) //1024//2048 //1024
#define MAX_FEC_PKT_NUM (1 << 14) //丢包率到达90%,会导致包数增加10倍以上

//#define PRINT_LEVEL     1

#ifdef PRINT_LEVEL
#define MYPRINT printf
#else
#define MYPRINT
#define MYPRINT2 printf
#endif

/****************************************************************************
 * File RTP Header parameters
 ****************************************************************************/

typedef struct
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;
    /**//* bytes 4-7 */
    unsigned  int timestamp;
    /**//* bytes 8-11 */
    unsigned int ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER;


typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;
} NALU_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2;
	unsigned char F:1;
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;  //Reserved
	unsigned char E:1;  //End
	unsigned char S:1;  //Start
} FU_HEADER; /**//* 1 BYTES */


//以256个数据块为单位
typedef struct {
	unsigned int codec_id : 3;//4; //3	// identifies the code/codec being used. In practice, the "FEC encoding ID" that identifies the FEC Scheme should
					 // be used instead (see [RFC5052]). In our example, we are not compliant with the RFCs anyway, so keep it simple.
	unsigned int k : 11;//8; //9 : 512
	unsigned int n : 14;//12
	unsigned int rsv0 : 4;
	unsigned short symbol_size : 10;//symbol_size >> 2
	unsigned short rsv1 : 6;
	unsigned short fec_seq_no : 14;//12
	unsigned short rsv2 : 2;
	//unsigned short resv;
	//8b
}FEC_HEADER;

//
//结构体大小必须是所有成员(不包括结构体)大小的整数倍
//注意在 linux 下时，整个结构体的大小应该是：
//char 对齐模数是 1，short 是 2，int 是 4，float 是 4，double(linux 是 4，windows 是 8)
typedef struct {
	unsigned short rtp_extend_profile;       //profile used
	unsigned short rtp_extend_length;        //扩展字段的长度，为4的整数倍；1表示4个字节
    //
    unsigned int rtp_pkt_size : 16;         //当前rtp包大小
    unsigned int data_type : 3;         //1:file start info;2:file end info; 0:raw data
    unsigned int enable_encrypt : 1;    //是否加密
    unsigned int enable_fec : 1;         //是否开启fec
    unsigned int blk_id : 9;
    unsigned int frame_id : 9; //[0, pic_blks)
    unsigned int pic_id : 9;                //only use for picture //[0, group_blks)
    unsigned int group_id : 30; //[0, 2^22)
    unsigned int pkt_idx;   //[0, 2^32)
    unsigned int rtp_xorcode;           //当前数据块（包括rtp头）异或码
    unsigned int time_stamp0;
    unsigned int time_stamp1;
} FILE_EXTEND_HEADER;
typedef struct{
    uint64_t filesize;    //文件大小（maxsize = 4T?）
    unsigned int min_blk_size : 11; //不能被block_size整除的余数
    unsigned int block_size : 11;   //mtu size, 默认1100bytes
    unsigned int frame_size : 16;    //default: 256
    unsigned int pic_size : 16;     //pic_size = n * frame_size
    unsigned int group_size : 16;    //default: 64MB
    unsigned int block_num;         //block 个数 = filesize / block_size
    unsigned int frame_num;
    unsigned int pic_num;
    unsigned int group_num;
    unsigned int img_size;
    unsigned int file_xorcode;      //文件异或码（文件首个64MBytes）
    unsigned char filename[256];    //文件名
}FileInfo;

typedef struct{
    unsigned int rtp_pkt_size : 16;     //当前rtp包大小
    unsigned int data_type : 3;         //1:file start info;2:file end info; 0:raw data
    unsigned int enable_encrypt : 1;    //是否加密
    unsigned int data_size : 10;        //当前索引数据块大小
    unsigned int frame_id : 9; //[0, pic_blks)
    unsigned int pic_id : 9;                //only use for picture //[0, group_blks)
    unsigned int group_id : 30; //[0, 2^22)
    unsigned int pkt_idx;   //[0, 2^32)
}CacheHead;
typedef struct{
    char data[512];
    int size;
}FileHead;

#if 0
struct FileInfo {
    char *data;
    int size;
    int num;
    int id;
    int max_num;
    struct FileInfo *tail;
    struct FileInfo *next;
};
typedef struct FileInfo FileNode;
//FEC
struct FrameInfo {
    //void *sock;//
    int num;
    int id;
    int max_num;
    FileNode *head;
    struct FrameInfo *tail;
    struct FrameInfo *next;
};
typedef struct FrameInfo FrameNode;

struct PicInfo {
    //void *sock;//
    int num;
    int id;
    int max_num;
    FrameNode *head;
    struct PicInfo *tail;
    struct PicInfo *next;
};
typedef struct PicInfo PicNode;

struct GroupInfo {
    //void *sock;//
    int num;
    int id;
    int max_num;
    PicNode *head;
    struct GroupInfo *tail;
    struct GroupInfo *next;
};
typedef struct GroupInfo GroupNode;
#endif
typedef struct
{
    int64_t start_time;
    int sum_size;
    int bw;
}BWCount;//Band wide
typedef struct
{
    //经过网络对时校正后的时间(不需要计算rtt,缺点是只知道单向传输延迟)
    int sum_delay;
    int num;
    int net_dealy;
}NDCount;//Net Delay
typedef struct
{
    //分奇偶段，只统计偶段d
    int64_t start_time;
    int pkt_num;
    //
    short last_frame_id;
    short last_pic_id;
    int last_group_id;
    //
    int loss_rate;
}LRCount;//lossrate
typedef struct
{
    char *data;
    int size;
    unsigned group_id;
}PktItem;
typedef struct
{
    int max_num;
    int num : 16;
    int complete : 2;
    int complete_num;
    PktItem *pktItem;
}FrameVector;
typedef struct
{
    int max_num;
    int num;
    int complete_num;
    //int64_t frame_time_stamp;
    //int64_t now_time;
    char *img;
    int img_size;
    //short *p_blk_size;


    int64_t start_check_time;
    int64_t last_check_time;//防止频繁检测
    int64_t frame_time_stamp;
    int64_t now_time;

    FrameVector *frameVector;
}PicVector;
typedef struct {
    int max_num;
    int num;
    int complete_num;
    PicVector *picVector;
}GroupVector;

typedef struct{
    FileInfo info;
    pthread_mutex_t lock;
    char *data;
    int data_size;
    unsigned short seq_no;          //block id
    unsigned int frame_id : 9; //[0, pic_blks)
    unsigned int pic_id : 9;                //only use for picture //[0, group_blks)
    unsigned int group_id : 30; //[0, 2^22)
    unsigned int data_xorcode;      //每一块数据的异或值与上一次的异或值的异或码
    unsigned int enable_encrypt;    //是否加密
    unsigned int enable_fec;         //是否开启fec
    uint64_t snd_size;
    unsigned int pkt_idx;//4*1024*1024*1024*1024;//4T
    int last_group_id;//已经处理过的
    int last_pic_id;////已经处理过的
    int64_t now_time;
    int64_t start_time;
    int net_time;
    //以下三个参数是相互关联的
    int cache_size;
    int max_delay;
    int bit_rate;
    //
    //GroupNode *head;
    //FrameNode *frameNode;
    //PicNode *picNode;
    GroupVector groupVector;
    BWCount bwCount;
    LRCount lrCount;
    NDCount ndCount;
    FileHead fileHead;
    FileHead fileTail;
    FILE *index_fp;
    FILE *raw_fp;
}FileRtpObj;

#if 1
#define HEARTBEAT_TIME  20000 //20s
//#define HEARTBEAT_TIME  2000 //2s //test
#define MAX_MTU_SIZE 1400

typedef enum{
    kReg = 1,
    kPeers,
    kPing,
    kHeartBeat,
    kBye,
    kExit,
}CMDType;
typedef enum{
    kFather = 1,
    kSon,
}ACTORType;
typedef enum{
    kFile = 1,
    kText,
    kAudio,
    kAV,
}ACTOINType;

typedef struct{
    char local_ip[16];
    char remote_ip[16];//255.255.255.255//external
    unsigned int local_port : 16;
    unsigned int remote_port : 16;//external
    unsigned int session_id;
    unsigned int self_session_id;
    unsigned int passwd;
    unsigned int cmdtype : 8;
    unsigned int actor : 3;
    unsigned int client_id : 16;
    unsigned int action : 3;
    unsigned int ip_type : 1;//0:local; 1: external
    unsigned int ack : 1;
    unsigned int time_stamp0;
    unsigned int time_stamp1;
}StunInfo;

struct CStunInfo {
    int num;
    int id;
    StunInfo *data;
    int size;
    int cnn_status;//1:local; 2: exteranl
    int64_t last_send_time;
    int ping_times;
    unsigned int session_id;
    struct sockaddr_in addr_client;
    struct CStunInfo *tail;
    struct CStunInfo *next;
};
typedef struct CStunInfo CStunNode;

typedef struct{
    unsigned int session_id;
    CStunNode *head;
}ClientInfo;
typedef struct{
    SOCKFD sock_fd;
    struct sockaddr_in addr_serv;
    int port;
    char server_ip[64];
    pthread_mutex_t lock;
    pthread_t recv_pid;
    pthread_t hb_pid;
    pthread_t ping_pid0;
    pthread_t ping_pid1;
    int status;
    StunInfo stunInfo;
    char local_ip[16];
    unsigned short local_port;
    ClientInfo *pClientInfo;
    int type;
    int64_t last_send_time;//to server
}SocketObj;
#endif

#ifdef __cplusplus
}
#endif

#endif
