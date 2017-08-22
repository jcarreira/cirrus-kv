#ifndef _ML_UTILS_H_
#define _ML_UTILS_H_

namespace mlutils {

/**
  * Computes safe sigmoid of value x
  * @param x Input value
  * @return Sigmoid of x
  */
double s_1(double x);

/**
  * Computes logarithm
  * Check for NaN and Inf values
  * Clip values if they are too small (can lead to problems)
  * @param x Input value
  * @return Logarithm of x
  */
double log_aux(double x);

}  // namespace mlutils

#endif  // _ML_UTILS_H_
