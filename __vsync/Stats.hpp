#ifndef STATS_HPP
#define STATS_HPP

#include <array>
#include <stack>
#include <vector>
#include <string>
#include <inttypes.h>

#include <vulkan/vulkan.hpp>

struct Stats 
{
private:
    std::stack<int> pushed_idx_stack;
    std::vector<std::pair<std::string, std::array<uint64_t, 2>>> time_range_vec;
public:
    void push(const char* name);
    void pop();
    void reset();

    const std::vector<std::pair<std::string, std::array<uint64_t, 2>>>& get_cpu_data() { return time_range_vec; } 
};

uint64_t get_calibrated_cpu_gpu_timestamp_delta();

#endif