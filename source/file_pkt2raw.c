
#include "file_rtp.h"

int create_vector(FileRtpObj *obj)
{
    int ret = 0;
    unsigned int frame_size = obj->info.frame_size;//256
    unsigned int pic_size = obj->info.pic_size;//12
    unsigned int group_size = obj->info.group_size;//256
    GroupVector *p0 = &obj->groupVector;
    p0->max_num = group_size;
    p0->picVector = (PicVector *)calloc(1, group_size * sizeof(PicVector));
    ret += group_size * sizeof(PicVector);
    for(int i = 0; i < group_size; i++)
    {
        PicVector *p1 = &p0->picVector[i];
        p1->max_num = pic_size;
        p1->frameVector = (FrameVector *)calloc(1, pic_size * sizeof(FrameVector));
        ret += pic_size * sizeof(FrameVector);
        for(int j = 0; j < pic_size; j++)
        {
            FrameVector *p2 = &p1->frameVector[j];
            p2->max_num = frame_size;
            p2->pktItem = (PktItem *)calloc(1, frame_size * sizeof(PktItem));
            ret += frame_size * sizeof(PktItem);
        }
    }
    return ret;
}
int release_vector(FileRtpObj *obj)
{
    int ret = 0;
    unsigned int frame_size = obj->info.frame_size;//256
    unsigned int pic_size = obj->info.pic_size;//12
    unsigned int group_size = obj->info.group_size;//256
    GroupVector *p0 = &obj->groupVector;
    if(p0->picVector)
    {
        ret += group_size * sizeof(PicVector);
        for(int i = 0; i < group_size; i++)
        {
            PicVector *p1 = &p0->picVector[i];
            if(p1->frameVector)
            {
                ret += pic_size * sizeof(FrameVector);
                for(int j = 0; j < pic_size; j++)
                {
                    FrameVector *p2 = &p1->frameVector[j];
                    if(p2->pktItem)
                    {
                        ret += frame_size * sizeof(PktItem);
                        for(int k = 0; k < frame_size; k++)
                        {
                            PktItem *p3 = (PktItem *)&p2->pktItem[k];
                            if(p3->data)
                            {
                                ret += p3->size;
                                free(p3->data);
                                p3->data = NULL;
                                p3->size = 0;
                            }
                        }//k
                        free(p2->pktItem);
                        p2->pktItem = NULL;
                        p2->max_num = 0;
                    }
                }//j
                free(p1->frameVector);
                p1->frameVector = NULL;
                p1->max_num = 0;
            }
        }//i
        free(p0->picVector);
        p0->picVector = NULL;
        p0->max_num = 0;
    }
    return ret;
}
int count_bw(FileRtpObj *obj, long long now_time, int size)
{
    int ret = 0;
    obj->bwCount.sum_size += size;
    int sum_size = obj->bwCount.sum_size;
    long long start_time = obj->bwCount.start_time;
    if(!start_time)
    {
        start_time = now_time;
        obj->bwCount.start_time = now_time;
    }
    if(sum_size > (100 * 1024))
    {
        int difftime = (int)(now_time - start_time);
        if(difftime > 1000)
        {
            ret = (((sum_size << 3) * 1000) >> 10) / difftime;//kbps
            obj->bwCount.bw = ret;
            obj->bwCount.start_time = now_time;
            obj->bwCount.sum_size = 0;
            MYPRINT2("pkt2cache: obj->bwCount.bw=%d (kbps) \n", ret);
        }
    }
    return ret;
}
int fec_decode(FileRtpObj *obj)
{
    int ret = 0;

    return ret;
}
int get_picture(FileRtpObj *obj)
{
    int ret = 0;

    return ret;
}
int count_net_delay(FileRtpObj *obj, int pkt_time)
{
    int ret = -1;
    long long now_time = api_get_sys_time(obj->net_time);
    int difftime = (int)(now_time - pkt_time);
    if(obj->ndCount.num >= 3 )
    {
        ret = obj->ndCount.sum_delay / obj->ndCount.num;
        ret = ret < 0 ? 0 : ret;
        obj->ndCount.net_dealy = ret;
        obj->ndCount.num = 0;
    }
    else{
        obj->ndCount.sum_delay += difftime;
        obj->ndCount.num++;
    }
    return ret;
}
int get_rtx(FileRtpObj *obj, int start, int end)
{
    int ret = 0;
    GroupVector *p0 = &obj->groupVector;
    for(int i = start; i < end; i++)
    {
        int pic_id = i % p0->max_num;
        PicVector *p1 = &p0->picVector[pic_id];
        if(p1->complete_num >= p1->max_num)
        {
            //get_picture(obj);
        }
        else{
            for(int j = 0; j < p1->max_num; j++)
            {
                int frame_id = j;
                FrameVector *p2 = &p1->frameVector[frame_id];
                if(p2->complete > 0)
                {
                }
                else{
                    for(int k = 0; k < p2->max_num; k++)
                    {
                        PktItem *p3 = (PktItem *)&p2->pktItem[k];
                        if(p3->size > 0)
                        {
                        }
                        else{
                            //hit loss pkt
                        }
                    }
                }
            }

        }
    }

    return ret;
}
//在有限的时间内，完成绝大多数情况下的数据安全传输;
//通过索引文件，进行后期补救；
//时间间隔要与cache_size相匹配
int count_lossrate(FileRtpObj *obj, long long now_time)
{
    int ret = -1;
    GroupVector *p0 = &obj->groupVector;
    long long start_time = obj->lrCount.start_time;
    if(!start_time)
    {
        obj->lrCount.start_time = now_time;
    }
    else if(p0->num > 0 && obj->lrCount.pkt_num > 100)
    {
        long long start_time = obj->lrCount.start_time;
        int difftime = (int)(now_time - start_time);
        if(difftime >= 2000)//obj->max_delay
        {
            last_frame_id = obj->lrCount.last_frame_id;
            last_pic_id = obj->lrCount.last_pic_id;//主要检测对象(以帧为单元)
            last_group_id = obj->lrCount.last_group_id;
            int start_id = last_pic_id + 1;
            start_id = (!last_group_id && !last_pic_id && !last_frame_id) ? 0 : start_id;
            int i = start_id;
            int j = 0;
            int pic_id = i % p0->max_num;
            PicVector *p1 = &p0->picVector[pic_id];
            PicVector *pstart = p1;
            long long start_check_time = pstart->start_check_time;
            //
            int flag = !start_check_time;
            if(start_check_time)
            {
                difftime = (int)(now_time - start_check_time);
                if(difftime >= 1000)
                {
                    return get_picture(obj);
                }
                long long last_check_time = pstart->last_check_time;
                difftime = (int)(now_time - last_check_time);
                int net_dealy = obj->ndCount.net_dealy;
                if(net_dealy > 1000)
                {
                    printf("warning: count_lossrate: net_dealy=%d \n", net_dealy);

                }
                else if(net_dealy > 50)
                {
                    flag |= difftime > (net_dealy << 1);
                }
                else{
                    flag |= difftime > 100;
                }
            }
            flag |= start_check_time > 0 ? ()
            //
            if(flag)
            {
                long long packet_time_stamp0 = p1->frame_time_stamp;
                long long now_time0 = p1->now_time;
                i++; j++;
                while(1)
                {
                    pic_id = i % p0->max_num;
                    p1 = &p0->picVector[pic_id];
                    long long packet_time_stamp1 = p1->frame_time_stamp;
                    long long now_time1 = p1->now_time;
                    int delay_time0 = (int)(packet_time_stamp1 - packet_time_stamp0);
                    int delay_time1 = (int)(now_time1 - now_time0);
                    if(delay_time0 >= 1000 || j >= p0->max_num)
                    {
                        if(delay_time0 >= 1000)
                        {
                            if(!pstart->start_check_time)
                            {
                                pstart->start_check_time = now_time;//防止频繁检测
                            }
                            pstart->last_check_time = now_time;
                            get_rtx(obj, start_id, i);
                        }
                        break;
                    }
                    i++; j++;
                }//while
            }//flag
        }
    }

    return ret;
}


int pkt2cache(FileRtpObj *obj, char *data, int size)
{
    int ret = 0;
    int rtp_header_size = sizeof(RTP_FIXED_HEADER);
    int ext_size = sizeof(FILE_EXTEND_HEADER);
    int rtp_extend_length = (sizeof(FILE_EXTEND_HEADER) >> 2) - 1;
    char *src_ptr = (char *)data;
    RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&data[0];
    FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&data[rtp_header_size];
    char *payload_ptr = (char *)&data[rtp_header_size + ext_size];
    int flag = 1;
    flag &= rtp_hdr->payload     == FILE_PLT;  //负载类型号，									PT
    flag &= rtp_hdr->version     == 2;  //版本号，此版本固定为2								V
    flag &= rtp_hdr->padding	 == 0;//														P
    flag &= rtp_hdr->csrc_len	 == 0;//														CC
    flag &= rtp_hdr->marker		 == 0;//(size >= len);   //标志位，由具体协议规定其值。		M
    //rtp_hdr->ssrc        = ssrc;//(unsigned int)svc_nalu;;//htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一	SSRC
    flag &= rtp_hdr->extension	 == 1;//														X
    //rtp_hdr->timestamp   = 0;//?
    //printf("pkt2cache: rtp_hdr->payload=%d \n", rtp_hdr->payload);
    //printf("pkt2cache: flag=%d \n", flag);
    if(flag)
    {
        unsigned short extprofile = rtp_ext->rtp_extend_profile;// & 7;
        unsigned short rtp_extend_length = rtp_ext->rtp_extend_length;
        rtp_extend_length = ((rtp_extend_length & 0xFF) << 8) | ((rtp_extend_length >> 8));
        unsigned short extlen = (rtp_extend_length + 1) << 2;
        unsigned short rtp_pkt_size = rtp_ext->rtp_pkt_size;
        unsigned short this_pkt_size = rtp_pkt_size + extlen + rtp_header_size;
        //printf("pkt2cache: this_pkt_size=%d, pkt_size=%d, extlen=%d, ext_size=%d \n", this_pkt_size, pkt_size, extlen, ext_size);
        if(this_pkt_size == size && extlen == ext_size)
        {
            long long time_stamp0 = rtp_ext->time_stamp0;
	        long long time_stamp1 = rtp_ext->time_stamp1;
	        long long packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	        int data_type = rtp_ext->data_type;
            int frame_id = rtp_ext->frame_id;
            int pic_id = rtp_ext->pic_id;
            int group_id = rtp_ext->group_id;
            unsigned int pkt_idx = rtp_ext->pkt_idx;
            int payload_size = rtp_ext->rtp_pkt_size;

            if(!data_type)
            {
                //lock
                pthread_mutex_lock(&obj->lock);
                GroupVector *p0 = &obj->groupVector;
                if(pic_id < p0->max_num)
                {
                    PicVector *p1 = &p0->picVector[pic_id];
                    if(frame_id < p1->max_num)
                    {
                        FrameVector *p2 = &p1->frameVector[frame_id];
                        int k = (int)(pkt_idx % p2->max_num);
                        PktItem *p3 = (PktItem *)&p2->pktItem[k];
                        if(p3->size > 0 && p3->group_id != group_id)
                        {
                            MYPRINT2("error: pkt2cache: too fast: p3->size=%d, p3->group_id=%u, group_id=%u \n", p3->size, p3->group_id, group_id);
                        }
                        else
                        {
                            long long now_time = api_get_sys_time(obj->net_time);
                            int not_rtx = !p3->size;
                            p3->data = data;
                            p3->size = size;
                            p3->group_id = group_id;
                            //===count bw
                            count_bw(obj, now_time, size);
                            //===
                            //该域下收到第一个数据
                            if(!p2->num)
                            {
                                if(!p1->num)
                                {
                                    p0->num++;
                                    p1->frame_time_stamp = packet_time_stamp;
                                    p1->now_time = now_time;
                                }
                                p1->num++;
                            }
                            p2->num += not_rtx;
                            obj->lrCount.pkt_num += not_rtx;
                            if(p2->num >= p2->max_num)
                            {
                                p2->complete = 2;
                                p1->complete_num++;
                            }
                            else{
                                p2->complete = fec_decode(obj);
                                p1->complete_num += p2->complete > 0;
                            }
                        }
                        //获取帧
                        if(p1->complete_num >= p1->max_num)
                        {
                            get_picture(obj);
                        }
                        //丢包统计
                        count_lossrate(obj);
                    }
                    //
                }
                pthread_mutex_unlock(&obj->lock);
                //unlock
            }
            else if(data_type == 1)
            {
                memcpy((void *)&obj->info, payload_ptr, sizeof(FileInfo));
                create_vector(obj);
            }
            else if(data_type == 2)
            {
                FileInfo info;
                memcpy((void *)&info, payload_ptr, sizeof(FileInfo));
            }
        }

    }
    return ret;
}
int file_unpacket(FileRtpObj *obj, char *out_buf, int out_size, short *oSize)
{
    int ret = 0;
    MYPRINT("file_unpacket: start: \n");
    FileInfo *info = &obj->info;
    char *data = obj->data;
    int data_size = obj->data_size;
    unsigned short seq_no = obj->seq_no;
    unsigned int frame_id = obj->frame_id;
    unsigned int pic_id = obj->pic_id;
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
                dst_head->pic_id = rtp_ext->pic_id;
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
    MYPRINT("file_unpacket: ret=%d \n", ret);
    return ret;
}