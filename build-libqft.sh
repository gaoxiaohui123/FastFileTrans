#!/bin/sh

echo 'input a number'
read Num
###############################
case $Num in
1)
  echo "build for linux"
  export LD_LIBRARY_PATH=$LIBVA_DRIVERS_PATH
  gcc -o ./libqft.so -shared -fPIC  -Wformat=0 -Wl,-Bsymbolic -fvisibility=hidden -ldl -Wl,-rpath=. \
  -Xlinker --unresolved-symbols=ignore-in-shared-libs \
  -Wno-deprecated-declarations \
  -Wno-incompatible-pointer-types \
  -Wno-implicit-function-declaration \
  -Wno-int-conversion \
  -Wno-pointer-to-int-cast \
  -Wno-discarded-qualifiers \
  ./source/file_rtp.c \
  ./source/file_list.c \
  ./source/file_frame.c \
  ./source/file_pic.c \
  ./source/file_group.c \
  ./source/file_raw2pkt.c \
  ./source/file_pkt2raw.c \
  -I./include/ \
  -L./ \
  -L/usr/local/lib \
  -L/usr/lib \
  -lpthread -ldl \
  -lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp -lX11 -lXext -lXv \
  -lasound -lz -lstdc++ -lm -lbz2 -lva -lva-drm -lva-x11 -lvdpau -llzma
;;
2)

;;
3)

;;
4)

;;
5)

;;

esac