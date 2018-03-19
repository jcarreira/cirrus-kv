#ifndef EXAMPLES_ML_CONFIGURATION_H_
#define EXAMPLES_ML_CONFIGURATION_H_

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
      * Get size of each object in S3
      * @returns s3 object size
      */
    uint64_t get_s3_size() const;

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
        SOFTMAX,
        COLLABORATIVE_FILTERING,
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
      * Get max number of training samples
      */
    uint64_t get_limit_samples() const;

    /**
      * Get max number of training samples
      */
    uint64_t get_num_features() const;

    std::string get_s3_bucket() const;

    void check() const;

    std::pair<int, int> get_train_range() const;
    std::pair<int, int> get_test_range() const;

    bool get_use_bias() const;

    /**
      * Netflix specific
      */
    int get_users() const;
    int get_items() const;

 public:
    /**
      * Parse a specific line in the config file
      * @param line Configuration line
      */
    void parse_line(const std::string& line);

    uint64_t n = 0;          //< number of samples
    uint64_t d = 0;          //< number of sample features
    uint64_t n_workers = 0;  //< number of system workers

    uint64_t minibatch_size = 0;  //< size of minibatch
    uint64_t s3_size = 0;  //< size of samples chunk stored in each s3 object

    double learning_rate = 0;     //< sgd learning rate
    double epsilon = 0;           //< regularization rate

    int prefetching = 0;       //< whether to prefetch input data
    uint64_t num_classes = 0;  //< number of sample classes

    std::string input_path;  //< path to dataset input
    std::string input_type;  //< dataset input format

    std::string samples_path;  //< path to dataset samples
    std::string labels_path;   //< path to dataset labels

    Configuration::ModelType model_type = UNKNOWN;  //< type of the model

    // max number of columns to read from dataset input
    uint64_t limit_cols = 0;
    bool normalize = false;    //< whether to normalize the dataset

    uint64_t limit_samples = 0;  //< max number of training input samples
    uint64_t num_features = 0;   //< number of features in each sample

    std::string s3_bucket_name; //< bucket used for training dataset

    std::pair<int, int> train_set_range;
    std::pair<int, int> test_set_range;

    bool use_bias = false;

    int nusers = 0;
    int nitems = 0;
};

#endif  // EXAMPLES_ML_CONFIGURATION_H_
