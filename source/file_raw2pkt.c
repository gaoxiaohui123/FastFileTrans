
#include "file_rtp.h"

extern int create_vector(FileRtpObj *obj);
extern int release_vector(FileRtpObj *obj);

int raw2pkt(FileRtpObj *obj, char *out_buf, int out_size, short *rtpSize)
{
    int ret = 0;
    MYPRINT("file_packet: start \n");
    //FrameNode *frameNode = obj->frameNode;
    FileInfo *info = &obj->info;
    char *data = obj->data;
    int data_size = obj->data_size;
    unsigned short seq_no = obj->seq_no;
    unsigned int frame_id = obj->frame_id;
    unsigned int pic_id = obj->pic_id % obj->info.group_size;
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
        int flag = !seq_no && !frame_id && !pic_id && !group_id;
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
            printf("raw2pkt: 000000000000000000000 uuuuuuuuuuuuuuu \n");
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 1;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = frame_id;
            rtp_ext->pic_id = pic_id;
            rtp_ext->group_id = group_id;
            rtp_ext->pkt_idx = 0;
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
            //
            memcpy((void *)&obj->FileHead, (void *)rtp_hdr, (rtp_header_size + ext_size + payload_size));
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
            int frame_id = obj->frame_id;
            unsigned int pkt_idx = obj->pkt_idx;
            rtp_ext->frame_id = frame_id;
            rtp_ext->pic_id = pic_id;
            rtp_ext->group_id = group_id;
            rtp_ext->pkt_idx = pkt_idx;
			rtp_ext->time_stamp0 = now_time & 0xFFFFFFFF;
			rtp_ext->time_stamp1 = (now_time >> 32) & 0xFFFFFFFF;
            rtp_ext->rtp_xorcode = 0;
            //
            char *src_ptr = (char *)&data[offset];
            memcpy((void *)payload_ptr, (void *)src_ptr, payload_size);
            //lock
            pthread_mutex_lock(&obj->lock);
            GroupVector *p0 = &obj->groupVector;
            if(pic_id < p0->max_num) //obj.info.group_size
            {
                PicVector *p1 = &p0->picVector[pic_id];
                if(frame_id < p1->max_num)
                {
                    FrameVector *p2 = &p1->frameVector[frame_id];
                    int k = (int)(pkt_idx % p2->max_num);
                    PktItem *p3 = (PktItem *)&p2->pktItem[k];
#if 0
                    if(p3->size > 0 && p3->group_id != group_id)
                    {
                        MYPRINT2("error: raw2pkt: too fast: p3->size=%d, p3->group_id=%u, group_id=%u \n", p3->size, p3->group_id, group_id);
                    }
                    else
#endif
                    {
                        p3->data = src_ptr;
                        p3->size = (rtp_header_size + ext_size + payload_size);
                        p3->group_id = group_id;
                    }
                }
            }
            pthread_mutex_unlock(&obj->lock);
            //unlock
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
            printf("raw2pkt: obj->snd_size=%d, filesize=%d uuuuuuuuuuuuuuu \n", obj->snd_size, filesize);
            short payload_size = sizeof(FileInfo);
            rtp_ext->rtp_extend_profile = EXTEND_PROFILE_ID;
            rtp_ext->rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
            rtp_ext->rtp_pkt_size = sizeof(FileInfo);         //当前rtp包大小
            rtp_ext->data_type = 2;         //1:file start info;2:file end info; 0:raw data
            rtp_ext->enable_encrypt = obj->enable_encrypt;    //是否加密
            rtp_ext->enable_fec = obj->enable_fec;         //是否开启fec
            rtp_ext->frame_id = frame_id;
            rtp_ext->pic_id = pic_id;
            rtp_ext->group_id = group_id;
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
            //
            memcpy((void *)&obj->FileTail, (void *)rtp_hdr, (rtp_header_size + ext_size + payload_size));
        }
        //printf("file_packet: offset=%d, i=%d \n", offset, i);
    }
    //obj->frame_id++;
    obj->seq_no = seq_no;
    ret = offset2;
    MYPRINT2("raw2pkt: ret=%d \n", ret);
    return ret;
}

