/* Copyright Joao Carreira 2017 */

#ifndef SRC_INPUT_H_
#define SRC_INPUT_H_

#include <Dataset.h>
#include <string>
#include <vector>
#include <mutex>

class Input {
 public:
    /**
      * Reads criteo dataset in binary format
      * @param samples_input_file Path to file that contains input samples
      * @param labels_input_file Path to file containing labels
      */
    Dataset read_input_criteo(const std::string& samples_input_file,
            const std::string& labels_input_file);

    /**
      * Read dataset in csv file with given delimiter (e.g., tab, space)
      * and specific number of threads
      * We assume labels are the first feature in each line
      * @param input_file Path to csv file with dataset
      * @param delimiter Delimiter string between every feature in the csv file
      * @param nthreads Number of threads to read the csv file
      * @param limit_cols Maximum number of columns/features to read from file
      * @returns The dataset
      */
    Dataset read_input_csv(const std::string& input_file,
            std::string delimiter, uint64_t nthreads, uint64_t limit_cols = 0,
            bool to_normalize = false);

    /**
      * Read a csv file with the mnist dataset
      * Does not split between samples and labels (see split_data_labels)
      * @param input_file Path to input csv dataset file
      * @param delimiter Delimiter string in the csv
      * @returns the mnist data
      */
    std::vector<std::vector<double>> read_mnist_csv(
            const std::string& input_file,
            std::string delimiter);

    /**
      * Splits dataset vector of doubles into features and labels
      * @param input Dataset in std::vector<std::vector<double>> format
      * @param label_col Column number where labels are
      * @param training_data Output for samples/features
      * @param labels Output for labels data
      */
    void split_data_labels(const std::vector<std::vector<double>>& input,
            unsigned int label_col,
            std::vector<std::vector<double>>& training_data,
            std::vector<double>& labels);

    /**
      * Normalizes a dataset in the std::vector<std::vector<double>>
      * format in place
      * @param data Dataset
      */
    void normalize(std::vector<std::vector<double>>& data);

 private:
    /**
      * Thread worker that reads raw lines of text from queue and appends labels and features
      * into formated samples queue
      * @param input_mutex Mutex used to synchronize access to input data
      * @param output_mutex Mutex used to synch access to output store
      * @param delimiter Delimiter between input data entries
      * @param lines Input lines
      * @param samples Output store for samples
      * @param labels Output store for labels
      * @param terminate Indicate whether threads should terminate
      * @param limit_cols Maximum number of columns/features to read from file
      */
    void read_csv_thread(std::mutex& input_mutex, std::mutex& output_mutex,
            const std::string& delimiter,
            std::vector<std::string>& lines,  //< content produced by producer
            std::vector<std::vector<double>>& samples,
            std::vector<double>& labels,
            bool& terminate,
            uint64_t limit_cols = 0);

    /**
      * Prints a single sample
      * @param sample Sample to be printed
      */
    void print_sample(const std::vector<double>& sample) const;
};

#endif  // SRC_INPUT_H_
