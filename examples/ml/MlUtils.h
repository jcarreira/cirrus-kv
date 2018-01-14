#ifndef EXAMPLES_ML_MLUTILS_H_
#define EXAMPLES_ML_MLUTILS_H_

namespace mlutils {

/**
  * Computes safe sigmoid of value x
  * @param x Input value
  * @return Sigmoid of x
  */
double s_1(double x);
float s_1_float(float x);

/**
  * Computes logarithm
  * Check for NaN and Inf values
  * Clip values if they are too small (can lead to problems)
  * @param x Input value
  * @return Logarithm of x
  */
double log_aux(double x);

}  // namespace mlutils

#endif  // EXAMPLES_ML_MLUTILS_H_
