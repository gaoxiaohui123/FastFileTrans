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

if sys.version_info >= (3, 0):
    import tkinter as tk
    from tkinter import *
    import tkinter.ttk as ttk
else:
    import Tkinter as tk
    from Tkinter import *
    import ttk


#sys.stdout = io.TextIOWrapper(sys.stdout.buffer,encoding='utf-8')

sys.path.append(".")

gload = None

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

def TestLocal():
    start_time = time.time()
    ifilename = '/home/gxh/works/datashare/foreman_cif.yuv'
    ofilename = '/home/gxh/works/foreman_cif.yuv'
    idxfilename = '/home/gxh/works/foreman_cif.idx'
    ret = gload.lib.call_test(ifilename.encode('utf-8'), ofilename.encode('utf-8'), idxfilename.encode('utf-8'))
    end_time = time.time()
    difftime = int((end_time - start_time) * 1000)
    print("TestLocal: difftime(ms)= ", difftime)
    print('TestLocal: difftime={}(ms)'.format(difftime))
if __name__ == '__main__':
    print('Start pycall.')
    TestLocal()
    print('End pycall.')
