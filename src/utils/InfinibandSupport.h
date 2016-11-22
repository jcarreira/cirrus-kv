/* Coprytight 2016 Joao Carreira */

#ifndef _INFINIBAND_SUPPORT_H_
#define _INFINIBAND_SUPPORT_H_

#include <mutex>
#include <rdma/rdma_cma.h>
#include "src/utils/logging.h"

namespace sirius {

class InfinibandSupport {
public:
    InfinibandSupport() = default;

    void check_mw_support(ibv_context* ctx);
    void check_odp_support(ibv_context* ctx);

    std::once_flag mw_support_check_;
    std::once_flag odp_support_check_;
};

} // sirius

#endif // _INFINIBAND_SUPPORT_H_
