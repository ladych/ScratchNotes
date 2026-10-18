#pragma once

#include <complex>
#include <cstddef>
#include <span>
#include <vector>

namespace snotes::rhythm
{

/// Power spectrum of real frames with an iterative radix-2 FFT.
class PowerSpectrum
{
public:
    /// size must be a power of two.
    explicit PowerSpectrum(std::size_t size);

    /// |X[k]|^2 for k = 0 .. size / 2 of a frame of size samples.
    void compute(std::span<const double> frame, std::span<double> power);

    [[nodiscard]] std::size_t size() const { return size_; }

private:
    std::size_t size_;
    std::vector<std::size_t> reversed_;
    std::vector<std::complex<double>> twiddles_;
    std::vector<std::complex<double>> buffer_;
};

} // namespace snotes::rhythm
