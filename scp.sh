# !/bin/bash

#ssh -o ServerAliveInterval=60 root@47.92.7.66
# bizconf123!@#
#10.200.3.208

SRC_DIR=root@47.92.7.66:/root/works/
DST_DIR=/home/gxh/works/huichang/

#scp -r $DST_DIR/FastFileTrans $SRC_DIR/
scp $DST_DIR/FastFileTrans/scp.sh $SRC_DIR/FastFileTrans/
scp $DST_DIR/FastFileTrans/source/base_stun.c $SRC_DIR/FastFileTrans/source/
#scp $DST_DIR/FastFileTrans/source/stun_list.c $SRC_DIR/FastFileTrans/source/
scp $DST_DIR/FastFileTrans/include/file_rtp.h $SRC_DIR/FastFileTrans/include/
scp $DST_DIR/FastFileTrans/test.py $SRC_DIR/FastFileTrans/
#scp -r $SRC_DIR/FastFileTrans $DST_DIR/ 

















