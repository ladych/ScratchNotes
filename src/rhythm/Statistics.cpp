#include "rhythm/Statistics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>

namespace snotes::rhythm
{

double percentile(std::vector<double> values, double q)
{
    std::ranges::sort(values);
    const double position = q / 100.0 * static_cast<double>(values.size() - 1);
    const auto below = static_cast<std::size_t>(std::floor(position));
    const std::size_t above = std::min(below + 1, values.size() - 1);
    return values[below] + ((values[above] - values[below]) * (position - static_cast<double>(below)));
}

double median(std::vector<double> values)
{
    return percentile(std::move(values), 50.0);
}

double mean(const std::vector<double>& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double variance(const std::vector<double>& values)
{
    const double average = mean(values);
    double squares = 0.0;
    for (const double value : values)
        squares += (value - average) * (value - average);
    return squares / static_cast<double>(values.size());
}

} // namespace snotes::rhythm
