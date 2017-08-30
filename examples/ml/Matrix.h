#ifndef EXAMPLES_ML_MATRIX_H_
#define EXAMPLES_ML_MATRIX_H_

#include <cstring>
#include <iostream>
#include <vector>
#include <memory>

class Matrix {
 public:
    /**
      * Build matrix
      * @param m Contents of matrix in a vector of doubles format
      */
    Matrix(std::vector<std::vector<double>> m =
            std::vector<std::vector<double>>());

    /**
      * Build matrix
      * @param data Data in row major order format
      * @param rows Number of rows of matrix
      * @param cols Number of columns of matrix
      */
    Matrix(const double* data, uint64_t rows, uint64_t cols);

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
      * Returns number of rows of the matrix
      * @returns Number of rows
      */
    uint64_t rows() const;

    /**
      * Returns number of columns of the matrix
      * @returns Number of columns
      */
    uint64_t cols() const;

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
    uint64_t r;  //< number of rows of matrix
    uint64_t c;  //< number of columns of matrix
    std::shared_ptr<const double> data;  //< pointer to matrix contents
    mutable std::shared_ptr<Matrix> cached_data;  //< cache for the transpose
};

#endif  // EXAMPLES_ML_MATRIX_H_
