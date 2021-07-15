# -*- coding: utf-8 -*


import sys
import os
import io
import shutil
import time

# import matplotlib.pyplot as plt # plt 用于显示图片
# from matplotlib import pyplot as plt
# import matplotlib.image as mpimg # mpimg 用于读取图片
#import numpy as np
# from PIL import Image
import math
import threading
if sys.version > '3':
    import queue
else:
    import Queue as queue
import inspect
import platform


import ctypes
from ctypes import *
from ctypes import c_longlong as ll
import json
import errno
import struct
import binascii
import numpy as np
#import cv2
import random



#sys.stdout = io.TextIOWrapper(sys.stdout.buffer,encoding='utf-8')

sys.path.append(".")

gload = None

global_port = 10080
global_host = 'localhost'
global_type = 0

if len(sys.argv) > 3:
    global_port = int(sys.argv[3])
if len(sys.argv) > 2:
    global_host = sys.argv[2]
if len(sys.argv) > 1:
    global_type = int(sys.argv[1])

class LoadLib(object):
    def __init__(self):
        ll = ctypes.cdll.LoadLibrary

        print("executable path= ", os.path.dirname(sys.executable))
        print("中文乱码")
        print("load my dll")
        #print("sys.path= ", sys.path)
        thispath = os.path.abspath('./test.py')
        print("thispath= ", thispath)
        current_file_name = inspect.getfile(inspect.currentframe())
        print("current_file_name= ", current_file_name)
        current_working_dir = os.getcwd()
        print("current_working_dir=", current_working_dir)
        print(platform.system())
        if (platform.system() == 'Windows'):
            print('Windows系统')
            dllfile = os.path.join(current_working_dir, "libqft.dll")
        elif(platform.system() == 'Linux'):
            print('Linux系统')
            dllfile = os.path.join(current_working_dir, "libqft.so")
        elif (platform.system() == 'Darwin'):
            print('Darwin系统')
            dllfile = os.path.join(current_working_dir, "libqft.dylib")
        else:
            print('其他')

        print("dllfile= ", dllfile)
        is_exists = os.path.exists(dllfile)
        print("is_exists= ", is_exists)
        if is_exists:
            try:
                self.lib = ll(dllfile)
            #except IOError, error: #python2
            except IOError as error:  # python3
                print("LoadLib: error=", error)
                print("load failed !!!")
                return
        #self.lib.api_resort_packet_all.restype = ctypes.POINTER(StructPointer)
        #self.lib.api_get_sessionInfo.restype = ctypes.POINTER(SessInfoPointer)
        print("load sucess !!!")
        print("LoadLib ok")

gload = LoadLib()

def RunServer():
    print("RunServer: (global_host, global_port)=", global_host, global_port)
    handle_size = 8
    handle = create_string_buffer(handle_size)

    ret = gload.lib.api_socket_start(handle, global_host.encode('utf-8'), global_port, 0)
    idx = 0
    while idx >= 0 and idx < 4:
        if sys.version_info >= (3, 0):
            idx = int(input('please input to exit(eg: 0 ): '))
        else:
            idx = int(raw_input('please input to exit(eg: 0 ): '))
        print("idx= ", idx)
        #thread.status = False if idx == 0 else True
    print("RunServer: start stop...")
    ret = gload.lib.api_socket_stop(handle)
    print("RunServer: over")
def RunClient():
    print("RunClient: (global_host, global_port)=", global_host, global_port)
    handle_size = 8
    handle = create_string_buffer(handle_size)

    ret = gload.lib.api_socket_start(handle, global_host.encode('utf-8'), global_port, 1)
    ret = gload.lib.api_socket_test(handle)
    idx = 0
    while idx >= 0 and idx < 4:
        if sys.version_info >= (3, 0):
            idx = int(input('please input to exit(eg: 0 ): '))
        else:
            idx = int(raw_input('please input to exit(eg: 0 ): '))
        print("idx= ", idx)
        #thread.status = False if idx == 0 else True
    print("RunClient: start stop...")
    ret = gload.lib.api_socket_stop(handle)
    print("RunClient: over")
def TestLocal():
    start_time = time.time()
    ifilename = '/home/gxh/works/datashare/InToTree_1920x1080.yuv'
    ifilename = '/home/gxh/works/InToTree_1920x1080_0.yuv'
    ofilename = '/home/gxh/works/InToTree_1920x1080.yuv'
    idxfilename = '/home/gxh/works/InToTree_1920x1080.idx'
    img_size = (1920 * 1080 * 3) >> 1
    ret = gload.lib.call_test(ifilename.encode('utf-8'), ofilename.encode('utf-8'), idxfilename.encode('utf-8'), img_size)
    end_time = time.time()
    difftime = int((end_time - start_time) * 1000)
    t0 = difftime / 1000
    t1 = difftime % 1000
    #print("TestLocal: difftime(ms)= ", difftime)
    print('TestLocal: difftime={}(s){}(ms)'.format(t0, t1))
    avg = (ret * 1000) / difftime
    print('TestLocal: difftime={:.3f}(MB/s)'.format(avg))
    avg2 = (ret * 1000 * 8) / (difftime * 1024)
    print('TestLocal: difftime={:.3f}(Gbps)'.format(avg2))
def TestSock():
    server_ip = "127.0.0.1"
    port = 10080
    ret = gload.lib.api_sock_test(server_ip.encode('utf-8'), port)
    print("TestSock: ret=", ret)
if __name__ == '__main__':
    print('Start pycall.')
    #TestLocal()
    #TestSock()
    if global_type == 0:
        RunServer()
    else:
        RunClient()
    print('End pycall.')

#python test.py 0 10.200.3.208
#python test.py 1 47.92.7.66