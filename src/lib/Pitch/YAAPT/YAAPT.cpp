//
// Created by rika on 11/11/2019.
//

#include "YAAPT.h"

using namespace Eigen;

void YAAPT::getF0_slow(const ArrayXd & data, double fs, Result & res, const Params & prm)
{
    ArrayXd B, C, D;
    double newFs;
    nonlinear(ArrayXd::Ones(1024), fs, prm, B, C, D, newFs);

    ArrayXd energy;
    ArrayXb vUvEnergy;
    nlfer(B, newFs, prm, energy, vUvEnergy);

    ArrayXd sPitch, vUvSPitch;
    double pAvg, pStd;
    spec_trk(D, newFs, vUvEnergy, prm, sPitch, vUvSPitch, pAvg, pStd);

    ArrayXXd tPitch1, tMerit1;
    tm_trk(B, newFs, sPitch, pStd, pAvg, prm, tPitch1, tMerit1);
    
    ArrayXXd tPitch2, tMerit2;
    tm_trk(D, newFs, sPitch, pStd, pAvg, prm, tPitch2, tMerit2);

    int lenSpectral = sPitch.size();
    int lenTemporal = tPitch1.size();
    if (lenTemporal < lenSpectral) {
        int maxCands = prm.nccfMaxCands;

        ArrayXXd tPitch1_pad(maxCands, lenSpectral);
        tPitch1_pad << tPitch1, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tPitch1 = std::move(tPitch1_pad);

        ArrayXXd tMerit1_pad(maxCands, lenSpectral);
        tMerit1_pad << tMerit1, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tMerit1 = std::move(tMerit1_pad);

        ArrayXXd tPitch2_pad(maxCands, lenSpectral);
        tPitch2_pad << tPitch2, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tPitch2 = std::move(tPitch2_pad);

        ArrayXXd tMerit2_pad(maxCands, lenSpectral);
        tMerit2_pad << tMerit2, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tMerit2 = std::move(tMerit2_pad);
    }

    ArrayXXd rPitch, merit;
    refine(tPitch1, tMerit1, tPitch2, tMerit2, sPitch, energy, vUvEnergy, prm, rPitch, merit);

    dynamic(rPitch, merit, energy, prm, res.pitch);
    res.numFrames = res.pitch.size();
    res.framePeriod = prm.frameSpace;
}

void YAAPT::getF0_fast(const ArrayXd & data, double fs, Result & res, const Params & prm)
{
    ArrayXd B, C, D;
    double newFs;
    nonlinear(data, fs, prm, B, C, D, newFs);

    ArrayXd energy;
    ArrayXb vUvEnergy;
    nlfer(B, newFs, prm, energy, vUvEnergy);

    ArrayXd sPitch, vUvSPitch;
    double pAvg, pStd;
    spec_trk(D, newFs, vUvEnergy, prm, sPitch, vUvSPitch, pAvg, pStd);

    ArrayXXd tPitch1, tMerit1;
    tm_trk(B, newFs, sPitch, pStd, pAvg, prm, tPitch1, tMerit1);

    int lenSpectral = sPitch.size();
    int lenTemporal = tPitch1.size();
    if (lenTemporal < lenSpectral) {
        int maxCands = prm.nccfMaxCands;

        ArrayXXd tPitch1_pad(maxCands, lenSpectral);
        tPitch1_pad << tPitch1, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tPitch1 = std::move(tPitch1_pad);

        ArrayXXd tMerit1_pad(maxCands, lenSpectral);
        tMerit1_pad << tMerit1, ArrayXXd::Zero(3, lenSpectral - lenTemporal);
        tMerit1 = std::move(tMerit1_pad);
    }

    ArrayXXd rPitch, merit;
    refine(tPitch1, tMerit1, tPitch1, tMerit1, sPitch, energy, vUvEnergy, prm, rPitch, merit);

    dynamic(rPitch, merit, energy, prm, res.pitch);
    res.numFrames = res.pitch.size();
    res.framePeriod = prm.frameSpace;
}

void YAAPT::getF0_fastest(const ArrayXd & data, double fs, Result & res, const Params & prm)
{
    throw std::runtime_error("Unimplemented yet");
}