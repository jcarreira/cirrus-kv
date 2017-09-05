#ifndef __READ_MNIST_H__
#define __READ_MNIST_H__

#include <string>
#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;
using namespace std;

void MNIST_read_images(string filename, vector<Mat> &vec);
void MNIST_read_labels(string filename, vector<double> &vec);

#endif
