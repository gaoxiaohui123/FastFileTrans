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

extern int create_vector(FileRtpObj *obj);
extern int release_vector(FileRtpObj *obj);
extern int raw2pkt(FileRtpObj *obj, char *out_buf, int out_size, short *rtpSize);
extern int file_unpacket(FileRtpObj *obj, char *out_buf, int out_size, short *oSize);

#if 0
extern int group_create_node(GroupNode **head0);
extern void group_add_node(GroupNode *head0, GroupNode **pnew);
extern void *group_find_node_by_id(GroupNode *head, int id);
extern void group_delete_node_by_id(GroupNode *head, int id);
extern void group_free_node(GroupNode *head);
extern void *group_find_node(GroupNode *head);
extern void group_delete_node(GroupNode *head);

extern int pic_create_node(PicNode **head0);
extern void pic_add_node(PicNode *head0, PicNode **pnew);
extern void *pic_find_node_by_id(PicNode *head, int id);
extern void pic_delete_node_by_id(PicNode *head, int id);
extern void pic_free_node(PicNode *head);

extern int frame_create_node(FrameNode **head0);
extern void frame_add_node(FrameNode *head0, FrameNode **pnew);
extern void *frame_find_node_by_id(FrameNode *head, int id);
extern void frame_delete_node_by_id(FrameNode *head, int id);
extern void frame_free_node(FrameNode *head);

extern int file_create_node(FileNode **head0);
extern void file_add_node(FileNode *head0, FileNode **pnew);
extern void *file_find_node_by_id(FileNode *head, int id);
extern void file_delete_node_by_id(FileNode *head, int id);
extern void file_free_node(FileNode *head);
#endif


#if 1
int glob_time_offset = 0;
#ifdef _WIN32
#include <time.h>
#include <windows.h>
int64_t get_sys_time()
{
	struct timeval tBegin;
	//struct timeval tEnd;
	gettimeofday(&tBegin, NULL);
	//...process
	//gettimeofday(&tEnd, NULL);
	//long deltaTime = 1000000L * (tEnd.tv_sec - tBegin.tv_sec) + (tEnd.tv_usec - tBegin.tv_usec);
	int64_t time = (1000000L * (int64_t)tBegin.tv_sec + tBegin.tv_usec) / 1000;//us-->ms
	time -= glob_time_offset;
    return time;
}
int64_t get_sys_time2()
{
	struct timespec tBegin;
	//struct timeval tEnd;
	gettimeofday(&tBegin, NULL);
	//...process
	//gettimeofday(&tEnd, NULL);
	//long deltaTime = 1000000L * (tEnd.tv_sec - tBegin.tv_sec) + (tEnd.tv_usec - tBegin.tv_usec);
	//int64_t time = (1000000L * (int64_t)tBegin.tv_sec + tBegin.tv_usec);//us
	int64_t time = (1000000000L * (int64_t)tBegin.tv_sec + tBegin.tv_nsec);//ns
    return time;
}
#else
#include <sys/time.h>
int64_t get_sys_time()
{
	struct timeval tBegin;
	//struct timeval tEnd;
	gettimeofday(&tBegin, NULL);
	//...process
	//gettimeofday(&tEnd, NULL);
	//long deltaTime = 1000000L * (tEnd.tv_sec - tBegin.tv_sec) + (tEnd.tv_usec - tBegin.tv_usec);
	int64_t time = (1000000L * tBegin.tv_sec + tBegin.tv_usec) / 1000;//us-->ms
	time -= glob_time_offset;
    return time;
}
int64_t get_sys_time2()
{
	struct timespec tBegin;
	//struct timeval tEnd;
	gettimeofday(&tBegin, NULL);
	//...process
	//gettimeofday(&tEnd, NULL);
	//long deltaTime = 1000000L * (tEnd.tv_sec - tBegin.tv_sec) + (tEnd.tv_usec - tBegin.tv_usec);
	//int64_t time = (1000000L * (int64_t)tBegin.tv_sec + tBegin.tv_usec);//us
	int64_t time = (1000000000L * (int64_t)tBegin.tv_sec + tBegin.tv_nsec);//ns
    return time;
}
#endif


FQT_API
unsigned int api_create_id(unsigned int range)
{
    int ret = 0;
    unsigned int seed = (unsigned int)get_sys_time2();
    srand(seed);
    ret = rand() % range;
    return ret;
}
FQT_API
int64_t api_get_sys_time(int delay)
{
    int64_t ret = get_sys_time();
    if(delay)
    {
        ret += delay;
    }
    return ret;
}

#endif

void paced_send(int64_t bitrate, int sendDataLen, int difftime, int64_t sumbytes)
{
    int usedtime = difftime * 1000;//us
    //int64_t bitrate = 1 * 1024 * 1024 * 1024;//bps
    int64_t sendbits = sendDataLen << 3;
    int64_t pretime = (sendbits * 1000000) / bitrate; //us
    int64_t sumbits = sumbytes << 3;
    int64_t sumpretime = (sumbits * 1000000) / bitrate;
    //MYPRINT("PacedSend: sumpretime=%lld \n", sumpretime);
    int tailtime = (int)(sumpretime - usedtime); // us
    //MYPRINT("PacedSend: tailtime=%d \n", tailtime);
    int waittime = (int)(tailtime + pretime); //us
    //MYPRINT("PacedSend: waittime=%d \n", waittime);
    //waittime = pretime #test
    if(waittime > 0)
    {
        //MYPRINT("PacedSend: waittime=%d (us) \n", waittime);
        usleep(waittime);
    }
}

int pkt2file(FILE *idxfp, FILE * fp, char *pkt_buf, int size, short *pkt_size, unsigned int *p_pkt_idx)
{
    MYPRINT("pkt2file: start: \n");
    int ret = 0;
    int offset = 0;
    int offset2 = 0;
    int i = 0;
    unsigned int last_pkt_idx = p_pkt_idx[0];
    char pnull[2048] = {0};
    int sum = 0;
    //printf("pkt2file: last_pkt_idx=%u \n", last_pkt_idx);
    while(offset < size)
    {
        char *src_ptr = &pkt_buf[offset];
        CacheHead *src_head = (CacheHead *)src_ptr;
        unsigned int pkt_idx = src_head->pkt_idx;
        int data_type = src_head->data_type;
        int rtp_pkt_size = src_head->rtp_pkt_size;
        int rsize = pkt_size[i];
        int head_size = sizeof(CacheHead);
        int wsize = rsize - head_size;

        //printf("pkt2file: rsize=%d, pkt_idx=%u \n", rsize, pkt_idx);
        if(wsize > 0 && wsize == rtp_pkt_size)
        {
            int data_size = head_size;
            //????????????0
            int64_t loss_num = ((int64_t)pkt_idx - (int64_t)last_pkt_idx) - 1;
            if(loss_num > 0)
            {
                printf("pkt2file: loss_num=%d, i=%d vvvvvvvvvvvvvvv \n", loss_num, i);
#if 0
                for(int j = 0; j < loss_num; j++)
                {
                    ret = fwrite(pnull, 1, wsize, fp);
                }
#endif
            }
            //
            if(data_type)
            {
                data_size += rtp_pkt_size;
            }
            src_head->data_size = data_size;
            ret = fwrite(&src_ptr[0], 1, data_size, idxfp);
            if(ret != data_size)
            {
                printf("pkt2file: fail: ret=%d, head_size=%d \n", ret, head_size);
                ret = -1;
                break;
            }
            //fflush(idxfp);
#if 1
            if(!data_type)
            {
                ret = fwrite(&src_ptr[head_size], 1, wsize, fp);
                if(ret != wsize)
                {
                    printf("pkt2file: fail: ret=%d, wsize=%d \n", ret, wsize);
                    ret = -2;
                    break;
                }
                //fflush(fp);
                sum += ret;
            }

#endif
            last_pkt_idx = pkt_idx;
        }
        else{
            printf("pkt2file: fail: rtp_pkt_size=%d, wsize=%d \n", rtp_pkt_size, wsize);
        }
        offset += rsize;
        i++;
        //printf("pkt2file: offset=%d, i=%d \n", offset, i);
    }
    p_pkt_idx[0] = last_pkt_idx;
    ret = sum;
    //fflush(idxfp);
    //fflush(fp);
    MYPRINT("pkt2file: ret=%d \n", ret);
    return ret;
}
//./ffplay -f rawvideo -video_size $WXH2 $OUT_FILE
FQT_API
int call_test(char *ifilename, char *ofilename, char *idxfilename, int img_size)
{
    int ret = 0;
    char ofilename2[256] = "";
    strcpy(ofilename2, ofilename);
    strcat(ofilename2, ".tmp");

    //??????????????????
    FILE *rfp = fopen(ifilename, "rb");
    //
    FILE *wfp = fopen(ofilename2, "wb+");
    if(wfp)
    {
        fclose(wfp);
    }
    wfp = fopen(ofilename2, "r+b+");
    //
    FILE *idxfp = fopen(idxfilename, "wb+");
    printf("call_test: ifilename=%s \n", ifilename);
    printf("call_test: ofilename2=%s \n", ofilename2);
    printf("call_test: idxfilename=%s \n", idxfilename);
    printf("call_test: rfp=%x \n", rfp);
    printf("call_test: wfp=%x \n", wfp);
    printf("call_test: idxfp=%x \n", idxfp);
    if(rfp && wfp && idxfp)
    {
        printf("call_test: start 2 \n");
#if 1
        if(wfp)
        {
            fclose(wfp);
            wfp = NULL;
        }
        if(idxfp)
        {
            fclose(idxfp);
            idxfp = NULL;
        }
#endif
        //return;
        FileRtpObj rObj = {};
        FileRtpObj wObj = {};
        rObj.cache_size = 100 * 1024 * 1024;//100MB
        pthread_mutex_init(&rObj.lock,NULL);
        pthread_mutex_init(&wObj.lock,NULL);
        wObj.last_group_id = -1;//??????????????????
        wObj.last_pic_id = -1;////??????????????????
        memset(&wObj.bwCount, 0, sizeof(BWCount));
        memset(&wObj.lrCount, 0, sizeof(LRCount));
        wObj.lrCount.loss_rate = -1;
        //wObj.lrCount.last_check_time = 0;
        wObj.max_delay = 2000;
        wObj.index_fp = idxfp;
        wObj.raw_fp = wfp;
        //
        //group_create_node(&rObj.head);
        //group_create_node(&wObj.head);

        //
        fseek(rfp, 0, SEEK_END);
        int64_t total_size = ftell(rfp);
        printf("call_test: total_size=%d \n", total_size);
        rewind(rfp);
        //
        int mtu_size = 1100;
        int min_blk_size = mtu_size;
        rObj.info.filesize = total_size;    //???????????????maxsize = 4T????
        rObj.info.block_size = mtu_size;   //mtu size, ??????1100bytes
        rObj.info.block_num = (total_size / mtu_size) + ((total_size % mtu_size) != 0);         //block ?????? = filesize / block_size
        if(!img_size)
        {
            img_size = 256 * 12 * mtu_size;//test
        }
        int frame_blks = 256;
        rObj.info.frame_size = frame_blks;
        int frame_size = frame_blks * mtu_size;
        rObj.info.min_blk_size = mtu_size;
        //
        if(img_size > 0)
        {
            //img_size;//(1920 * 1080 * 3) / 2;
            frame_blks = img_size / mtu_size + ((img_size % mtu_size) != 0);
            frame_blks = frame_blks > 256 ? 256 : frame_blks;
            frame_size = frame_blks * mtu_size;
            rObj.info.frame_size = frame_blks;
            min_blk_size = img_size % mtu_size;
            rObj.info.min_blk_size = img_size % mtu_size;
            rObj.info.img_size = img_size;
            MYPRINT2("call_test: frame_blks=%d \n", frame_blks);
        }
        //
        rObj.info.frame_num = (total_size / frame_size) + ((total_size % frame_size) != 0);

        int pic_blks = 12;//256;//12;
        rObj.info.pic_size = pic_blks;
        int pic_size = pic_blks * frame_size;
        //
        if(img_size > 0)
        {
            //pic_size = img_size;//(1920 * 1080 * 3) / 2;
            pic_blks = img_size / frame_size + ((img_size % frame_size) != 0);
            rObj.info.pic_size = pic_blks;
            pic_size = pic_blks * frame_size;
            rObj.info.img_size = img_size;
            MYPRINT2("call_test: pic_blks=%d \n", pic_blks);
        }
        //
        rObj.info.pic_num = (total_size / pic_size) + ((total_size % pic_size) != 0);

        int group_blks = 256;//??????:256*12*256*1100 = 825MB;????????????????????????
        rObj.info.group_size = group_blks;
        int group_size = group_blks * pic_size;

        group_size = rObj.cache_size;//(1920 * 1080 * 3) / 2;
        group_blks = group_size / pic_size + ((group_size % pic_size) != 0);
        rObj.info.group_size = group_blks;
        MYPRINT2("call_test: group_blks=%d \n", group_blks);

        rObj.info.group_num = (total_size / pic_size) + ((total_size % pic_size) != 0);
        MYPRINT2("call_test: rObj.info.group_num=%d \n", rObj.info.group_num);

        char *read_buf = calloc(1, rObj.cache_size * sizeof(char));
        rObj.data = read_buf;
        int64_t unit_size = sizeof(RTP_FIXED_HEADER) + sizeof(FILE_EXTEND_HEADER) + mtu_size;
        int max_buf_size = (int)((unit_size * rObj.cache_size) / mtu_size) + unit_size;//(frame_blks + 1) * (mtu_size + 100);
        MYPRINT2("call_test: max_buf_size=%d \n", max_buf_size);
        int out_size = max_buf_size;
        char *out_buf = calloc(1, out_size);//for repeat trans
        short *rtpSize = calloc(1, frame_blks * sizeof(short));

        //int out_size2 = max_buf_size;
        //char *out_buf2 = calloc(1, out_size);
        //short *oSize = rtpSize;// calloc(1, (frame_blks + 1) * sizeof(short));

        rObj.info.file_xorcode = 0;      //??????????????????????????????64MBytes???
        strcpy(rObj.info.filename, ifilename);    //????????????????????????
        //
        int64_t sumsize = 0;
        int64_t sumsize2 = 0;
        int status = 0;
        int k = 0;
        unsigned int pkt_idx = 0;
        //
        rObj.snd_size = 0;
        rObj.seq_no = 0;
        rObj.group_id = 0;
        rObj.pic_id = 0;
        rObj.pkt_idx = 0;
        rObj.enable_encrypt = 0;
        rObj.enable_fec = 0;
        //rObj.picNode = NULL;
        //pic_create_node(&rObj.picNode);
        create_vector(&rObj);
        //
        printf("call_test: total_size=%d \n", total_size);
        //
        int offset = 0;
        int offset3 = 0;
        while((sumsize < total_size) && !status)
        {
            //group: 256 * 256 * 1100 > 64MB
            //rObj.group_id = 0;
            //GroupNode *groupNode = (GroupNode *)calloc(1, sizeof(GroupNode));
            //pic_create_node(&groupNode->head);
            //groupNode->head = rObj.picNode;
            //rObj.pic_id = 0;
            for(int i = 0; i < rObj.info.group_size; i++)
            {
                MYPRINT("call_test: rObj.info.group_size=%d, i=%d \n", rObj.info.group_size, i);
                if(status)
                {
                    break;
                }
                rObj.frame_id = 0;
                //PicNode *picNode = (PicNode *)calloc(1, sizeof(PicNode));
                //frame_create_node(&picNode->head);
                int offset1 = 0;
                int64_t time0 = api_get_sys_time(0);
                if((offset + img_size) > rObj.cache_size)
                {
                    MYPRINT2("call_test: offset=%d, k=%d, i=%d \n", offset, k, i);
                    offset = 0;
                    offset3 = 0;
                    //status = 1;//test
                }
                int rsize = fread(&read_buf[offset], 1, img_size, rfp);
                if(rsize != img_size)
                {
                    printf("call_test: rsize=%d, ######## \n", rsize);
                    status = 1;
                }
                sumsize += rsize;

                int64_t time1 = api_get_sys_time(0);
                int difftime = (int)(time1 - time0);
                if(difftime > 10)
                    MYPRINT("call_test: read: difftime=%d (ms) \n", difftime);

                for(int j = 0; j < rObj.info.pic_size; j++)
                {
                    if(status)
                    {
                        break;
                    }
                    //FrameNode *frameNode = (FrameNode *)calloc(1, sizeof(FrameNode));
                    //file_create_node(&frameNode->head);
                    //rObj.frameNode = frameNode;
                    rObj.data = &read_buf[offset + offset1];
                    rObj.data_size = frame_size;
                    if(j == (rObj.info.pic_size - 1))
                    {
                        //min_blk_size
                        rObj.data_size = img_size - (rObj.info.pic_size - 1) * frame_size;
                        MYPRINT2("call_test: rObj.data_size=%d \n", rObj.data_size);
                    }
                    offset1 += rObj.data_size;
                    rObj.data_xorcode = 0;
                    //
                    //int ret = file_packet(&rObj, out_buf, out_size, rtpSize);
                    int ret = raw2pkt(&rObj, &out_buf[offset3], out_size, rtpSize);
                    MYPRINT("call_test: raw2pkt: ret=%d \n", ret);
                    wObj.data = &out_buf[offset3];
                    wObj.data_size = ret;
                    int frame_pkt_size = ret;

                    //ret = file_unpacket(&wObj, out_buf2, out_size2, oSize);
                    //printf("call_test: file_unpacket: ret=%d \n", ret);
                    //
                    time0 = api_get_sys_time(0);
                    //ret = pkt2file(idxfp, wfp, out_buf2, ret, oSize, &pkt_idx);
                    ret = 0;
                    if(!i && !j && !k)
                    {
                        MYPRINT2("call_test: raw2pkt: rObj.pkt_idx=%d \n", rObj.pkt_idx);
                        ret += pkt2cache(&wObj, rObj.fileHead.data, rObj.fileHead.size);
                    }
                    int offset2 = 0;
                    int l = 0;

                    while(offset2 < frame_pkt_size)
                    {
                        if(rtpSize[l] > 0)
                        {
                            ret += pkt2cache(&wObj, &out_buf[offset3 + offset2], rtpSize[l]);
                            offset2 += rtpSize[l];
                            MYPRINT("call_test: rtpSize[l]=%d, l=%d \n", rtpSize[l], l);

                        }
                        l++;
                        if(l >= frame_blks)
                        {
                            MYPRINT("call_test: offset2=%d, frame_pkt_size=%d \n", offset2, frame_pkt_size);
                            break;
                        }
                    }

                    time1 = api_get_sys_time(0);
                    difftime = (int)(time1 - time0);
                    if(difftime > 10)
                        MYPRINT("call_test: write: difftime=%d (ms) \n", difftime);
                    sumsize2 += ret;
                    //printf("call_test: pkt2file: ret=%d \n", ret);
                    offset3 += frame_pkt_size;
                    //frame_add_node(picNode->head, &frameNode);
                    rObj.frame_id++;
                    if(sumsize >= total_size)
                    {
                        printf("call_test: sumsize=%lld, total_size=%lld ######## \n", sumsize, total_size);

                        ret = pkt2cache(&wObj, rObj.fileTail.data, rObj.fileTail.size);

                        status = 1;
                        break;
                    }
                    //return 1;//test
                }//j

                //pic_add_node(groupNode->head, &picNode);
                rObj.pic_id++;
                offset += img_size;
                MYPRINT2("call_test: offset1=%d, img_size=%d, i=%d \n", offset1, img_size, i);
                MYPRINT2("call_test: (offset >> 20)=%d, i=%d \n", (offset >> 20), i);
                //status = 1;//test
                //return 1;//test

            }//i
            if(sumsize >= sumsize2)
            {
                MYPRINT2("call_test: sumsize=%lld, sumsize2=%lld \n", sumsize, sumsize2);
            }
            MYPRINT2("call_test: wObj.info.group_size=%d \n", wObj.info.group_size);
            //group_add_node(rObj.head, &groupNode);
            rObj.group_id++;
            k++;
            printf("call_test: sumsize=%lld, sumsize2=%lld, total_size=%lld \n", sumsize, sumsize2, total_size);
            printf("call_test: sumsize=%d (MB), status=%d, k=%d \n", (sumsize >> 20), status, k);
            ret = (sumsize2 >> 20);
            //return 1;//test
        }
#if 0
        //??????????????????
        if(idxfp)
        {
            fclose(idxfp);
            idxfp = NULL;
        }
        FILE *idxrfp = fopen(idxfilename, "rb");
        if(idxrfp)
        {

            fseek(idxrfp, 0, SEEK_END);
            int64_t total_size2 = ftell(idxrfp);
            printf("call_test: total_size2=%d \n", total_size2);
            rewind(idxrfp);
            sumsize = 0;
            sumsize2 = 0;
            int head_size = sizeof(CacheHead);
            printf("call_test: head_size=%d \n", head_size);
            int i = 0;
            while((sumsize < total_size2))
            {
                int rsize = fread(read_buf, 1, head_size, idxrfp);
                if(rsize)
                {
                    CacheHead *src_head = (CacheHead *)read_buf;
                    int data_type = src_head->data_type;
                    int data_size = src_head->data_size;
                    int rtp_pkt_size = src_head->rtp_pkt_size;
                    if(data_type)
                    {
                        int rsize2 = fread(read_buf, 1, rtp_pkt_size, idxrfp);
                        if(rsize2 != rtp_pkt_size)
                        {
                            printf("error: call_test: rsize2=%d, rtp_pkt_size=%d \n", rsize2, rtp_pkt_size);
                        }
                    }
                    else{
                        sumsize2 += rtp_pkt_size;
                    }
                }
                if(rsize != head_size)
                {
                    break;
                }
                i++;
            }
            printf("call_test: sumsize2=%lld, total_size=%lld, i=%d \n", sumsize2, total_size, i);
            //
            fclose(idxrfp);
        }
#endif

#if 0
        //??????????????????
        if(wfp)
        {
            fclose(wfp);
            wfp = NULL;
        }
        FILE *wfp2 = fopen(ofilename2, "r+b+");
        if(wfp2)
        {
            fseek(wfp2, 0, SEEK_END);//SEEK_SET//SEEK_CUR
            int64_t total_size3 = ftell(wfp2);
            printf("call_test: total_size3=%lld \n", total_size3);
            rewind(wfp2);
            //
            char *file_data = malloc(total_size3);
            int rsize = fread(file_data, 1, total_size3, wfp2);
            if(rsize != total_size3)
            {
                printf("fail: call_test: rsize=%d \n", rsize);
            }
            rewind(wfp2);
            //
            sumsize2 = 0;
            int i = 0;
            int data_size = 1100;
            do{
                int rsize = fread(ptr, 1, data_size, wfp2);
                if(rsize)
                {
                    //printf("call_test: rsize=%d \n", rsize);
                    fseek(wfp2, -rsize, SEEK_CUR);
                    int wsize = fwrite(ptr, 1, rsize, wfp2);
                    if(wsize != rsize)
                    {
                        printf("call_test: wsize=%d \n", wsize);
                        break;
                    }
                    sumsize2 += wsize;
                    //printf("call_test: wsize=%d \n", wsize);
                }
                if(rsize != data_size)
                {
                    printf("call_test: rsize=%d \n", rsize);
                    //
                    for(int j = 0; j < 100; j++)
                    {
                        int wsize = fwrite(file_data, 1, total_size3, wfp2);
                        if(wsize != total_size3)
                        {
                            printf("call_test: wsize=%d \n", wsize);
                            break;
                        }
                        printf("call_test: j=%d \n", j);
                    }

                    //
                    break;
                }
                //break;
                i++;
            }while(1);
            printf("call_test: sumsize2=%lld, total_size=%lld, i=%d \n", sumsize2, total_size, i);
            //
            fclose(wfp2);
        }
        else{
            printf("call_test: wfp2=%x \n", wfp2);
        }

#endif
        //
        printf("call_test: start free \n");
        //
        if(out_buf)
        {
            free(out_buf);
        }
        if(rtpSize)
        {
            free(rtpSize);
        }
        //if(out_buf2)
        //{
        //    free(out_buf2);
        //}
        if(read_buf)
        {
            free(read_buf);
        }
        if(wfp)
        {
            int ret2 = rename(ofilename2, ofilename);
            printf("call_test: rename: ret2=%d \n", ret2);
        }
        printf("call_test: start obj free \n");
        release_vector(&wObj);
        release_vector(&rObj);

        pthread_mutex_destroy(&rObj.lock);
        pthread_mutex_destroy(&wObj.lock);
        printf("call_test: end free \n");
    }
    printf("call_test: close rfp \n");
    if(rfp)
    {
        fclose(rfp);
    }
    printf("call_test: close wfp \n");
    if(wfp)
    {
        fclose(wfp);
    }
    printf("call_test: close idxfp \n");
    if(idxfp)
    {
        fclose(idxfp);
    }
    printf("call_test: ok \n");
    return ret;
}
