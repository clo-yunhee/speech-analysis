//
// Created by rika on 16/11/2019.
//

#include "../Analyser.h"
#include "../../lib/LPC/Frame/LPC_Frame.h"

using namespace Eigen;

void Analyser::analyseFormantLp() {
    LPC::toFormantFrame(lpcFrame, lastFormantFrame, fs);
}

void Analyser::analyseFormantDeep() {

    bool isVoiced = pitchTrack.back() > 0;

}
