#ifndef SRC_SERVER_GENERALCONTEXT_H_
#define SRC_SERVER_GENERALCONTEXT_H_

#include <thread>

namespace cirrus {

class GeneralContext {
 public:
    GeneralContext() :
        ctx(0),
        pd(0),
        cq(0),
        comp_channel(0),
        cq_poller_thread(0) {}

    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;
    std::thread* cq_poller_thread;
};

}  // namespace cirrus

#endif  // SRC_SERVER_GENERALCONTEXT_H_
