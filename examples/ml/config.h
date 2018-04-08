#define USE_REDIS_CHANNEL

// #define USE_CIRRUS
#define DATASET_IN_S3
#define USE_REDIS
// #define USE_PREFETCH
// #define USE_S3

// #define PRELOAD_DATA

#define LOADING_TASK_RANK -100000
#define PERFORMANCE_LAMBDA_RANK (-100000)

#define WORKER_SPARSE_TASK_RANK (3)
#define ERROR_SPARSE_TASK_RANK (2)
#define PS_SPARSE_SERVER_TASK_RANK (1)
#define LOADING_SPARSE_TASK_RANK (0)
#define WORKERS_BASE 3 // used in wait_for_start


// parameter server params here
#define NUM_PS 2
#define PS_PORTS_LST 1337, 1338
#define PS_IPS_LST "127.0.0.1", "127.0.0.1"

// For Netflix's stuff
# define PS_IP "127.0.0.1"
# define PS_PORT 1337

using FEATURE_TYPE = float;

//#define CRITEO_HASH_BITS 19
#define RCV1_HASH_BITS 19

#define LIMIT_NUMBER_PASSES 3

#define SYNC_N_STEPS (100)

// number of factors for neflix workload
#define NUM_FACTORS 10
