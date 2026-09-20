#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <random>

constexpr double PI = 3.14159265358979323846;

int main()
{
    // ==========================================
    // 1. НАСТРОЙКИ
    // ==========================================

    const double sampleRate = 48000.0;
    const int sampleCount = 4096;

    const double freq1 = 1000.0;
    const double freq2 = 5000.0;
    const double freq3 = 12000.0;

    const double noiseLevel = 3;


    // ==========================================
    // 2. ГЕНЕРАЦИЯ СИГНАЛА
    // ==========================================

    std::vector<double> signal(sampleCount);

    std::mt19937 generator(42);

    std::normal_distribution<double> noise(
        0.0,
        noiseLevel
    );

    for (int n = 0; n < sampleCount; ++n)
    {
        double t = n / sampleRate;

        signal[n] =
            1.0 * std::sin(2.0 * PI * freq1 * t)
            + 0.7 * std::sin(2.0 * PI * freq2 * t)
            + 0.4 * std::sin(2.0 * PI * freq3 * t)
            + noise(generator);
    }


    // ==========================================
    // 3. DFT + HANN WINDOW
    // ==========================================

    std::vector<double> spectrum(sampleCount / 2);

    for (int k = 0; k < sampleCount / 2; ++k)
    {
        std::complex<double> sum(0.0, 0.0);

        for (int n = 0; n < sampleCount; ++n)
        {
            double angle =
                -2.0 * PI * k * n / sampleCount;

            std::complex<double> exponential(
                std::cos(angle),
                std::sin(angle)
            );

            // Hann window
            double window =
                0.5 * (
                    1.0 -
                    std::cos(
                        2.0 * PI * n /
                        (sampleCount - 1)
                    )
                    );

            sum +=
                signal[n]
                * window
                * exponential;
        }

        double magnitude =
            std::abs(sum);

        // Нормализация для Hann
        double normalized =
            (4.0 * magnitude)
            / sampleCount;

        spectrum[k] =
            normalized;
    }


    // ==========================================
    // 4. ПЕРЕВОД СПЕКТРА В dB
    // ==========================================

    std::vector<double> spectrumDb(
        spectrum.size()
    );

    for (size_t k = 0; k < spectrum.size(); ++k)
    {
        double amplitude =
            std::max(
                spectrum[k],
                1e-12
            );

        spectrumDb[k] =
            20.0
            * std::log10(amplitude);
    }


    // ==========================================
    // 5. ОЦЕНКА NOISE FLOOR
    // ==========================================

    std::vector<double> sortedDb =
        spectrumDb;

    std::sort(
        sortedDb.begin(),
        sortedDb.end()
    );

    double noiseFloor =
        sortedDb[
            sortedDb.size() / 2
        ];

    double detectionThreshold =
        noiseFloor + 15.0;


    // ==========================================
    // 6. ВЫВОД СПЕКТРА
    // ==========================================

    std::cout
        << "\n--- SPECTRUM ---\n\n";

    const double bandWidthHz = 500.0;
    const double nyquist =
        sampleRate / 2.0;

    for (
        double bandStart = 0.0;
        bandStart < nyquist;
        bandStart += bandWidthHz
        )
    {
        double bandEnd =
            bandStart + bandWidthHz;

        int startBin =
            static_cast<int>(
                bandStart
                * sampleCount
                / sampleRate
                );

        int endBin =
            static_cast<int>(
                bandEnd
                * sampleCount
                / sampleRate
                );

        if (endBin > sampleCount / 2)
        {
            endBin =
                sampleCount / 2;
        }

        double maxAmplitude = 0.0;
        int maxBin = startBin;

        for (
            int k = startBin;
            k < endBin;
            ++k
            )
        {
            if (
                spectrum[k]
            > maxAmplitude
                )
            {
                maxAmplitude =
                    spectrum[k];

                maxBin = k;
            }
        }

        double peakFrequency =
            maxBin
            * sampleRate
            / sampleCount;

        double dB =
            20.0
            * std::log10(
                std::max(
                    maxAmplitude,
                    1e-12
                )
            );

        int barLength =
            static_cast<int>(
                maxAmplitude * 50.0
                );

        if (barLength > 50)
        {
            barLength = 50;
        }

        std::cout
            << std::setw(8)
            << static_cast<int>(
                peakFrequency
                )
            << " Hz | "
            << std::setw(6)
            << std::fixed
            << std::setprecision(1)
            << dB
            << " dB | ";

        for (
            int i = 0;
            i < barLength;
            ++i
            )
        {
            std::cout << '#';
        }

        std::cout << '\n';
    }


    // ==========================================
    // 7. NOISE FLOOR
    // ==========================================

    std::cout
        << "\nEstimated noise floor: "
        << std::fixed
        << std::setprecision(1)
        << noiseFloor
        << " dB\n";

    std::cout
        << "Detection threshold: "
        << detectionThreshold
        << " dB\n";


    // ==========================================
    // 8. АВТОМАТИЧЕСКОЕ ОБНАРУЖЕНИЕ
    // ==========================================

    std::cout
        << "\n--- DETECTED SIGNALS ---\n";

    for (
        size_t k = 1;
        k < spectrumDb.size() - 1;
        ++k
        )
    {
        bool localMaximum =
            spectrumDb[k]
        > spectrumDb[k - 1]
            &&
            spectrumDb[k]
            > spectrumDb[k + 1];

        bool aboveThreshold =
            spectrumDb[k]
        > detectionThreshold;

        if (
            localMaximum
            &&
            aboveThreshold
            )
        {
            double frequency =
                k
                * sampleRate
                / sampleCount;

            double snr =
                spectrumDb[k]
                - noiseFloor;

            std::cout
                << std::fixed
                << std::setprecision(1)
                << frequency
                << " Hz"
                << " | level = "
                << spectrumDb[k]
                << " dB"
                << " | SNR = "
                << snr
                << " dB\n";
        }
    }


    return 0;
}