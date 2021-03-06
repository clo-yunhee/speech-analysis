#include "dataviswrapper.h"

#include <QSplineSeries>

using namespace Main;

DataVisWrapper::DataVisWrapper()
{
}

void DataVisWrapper::setSound(const rpm::vector<double>& signal, double fs)
{
    std::lock_guard<std::mutex> lock(mSoundMutex);
    std::lock_guard<std::mutex> lock2(mGifMutex);

    double absMax = 0;
    for (int k = 0; k < signal.size(); ++k) {
        if (std::abs(signal[k]) > absMax)
            absMax = std::abs(signal[k]);
    }

    int k1 = mGifStart * fs / 1000;
    int k2 = mGifEnd * fs / 1000;

    mSound.resize(signal.size()); 
    for (int k = 0; k < signal.size(); ++k) {
        mSound[k] = QPointF((k * 1000) / fs, (absMax > 0) ? signal[k] / absMax : signal[k]);
    }
    emit soundChanged();
}

void DataVisWrapper::setGif(const rpm::vector<double>& signal, double fs)
{
    std::lock_guard<std::mutex> lock(mSoundMutex);
    std::lock_guard<std::mutex> lock2(mGifMutex);

    double absMax = 0;
    for (int k = 0; k < signal.size(); ++k) {
        if (std::abs(signal[k]) > absMax)
            absMax = std::abs(signal[k]);
    }

    int zcr = signal.size() - 1;
    
    for (int i = zcr; i >= 1; --i) {
        if (signal[i - 1] * signal[i] < 0 && signal[i - 1] > 0) {
            zcr = i;
            break;
        }
    }

    mGifEnd = (double) zcr / fs * 1000;
    
    constexpr int periods = 5;
    int periodNum = 0;

    for (int i = zcr; i >= 1; --i) {
        if (signal[i - 1] * signal[i] < 0 && signal[i - 1] > 0) {
            zcr = i;
            if (++periodNum >= periods)
                break;
        }
    }

    mGifStart = (double) zcr / fs * 1000;

    int k1 = mGifStart * fs / 1000;
    int k2 = mGifEnd * fs / 1000;

    mGif.resize(k2 - k1 + 1);
    for (int i = 0, k = k1; k <= k2; ++k, ++i) {
        mGif[i] = QPointF((k * 1000) / fs, (absMax > 0) ? signal[k] / absMax : signal[k]);
    }
    emit gifChanged();
}

void DataVisWrapper::updateSoundSeries(QXYSeries* series, QValueAxis* xAxis, QValueAxis* yAxis)
{
    std::lock_guard<std::mutex> lock(mSoundMutex);
    std::lock_guard<std::mutex> lock2(mGifMutex);
    
    series->replace(mSound);
    xAxis->setRange(mGifStart, mGifEnd);
    yAxis->setRange(-1.5, 1.5);
}

void DataVisWrapper::updateGifSeries(QXYSeries* series, QValueAxis* xAxis, QValueAxis* yAxis)
{
    std::lock_guard<std::mutex> lock(mSoundMutex);
    std::lock_guard<std::mutex> lock2(mGifMutex);

    series->replace(mGif);
    xAxis->setRange(mGifStart, mGifEnd);
    yAxis->setRange(-1.5, 1.5);
}
