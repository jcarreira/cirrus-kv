/* Copyright Joao Carreira 2016 */

#include <cstring>
#include "src/utils/InfinibandSupport.h"
#include "src/utils/utils.h"
#include "src/utils/logging.h"

namespace sirius {

void InfinibandSupport::check_mw_support(ibv_context* ctx) {
    struct ibv_exp_device_attr device_attr;

    std::memset(&device_attr, 0, sizeof(device_attr));
    device_attr.comp_mask = IBV_EXP_DEVICE_ATTR_RESERVED - 1;

    ibv_exp_query_device(ctx, &device_attr);
    if (device_attr.exp_device_cap_flags & IBV_EXP_DEVICE_MEM_WINDOW ||
            device_attr.exp_device_cap_flags & IBV_EXP_DEVICE_MW_TYPE_2B) {
        LOG<INFO>("MW supported");
    } else {
        DIE("MW not supported");
    }
}

void InfinibandSupport::check_odp_support(ibv_context* ctx) {
    struct ibv_exp_device_attr device_attr;

    std::memset(&device_attr, 0, sizeof(device_attr));
    device_attr.comp_mask =
        IBV_EXP_DEVICE_ATTR_ODP | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;

    ibv_exp_query_device(ctx, &device_attr);
    if (device_attr.exp_device_cap_flags & IBV_EXP_DEVICE_ODP) {
        LOG<INFO>("ODP supported");
    } else {
        LOG<INFO>("ODP not supported!");
    }
}

}  // namespace  sirius
