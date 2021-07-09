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
#include <math.h>
#include <fcntl.h>

#define FILE_PLT 127
#define FIX_MTU_SIZE 1400
#define MTU_SIZE 1100
#define RAW_OFFSET 4//2
#define EXTEND_PROFILE_ID   0xA55A


#define MAX_PKT_BUF_SIZE (1 << 16) //utility_server.c
#define LEFT_SHIFT32 ((long long)1 << 32) //注意类型，防止当成负数
#define HALF_UINT ((long long)1 << 31)
#define QUART_UINT ((long long)1 << 30)
#define HALF_QUART_UINT (HALF_UINT + QUART_UINT) //注意类型，防止当成负数
#define MAX_UINT    (((long long)1 << 32) - 1)
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
    unsigned int frame_id : 16;
    unsigned int group_id : 16;
    unsigned int pic_id;                //only use for picture
    unsigned int pkt_idx;
    unsigned int rtp_xorcode;           //当前数据块（包括rtp头）异或码
    unsigned int time_stamp0;
    unsigned int time_stamp1;
} FILE_EXTEND_HEADER;
typedef struct{
    unsigned long long filesize;    //文件大小（maxsize = 4T?）
    unsigned int block_size : 16;   //mtu size, 默认1100bytes
    unsigned int frame_size : 16;    //default: 256
    unsigned int pic_size : 16;     //pic_size = n * frame_size
    unsigned int group_size : 16;    //default: 64MB
    unsigned int block_num;         //block 个数 = filesize / block_size
    unsigned int frame_num;
    unsigned int pic_num;
    unsigned int group_num;
    unsigned int file_xorcode;      //文件异或码（文件首个64MBytes）
    unsigned char filename[256];    //文件名
}FileInfo;

typedef struct{
    unsigned int rtp_pkt_size : 16;     //当前rtp包大小
    unsigned int data_type : 3;         //1:file start info;2:file end info; 0:raw data
    unsigned int enable_encrypt : 1;    //是否加密
    unsigned int data_size : 10;        //当前索引数据块大小
    unsigned int frame_id : 16;
    unsigned int group_id : 16;
    unsigned int pic_id;
    unsigned int pkt_idx;
}CacheHead;

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

typedef struct
{
    char *data;
    int size;
}PktItem;
typedef struct
{
    int max_num;
    PktItem *pktItem;
}FrameVector;
typedef struct
{
    int max_num;
    FrameVector *frameVector;
}PicVector;
struct GroupVector {
    //void *sock;//
    int num;
    int id;
    int max_num;
    PicVector *picVector;
    struct GroupVector *tail;
    struct GroupVector *next;
};
typedef struct GroupVector GroupVectorNode;

typedef struct{
    FileInfo info;
    char *data;
    int data_size;
    unsigned short seq_no;          //block id
    unsigned int frame_id;
    unsigned int group_id;          //
    unsigned int data_xorcode;      //每一块数据的异或值与上一次的异或值的异或码
    unsigned int enable_encrypt;    //是否加密
    unsigned int enable_fec;         //是否开启fec
    unsigned long long snd_size;
    unsigned int pic_id;
    unsigned int pkt_idx;//4*1024*1024*1024*1024;//4T
    long long now_time;
    long long start_time;
    int net_time;
    GroupNode *head;
    FrameNode *frameNode;
    PicNode *picNode;
    GroupVectorNode *vectorNode;
    FILE *index_fp;
    FILE *raw_fp;
}FileRtpObj;






#ifdef __cplusplus
}
#endif

#endif
