/**
  * This executable loads data into Cirrus
  */

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <signal.h>
#include <ctype.h>
#include <algorithm>
#include <sys/time.h>

#include "read_mnist.h"

#include "object_store/FullBladeObjectStore.h"
#include "client/TCPClient.h"
#include "Serializers.h"

#define IP "10.10.49.94"
#define PORT "12345"

#define IMAGE_BASE (0)
#define LABEL_BASE (100000000UL)

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_sec = (tv.tv_sec) + ((double) tv.tv_usec * (double) 10e-7);
    return time_in_sec;
}

void shuffle_vectors(auto train_images, auto train_labels) {
    std::srand(42);
    std::random_shuffle(train_images.begin(), train_images.end());
    std::srand(42);
    std::random_shuffle(train_labels.begin(), train_labels.end());
}

int BATCH_SIZE = 32;

int main() {
    std::srand(1);

    std::vector<cv::Mat> train_images;
    std::vector<double> train_labels;

    image_vector_serializer iser;
    image_vector_deserializer ideser;
    c_array_serializer<double> lser(BATCH_SIZE);
    c_array_deserializer<double> ldeser(BATCH_SIZE);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<image_vector_ptr>
        image_store(IP, PORT, &client, iser, ideser);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        label_store(IP, PORT, &client, lser, ldeser);

    fprintf(stderr, "Loading data...\n");

    MNIST_read_images("train-images-idx3-ubyte", train_images);
    MNIST_read_labels("train-labels-idx1-ubyte", train_labels);

    shuffle_vectors(train_images, train_labels);

    std::cout << "Images size: " << train_images.size() << std::endl;
    std::cout << "Labels size: " << train_labels.size() << std::endl;

    uint64_t l = 0;
    for (int i = 0; i < train_images.size() / BATCH_SIZE; ++i) {
        std::vector<cv::Mat> image_copy = std::vector<cv::Mat>(
                train_images.begin() + l,
                train_images.begin() + l + BATCH_SIZE);
        auto p_img = std::make_shared<std::vector<cv::Mat>>(image_copy);
       
        std::vector<double> label_copy = std::vector<double>(
                train_labels.begin() + l,
                train_labels.begin() + l + BATCH_SIZE);
        uint64_t labels_raw_size = BATCH_SIZE * sizeof(double); 
        std::shared_ptr<double> p_label(
                new double[labels_raw_size],
                array_deleter<double>());
        memcpy(p_label.get(), train_labels.data() + l, labels_raw_size);

        std::cout << "Storing image vector in index: " << i << std::endl;
        image_store.put(IMAGE_BASE + i, p_img);
        std::cout << "Storing label vector in index: " << i << std::endl;
        label_store.put(LABEL_BASE + i, p_label);

        l += BATCH_SIZE;
    }

    return 0;
}

