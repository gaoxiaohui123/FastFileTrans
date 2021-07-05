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

int file_packet(FileRtpObj *obj, char *out_buf, int out_size, short *rtpSize)
{
    int ret = 0;
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
        if (obj->seq_no >= MAX_USHORT)
        {
            obj->seq_no = 0;
        }
        else{
            obj->seq_no++;
        }
        if(flag)
        {
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 1;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = obj->frame_id;
            rtp_ext->group_id = obj->group_id;
            rtp_ext->rtp_xorcode = 0;
            //
            FileInfo *info_data = (FileInfo *)payload_ptr;//&out_buf[offset2 + rtp_header_size + ext_size];
            memcpy((void *)info_data, (void *)info, payload_size);
            //
            offset2 += rtp_header_size + ext_size + payload_size;
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
            rtp_ext->rtp_xorcode = 0;
            //
            char *src_ptr = (char *)&data[offset];
            memcpy((void *)payload_ptr, (void *)src_ptr, payload_size);
            //
            offset += payload_size;
            offset2 += rtp_header_size + ext_size + payload_size;
            obj->snd_size += payload_size;
            rtpSize[i] = rtp_header_size + ext_size + payload_size;
            obj->pkt_idx++;
        }
        if(obj->snd_size == filesize)
        {
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 2;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = obj->frame_id;
            rtp_ext->group_id = obj->group_id;
            rtp_ext->rtp_xorcode = 0;
            //
            FileInfo *info_data = (FileInfo *)payload_ptr;//&out_buf[offset2 + rtp_header_size + ext_size];
            memcpy((void *)info_data, (void *)info, payload_size);
            //
            offset2 += rtp_header_size + ext_size + payload_size;
            rtpSize[i] = rtp_header_size + ext_size + payload_size;
        }
        i++;
    }
    obj->frame_id++;
    //obj->seq_no = 0;
    ret = offset2;
    return ret;
}
int file_unpacket(FileRtpObj *obj, char *out_buf, int out_size, short *oSize)
{
    int ret = 0;
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
    while(offset < data_size)
    {
        int pkt_size = oSize[i];
        char *src_ptr = (char *)&data[offset];
        RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&data[offset];
        FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&data[offset + rtp_header_size];
        char *payload_ptr = (char *)&data[offset + rtp_header_size + ext_size];
        //
        int flag = 0;
        flag &= rtp_hdr->payload     == FILE_PLT;  //负载类型号，									PT
        flag &= rtp_hdr->version     == 2;  //版本号，此版本固定为2								V
        flag &= rtp_hdr->padding	 == 0;//														P
        flag &= rtp_hdr->csrc_len	 == 0;//														CC
        flag &= rtp_hdr->marker		 == 0;//(size >= len);   //标志位，由具体协议规定其值。		M
        //rtp_hdr->ssrc        = ssrc;//(unsigned int)svc_nalu;;//htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一	SSRC
        flag &= rtp_hdr->extension	 == 1;//														X
        //rtp_hdr->timestamp   = 0;//?
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
            if(this_pkt_size == pkt_size && extlen == (ext_size + 4))
            {
                char *dst_ptr = (char *)&out_buf[offset2];
                CacheHead *dst_head = (CacheHead *)dst_ptr;
                char *dst_payload = (char *)&dst_ptr[sizeof(CacheHead)];
                unsigned short dst_pkt_size = rtp_pkt_size + sizeof(CacheHead);
                memcpy((void *)dst_payload, payload_ptr, rtp_pkt_size);
                dst_head->rtp_pkt_size = rtp_ext->rtp_pkt_size + sizeof(CacheHead);
                dst_head->data_type = rtp_ext->data_type;
                dst_head->enable_encrypt = rtp_ext->enable_encrypt;
                dst_head->frame_id = rtp_ext->frame_id;
                dst_head->group_id = rtp_ext->group_id;
                dst_head->pkt_idx = rtp_ext->pkt_idx;
                //
                offset2 += dst_pkt_size;
            }
        }
        offset += pkt_size;
        i++;
    }
    ret = offset2;
    return ret;
}
FQT_API
void call_test(char *ifilename, char *ofilename)
{
    //文件或文件夹
    FILE *rfp = fopen(ifilename, "rb");
    FILE *wfp = fopen(ofilename, "wb");
    if(rfp && wfp)
    {
        FileRtpObj rObj = {};
        FileRtpObj wObj = {};
        //
        fseek(rfp, 0, SEEK_END);
        long long total_size = ftell(rfp);
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

        rObj.data = calloc(1, frame_size);
        int out_size = (frame_blks * (mtu_size + 100));
        char *out_buf = calloc(1, out_size);
        short *rtpSize = calloc(1, (frame_blks + 1) * sizeof(short));

        int out_size2 = (frame_blks * (mtu_size + 100));
        char *out_buf2 = calloc(1, out_size);
        short *oSize =rtpSize;// calloc(1, (frame_blks + 1) * sizeof(short));

        rObj.info.file_xorcode = 0;      //文件异或码（文件首个64MBytes）
        strcpy(rObj.info.filename, ifilename);    //去除路径的文件名
        //
        long long sumsize = 0;
        int status = 0;
        int k = 0;
        //
        rObj.snd_size = 0;
        rObj.seq_no = 0;
        rObj.group_id = 0;
        rObj.enable_encrypt = 0;
        rObj.enable_fec = 0;
        //
        while((sumsize < total_size) && !status)
        {
            //group: 256 * 256 * 1100
            rObj.frame_id = 0;
            for(int i = 0; i < rObj.info.group_size; i++)
            {
                if(status)
                {
                    break;
                }
                int offset = 0;
                char *ptr = rObj.data;
                //
                rObj.data_size = frame_size;
                rObj.data_xorcode = 0;
                //
                for(int j = 0; j < rObj.info.frame_size; j++)
                {
                    int rsize = fread(&ptr[offset], 1, mtu_size, rfp);
                    if(rsize < 0)
                    {
                        status = 1;
                        break;
                    }
                    sumsize += rsize;
                    offset += rsize;
                }
                //
                int ret = file_packet(&rObj, out_buf, out_size, rtpSize);

                wObj.data = out_buf;
                wObj.data_size = ret;//rObj.data_size;
                ret = file_unpacket(&wObj, out_buf2, out_size2, oSize);

                ///int wsize = fwrite(, 1, wsize, wfp);
            }
            k++;
        }

        //
        fclose(rfp);
        fclose(wfp);
    }
}
