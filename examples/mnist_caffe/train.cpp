#include <cstdio>
#include <cstdlib>
#include <vector>
#include <signal.h>
#include <ctype.h>
#include <algorithm>
#include <opencv/cv.h>

#include <caffe/caffe.hpp>
#include <caffe/solver.hpp>
#include <caffe/solver_factory.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/util/io.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/layers/sigmoid_cross_entropy_loss_layer.hpp>
#include <caffe/layers/euclidean_loss_layer.hpp>
#include <caffe/layers/inner_product_layer.hpp>

#include "object_store/FullBladeObjectStore.h"
#include "client/TCPClient.h"
#include "Serializers.h"

#define IP "10.10.49.94"
#define PORT "12345"
#define IMAGE_BASE (0)
#define LABEL_BASE (100000000UL)
    
int BATCH_SIZE = 32;

using cv::Mat;

int requested_to_exit = 0;

using namespace std;
using namespace caffe;

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t time_ns = ts.tv_sec*1000000000UL + ts.tv_nsec;
    return time_ns;
}

vector<caffe::Datum> build_datum_vector(vector<Mat> train_images,
        vector<double> train_labels, int batch_size) {
    Mat m;
    double label;
    vector<caffe::Datum> datums;

    for (int i = 0; i < batch_size; i++) {
        caffe::Datum d;

        //FIXME use operator[]
        m = train_images.at(i);
        label = train_labels.at(i);

        d.set_channels(m.channels());
        d.set_height(m.rows);
        d.set_width(m.cols);
        d.set_label(label);

        for (int j = 0; j < m.rows * m.cols * m.channels(); j++)
            d.add_float_data(m.data[j]);

        datums.push_back(d);
    }

    return datums;
}

void shutdown(int sign) {
    if (sign == SIGINT) {
        printf("Exit requested\n");
        requested_to_exit = 1;
    }
}

void build_train_and_validation_indices(vector<int> &train_set_indices,
        vector<int> &validation_set_indices, int num_total_images,
        int num_images_to_train, int num_images_to_validate) {
    int i;
    vector<int> all_images;

    for (i = 0; i < num_total_images; i++)
        all_images.push_back(i);

    // XXX we disable shuffle and instead do it in the loader
    //random_shuffle(all_images.begin(), all_images.end());

    for (i = 0; i < num_images_to_train; i++)
        train_set_indices.push_back(all_images[i]);

    for (i = 0; i < num_images_to_validate; i++)
        validation_set_indices.push_back(all_images[i + num_images_to_train]);
}

int main() {
    srand(1);

    //vector<Mat> train_images;
    //vector<double> train_labels;

    Caffe::set_mode(Caffe::GPU);
   
    image_vector_serializer iser;
    image_vector_deserializer ideser;
    c_array_serializer<double> lser(BATCH_SIZE);
    c_array_deserializer<double> ldeser(BATCH_SIZE);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<image_vector_ptr>
        image_store(IP, PORT, &client, iser, ideser);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        label_store(IP, PORT, &client, lser, ldeser);

    //fprintf(stderr, "Loading data...\n");

    ////MNIST_read_images("train-images-idx3-ubyte", train_images);
    ////MNIST_read_labels("train-labels-idx1-ubyte", train_labels);

    //fprintf(stderr, "Done\n");

    //FIXME Harcoded number of samples
    int NUM_IMAGES = 60000;
    vector<int> train_set_indices, validation_set_indices;
    build_train_and_validation_indices(train_set_indices,
            validation_set_indices, NUM_IMAGES, 10000, 1000);
            //validation_set_indices, train_images.size(), 10000, 1000);

    boost::shared_ptr<caffe::Solver<float>> solver;
    boost::shared_ptr<caffe::Net<float>> net;

    caffe::SolverParameter solver_param;

    caffe::ReadProtoFromTextFileOrDie("solver.prototxt", &solver_param);
    solver.reset(caffe::SolverRegistry<float>::CreateSolver(solver_param));

    net = solver->net();

    boost::shared_ptr<caffe::MemoryDataLayer<float>> input =
        boost::dynamic_pointer_cast<caffe::MemoryDataLayer<float>>(
                net->layer_by_name("input"));

    int epoch = 1;

    int sample, k, l;
    int experiment_precision;
    double y, yprob, net_output;
    float loss;

    int NUM_EPOCHS = 20000;
    int TEST_FREQUENCY = 5;
    int SHOW_OUTPUT_OF_SAMPLES = 0;

    signal(SIGINT, shutdown);

    caffe::shared_ptr<caffe::Blob<float>> output_blob = net->blob_by_name("fc3_softmax");

    for (epoch = 0; epoch < NUM_EPOCHS; epoch++) {
        experiment_precision = 0;

        for (sample = 0; sample < NUM_IMAGES / BATCH_SIZE; ++sample) {
            if (requested_to_exit) {
                goto exit;
            }

            std::cout << "Training batch #" << sample << std::endl;

            /**
              * We the the batch sample and label here
              */

            auto now = get_time_ns();
            std::cout << "Getting sample: " << sample << std::endl;
            auto train_images = *image_store.get(IMAGE_BASE + sample);
            std::cout << "Getting label: " << sample << std::endl;
            auto labels = label_store.get(LABEL_BASE + sample);
            auto train_labels = std::vector<double>(
                    labels.get(), labels.get() + BATCH_SIZE);
            auto elapsed_get_ns = get_time_ns() - now;

            //std::cout << "Got train images with size: " << train_images.size()
            //    << std::endl;
            //std::cout << "Got train labels with size: " << train_labels.size()
            //    << std::endl;
            std::cout << "Get elapsed (ns): " << elapsed_get_ns
                << std::endl;

            //vector<int> batch_samples(train_set_indices.begin() + sample,
            //        train_set_indices.begin() + sample + BATCH_SIZE);
            vector<caffe::Datum> datum = build_datum_vector(train_images,
                    train_labels, BATCH_SIZE);

            now = get_time_ns();
            input->AddDatumVector(datum);
            net->Forward(&loss);
            auto elapsed_train_ns = get_time_ns() - now;
            std::cout << "Train elapsed (ns): " << elapsed_train_ns
                << std::endl;

            for (k = 0; k < BATCH_SIZE; k++) {
                y = 0;
                yprob = -DBL_MAX;

                // get the class with max prob
                for (l = 0; l < output_blob->channels(); l++) {
                    net_output = output_blob->cpu_data()[
                        k * output_blob->channels() + l];

                    if (yprob < net_output) {
                        y = l;
                        yprob = net_output;
                    }
                }

                if (y == train_labels[k]) {
                    experiment_precision++;
                }

                if (SHOW_OUTPUT_OF_SAMPLES) {
                    printf("TRAIN EPOCH %d SAMPLE %d DES: %.4f EST: %.4f\n",
                            epoch, sample, train_labels[k], y);
                }
            }

            solver->Step(1);
        }

        printf("REPORT TRAIN EPOCH %d precision: %.2lf\n",
                epoch, 100.0 * ((double) experiment_precision
                    / (double) NUM_IMAGES));

#if 0
        if (epoch % TEST_FREQUENCY == 0) {
            experiment_precision = 0;

            for (sample = 0;
                    sample < validation_set_indices.size() - BATCH_SIZE;
                    sample += BATCH_SIZE) {
                if (requested_to_exit) {
                    goto exit;
                }

                vector<int> batch_samples(
                        validation_set_indices.begin() + sample,
                        validation_set_indices.begin() + sample + BATCH_SIZE);
                vector<caffe::Datum> datum = build_datum_vector(
                        train_images, train_labels, batch_samples);

                input->AddDatumVector(datum);
                net->Forward(&loss);

                for (k = 0; k < BATCH_SIZE; k++) {
                    y = 0;
                    yprob = -DBL_MAX;

                    // get the class with max prob
                    for (l = 0; l < output_blob->channels(); l++) {
                        net_output = output_blob->cpu_data()[
                            k * output_blob->channels() + l];

                        if (yprob < net_output) {
                            y = l;
                            yprob = net_output;
                        }
                    }

                    if (y == train_labels[batch_samples[k]]) {
                        experiment_precision++;
                    }

                    if (SHOW_OUTPUT_OF_SAMPLES) {
                        printf("TEST EPOCH %d SAMPLE %d DES: %.4f EST: %.4f <=\n",
                                epoch, sample,
                                train_labels[batch_samples[k]], y
                              );
                    }

                    fflush(stdout);
                }
            }

            printf("REPORT TEST EPOCH %d precision: %.2lf <=\n",
                    epoch, 100.0 * ((double) experiment_precision
                        / (double) validation_set_indices.size()));

        }
#endif
    }

exit:
    return 0;
}

