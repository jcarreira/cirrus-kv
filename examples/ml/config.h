#define USE_REDIS_CHANNEL

// #define USE_CIRRUS
#define DATASET_IN_S3
#define USE_REDIS
// #define USE_PREFETCH
// #define USE_S3

// #define PRELOAD_DATA

#ifdef USE_REDIS
#define REDIS_PORT (6379)
#define REDIS_IP "172.31.12.154"
#define PS_IP "172.31.12.154"
#define PS_PORT 1337
#endif

#define LOADING_TASK_RANK -100000
#define PERFORMANCE_LAMBDA_RANK (-100000)

#define PS_SPARSE_SERVER_TASK_RANK (1)
#define PS_SPARSE_TASK_RANK (-300)

#define WORKER_SPARSE_TASK_RANK (3)
#define ERROR_SPARSE_TASK_RANK (2)
#define LOADING_SPARSE_TASK_RANK (0)
#define PS_TASK_RANK -1
#define ERROR_TASK_RANK -2
#define WORKER_TASK_RANK -3
#define WORKERS_BASE 3 // used in wait_for_start

using FEATURE_TYPE = float;

//#define S3_BUCKET "cirrusonlambdas"
//#define S3_SPARSE_BUCKET "cirrusonlambdas-sparse"

#define CRITEO_HASH_BITS 20
#define RCV1_HASH_BITS 20

#define LIMIT_NUMBER_PASSES 3

#define SYNC_N_STEPS (100)

#define STALE_THRESHOLD (4)
