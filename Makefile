#for uos libqft.a

#make -f Makefile-1

# #define constexpr const

PWD := $(shell pwd)
SRC_ROOT = $(PWD)/


# 1、准备工作，编译方式、目标文件名、依赖库路径的定义。
# 目标文件名
DLL = ./libqft.so
TARGET = ./libqft.a
CC = gcc  
CPP = g++
AR  = ar

#这里只加入库头文件路径及库路径  
INCS = -I/usr/include \
-I/usr/local/include \
-I$(SRC_ROOT)/include

LIBS = -L/usr/lib \
-L/usr/local/lib \
-L$(PWD)/ \
-L./
SUBDIRS =  
#生成依赖信息时的搜索目录，比如到下列目录中搜索一个依赖文件(比如.h文件)，例如 -I"./***/" -I"./base/"  
DEFINC =  
#给INCS加上依赖搜索路径，分开写可能会产生不一致情况，而且繁琐  
INCS += $(DEFINC)  

#maintest.c tree/rbtree.c  多了子目录，那就直接添加 目录/*.c即可   所有的源文件--  .c文件列表  
CSRCS = $(wildcard $(SRC_ROOT)/source/file_rtp.c \
$(SRC_ROOT)/source/file_raw2pkt.c \
$(SRC_ROOT)/source/file_pkt2raw.c)

CPPSRCS =
#$(wildcard $(SRC_ROOT)/modules/video_processing/util/denoiser_filter_c.cc



#所有的.o文件列表 
COBJS := $(CSRCS:.c=.o)
CPPOBJS := $(CPPSRCS:.cc=.o)

#  
#生成依赖信息 -MM是只生成自己的头文件信息，-M 包含了标准库头文件信息。  
#-MT 或 -MQ都可以改变生成的依赖  xxx.o:src/xxx.h 为 src/xxx.o:src/xxx.h 当然。前面的 src/xxx.o需自己指定  
#格式为 -MM 输入.c或.cpp  查找依赖路径  -MT或-MQ  生成规则，比如src/xxx.o  
#MAKEDEPEND = gcc -MM -MT

#-g 生成调试信息  
#-pedantic参数与-ansi一起使用 会自动拒绝编译非ANSI程序  
#-fomit-frame-pointer 去除函数框架  
#-Wmissing-prototypes -Wstrict-prototypes 检查函数原型 
CFLAGS += $(INCS) 
#-Wall -O3 -std=c++0x 
#-O2 -Wall -fomit-frame-pointer -g -ansi
CFLAGS  += -O2 -Wall -g -fPIC \
-mavx \
-fpermissive \
-mmmx \
-msse \
-msse2 \
-mssse3 \
-msse4.1 \
-msse4.2 \
-msse4

CPPFLAGS += $(INCS) 
CPPFLAGS += -std=c++11  
CPPFLAGS += -O2 -Wall -g -fPIC \
-mavx \
-fpermissive \
-mmmx \
-msse \
-msse2 \
-mssse3 \
-msse4.1 \
-msse4.2 \
-msse4

 
#针对每个.c文件的.d依赖文件列表  
#CDEF = $(CSRCS:.c=.d)  
#CPPDEF = $(CPPSRCS:.cc=.d) 


all: $(TARGET) 

#生成.o的对自己目录中.h .c的依赖信息.d文件到.c所在的路径中  
#$(DEF)文件是.d文件名列表(含目录)，比如tree.d 匹配成功那么%就是tree，然后在尝试%.c，如果成功。则执行规则  
# $(<:.c=.o)是获取此.c文件的名字(含路径)，然后变为.o比如 src/xxx.o。 以形成如下  
# src/xxx.o : src/xxx.c ***.h  ***.h  最前面！！注意。    
# 此做法是每个.d都和生成他的.c在一个目录里，所以需要这样做。  
# $(<:.c=.o)之类的 。此时的<相当于变量$< 。切记  
# : : :  含义同下  
#$(CDEF) : %.d : %.c  
#	$(MAKEDEPEND) $(<:.c=.o) $< $(DEFINC) > $@  
#$(CPPDEF) : %.d : %.cc  
#	$(MAKEDEPEND) $(<:.cc=.o) $< $(DEFINC) > $@ 
	
#先删除依赖信息  
#重新生成依赖信息  
#这里出现了一个 $(MAKE) 没有定义的变量。这个变量是由 Make 自己定义的，它的值即为自己的位置，方便 Make 递归调用自己。  
#depend:  
#	rm $(CDEF)  
#	rm $(CPPDEF)  
#	$(MAKE) $(CDEF)  
#	$(MAKE) $(CPPDEF)  

#$(OBJS):%.o :%.c  先用$(OBJS)中的一项，比如foo.o: %.o : %.c  含义为:试着用%.o匹配foo.o。如果成功%就等于foo。如果不成功，  
# Make就会警告，然后。给foo.o添加依赖文件foo.c(用foo替换了%.c里的%)  
# 也可以不要下面的这个生成规则，因为下面的 include $(DEF)  就隐含了。此处为了明了，易懂。故留着  
$(COBJS) : %.o: %.c  
	$(CC) -c $< -o $@ $(CFLAGS)  
$(CPPOBJS) : %.o: %.cc  
	$(CPP) -c $< -o $@ $(CPPFLAGS)  


# $@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。每次$< $@ 代表的值就是列表中的  
#  
$(TARGET) : $(COBJS) #$(CPPOBJS)
	ar rcs $(TARGET) $^
	rm $(COBJS)
#	rm $(CPPOBJS)
#	rm $(CDEF)  
#	rm $(CPPDEF)  
# 链接为最终目标  
  
  
#引入了.o文件对.c和.h的依赖情况。以后.h被修改也会重新生成，可看看.d文件内容即知道为何  
#引入了依赖就相当于引入了一系列的规则，因为依赖内容例如： 目录/xxx.o:目录/xxx.c 目录/xxx.h 也相当于隐含的引入了生成规则  
#故上面不能在出现如： $(OBJS) : $(DEF)之类。切记  
#include $(CDEF)  
#include $(CPPDEF)  
.PHONY:clean cleanall  
  
#清除所有目标文件以及生成的最终目标文件  
clean:              
	#rm $(TARGET) $(COBJS) $(CPPOBJS)
	rm $(TARGET) $(COBJS) $(DLL)
#rm *.d  
cleanall:  
	#rm $(CDEF) $(CPPDEF)  
	#rm  $(COBJS) $(CPPOBJS)
	rm  $(COBJS)
	rm $(TARGET)
	rm $(DLL)
