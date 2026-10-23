/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Small timing helpers: a steady-clock stopwatch and summary statistics
(mean, standard deviation, min, max, percentiles) over a list of samples.
*/

#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

namespace swarm
{

class Stopwatch
{
public:
    Stopwatch() : m_start(Clock::now()) {}
    void restart() { m_start = Clock::now(); }
    double elapsedMs() const
    {
        return std::chrono::duration<double, std::milli>(Clock::now() - m_start).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_start;
};

struct SampleStats
{
    double mean = 0.0;
    double stddev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    std::size_t count = 0;
};

inline SampleStats summarize(std::vector<double> samples)
{
    SampleStats s;
    s.count = samples.size();
    if (samples.empty())
    {
        return s;
    }
    std::sort(samples.begin(), samples.end());
    double sum = 0.0;
    for (double v : samples) sum += v;
    s.mean = sum / static_cast<double>(samples.size());
    double var = 0.0;
    for (double v : samples) var += (v - s.mean) * (v - s.mean);
    s.stddev = samples.size() > 1 ? std::sqrt(var / static_cast<double>(samples.size() - 1)) : 0.0;
    s.min = samples.front();
    s.max = samples.back();
    const auto pct = [&samples](double p) {
        const double idx = p * static_cast<double>(samples.size() - 1);
        const std::size_t lo = static_cast<std::size_t>(std::floor(idx));
        const std::size_t hi = std::min(lo + 1, samples.size() - 1);
        const double frac = idx - static_cast<double>(lo);
        return samples[lo] * (1.0 - frac) + samples[hi] * frac;
    };
    s.p50 = pct(0.50);
    s.p95 = pct(0.95);
    return s;
}

}  // namespace swarm
