#include <immintrin.h> // Include SIMD instruction definitions
#include <vector>
#include <numeric>    // For std::accumulate
#include <iostream>

// Function: Use SIMD to accelerate summing an integer array
int sum_simd(const std::vector<int>& data) {
    size_t size = data.size();
    int sum = 0;

    // Handle empty input
    if (size == 0) return sum;

    // Initialize a vector for accumulating partial sums
    __m256i accumulator = _mm256_setzero_si256(); // Zero-initialize the accumulator for integers

    // SIMD accelerated loop
    for (size_t i = 0; i < size - (size % 8); i += 8) { // Process only the part that can be divided by 8
        // Load 8 consecutive integers into a SIMD register
        __m256i vec_data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i]));

        // Add vec_data to accumulator
        accumulator = _mm256_add_epi32(accumulator, vec_data);
    }

    // Horizontally sum the values in the SIMD register to get a scalar result
    alignas(32) int temp[8]; // Ensure temp array is 32-byte aligned
    _mm256_store_si256(reinterpret_cast<__m256i*>(temp), accumulator); // Store the accumulated values into temp array
    sum = std::accumulate(temp, temp + 8, 0); // Sum the elements of temp array

    // Handle remaining elements (if any)
    for (size_t i = size - (size % 8); i < size; ++i) {
        sum += data[i];
    }

    return sum;
}

double avg_simd(const std::vector<int>& data) {
    size_t size = data.size();
    if (size == 0) return 0.0;

    int sum = sum_simd(data);
    return static_cast<double>(sum) / size;
}

int count_nonzero_simd(const std::vector<int>& data) {
    size_t size = data.size();
    int count = 0;

    if (size == 0) return count;

    // SIMD accelerated loop
    for (size_t i = 0; i < size - (size % 8); i += 8) { // Process only the part that can be divided by 8
        __m256i vec_data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i]));

        // Compare each element with zero, resulting in a mask where non-zero elements are set to all bits 1
        __m256i cmp_result = _mm256_cmpgt_epi32(_mm256_setzero_si256(), vec_data);

        // Create a mask of non-zero elements
        __m256i nonzero_mask = _mm256_andnot_si256(cmp_result, _mm256_set1_epi32(-1));

        // Count the number of non-zero elements in the mask
        alignas(32) int temp[8]; // Ensure temp array is 32-byte aligned
        _mm256_store_si256(reinterpret_cast<__m256i*>(temp), nonzero_mask);
        for (int j = 0; j < 8; ++j) {
            count += (temp[j] != 0);
        }
    }

    // Handle remaining elements (if any)
    for (size_t i = size - (size % 8); i < size; ++i) {
        count += (data[i] != 0);
    }

    return count;
}

