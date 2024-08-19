#include "Stats.hpp"
#include "vk_core.hpp"

#include <chrono>

#ifdef RUN_WITH_NSIGHT
    #include <nvtx3/nvtx3.hpp>
#endif

static uint64_t get_time()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Stats::push(const char* name)
{
#ifdef RUN_WITH_NSIGHT
    nvtxRangePush(name);
#endif
    pushed_idx_stack.push(time_range_vec.size());

    time_range_vec.push_back({});
    time_range_vec.back().first = name;
    time_range_vec.back().second[0] = get_time();
}

void Stats::pop()
{
#ifdef RUN_WITH_NSIGHT
    nvtxRangePop();
#endif
    const auto idx = pushed_idx_stack.top();
    pushed_idx_stack.pop();

    time_range_vec[idx].second[1] = get_time();
}

void Stats::reset()
{
    while (!pushed_idx_stack.empty())
        pushed_idx_stack.pop();
    time_range_vec.clear();
}

uint64_t get_calibrated_cpu_gpu_timestamp_delta()
{
    constexpr VkCalibratedTimestampInfoKHR calibrated_timestamp_infos[2] {
        {
            .sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR,
            .pNext = nullptr,
            .timeDomain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR,
            .pNext = nullptr,
            .timeDomain = VK_TIME_DOMAIN_DEVICE_KHR,
        }
    };

    uint64_t max_deviation = 0lu;
    uint64_t calibrated_timestamps[2]; 

    vk_core::get_calibrated_timestamps(calibrated_timestamp_infos, calibrated_timestamps, 2, &max_deviation);

    return calibrated_timestamps[1] - calibrated_timestamps[0];
}

#if 0
void Stats::gui()
{
    std::vector<std::pair<std::string, std::pair<std::array<float, 2>, std::array<float, 2>>>> graph_data; 

    const auto ms_frame = std::chrono::duration_cast<std::chrono::microseconds>(frame_time_range.end - frame_time_range.start).count() / 1000.0f;

    graph_data.push_back({});
    graph_data.back().first = "Frame";
    graph_data.back().second.first[0] = 0.0f;
    graph_data.back().second.first[1] = ms_frame;
    graph_data.back().second.second[0] = 0.0f;
    graph_data.back().second.second[1] = 0.0f;

    int i = 1;

    for (const auto& time_range : time_range_vec)
    {
        const auto ms_since_sof_start = std::chrono::duration_cast<std::chrono::microseconds>(time_range.second.start - frame_time_range.start).count() / 1000.0f;
        const auto ms_since_sof_end = std::chrono::duration_cast<std::chrono::microseconds>(time_range.second.end - frame_time_range.start).count() / 1000.0f;

        graph_data.push_back({});
        graph_data.back().first = time_range.first;
        graph_data.back().second.first[0] = ms_since_sof_start;
        graph_data.back().second.first[1] = ms_since_sof_end;
        graph_data.back().second.second[0] = static_cast<float>(i);
        graph_data.back().second.second[1] = static_cast<float>(i);
        // graph_data.back().second.second[0] = 0.0f;
        // graph_data.back().second.second[1] = 0.0f;

        i++;
    }

    ImGui::Begin("Gui");

    if (ImPlot::BeginPlot("Line Plots")) 
    {
        ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes("Score", "Student", ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
        ImPlot::SetupAxesLimits(0.0, 34.0, -1.0, static_cast<double>(graph_data.size()), ImPlotCond_Always);
        
        static constexpr std::array<ImVec4, 7> color_array {
            ImVec4(1, 0, 0, 1),
            ImVec4(0, 1, 0, 1),
            ImVec4(0, 0, 1, 1),
            ImVec4(1, 1, 0, 1),
            ImVec4(1, 0, 1, 1),
            ImVec4(0, 1, 1, 1),
            ImVec4(1, 1, 1, 1),
        };

        for (int i = 0; i < graph_data.size(); i++)
        {
            ImPlot::SetNextLineStyle(color_array[i % color_array.size()], 8.0f);
            ImPlot::PlotLine(graph_data[i].first.c_str(), graph_data[i].second.first.data(), graph_data[i].second.second.data(), 2, ImPlotLineFlags_Segments);
        }

        ImPlot::EndPlot();
    }

    ImGui::End();
}
#endif