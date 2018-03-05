#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

enum PS_OP {
  SEND_LR_GRADIENT,
  SEND_MF_GRADIENT,
  GET_LR_FULL_MODEL,
  GET_MF_FULL_MODEL,
  GET_LR_SPARSE_MODEL,
  GET_MF_SPARSE_MODEL,
  SET_TASK_STATUS,
  GET_TASK_STATUS
};

#endif // _CONSTANTS_H_