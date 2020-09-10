#include "resampler.h"
#include <memory>
#include <cstring>

using namespace Nodes;

Resampler::Resampler(int initialInSampleRate, int outSampleRate)
    : Node(NodeDescriptor {
            .inputCount = 1,
            .inputs = &kNodeIoTypeAudioTime,
            .outputCount = 1,
            .outputs = &kNodeIoTypeAudioTime,
        }),
      mResampler(initialInSampleRate, outSampleRate)
{
}

void Resampler::setOutputSampleRate(int outSampleRate)
{
    mResampler.setOutputRate(outSampleRate);
}

int Resampler::getRequiredInputLength(int outLength)
{
    return mResampler.getRequiredInLength(outLength);
}

void Resampler::process(const NodeIO *inputs[], NodeIO *outputs[])
{
    auto in = inputs[0]->as<IO::AudioTime>();
    auto out = outputs[0]->as<IO::AudioTime>();

    if (mResampler.getInputRate() != in->getSampleRate()) {
        mResampler.setInputRate(in->getSampleRate());
    }

    int inLength = in->getLength();
    int outLength = mResampler.getExpectedOutLength(inLength);

    auto array = std::make_unique<float[]>(outLength);
    mResampler.process(in->getConstData(), inLength, array.get(), outLength);
    
    int delay = mResampler.getDelay();
    int actualOutLength = outLength - delay;

    out->setSampleRate(mResampler.getOutputRate());
    out->setLength(actualOutLength);

    memcpy(out->getData(), std::next(array.get(), delay), actualOutLength * sizeof(float));
}
