
#include "file_rtp.h"

int create_vector(FileRtpObj *obj)
{
    unsigned int frame_size = obj->info.frame_size;
    unsigned int pic_size = obj->info.pic_size;
    unsigned int group_size = obj->info.group_size;
}
int push_pkt(FileRtpObj *obj, char *data, int size)
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
    //printf("file_unpacket: rtp_hdr->payload=%d \n", rtp_hdr->payload);
    //printf("file_unpacket: flag=%d \n", flag);
    if(flag)
    {
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
	        int data_type = rtp_ext->data_type;
            int frame_id = rtp_ext->frame_id;
            int pic_id = rtp_ext->pic_id;
            int group_id = rtp_ext->group_id;
            int pkt_idx = rtp_ext->pkt_idx;
            int payload_size = rtp_ext->rtp_pkt_size;

            if(!data_type)
            {
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
