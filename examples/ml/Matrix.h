#ifndef EXAMPLES_ML_MATRIX_H_
#define EXAMPLES_ML_MATRIX_H_

#include <cstring>
#include <iostream>
#include <vector>
#include <memory>

class Matrix {
 public:
   Matrix(const double* d, uint64_t r, uint64_t c, bool); // XXX FIX
    /**
      * Build matrix
      * This method copies all the inputs
      * @param m Contents of matrix in a vector of doubles format
      */
    Matrix(std::vector<std::vector<double>> m =
            std::vector<std::vector<double>>());

    /**
      * Build matrix
      * This method copies all the inputs
      * @param data Data in row major order format
      * @param rows Number of rows of matrix
      * @param cols Number of columns of matrix
      */
    Matrix(const double* data, uint64_t rows, uint64_t cols);

    /**
      * Build matrix
      * This method copies all the inputs
      * @param data Several minibatches. Each one is in row major order format
      * @param rows Number of rows in each minibatch
      * @param cols Number of columns in each minibatch
      */
    Matrix(const std::vector<std::shared_ptr<double>> d,
        uint64_t r, uint64_t c);

    /**
      * Returns constant pointer to a row of the matrix
      * @param l Index to the row
      * @returns Returns const pointer to contents of the row
      */
    const double* row(uint64_t l) const;

    /**
      * Computes and returns transpose of the matrix
      * @returns Transpose of matrix
      */
    Matrix T() const;

    /**
      * Returns size (in bytes) of the matrix contents
      * @returns Size (bytes) of the matrix contents
      */
    uint64_t sizeBytes() const;

    /**
      * Sanity check of values in this matrix
      */
    void check_values() const;

    /**
      * Compute checksum of values in this matrix
      * @return Matrix checksum
      */
    double checksum() const;

    /**
      * Print matrix values
      */
    void print() const;

 public:
    uint64_t rows;   //< number of rows of matrix
    uint64_t cols;   //< number of columns of matrix
    std::shared_ptr<const double> data;  //< pointer to matrix contents
    mutable std::shared_ptr<Matrix> cached_transpose;  //< cache for the transpose
};

#endif  // EXAMPLES_ML_MATRIX_H_
