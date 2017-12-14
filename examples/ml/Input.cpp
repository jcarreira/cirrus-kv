/**
  * The Input class is used to aggregate all the routines to read datasets
  * We support datasets in CSV and binary
  * We try to make the process of reading data as efficient as possible
  * Note: this code is still pretty slow reading datasets
  */

#include <examples/ml/Input.h>
#include <Utils.h>

#include <string>
#include <vector>
#include <thread>
#include <cassert>
#include <memory>
#include <algorithm>

static const int REPORT_LINES = 10000;  // how often to report readin progress
static const int REPORT_THREAD = 100000;  // how often proc. threads report
static const int STR_SIZE = 10000;        // max size for dataset line

Dataset Input::read_input_criteo(const std::string& samples_input_file,
        const std::string& labels_input_file) {
    uint64_t samples_file_size = filesize(samples_input_file);
    uint64_t labels_file_size = filesize(samples_input_file);

    std::cout
        << "Reading samples input file: " << samples_input_file
        << " labels input file: " << labels_input_file
        << " samples file size(MB): " << (samples_file_size / 1024.0 / 1024)
        << std::endl;

    // SPECIFIC of criteo dataset
    uint64_t n_cols = 13;
    uint64_t samples_entries = samples_file_size / (sizeof(double) * n_cols);
    uint64_t labels_entries = labels_file_size / (sizeof(double) * n_cols);

    if (samples_entries != labels_entries) {
        puts("Number of sample / labels entries do not match");
        exit(-1);
    }

    std::cout << "Reading " << samples_entries << " entries.." << std::endl;

    double* samples = new double[samples_entries * n_cols];
    double* labels  = new double[samples_entries];

    FILE* samples_file = fopen(samples_input_file.c_str(), "r");
    FILE* labels_file = fopen(labels_input_file.c_str(), "r");

    if (!samples_file || !labels_file) {
        throw std::runtime_error("Error opening input files");
    }

    std::cout
        << " Reading " << sizeof(double) * n_cols
        << " bytes"
        << std::endl;

    uint64_t ret = fread(samples, sizeof(double) * n_cols, samples_entries,
            samples_file);
    if (ret != samples_entries) {
        throw std::runtime_error("Did not read enough data");
    }

    ret = fread(labels, sizeof(double), samples_entries, labels_file);
    if (ret != samples_entries) {
        throw std::runtime_error("Did not read enough data");
    }

    fclose(samples_file);
    fclose(labels_file);

    // we transfer ownership of the samples and labels here
    Dataset ds(samples, labels, samples_entries, n_cols);

    delete[] samples;

    return ds;
}

void Input::read_csv_thread(std::mutex& input_mutex, std::mutex& output_mutex,
        const std::string& delimiter,
        std::queue<std::string>& lines,  //< content produced by producer
        std::vector<std::vector<double>>& samples,
        std::vector<double>& labels,
        bool& terminate,
        uint64_t limit_cols) {
    uint64_t count_read = 0;
    uint64_t read_at_a_time = 1000;

    while (1) {
        if (terminate)
            break;

        std::vector<std::string> thread_lines;

        // Read up to read_at_a_time limes
        std::cout << "Popping lines size: " << lines.size() << std::endl;
        input_mutex.lock();
        while (lines.size() && thread_lines.size() < read_at_a_time) {
            thread_lines.push_back(lines.front());
            lines.pop();
        }

        if (thread_lines.size() == 0) {
            input_mutex.unlock();
            continue;
        }

        input_mutex.unlock();

        std::vector<std::vector<double>> thread_samples;
        std::vector<double> thread_labels;

        char str[STR_SIZE];

        while (!thread_lines.empty()) {
            std::string line = thread_lines.back();
            thread_lines.pop_back();
            /*
             * We have the line, now split it into features
             */ 
            assert(line.size() < STR_SIZE);
            strncpy(str, line.c_str(), STR_SIZE);
            char* s = str;

            uint64_t k = 0;
            std::vector<double> sample;
            while (char* l = strsep(&s, delimiter.c_str())) {
                double v = string_to<double>(l);
                sample.push_back(v);
                k++;
                if (limit_cols && k == limit_cols)
                    break;
            }

            // we assume first column is label
            double label = sample.front();
            sample.erase(sample.begin());

            thread_labels.push_back(label);
            thread_samples.push_back(sample);
        }

        output_mutex.lock();
        while (thread_samples.size()) {
            samples.push_back(thread_samples.back());
            labels.push_back(thread_labels.back());
            thread_samples.pop_back();
            thread_labels.pop_back();
        }
        output_mutex.unlock();

        if (count_read % REPORT_THREAD == 0) {
            std::cout
                << "Thread processed line: " << count_read
                << std::endl;
        }
        count_read += read_at_a_time;
    }
}

void Input::print_sample(const std::vector<double>& sample) const {
    for (const auto& v : sample) {
        std::cout << " " << v;
    }
    std::cout << std::endl;
}

std::vector<std::vector<double>> Input::read_mnist_csv(
        const std::string& input_file,
        std::string delimiter) {
    FILE* fin = fopen(input_file.c_str(), "r");
    if (!fin) {
        throw std::runtime_error("Can't open file: " + input_file);
    }

    std::vector<std::vector<double>> samples;

    std::string line;
    char str[STR_SIZE + 1] = {0};
    while (fgets(str, 1000000, fin) != NULL) {
        char* s = str;

        std::vector<double> sample;
        while (char* l = strsep(&s, delimiter.c_str())) {
            double v = string_to<double>(l);
            sample.push_back(v);
        }

        samples.push_back(sample);
    }

    return samples;
}

void Input::split_data_labels(const std::vector<std::vector<double>>& input,
        unsigned int label_col,
        std::vector<std::vector<double>>& training_data,
        std::vector<double>& labels
        ) {
    if (input.size() == 0) {
        throw std::runtime_error("Error: Input data has 0 columns");
    }

    if (input[0].size() < label_col) {
      throw std::runtime_error("Error: label column is too big");
    }

    // for every sample split it into labels and training data
    for (unsigned int i = 0; i < input.size(); ++i) {
      labels.push_back(input[i][label_col]);  // get label

      std::vector<double> left, right;
      // get all data before label
      left = std::vector<double>(input[i].begin(),
          input[i].begin() + label_col);
      // get all data after label
      right = std::vector<double>(input[i].begin() + label_col + 1,
          input[i].end());

      left.insert(left.end(), right.begin(), right.end());
      training_data.push_back(left);
    }
}

void shuffle_samples_labels(auto& samples, auto& labels) {
  std::srand(42);
  std::random_shuffle(samples.begin(), samples.end());
  std::srand(42);
  std::random_shuffle(labels.begin(), labels.end());
}

Dataset Input::read_input_csv(const std::string& input_file,
        std::string delimiter, uint64_t nthreads,
        uint64_t limit_lines, uint64_t limit_cols,
        bool to_normalize) {
    std::cout << "Reading input file: " << input_file << std::endl;

    std::ifstream fin(input_file, std::ifstream::in);
    if (!fin) {
        throw std::runtime_error("Error opening input file");
    }

    std::vector<std::vector<double>> samples; // final result
    std::vector<double> labels; // final result
    std::queue<std::string> lines[nthreads]; // input to threads

    std::mutex input_mutex[nthreads];   // mutex to protect queue of raw samples
    std::mutex output_mutex;  // mutex to protect queue of processed samples
    bool terminate = false;   // indicates when worker threads should terminate
    std::vector<std::shared_ptr<std::thread>> threads;  // vector of worker threads

    for (uint64_t i = 0; i < nthreads; ++i) {
        threads.push_back(
                std::make_shared<std::thread>(
                        /**
                          * We could also declare read_csv_thread static and
                          * avoid this ugliness
                          */
                        std::bind(&Input::read_csv_thread, this,
                            std::placeholders::_1, std::placeholders::_2,
                            std::placeholders::_3, std::placeholders::_4,
                            std::placeholders::_5, std::placeholders::_6,
                            std::placeholders::_7, std::placeholders::_8),
                    std::ref(input_mutex[i]), std::ref(output_mutex),
                    delimiter, std::ref(lines[i]), std::ref(samples),
                    std::ref(labels), std::ref(terminate),
                    limit_cols));
    }

    const int batch_size = 100; // we push things into shared queue in batches
    std::vector<std::string> input;
    input.reserve(batch_size);
    uint64_t lines_count = 0;
    uint64_t thread_index = 0; // we push input to threads in round robin
    while (1) {
       int i;
       for (i = 0; i < batch_size; ++i, lines_count++) {
          std::string line;
          if (!getline(fin, line))
             break;
          // enforce max number of lines read
          if (lines_count && lines_count >= limit_lines)
            break;
          input[i] = line;
       }
        if (i != batch_size)
           break;

        input_mutex[thread_index].lock();
        for (int j = 0; j < i; ++j) {
           lines[thread_index].push(input[j]);
        }
        input_mutex[thread_index].unlock();

        if (lines_count % REPORT_LINES == 0)
            std::cout << "Read: " << lines_count << " lines." << std::endl;
        thread_index = (thread_index + 1) % nthreads;
    }

    while (1) {
       usleep(100000);
       for (thread_index = 0; thread_index < nthreads; ++thread_index) {
           input_mutex[thread_index].lock();
           // check if a thread is still working
           if (!lines[thread_index].empty()) {
               input_mutex[thread_index].unlock();
              break;
           }
           input_mutex[thread_index].unlock();
       }
       if (thread_index == nthreads)
          break;
    }

    terminate = true;
    for (auto thread : threads)
        thread->join();

    assert(samples.size() == labels.size());

    std::cout << "Read " << samples.size() << " samples" << std::endl;
    std::cout << "Printing first sample" << std::endl;
    print_sample(samples[0]);

    if (to_normalize)
        normalize(samples);

    shuffle_samples_labels(samples, labels);

    std::cout << "Printing first sample after normalization" << std::endl;
    print_sample(samples[0]);
    return Dataset(samples, labels);
}

void Input::normalize(std::vector<std::vector<double>>& data) {
    std::vector<double> means(data[0].size());
    std::vector<double> sds(data[0].size());

    // calculate mean of each feature
    for (unsigned int i = 0; i < data.size(); ++i) {  // for each sample
        for (unsigned int j = 0; j < data[0].size(); ++j) {  // for each feature
            means[j] += data[i][j] / data.size();
        }
    }

    // calculate standard deviations
    for (unsigned int i = 0; i < data.size(); ++i) {
        for (unsigned int j = 0; j < data[0].size(); ++j) {
            sds[j] += std::pow(data[i][j] - means[j], 2);
        }
    }
    for (unsigned int j = 0; j < data[0].size(); ++j) {
        sds[j] = std::sqrt(sds[j] / data.size());
    }

    for (unsigned i = 0; i < data.size(); ++i) {
        for (unsigned int j = 0; j < data[0].size(); ++j) {
            if (means[j] != 0) {
                data[i][j] = (data[i][j] - means[j]) / sds[j];
            }
            if (std::isnan(data[i][j]) || std::isinf(data[i][j])) {
                std::cout << data[i][j] << " " << means[j]
                          << " " << sds[j]
                          << std::endl;
                throw std::runtime_error(
                        "Value is not valid after normalization");
            }
        }
    }
}

