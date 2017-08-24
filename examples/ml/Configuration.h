#ifndef SRC_CONFIGURATION_H_
#define SRC_CONFIGURATION_H_

#include <string>

class Configuration {
 public:
    Configuration();

    /**
      * Read configuration file
      * @param path Path to config file
      */
    void read(const std::string& path);

    /**
      * Get learning rate used for SGD
      * @returns Learning rate
      */
    double get_learning_rate() const;

    /**
      * Get regularization rate
      * @returns Regularization rate
      */
    double get_epsilon() const;

    /**
      * Get size of each minibatch
      * @returns minibatch size
      */
    uint64_t get_minibatch_size() const;

    /**
      * Get flag indicating whether to use prefetching
      * @returns Prefetching flag
      */
    int get_prefetching() const;

    /**
      * Get number of classes in the dataset
      * @returns Number of sample classes
      */
    uint64_t get_num_classes() const;

    /**
      * Get path to the input file
      */
    std::string get_input_path() const;

    /**
      * Get maximum value of features used by the system
      * When set, the system only reads the first X features from the input datasets
      * @params returns the max number of features to use
      */
    uint64_t get_limit_cols() const;

    /**
      * Get the path to the file with the samples data
      */
    std::string get_samples_path() const;

    /**
      * Get the path to the file with the labels data
      */
    std::string get_labels_path() const;

    /**
      * Get the type of input file used
      */
    std::string get_input_type() const;

    /**
      * print the configuration parameters
      */
    void print() const;

    enum ModelType {
        UNKNOWN = 0,
        LOGISTICREGRESSION,
        SOFTMAX
    };

    /**
      * Get the type of model used by the system
      */
    ModelType get_model_type() const;

    /**
      * Return flag indicating whether to normalize the dataset
      */
    bool get_normalize() const;

    /**
      * Get number of training samples
      */
    uint64_t get_num_samples() const;

 private:
    /**
      * Parse a specific line in the config file
      * @param line Configuration line
      */
    void parse_line(const std::string& line);

    uint64_t n;          //< number of samples
    uint64_t d;          //< number of sample features
    uint64_t n_workers;  //< number of system workers

    uint64_t minibatch_size;  //< size of minibatch
    double learning_rate;     //< sgd learning rate
    double epsilon;           //< regularization rate

    int prefetching;       //< whether to prefetch input data
    uint64_t num_classes;  //< number of sample classes

    std::string input_path;  //< path to dataset input
    std::string input_type;  //< dataset input format

    std::string samples_path;  //< path to dataset samples
    std::string labels_path;   //< path to dataset labels

    Configuration::ModelType model_type;  //< type of the model

    uint64_t limit_cols;  //< max number of columns to read from dataset input
    bool normalize;       //< whether to normalize the dataset

    uint64_t num_samples;  //< number of training input samples
};

#endif  // SRC_CONFIGURATION_H_
