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

#if 1
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
	long long time = (1000000L * (long long)tBegin.tv_sec + tBegin.tv_usec) / 1000;//us-->ms
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
	long long time = (1000000L * tBegin.tv_sec + tBegin.tv_usec) / 1000;//us-->ms
    return time;
}
#endif
FQT_API
long long api_get_sys_time(int delay)
{
    long long ret = get_sys_time();
    if(delay)
    {
        ret += delay;
    }
    return ret;
}

#endif

int file_packet(FileRtpObj *obj, char *out_buf, int out_size, short *rtpSize)
{
    int ret = 0;
    printf("file_packet: start \n");
    FileInfo *info = &obj->info;
    char *data = obj->data;
    int data_size = obj->data_size;
    unsigned short seq_no = obj->seq_no;
    unsigned int frame_id = obj->frame_id;
    unsigned int group_id = obj->group_id;
    unsigned int data_xorcode = obj->data_xorcode;
    
    unsigned long long filesize = info->filesize;    //文件大小（maxsize = 4T?）
    unsigned int block_size = info->block_size;   //mtu size, 默认1100bytes
    unsigned int block_num = info->block_num;         //block 个数 = filesize / block_size
    unsigned int file_xorcode = info->file_xorcode;      //文件异或码（文件首个64MBytes）
    unsigned char filename = info->filename;    //文件名
    
    int rtp_header_size = sizeof(RTP_FIXED_HEADER);
    int ext_size = sizeof(FILE_EXTEND_HEADER);
    int rtp_extend_length = (sizeof(FILE_EXTEND_HEADER) >> 2) - 1;
    int offset  = 0;
    int offset2  = 0;
    int i = 0;
    //printf("file_packet: data_size=%d \n", data_size);
    long long now_time = api_get_sys_time(0);
    while(offset < data_size)
    {
        int flag = !seq_no && !frame_id && !group_id;
        RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&out_buf[offset2];
        FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&out_buf[offset2 + rtp_header_size];
        char *payload_ptr = (char *)&out_buf[offset2 + rtp_header_size + ext_size];
        rtp_hdr->payload     = FILE_PLT;  //负载类型号，									PT
        rtp_hdr->version     = 2;  //版本号，此版本固定为2								V
        rtp_hdr->padding	 = 0;//														P
        rtp_hdr->csrc_len	 = 0;//														CC
        rtp_hdr->marker		 = 0;//(size >= len);   //标志位，由具体协议规定其值。		M
        rtp_hdr->ssrc        = 0;//ssrc;//(unsigned int)svc_nalu;;//htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一	SSRC
        rtp_hdr->extension	 = 1;//														X
        rtp_hdr->timestamp   = 0;//?
        rtp_hdr->seq_no = obj->seq_no;//(*seq_num);///htons(seq_num ++); //序列号，每发送一个RTP包增1
        if (seq_no >= MAX_USHORT)
        {
            seq_no = 0;
        }
        else{
            seq_no++;
        }
        if(flag)
        {
            printf("file_packet: 000000000000000000000 uuuuuuuuuuuuuuu \n");
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 1;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = obj->frame_id;
            rtp_ext->group_id = obj->group_id;
            rtp_ext->pkt_idx = obj->pkt_idx;
			rtp_ext->time_stamp0 = now_time & 0xFFFFFFFF;
			rtp_ext->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            rtp_ext->rtp_xorcode = 0;
            //obj->pkt_idx++;
            //
            FileInfo *info_data = (FileInfo *)payload_ptr;//&out_buf[offset2 + rtp_header_size + ext_size];
            memcpy((void *)info_data, (void *)info, payload_size);
            //
            offset2 += (int)(rtp_header_size + ext_size + payload_size);
            rtpSize[i] = rtp_header_size + ext_size + payload_size;
        }
        else{
            int tail = data_size - offset;
            int payload_size = tail >= block_size ? block_size : tail;
            
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = payload_size;         //当前rtp包大小
            rtp_ext->data_type = 0;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = obj->frame_id;
            rtp_ext->group_id = obj->group_id;
            rtp_ext->pkt_idx = obj->pkt_idx;
			rtp_ext->time_stamp0 = now_time & 0xFFFFFFFF;
			rtp_ext->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            rtp_ext->rtp_xorcode = 0;
            //
            char *src_ptr = (char *)&data[offset];
            memcpy((void *)payload_ptr, (void *)src_ptr, payload_size);
            //
            offset += (int)payload_size;
            offset2 += (int)(rtp_header_size + ext_size + payload_size);
            obj->snd_size += payload_size;
            rtpSize[i] = rtp_header_size + ext_size + payload_size;
            obj->pkt_idx++;
        }
        i++;
        if(obj->snd_size == filesize)
        {
            RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&out_buf[offset2];
            FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&out_buf[offset2 + rtp_header_size];
            char *payload_ptr = (char *)&out_buf[offset2 + rtp_header_size + ext_size];
            rtp_hdr->payload     = FILE_PLT;  //负载类型号，									PT
            rtp_hdr->version     = 2;  //版本号，此版本固定为2								V
            rtp_hdr->padding	 = 0;//														P
            rtp_hdr->csrc_len	 = 0;//														CC
            rtp_hdr->marker		 = 0;//(size >= len);   //标志位，由具体协议规定其值。		M
            rtp_hdr->ssrc        = 0;//ssrc;//(unsigned int)svc_nalu;;//htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一	SSRC
            rtp_hdr->extension	 = 1;//														X
            rtp_hdr->timestamp   = 0;//?
            rtp_hdr->seq_no = obj->seq_no;//(*seq_num);///htons(seq_num ++); //序列号，每发送一个RTP包增1
            if (seq_no >= MAX_USHORT)
            {
                seq_no = 0;
            }
            else{
                seq_no++;
            }
            printf("file_packet: obj->snd_size=%d, filesize=%d uuuuuuuuuuuuuuu \n", obj->snd_size, filesize);
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 2;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = obj->frame_id;
            rtp_ext->group_id = obj->group_id;
            rtp_ext->pkt_idx = obj->pkt_idx - 1;
            rtp_ext->pkt_idx = obj->pkt_idx;//test
			rtp_ext->time_stamp0 = now_time & 0xFFFFFFFF;
			rtp_ext->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            rtp_ext->rtp_xorcode = 0;
            //
            FileInfo *info_data = (FileInfo *)payload_ptr;//&out_buf[offset2 + rtp_header_size + ext_size];
            memcpy((void *)info_data, (void *)info, payload_size);
            //
            offset2 += (int)(rtp_header_size + ext_size + payload_size);
            rtpSize[i] = rtp_header_size + ext_size + payload_size;
            //obj->pkt_idx++;
            i++;
        }
        //printf("file_packet: offset=%d, i=%d \n", offset, i);
    }
    obj->frame_id++;
    obj->seq_no = seq_no;
    ret = offset2;
    printf("file_packet: ret=%d \n", ret);
    return ret;
}
int file_unpacket(FileRtpObj *obj, char *out_buf, int out_size, short *oSize)
{
    int ret = 0;
    printf("file_unpacket: start: \n");
    FileInfo *info = &obj->info;
    char *data = obj->data;
    int data_size = obj->data_size;
    unsigned short seq_no = obj->seq_no;
    unsigned int frame_id = obj->frame_id;
    unsigned int group_id = obj->group_id;
    unsigned int data_xorcode = obj->data_xorcode;
    
    unsigned long long filesize = info->filesize;    //文件大小（maxsize = 4T?）
    unsigned int block_size = info->block_size;   //mtu size, 默认1100bytes
    unsigned int block_num = info->block_num;         //block 个数 = filesize / block_size
    unsigned int file_xorcode = info->file_xorcode;      //文件异或码（文件首个64MBytes）
    unsigned char filename = info->filename;    //文件名
    
    int rtp_header_size = sizeof(RTP_FIXED_HEADER);
    int ext_size = sizeof(FILE_EXTEND_HEADER);
    int rtp_extend_length = (sizeof(FILE_EXTEND_HEADER) >> 2) - 1;
    int offset  = 0;
    int offset2  = 0;
    int i = 0;
    long long now_time = api_get_sys_time(0);
    //printf("file_unpacket: data_size=%d \n", data_size);
    while(offset < data_size)
    {
        int pkt_size = oSize[i];
        oSize[i] = 0;
        char *src_ptr = (char *)&data[offset];
        RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&data[offset];
        FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&data[offset + rtp_header_size];
        char *payload_ptr = (char *)&data[offset + rtp_header_size + ext_size];
        //
        int flag = 1;
        flag &= rtp_hdr->payload     == FILE_PLT;  //负载类型号，									PT
        flag &= rtp_hdr->version     == 2;  //版本号，此版本固定为2								V
        flag &= rtp_hdr->padding	 == 0;//														P
        flag &= rtp_hdr->csrc_len	 == 0;//														CC
        flag &= rtp_hdr->marker		 == 0;//(size >= len);   //标志位，由具体协议规定其值。		M
        //rtp_hdr->ssrc        = ssrc;//(unsigned int)svc_nalu;;//htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一	SSRC
        flag &= rtp_hdr->extension	 == 1;//														X
        //rtp_hdr->timestamp   = 0;//?
        //printf("file_unpacket: rtp_hdr->payload=%d \n", rtp_hdr->payload);
        //printf("file_unpacket: flag=%d \n", flag);
        if(flag)
        {
            unsigned short seq_no = rtp_hdr->seq_no;
            //
            unsigned short extprofile = rtp_ext->rtp_extend_profile;// & 7;
            unsigned short rtp_extend_length = rtp_ext->rtp_extend_length;
            rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            unsigned short extlen = (rtp_extend_length + 1) << 2;
            unsigned short rtp_pkt_size = rtp_ext->rtp_pkt_size;
            unsigned short this_pkt_size = rtp_pkt_size + extlen + rtp_header_size;
            //printf("file_unpacket: this_pkt_size=%d, pkt_size=%d, extlen=%d, ext_size=%d \n", this_pkt_size, pkt_size, extlen, ext_size);
            if(this_pkt_size == pkt_size && extlen == ext_size)
            {
                long long time_stamp0 = rtp_ext->time_stamp0;
	            long long time_stamp1 = rtp_ext->time_stamp1;
	            long long packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	            if(!obj->net_time)
	            {
	                //与对端进行时间对准
	                obj->net_time = now_time - packet_time_stamp;
	            }

                char *dst_ptr = (char *)&out_buf[offset2];
                CacheHead *dst_head = (CacheHead *)dst_ptr;
                char *dst_payload = (char *)&dst_ptr[sizeof(CacheHead)];
                unsigned short dst_pkt_size = rtp_pkt_size + sizeof(CacheHead);
                memcpy((void *)dst_payload, payload_ptr, rtp_pkt_size);
                dst_head->rtp_pkt_size = rtp_ext->rtp_pkt_size;// + sizeof(CacheHead);
                dst_head->data_type = rtp_ext->data_type;
                dst_head->enable_encrypt = rtp_ext->enable_encrypt;
                dst_head->frame_id = rtp_ext->frame_id;
                dst_head->group_id = rtp_ext->group_id;
                dst_head->pkt_idx = rtp_ext->pkt_idx;
                //
                offset2 += (int)dst_pkt_size;
                oSize[i] = rtp_ext->rtp_pkt_size + sizeof(CacheHead);
                //printf("file_unpacket: offset2=%d, i=%d \n", offset2, i);
            }
        }
        offset += pkt_size;
        i++;
        //printf("file_unpacket: offset=%d, i=%d \n", offset, i);
    }
    ret = offset2;
    printf("file_unpacket: ret=%d \n", ret);
    return ret;
}
int pkt2file(FILE *idxfp, FILE * fp, char *pkt_buf, int size, short *pkt_size, unsigned int *p_pkt_idx)
{
    printf("pkt2file: start: \n");
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
            //空缺块填0
            long long loss_num = ((long long)pkt_idx - (long long)last_pkt_idx) - 1;
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
    printf("pkt2file: ret=%d \n", ret);
    return ret;
}
//./ffplay -f rawvideo -video_size $WXH2 $OUT_FILE
FQT_API
int call_test(char *ifilename, char *ofilename, char *idxfilename)
{
    int ret = 0;
    //文件或文件夹
    FILE *rfp = fopen(ifilename, "rb");
    //
    FILE *wfp = fopen(ofilename, "wb+");
    if(wfp)
    {
        fclose(wfp);
    }
    wfp = fopen(ofilename, "r+b+");
    //
    FILE *idxfp = fopen(idxfilename, "wb+");
    printf("call_test: ifilename=%s \n", ifilename);
    printf("call_test: ofilename=%s \n", ofilename);
    printf("call_test: idxfilename=%s \n", idxfilename);
    printf("call_test: rfp=%x \n", rfp);
    printf("call_test: wfp=%x \n", wfp);
    printf("call_test: idxfp=%x \n", idxfp);
    if(rfp && wfp && idxfp)
    {
        printf("call_test: start 2 \n");
        //return;
        FileRtpObj rObj = {};
        FileRtpObj wObj = {};
        //
        fseek(rfp, 0, SEEK_END);
        long long total_size = ftell(rfp);
        printf("call_test: total_size=%d \n", total_size);
        rewind(rfp);
        //
        int mtu_size = 1100;
        rObj.info.filesize = total_size;    //文件大小（maxsize = 4T?）
        rObj.info.block_size = mtu_size;   //mtu size, 默认1100bytes
        rObj.info.block_num = (total_size / mtu_size) + ((total_size % mtu_size) != 0);         //block 个数 = filesize / block_size

        int frame_blks = 256;
        rObj.info.frame_size = frame_blks;
        int frame_size = frame_blks * mtu_size;
        rObj.info.frame_num = (total_size / frame_size) + ((total_size % frame_size) != 0);

        int group_blks = 256;
        rObj.info.group_size = group_blks;
        int group_size = group_blks * frame_size;
        rObj.info.group_num = (total_size / group_size) + ((total_size % group_size) != 0);

        char *ptr = calloc(1, frame_size);
        rObj.data = ptr;
        int out_size = (frame_blks * (mtu_size + 100));
        char *out_buf = calloc(1, out_size);
        short *rtpSize = calloc(1, (frame_blks + 1) * sizeof(short));

        int out_size2 = (frame_blks * (mtu_size + 100));
        char *out_buf2 = calloc(1, out_size);
        short *oSize = rtpSize;// calloc(1, (frame_blks + 1) * sizeof(short));

        rObj.info.file_xorcode = 0;      //文件异或码（文件首个64MBytes）
        strcpy(rObj.info.filename, ifilename);    //去除路径的文件名
        //
        long long sumsize = 0;
        long long sumsize2 = 0;
        int status = 0;
        int k = 0;
        unsigned int pkt_idx = 0;
        //
        rObj.snd_size = 0;
        rObj.seq_no = 0;
        rObj.group_id = 0;
        rObj.enable_encrypt = 0;
        rObj.enable_fec = 0;
        //
        printf("call_test: total_size=%d \n", total_size);
        //
        while((sumsize < total_size) && !status)
        {
            //group: 256 * 256 * 1100
            rObj.frame_id = 0;
            for(int i = 0; i < rObj.info.group_size; i++)
            {
                printf("call_test: rObj.info.group_size=%d, i=%d \n", rObj.info.group_size, i);
                if(status)
                {
                    break;
                }
                int offset = 0;
                char *ptr = rObj.data;

                for(int j = 0; j < rObj.info.frame_size; j++)
                {
                    if(sumsize >= total_size)
                    {
                        printf("call_test: sumsize=%lld, total_size=%lld ######## \n", sumsize, total_size);
                        status = 1;
                        break;
                    }
                    //printf("call_test: rObj.info.frame_size=%d \n", rObj.info.frame_size);
                    int rsize = fread(&ptr[offset], 1, mtu_size, rfp);
                    //printf("call_test: rsize=%d, j=%d \n", rsize, j);
                    if(rsize != mtu_size)
                    {
                        printf("call_test: rsize=%d, j=%d ######## \n", rsize, j);
                        status = 1;
                        sumsize += rsize;
                        offset += rsize;
                        break;
                    }
                    sumsize += rsize;
                    offset += rsize;
                }
                //
                rObj.data_size = offset;//frame_size;
                rObj.data_xorcode = 0;
                //
                int ret = file_packet(&rObj, out_buf, out_size, rtpSize);
                //printf("call_test: file_packet: ret=%d \n", ret);
                wObj.data = out_buf;
                wObj.data_size = ret;//rObj.data_size;
                ret = file_unpacket(&wObj, out_buf2, out_size2, oSize);
                //printf("call_test: file_unpacket: ret=%d \n", ret);
                //
                ret = pkt2file(idxfp, wfp, out_buf2, ret, oSize, &pkt_idx);
                sumsize2 += ret;
                //printf("call_test: pkt2file: ret=%d \n", ret);
                if(sumsize != sumsize2)
                {
                    printf("call_test: sumsize=%lld, sumsize2=%lld \n", sumsize, sumsize2);
                    printf("call_test: i=%d, k=%d TTTTTTTTTTTTTTTTT \n", i, k);
                }
                if(sumsize >= total_size)
                {
                    printf("call_test: sumsize=%lld, total_size=%lld ######## \n", sumsize, total_size);
                    status = 1;
                    break;
                }
            }
            k++;
            printf("call_test: sumsize=%lld, sumsize2=%lld, total_size=%lld \n", sumsize, sumsize2, total_size);
            printf("call_test: sumsize=%d (MB), status=%d, k=%d \n", (sumsize >> 20), status, k);
            ret = (sumsize2 >> 20);
        }
#if 1
        //验证索引文件
        if(idxfp)
        {
            fclose(idxfp);
            idxfp = NULL;
        }
        FILE *idxrfp = fopen(idxfilename, "rb");
        if(idxrfp)
        {

            fseek(idxrfp, 0, SEEK_END);
            long long total_size2 = ftell(idxrfp);
            printf("call_test: total_size2=%d \n", total_size2);
            rewind(idxrfp);
            sumsize = 0;
            sumsize2 = 0;
            int head_size = sizeof(CacheHead);
            printf("call_test: head_size=%d \n", head_size);
            int i = 0;
            while((sumsize < total_size2))
            {
                int rsize = fread(ptr, 1, head_size, idxrfp);
                if(rsize)
                {
                    CacheHead *src_head = (CacheHead *)ptr;
                    int data_type = src_head->data_type;
                    int data_size = src_head->data_size;
                    int rtp_pkt_size = src_head->rtp_pkt_size;
                    if(data_type)
                    {
                        int rsize2 = fread(ptr, 1, rtp_pkt_size, idxrfp);
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

#if 1
        //验证文件改写
        if(wfp)
        {
            fclose(wfp);
            wfp = NULL;
        }
        FILE *wfp2 = fopen(ofilename, "r+b+");
        if(wfp2)
        {
            fseek(wfp2, 0, SEEK_END);//SEEK_SET//SEEK_CUR
            long long total_size3 = ftell(wfp2);
            printf("call_test: total_size3=%lld \n", total_size3);
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
                    for(int j = 0; j < 1000; j++)
                    {
                        int wsize = fwrite(ptr, 1, data_size, wfp2);
                        if(wsize != data_size)
                        {
                            printf("call_test: wsize=%d \n", wsize);
                            break;
                        }
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
        if(out_buf2)
        {
            free(out_buf2);
        }
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
