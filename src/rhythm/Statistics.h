#pragma once

#include <vector>

namespace snotes::rhythm
{

/// numpy.percentile with linear interpolation; q from 0 to 100. values must not be empty.
[[nodiscard]] double percentile(std::vector<double> values, double q);
/// numpy.median.
[[nodiscard]] double median(std::vector<double> values);
[[nodiscard]] double mean(const std::vector<double>& values);
/// Population variance (numpy.var).
[[nodiscard]] double variance(const std::vector<double>& values);

} // namespace snotes::rhythm
