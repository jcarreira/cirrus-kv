#include <Matrix.h>
#include <Utils.h>

Matrix::Matrix(std::vector<std::vector<double>> m) :
        r(0), c(0), data(0) {
    if (!m.size()) {
        return;
    }

    r = m.size();
    c = m[0].size();

    double* new_array = new double[r * c];

    uint64_t index = 0;
    for (const auto& v : m) {
        for (const auto& vv : v) {
            new_array[index++] = vv;
        }
    }

    // bug here
    data.reset(const_cast<const double*>(new_array),
            []( const double *p ) {
                delete[] p; 
            });
}
    
Matrix::Matrix(const double* d, uint64_t rows, uint64_t cols) {
    r = rows;
    c = cols;

    double* copy = new double[rows * cols];

    data.reset(copy, []( const double *p ) {
            delete[] p; 
         });
}

const double* Matrix::row(uint64_t l) const {
    const double* data_start = reinterpret_cast<const double*>(data.get());
    return &data_start[l * c];
}

Matrix Matrix::T() const {
    if (cached_data.get()) {
        return *cached_data;
    }
    
    const double* data_start = reinterpret_cast<const double*>(data.get());

    double* res = new double[rows() * cols()];
    for (uint64_t l = 0; l < rows(); ++l) {
        for (uint64_t c = 0; c < cols(); ++c) {
            res[c * rows() + l] = data_start[l * cols() + c];
        }
    }
    cached_data.reset(new Matrix(res, cols(), rows()));

    return *cached_data;
}

uint64_t Matrix::rows() const {
    return r;
}

uint64_t Matrix::cols() const {
    return c;
}

uint64_t Matrix::sizeBytes() const {
    return r * c * sizeof(double);
}

