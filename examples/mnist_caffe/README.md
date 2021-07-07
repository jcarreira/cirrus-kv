# image_training_caffe

To be run in f12

[joao@f12:/data/joao/image_classification_caffe]% make
g++ -o train train.cpp read_mnist.cpp -O3 -Wall -Wno-sign-compare -Wno-write-strings -Wno-unused-result -std=c++11  -I /data/joao/caffe/include -I /data/joao/caffe/build/src/ -I /usr/local/cuda/include `pkg-config --cflags opencv` -L /data/joao/caffe/lib -lcaffe -L /usr/local/cuda/lib64 `pkg-config --libs opencv` -lglog -lboost_system
[joao@f12:/data/joao/image_classification_caffe]% 

