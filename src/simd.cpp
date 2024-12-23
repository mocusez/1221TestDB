#include <immintrin.h> // Include SIMD instruction definitions
#include <vector>
#include <numeric>    // For std::accumulate
#include <iostream>
#include <algorithm>   // For std::sort
#include <unordered_map>

enum class JoinType { LEFT, RIGHT };

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

// Function: Use SIMD to perform an inner join on two sorted integer arrays
std::vector<int> inner_join_simd(const std::vector<int>& arr1, const std::vector<int>& arr2) {
    // Ensure both arrays are sorted
    std::vector<int> sorted_arr1 = arr1;
    std::vector<int> sorted_arr2 = arr2;
    std::sort(sorted_arr1.begin(), sorted_arr1.end());
    std::sort(sorted_arr2.begin(), sorted_arr2.end());

    size_t size1 = sorted_arr1.size();
    size_t size2 = sorted_arr2.size();
    std::vector<int> result;

    size_t i = 0, j = 0;
    while (i < size1 && j < size2) {
        // Load 8 integers from each array into SIMD registers
        if (i + 8 <= size1 && j + 8 <= size2) {
            __m256i vec1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&sorted_arr1[i]));
            __m256i vec2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&sorted_arr2[j]));

            // Compare the two vectors
            __m256i cmp_result = _mm256_cmpeq_epi32(vec1, vec2);

            // Extract the mask of equal elements
            int mask = _mm256_movemask_epi8(cmp_result);

            // Process the mask to find matching elements
            for (int k = 0; k < 8; ++k) {
                if ((mask >> (4 * k)) & 0xF == 0xF) { // Check if all 4 bytes in a 32-bit word are set
                    result.push_back(sorted_arr1[i + k]);
                }
            }

            // Advance pointers based on comparison results
            int min_value1 = _mm256_extract_epi32(vec1, 0);
            int max_value1 = _mm256_extract_epi32(vec1, 7);
            int min_value2 = _mm256_extract_epi32(vec2, 0);
            int max_value2 = _mm256_extract_epi32(vec2, 7);

            if (max_value1 <= max_value2) {
                i += 8;
            } else {
                j += 8;
            }
        } else {
            // Handle remaining elements that do not fit into a full SIMD register
            while (i < size1 && j < size2) {
                if (sorted_arr1[i] == sorted_arr2[j]) {
                    result.push_back(sorted_arr1[i]);
                    ++i;
                    ++j;
                } else if (sorted_arr1[i] < sorted_arr2[j]) {
                    ++i;
                } else {
                    ++j;
                }
            }
            break;
        }
    }

    return result;
}

// Function: Use SIMD and hash table to perform a left or right join on two integer arrays
std::vector<std::pair<int, int>> join_simd(const std::vector<int>& left, const std::vector<int>& right, JoinType join_type) {
    // Build a hash map for the secondary array to allow O(1) lookups
    std::unordered_map<int, int> secondary_map;
    for (int value : (join_type == JoinType::LEFT ? right : left)) {
        if (secondary_map.find(value) == secondary_map.end()) {
            secondary_map[value] = value; // Store the value itself or any other associated data
        }
    }

    const std::vector<int>& primary = (join_type == JoinType::LEFT ? left : right);
    const std::vector<int>& secondary = (join_type == JoinType::LEFT ? right : left);
    size_t primary_size = primary.size();

    std::vector<std::pair<int, int>> result;

    // SIMD accelerated loop for bulk processing the primary array
    for (size_t i = 0; i <= primary_size - 8; i += 8) {
        __m256i vec_primary = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&primary[i]));
        alignas(32) int temp[8]; // Ensure temp array is 32-byte aligned

        // Extract elements from SIMD register
        _mm256_store_si256(reinterpret_cast<__m256i*>(temp), vec_primary);

        for (int j = 0; j < 8; ++j) {
            int primary_val = temp[j];
            auto it = secondary_map.find(primary_val);
            int secondary_val = (it != secondary_map.end()) ? it->second : -1; // Default value for no match in secondary

            if (join_type == JoinType::LEFT) {
                result.emplace_back(primary_val, secondary_val);
            } else { // JoinType::RIGHT
                result.emplace_back(secondary_val, primary_val);
            }
        }
    }

    // Handle remaining elements (if any) in the primary array
    for (size_t i = primary_size - (primary_size % 8); i < primary_size; ++i) {
        int primary_val = primary[i];
        auto it = secondary_map.find(primary_val);
        int secondary_val = (it != secondary_map.end()) ? it->second : -1; // Default value for no match in secondary

        if (join_type == JoinType::LEFT) {
            result.emplace_back(primary_val, secondary_val);
        } else { // JoinType::RIGHT
            result.emplace_back(secondary_val, primary_val);
        }
    }

    return result;
}
