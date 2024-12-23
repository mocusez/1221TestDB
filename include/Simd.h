#include<vector>

enum class JoinType { LEFT, RIGHT };

int sum_simd(const std::vector<int>& data);
double avg_simd(const std::vector<int>& data);
int count_nonzero_simd(const std::vector<int>& data);
std::vector<int> inner_join_simd(const std::vector<int>& arr1, const std::vector<int>& arr2);
std::vector<std::pair<int, int>> join_simd(const std::vector<int>& left, const std::vector<int>& right, JoinType join_type);
