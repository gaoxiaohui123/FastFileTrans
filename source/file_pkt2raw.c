
#include "file_rtp.h"

int my_fwrite(FILE *fp, char *data, int size, char *func_name)
{
    int ret = fwrite(data, 1, size, fp);
    if(ret != size)
    {
        char text[512] = "error: my_fwrite: \n";
        strcat(text, func_name);
        strcat(text, ": ret=%d, size=%d \n");
        MYPRINT2(text, ret, size);
    }
    return ret;
}
int create_vector(FileRtpObj *obj)
{
    int ret = 0;
    unsigned int frame_size = obj->info.frame_size;//256
    unsigned int pic_size = obj->info.pic_size;//12
    unsigned int group_size = obj->info.group_size;//256
    MYPRINT2("create_vector: group_size=%d \n", group_size);
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
int free_picture(FileRtpObj *obj, PicVector *p1)
{
    int ret = 0;
    unsigned int frame_size = obj->info.frame_size;//256
    unsigned int pic_size = obj->info.pic_size;//12
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
                //free(p2->pktItem);
                //p2->pktItem = NULL;
                //p2->max_num = 0;
                p2->num = 0;
                p2->complete = 0;
                p2->complete_num = 0;
            }
        }//j
        if(p1->img)
        {
            free(p1->img);
            p1->img = NULL;
        }
        //if(p1->p_blk_size)
        //{
        //    free(p1->p_blk_size);
        //    p1->p_blk_size = NULL;
        //}
        p1->img_size = 0;
        p1->num = 0;
        p1->complete_num = 0;
        //free(p1->frameVector);
        //p1->frameVector = NULL;
        //p1->max_num = 0;
    }
    return ret;
}
int release_vector(FileRtpObj *obj)
{
    int ret = 0;
    MYPRINT2("release_vector: start \n");
    unsigned int frame_size = obj->info.frame_size;//256
    unsigned int pic_size = obj->info.pic_size;//12
    unsigned int group_size = obj->info.group_size;//256
    GroupVector *p0 = &obj->groupVector;
    MYPRINT2("release_vector: p0->picVector=%x \n", p0->picVector);
    MYPRINT2("release_vector: group_size=%d \n", group_size);
    if(p0->picVector)
    {
        ret += group_size * sizeof(PicVector);
        for(int i = 0; i < group_size; i++)
        {
            PicVector *p1 = &p0->picVector[i];
            MYPRINT("release_vector: p1->frameVector=%x \n", p1->frameVector);
            if(p1->frameVector)
            {
                ret += pic_size * sizeof(FrameVector);
                for(int j = 0; j < pic_size; j++)
                {
                    FrameVector *p2 = &p1->frameVector[j];
                    MYPRINT("release_vector: p2->pktItem=%x \n", p2->pktItem);
                    if(p2->pktItem)
                    {
                        ret += frame_size * sizeof(PktItem);
                        for(int k = 0; k < frame_size; k++)
                        {
                            PktItem *p3 = (PktItem *)&p2->pktItem[k];
                            MYPRINT("release_vector: i=%d, j=%d, k=%d \n", i, j, k);
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
                        p2->num = 0;
                    }
                    else{
                        MYPRINT2("release_vector: p2->pktItem=%x, j=%d \n", p2->pktItem, j);
                    }
                }//j
                if(p1->img)
                {
                    free(p1->img);
                    p1->img = NULL;
                }
                //if(p1->p_blk_size)
                //{
                //    free(p1->p_blk_size);
                //    p1->p_blk_size = NULL;
                //}
                p1->img_size = 0;
                free(p1->frameVector);
                p1->frameVector = NULL;
                p1->max_num = 0;
                p1->num = 0;
            }
        }//i
        free(p0->picVector);
        p0->picVector = NULL;
        p0->max_num = 0;
    }
    MYPRINT2("release_vector: (ret)=%d \n", (ret));
    return ret;
}
int count_bw(FileRtpObj *obj, int64_t now_time, int size)
{
    int ret = 0;
    obj->bwCount.sum_size += size;
    int sum_size = obj->bwCount.sum_size;
    int64_t start_time = obj->bwCount.start_time;
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

int picture2render(FileRtpObj *obj, PicVector *p1)
{
    int ret = 0;
    MYPRINT2("picture2render: start \n");
    free_picture(obj, p1);
    MYPRINT2("picture2render: ok \n");
    return ret;
}
int get_picture(FileRtpObj *obj, PicVector *p1)
{
    int ret = 0;
    MYPRINT2("get_picture: start \n");
    int64_t now_time = api_get_sys_time(0);
    MYPRINT2("get_picture: p1->img=%x \n", p1->img);
    if(p1->img)
    {
        free(p1->img);
        p1->img = NULL;
        p1->img_size = 0;
    }
    p1->img_size = obj->info.img_size;//pic_size * obj->info.frame_size * (obj->info.block_size + sizeof(CacheHead));
    //p1->p_blk_size = (short *)calloc(1, (obj->info.pic_size * obj->info.frame_size) * sizeof(short));
    p1->img = (char *)calloc(1, p1->img_size * sizeof(char));
    MYPRINT2("get_picture: p1->img_size=%d \n", p1->img_size);
    //file_unpacket(FileRtpObj *obj, char *out_buf, int out_size, short *oSize)
    char pnull[2048] = {0};
    FILE *index_fp = obj->index_fp;
    FILE *raw_fp = obj->raw_fp;
    int rtp_header_size = sizeof(RTP_FIXED_HEADER);
    int ext_size = sizeof(FILE_EXTEND_HEADER);
    int rtp_extend_length = (sizeof(FILE_EXTEND_HEADER) >> 2) - 1;
    int block_size = obj->info.block_size;
    int mtu_size = obj->info.block_size;
    int min_blk_size = obj->info.min_blk_size;
    int offset = 0;
    int sum_size = 0;
    char *out_buf = p1->img;
    //short *p_blk_size = p1->p_blk_size;
    int enable_fec = 0;
    int i = 0;
    for(int j = 0; j < p1->max_num; j++)
    {
        if(offset >= p1->img_size)
        {
            break;
        }
        int frame_id = j;
        FrameVector *p2 = &p1->frameVector[frame_id];
        MYPRINT("get_picture: p2=%x \n", p2);

        for(int k = 0; k < p2->max_num; k++)
        {
            PktItem *p3 = (PktItem *)&p2->pktItem[k];
            MYPRINT("get_picture: p2->max_num=%d, p3=%x \n", p2->max_num, p3);
            int lost = 0;
            //int is_last_blk = (j == (p1->max_num - 1)) && (k == (p2->max_num - 1));
            if(enable_fec)
            {
                //
            }
            if(p3->size > 0 && p3->data)
            {
                MYPRINT("get_picture: p3->data=%x, p3->size=%d \n", p3->data, p3->size);
                char *src_ptr = (char *)p3->data;
                RTP_FIXED_HEADER *rtp_hdr    = (RTP_FIXED_HEADER *)&src_ptr[0];
                FILE_EXTEND_HEADER *rtp_ext  = (FILE_EXTEND_HEADER *)&src_ptr[rtp_header_size];
                char *payload_ptr = (char *)&src_ptr[rtp_header_size + ext_size];
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
                    if(this_pkt_size == p3->size && extlen == ext_size)
                    {
                        enable_fec = rtp_ext->enable_fec;
                        int64_t time_stamp0 = rtp_ext->time_stamp0;
	                    int64_t time_stamp1 = rtp_ext->time_stamp1;
	                    int64_t packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	                    if(!obj->net_time)
	                    {
	                        //与对端进行时间对准
	                        obj->net_time = now_time - packet_time_stamp;
	                    }
                        char *dst_ptr = (char *)&out_buf[offset];
                        CacheHead dst_head;// = (CacheHead *)dst_ptr;
                        //char *dst_payload = (char *)&dst_ptr[sizeof(CacheHead)];
                        //unsigned short dst_pkt_size = rtp_pkt_size + sizeof(CacheHead);
                        memcpy((void *)dst_ptr, payload_ptr, rtp_pkt_size);
                        dst_head.rtp_pkt_size = rtp_ext->rtp_pkt_size;// + sizeof(CacheHead);
                        dst_head.data_type = rtp_ext->data_type;
                        dst_head.enable_encrypt = rtp_ext->enable_encrypt;
                        dst_head.frame_id = rtp_ext->frame_id;
                        dst_head.pic_id = rtp_ext->pic_id;
                        dst_head.group_id = rtp_ext->group_id;
                        dst_head.pkt_idx = rtp_ext->pkt_idx;
                        //
                        sum_size += (int)rtp_pkt_size;
                        if(rtp_pkt_size != mtu_size)
                        {
                            MYPRINT2("tail: get_picture: rtp_pkt_size=%d, mtu_size=%d \n", rtp_pkt_size, mtu_size);
                            MYPRINT2("tail: get_picture: k=%d, j=%d, i=%d \n", k, j, i);
                        }
                        //p_blk_size[i] = dst_pkt_size;
                        if(index_fp)
                        {
                            my_fwrite(index_fp, &dst_head, sizeof(CacheHead), "get_picture");
                        }
                        if(raw_fp && false)
                        {
                            my_fwrite(raw_fp, dst_ptr, rtp_pkt_size, "get_picture");
                        }
                        //printf("file_unpacket: offset2=%d, i=%d \n", offset2, i);
                        MYPRINT("get_picture: offset=%d, i=%d \n", offset, i);
                        i++;
                    }
                }//flag
                else{
                    MYPRINT2("not rtp: get_picture: p3->size=%d, j=%d, k=%d \n", p3->size, j, k);
                    MYPRINT2("not rtp: get_picture: rtp_hdr->payload=%d, rtp_hdr->version=%d \n", rtp_hdr->payload, rtp_hdr->version);
                    MYPRINT2("not rtp: get_picture: rtp_hdr->csrc_len=%d, rtp_hdr->extension=%d \n", rtp_hdr->csrc_len, rtp_hdr->extension);
                }
            }
            else
            {
                MYPRINT2("lost: get_picture: p3->size=%d, p3->data=%x, j=%d, k=%d \n", p3->size, p3->data, j, k);
                lost = 1;
            }
            offset += (int)mtu_size;
            if(offset >= p1->img_size)
            {
                break;
            }
        }//k
    }//j
    //if(sum_size != p1->img_size)
    {
        int diff_size = p1->img_size - sum_size;
        MYPRINT2("get_picture: sum_size=%d, p1->img_size=%d, diff_size=%d \n", sum_size, p1->img_size, diff_size);
    }
    if(raw_fp)
    {
        my_fwrite(raw_fp, p1->img, p1->img_size, "get_picture");
    }
    MYPRINT2("get_picture: offset=%d, i=%d \n", offset, i);
    ret = offset;
    picture2render(obj, p1);//test
    MYPRINT2("get_picture: ok \n");
    return ret;
}
int get_picture_all(FileRtpObj *obj)
{
    int ret = 0;int max_delay = obj->max_delay;
    GroupVector *p0 = &obj->groupVector;
    int last_frame_id = obj->lrCount.last_frame_id;
    int last_pic_id = obj->lrCount.last_pic_id;//主要检测对象(以帧为单元)
    int last_group_id = obj->lrCount.last_group_id;
    int start_id = last_pic_id + 1;
    start_id = (!last_group_id && !last_pic_id && !last_frame_id) ? 0 : start_id;
    int i = start_id;
    int j = 0;
    int pic_id = i % p0->max_num;
    PicVector *p1 = &p0->picVector[pic_id];
    get_picture(obj, p1);
    int64_t packet_time_stamp0 = p1->frame_time_stamp;
    int64_t now_time0 = p1->now_time;
    i++; j++;
    while(1)
    {
        pic_id = i % p0->max_num;
        p1 = &p0->picVector[pic_id];
        int64_t packet_time_stamp1 = p1->frame_time_stamp;
        int64_t now_time1 = p1->now_time;
        int delay_time0 = (int)(packet_time_stamp1 - packet_time_stamp0);
        int delay_time1 = (int)(now_time1 - now_time0);
        if(delay_time0 >= (max_delay >> 1) || j >= p0->max_num)
        {
            break;
        }
        get_picture(obj, p1);
        i++; j++;
    }//while
    return ret;
}
int count_net_delay(FileRtpObj *obj, int pkt_time)
{
    int ret = -1;
    int64_t now_time = api_get_sys_time(obj->net_time);
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
            //get_picture(obj, p1);
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
int count_lossrate(FileRtpObj *obj, int64_t now_time)
{
    int ret = -1;
    int max_delay = obj->max_delay;
    GroupVector *p0 = &obj->groupVector;
    int64_t start_time = obj->lrCount.start_time;
    if(!start_time)
    {
        obj->lrCount.start_time = now_time;
    }
    else if(p0->num > 0 && obj->lrCount.pkt_num > 100)
    {
        int64_t start_time = obj->lrCount.start_time;
        int difftime = (int)(now_time - start_time);
        MYPRINT("count_lossrate: difftime=%d \n", difftime);
        if(difftime >= max_delay)//obj->max_delay
        {
            int last_frame_id = obj->lrCount.last_frame_id;
            int last_pic_id = obj->lrCount.last_pic_id;//主要检测对象(以帧为单元)
            int last_group_id = obj->lrCount.last_group_id;
            int start_id = last_pic_id + 1;
            start_id = (!last_group_id && !last_pic_id && !last_frame_id) ? 0 : start_id;
            int i = start_id;
            int j = 0;
            int pic_id = i % p0->max_num;
            PicVector *p1 = &p0->picVector[pic_id];
            PicVector *pstart = p1;
            int64_t start_check_time = pstart->start_check_time;
            //
            int flag = !start_check_time;
            MYPRINT("count_lossrate: start_check_time=%d \n", start_check_time);
            if(start_check_time)
            {
                difftime = (int)(now_time - start_check_time);
                if(difftime >= (max_delay >> 1))
                {
                    return get_picture_all(obj);
                }
                int64_t last_check_time = pstart->last_check_time;
                difftime = (int)(now_time - last_check_time);
                int net_dealy = obj->ndCount.net_dealy;
                if(net_dealy > (max_delay >> 1))
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
            //
            MYPRINT("count_lossrate: flag=%d \n", flag);
            if(flag)
            {
                int64_t packet_time_stamp0 = p1->frame_time_stamp;
                int64_t now_time0 = p1->now_time;
                i++; j++;
                while(1)
                {
                    pic_id = i % p0->max_num;
                    p1 = &p0->picVector[pic_id];
                    int64_t packet_time_stamp1 = p1->frame_time_stamp;
                    int64_t now_time1 = p1->now_time;
                    int delay_time0 = (int)(packet_time_stamp1 - packet_time_stamp0);
                    int delay_time1 = (int)(now_time1 - now_time0);
                    if(delay_time0 >= (max_delay >> 1) || j >= p0->max_num)
                    {
                        if(delay_time0 >= (max_delay >> 1))
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
    MYPRINT("count_lossrate: ret=%d \n", ret);
    return ret;
}

int pkt2cache(FileRtpObj *obj, char *data, int size)
{
    MYPRINT("pkt2cache: start \n");
    int ret = 0;
    int mtu_size = obj->info.block_size;
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
        MYPRINT("pkt2cache: this_pkt_size=%d, size=%d, extlen=%d, ext_size=%d \n", this_pkt_size, size, extlen, ext_size);
        if(this_pkt_size == size && extlen == ext_size)
        {
            int64_t time_stamp0 = rtp_ext->time_stamp0;
	        int64_t time_stamp1 = rtp_ext->time_stamp1;
	        int64_t packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
	        int data_type = rtp_ext->data_type;
	        int blk_id = rtp_ext->blk_id;
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
                        int64_t now_time = api_get_sys_time(obj->net_time);
                        int k = blk_id;//(int)(pkt_idx % p2->max_num);
                        PktItem *p3 = (PktItem *)&p2->pktItem[k];
                        if(p3->size > 0 && p3->group_id != group_id)
                        {
                            MYPRINT2("error: pkt2cache: too fast: p3->size=%d, p3->group_id=%u, group_id=%u \n", p3->size, p3->group_id, group_id);
                        }
                        else
                        {

                            int not_rtx = !p3->size;
#if 1
                            char *data2 = (char *)calloc(1, size * sizeof(char));
                            memcpy(data2, data, size);
                            p3->data = data2;
#else
                            p3->data = data;
#endif

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
                            MYPRINT("pkt2cache: p2->num=%d, frame_id=%d \n", p2->num, frame_id);

                            if(p3->size < mtu_size || (frame_id == p2->max_num))
                            {
                                MYPRINT("tail: pkt2cache: p3->size=%d \n", p3->size);
                                MYPRINT("tail: pkt2cache: frame_id=%d \n", frame_id);
                            }
                            if(p2->num >= p2->max_num || (p3->size < mtu_size))//???
                            //if(blk_id >= p2->max_num || (p3->size < mtu_size))
                            {
                                MYPRINT("pkt2cache: p3->size=%d, mtu_size=%d \n", p3->size, mtu_size);
                                MYPRINT("pkt2cache: p2->num=%d, p2->max_num=%d \n", p2->num, p2->max_num);
                                MYPRINT("pkt2cache: frame_id=%d, blk_id=%d \n", frame_id, blk_id);
                                p2->complete = 2;
                                p1->complete_num++;
                            }
                            else{
                                p2->complete = fec_decode(obj);
                                p1->complete_num += p2->complete > 0;
                            }
                        }
                        MYPRINT("pkt2cache: p1->complete_num=%d, p1->max_num=%d \n", p1->complete_num, p1->max_num);
                        //获取帧
                        if(p1->complete_num >= p1->max_num)
                        {
                            MYPRINT2("pkt2cache: get_picture: pic_id=%d \n", pic_id);
                            get_picture(obj, p1);
                        }
                        //丢包统计
                        ///count_lossrate(obj, now_time);
                    }
                    //
                }
                pthread_mutex_unlock(&obj->lock);
                //unlock
                //ret = payload_size;//in case of fec and rtx
            }
            else if(data_type == 1)
            {
                obj->fileHead.size = size;
                memcpy((void *)obj->fileHead.data, (void *)data, size);
                memcpy((void *)&obj->info, payload_ptr, sizeof(FileInfo));
                create_vector(obj);
                FILE *index_fp = obj->index_fp;
                if(index_fp)
                {
                    CacheHead dst_head = {};
                    dst_head.rtp_pkt_size = rtp_ext->rtp_pkt_size;// + sizeof(CacheHead);
                    dst_head.data_type = rtp_ext->data_type;
                    dst_head.enable_encrypt = rtp_ext->enable_encrypt;
                    dst_head.frame_id = rtp_ext->frame_id;
                    dst_head.pic_id = rtp_ext->pic_id;
                    dst_head.group_id = rtp_ext->group_id;
                    dst_head.pkt_idx = rtp_ext->pkt_idx;
                    my_fwrite(index_fp, (void *)&dst_head, sizeof(CacheHead), "pkt2cache");
                    my_fwrite(index_fp, payload_ptr, sizeof(FileInfo), "pkt2cache");
                }
            }
            else if(data_type == 2)
            {
                FileInfo info;
                obj->fileTail.size = size;
                memcpy((void *)obj->fileTail.data, (void *)data, size);
                memcpy((void *)&info, payload_ptr, sizeof(FileInfo));
            }
            ret = size;
        }
    }
    else{
        MYPRINT2("not rtp: pkt2cache: rtp_hdr->payload=%d, rtp_hdr->version=%d \n", rtp_hdr->payload, rtp_hdr->version);
        MYPRINT2("not rtp: pkt2cache: rtp_hdr->csrc_len=%d, rtp_hdr->extension=%d \n", rtp_hdr->csrc_len, rtp_hdr->extension);
    }
    MYPRINT("pkt2cache: ret=%d \n", ret);
    return ret;
}
#if 0
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

    uint64_t filesize = info->filesize;    //文件大小（maxsize = 4T?）
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
    int64_t now_time = api_get_sys_time(0);
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
                int64_t time_stamp0 = rtp_ext->time_stamp0;
	            int64_t time_stamp1 = rtp_ext->time_stamp1;
	            int64_t packet_time_stamp = time_stamp0 | (time_stamp1 << 32);
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
#endif