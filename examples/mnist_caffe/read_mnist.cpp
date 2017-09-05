/**
  * Some code from https://github.com/filipemtz/caffe_examples
  */

#include "read_mnist.h"

#include <math.h>
#include <iostream>
#include <fstream>

using namespace std;

int MNIST_reverse_int(int i) {
    unsigned char ch1, ch2, ch3, ch4;

    ch1 = i & 255;
    ch2 = (i >> 8) & 255;
    ch3 = (i >> 16) & 255;
    ch4 = (i >> 24) & 255;

    return((int) ch1 << 24) + ((int)ch2 << 16) + ((int)ch3 << 8) + ch4;
}


void MNIST_read_images(string filename, vector<cv::Mat> &vec) {
    vec.clear();
    ifstream file (filename.c_str(), ios::binary);

    if (file.is_open()) {
        int magic_number = 0;
        int number_of_images = 0;
        int n_rows = 0;
        int n_cols = 0;

        file.read((char*) &magic_number, sizeof(magic_number));
        magic_number = MNIST_reverse_int(magic_number);

        file.read((char*) &number_of_images,sizeof(number_of_images));
        number_of_images = MNIST_reverse_int(number_of_images);

        file.read((char*) &n_rows, sizeof(n_rows));
        n_rows = MNIST_reverse_int(n_rows);

        file.read((char*) &n_cols, sizeof(n_cols));
        n_cols = MNIST_reverse_int(n_cols);

        for(int i = 0; i < number_of_images; ++i) {
            cv::Mat tp = Mat::zeros(n_rows, n_cols, CV_8UC1);

            for(int r = 0; r < n_rows; ++r) {
                for(int c = 0; c < n_cols; ++c) {
                    unsigned char temp = 0;
                    file.read((char*) &temp, sizeof(temp));
                    tp.at<uchar>(r, c) = (int) temp;
                }
            }

            vec.push_back(tp);
        }
    }
}


void MNIST_read_labels(string filename, vector<double> &vec) {
    vec.clear();
    ifstream file (filename.c_str(), ios::binary);

    if (file.is_open()) {
        int magic_number = 0;
        int number_of_images = 0;

        file.read((char*) &magic_number, sizeof(magic_number));
        magic_number = MNIST_reverse_int(magic_number);

        file.read((char*) &number_of_images,sizeof(number_of_images));
        number_of_images = MNIST_reverse_int(number_of_images);

        for(int i = 0; i < number_of_images; ++i) {
            unsigned char temp = 0;
            file.read((char*) &temp, sizeof(temp));
            vec.push_back((double) temp);
        }
    }
}
