#ifndef MAIN_CONTEXT_CONFIG_H
#define MAIN_CONTEXT_CONFIG_H

#include <QObject>
#include "../filesystem.hpp"
#include <fstream>
#include <toml++/toml.h>

#include "solvermakers.h"
#include "../gui/qpainterwrapper.h"
#include "../modules/audio/base/base.h"

namespace Main {

    fs::path getConfigPath();
    toml::table getConfigTable();

    class Config : public QObject {
        Q_OBJECT
        Q_PROPERTY(int pitchAlgorithm       READ getPitchAlgorithmNumeric   WRITE setPitchAlgorithm         NOTIFY pitchAlgorithmChanged)
        Q_PROPERTY(int linpredAlgorithm     READ getLinpredAlgorithmNumeric WRITE setLinpredAlgorithm       NOTIFY linpredAlgorithmChanged)
        Q_PROPERTY(int formantAlgorithm     READ getFormantAlgorithmNumeric WRITE setFormantAlgorithm       NOTIFY formantAlgorithmChanged)
        Q_PROPERTY(int invglotAlgorithm     READ getInvglotAlgorithmNumeric WRITE setInvglotAlgorithm       NOTIFY invglotAlgorithmChanged)
        Q_PROPERTY(int viewMinFrequency     READ getViewMinFrequency        WRITE setViewMinFrequency       NOTIFY viewMinFrequencyChanged)
        Q_PROPERTY(int viewMaxFrequency     READ getViewMaxFrequency        WRITE setViewMaxFrequency       NOTIFY viewMaxFrequencyChanged)
        Q_PROPERTY(bool viewShowSpectrogram READ getViewShowSpectrogram     WRITE setViewShowSpectrogram    NOTIFY viewShowSpectrogramChanged)
        Q_PROPERTY(bool viewShowPitch       READ getViewShowPitch           WRITE setViewShowPitch          NOTIFY viewShowPitchChanged)
        Q_PROPERTY(bool viewShowFormants    READ getViewShowFormants        WRITE setViewShowFormants       NOTIFY viewShowFormantsChanged)
        Q_PROPERTY(bool uiShowSidebar       READ getUiShowSidebar           WRITE setUiShowSidebar          NOTIFY uiShowSidebarChanged)
    
    signals:
        void pitchAlgorithmChanged(int);
        void linpredAlgorithmChanged(int);
        void formantAlgorithmChanged(int);
        void invglotAlgorithmChanged(int);
        void audioBackendChanged(int);
        void viewMinFrequencyChanged(int);
        void viewMaxFrequencyChanged(int);
        void viewShowSpectrogramChanged(bool);
        void viewShowPitchChanged(bool);
        void viewShowFormantsChanged(bool);
        void uiShowSidebarChanged(bool);

    public:
        Config();
        virtual ~Config();
        
        Module::Audio::Backend getAudioBackend();
        void setAudioBackend(Module::Audio::Backend b);

        PitchAlgorithm getPitchAlgorithm();
        void setPitchAlgorithm(PitchAlgorithm alg);
        
        int getPitchAlgorithmNumeric();
        void setPitchAlgorithm(int alg);

        LinpredAlgorithm getLinpredAlgorithm();
        void setLinpredAlgorithm(LinpredAlgorithm alg);
        
        int getLinpredAlgorithmNumeric();
        void setLinpredAlgorithm(int alg);

        FormantAlgorithm getFormantAlgorithm();
        void setFormantAlgorithm(FormantAlgorithm alg);
        
        int getFormantAlgorithmNumeric();
        void setFormantAlgorithm(int alg);

        InvglotAlgorithm getInvglotAlgorithm();
        void setInvglotAlgorithm(InvglotAlgorithm alg);
        
        int getInvglotAlgorithmNumeric();
        void setInvglotAlgorithm(int alg);

        int getViewMinFrequency();
        void setViewMinFrequency(int f);

        int getViewMaxFrequency();
        void setViewMaxFrequency(int f);

        int getViewFFTSize();
        int getViewMinGain();
        int getViewMaxGain();
        QPainterWrapper::FrequencyScale getViewFrequencyScale();
        int getViewFormantCount();
        std::array<double, 3> getViewFormantColor(int i);

        bool getViewShowSpectrogram();
        void setViewShowSpectrogram(bool b);

        bool getViewShowPitch();
        void setViewShowPitch(bool b);

        bool getViewShowFormants();
        void setViewShowFormants(bool b);

        bool getUiShowSidebar();
        void setUiShowSidebar(bool b);

        int getAnalysisMaxFrequency();
        int getAnalysisLpOffset();
        int getAnalysisPitchSampleRate();

    private:
        toml::table mTbl;
    };

}

#endif // MAIN_CONTEXT_CONFIG_H