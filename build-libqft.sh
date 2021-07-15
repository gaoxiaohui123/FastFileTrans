#!/bin/sh

echo 'input a number'
read Num
###############################
case $Num in
1)
  echo "build for linux"
  export LD_LIBRARY_PATH=`pwd`
  gcc -o ./libqft.so -shared -fPIC  -Wformat=0 -Wl,-Bsymbolic -fvisibility=hidden -ldl -Wl,-rpath=. \
  -Xlinker --unresolved-symbols=ignore-in-shared-libs \
  -Wno-deprecated-declarations \
  -Wno-incompatible-pointer-types \
  -Wno-implicit-function-declaration \
  -Wno-int-conversion \
  -Wno-pointer-to-int-cast \
  -Wno-discarded-qualifiers \
  ./source/file_rtp.c \
  ./source/file_raw2pkt.c \
  ./source/file_pkt2raw.c \
  ./source/base_stun.c \
  ./source/stun_list.c \
  -I./include/ \
  -L./ \
  -L/usr/local/lib \
  -L/usr/lib \
  -lpthread -ldl \
  -lz -lstdc++ -lm
  #-lbz2 -llzma
  #-lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp -lX11 -lXext -lXv \
  #-lasound  -lva -lva-drm -lva-x11 -lvdpau
;;
2)
  echo "build for macosx"
  eexport DYLD_LIBRARY_PATH=`pwd`

  gcc -dynamiclib -o .libqft.dylib \
  -Wformat=0 -fvisibility=hidden -ldl \
  -Wl,-rpath,./ \
  -Xlinker \
  -w \
  -Wno-deprecated-declarations \
  -Wno-incompatible-pointer-types \
  -Wno-implicit-function-declaration \
  -Wno-int-conversion \
  -Wno-pointer-to-int-cast \
  -Wno-ignored-qualifiers \
  -Wno-unused-value \
  -Wno-pointer-sign \
  -Wno-absolute-value \
  -Wno-parentheses-equality \
  -Wno-logical-not-parentheses \
  -Wno-pointer-bool-conversion \
  -Wno-return-type \
  ./source/file_rtp.c \
  ./source/file_raw2pkt.c \
  ./source/file_pkt2raw.c \
  ./source/base_stun.c \
  ./source/stun_list.c \
  -I./include/ \
  -L./ \
  -L/usr/local/lib \
  -L/usr/lib \
  -lpthread -ldl \
  -lz -lstdc++ -lm
;;
3)

;;
4)

;;
5)

;;

esac