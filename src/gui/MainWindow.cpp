//
// Created by rika on 06/12/2019.
//

#ifdef HAS_ML_FORMANTS
#   include "DeepFormants/df.h"
#   include <QByteArray>
#   include <QFile>
#endif

#include <cstdio>
#include "MainWindow.h"
#include "ColorMaps.h"
#include "FFT/FFT.h"
#include "../log/simpleQtLogger.h"
#include "rpmalloc.h"
#ifdef Q_OS_ANDROID
#   include <QtAndroid>
#   include <QAndroidIntent>
#   include <jni.h>
#   include "../jni/JniInstance.h"
#endif
#ifdef Q_OS_WASM
#   include <emscripten/html5.h>
#endif

constexpr int maxWidthComboBox = 200;

MainWindow::MainWindow()
    : timer(this)
{
    setObjectName("MainWindow");

    buildColorMaps();

    for (int nfft = 64; nfft <= 2048; nfft *= 2) {
        availableFftSizes << QString::number(nfft);
    }

    availablePitchAlgs
        << "Wavelet"
        << "McLeod"
        << "YIN"
        << "AMDF";
        //<< "CREPE";

    availableFormantAlgs
        << "LP"
        << "KARMA"
#ifdef HAS_ML_FORMANTS
        << "DeepFormants"
#endif
        ;

    availableFreqScales
        << "Linear"
        << "Logarithmic"
        << "Mel";

    for (auto & [name, map] : colorMaps) {
        (void) map;
        availableColorMaps << name;
    }

#ifdef HAS_ML_FORMANTS
    L_INFO("Loading pretrained neural network...");

    QFile file(":/lib/DeepFormants/model.pt");
    file.open(QIODevice::ReadOnly);
    QByteArray bytes = file.readAll();
    loadModuleFromBuffer(bytes.data(), bytes.size());
    file.close();
#endif

    L_INFO("Initialising miniaudio context...");

    rpm::vector<ma_backend> backends{
        ma_backend_wasapi,
        ma_backend_winmm,
        ma_backend_dsound,

        ma_backend_coreaudio,

        ma_backend_pulseaudio,
        ma_backend_jack,
        ma_backend_alsa,
    
        ma_backend_sndio,
        ma_backend_audio4,
        ma_backend_oss,

        ma_backend_aaudio,
        ma_backend_opensl,

        ma_backend_webaudio,

        ma_backend_null
    };

    ma_context_config ctxCfg = ma_context_config_init();
    ctxCfg.threadPriority = ma_thread_priority_realtime;
    ctxCfg.alsa.useVerboseDeviceEnumeration = false;
    ctxCfg.pulse.tryAutoSpawn = true;
    ctxCfg.jack.tryStartServer = true;
    ctxCfg.logCallback = &qtLogCallback;

    if (ma_context_init(backends.data(), backends.size(), &ctxCfg, &maCtx) != MA_SUCCESS) {
        L_FATAL("Failed to initialise miniaudio context");

        throw AudioException("Failed to initialise miniaudio context");
    }
    
    devs = new AudioDevices(&maCtx);

    sineWave = new SineWave();
    noiseFilter = new NoiseFilter();

#if defined(Q_OS_MAC)
    audioInterfaceMem = malloc(sizeof(AudioInterface));
    audioInterface = new (audioInterfaceMem) AudioInterface(&maCtx, sineWave, noiseFilter);
#else
    audioInterface = new AudioInterface(&maCtx, sineWave, noiseFilter);
#endif

    analyser = new Analyser(audioInterface);

    canvas = new AnalyserCanvas(analyser, sineWave, noiseFilter);
    powerSpectrum = new PowerSpectrum(analyser, canvas);
    keybinds = new Keybinds();

#ifdef UI_OSCILLOSCOPE
    oscilloscope = new Oscilloscope;
#endif

#ifdef Q_OS_ANDROID
    JniInstance::createInstance(analyser, canvas, powerSpectrum);
#endif 

    auto fields = uiFields();
    
#ifndef UI_BAR_FIELDS
    addDockWidget(Qt::TopDockWidgetArea, uiFieldsDock(fields));
#endif

#ifdef UI_SHOW_SETTINGS
    auto analysisSettings = uiAnalysisSettings();
    auto displaySettings = uiDisplaySettings();

#ifdef UI_DISPLAY_SETTINGS_IN_DIALOG
    displaySettings->setParent(this);
    displaySettings->setWindowFlag(Qt::Tool);
    displaySettings->setVisible(false);
#endif // UI_DISPLAY_SETTINGS_IN_DIALOG

    addDockWidget(Qt::LeftDockWidgetArea, uiSettingsDock(analysisSettings, displaySettings));
#endif // UI_SHOW_SETTINGS

    auto central = new QWidget;
    setCentralWidget(central);
    auto layout = new QVBoxLayout(central);
    {
        layout->addWidget(uiBar(fields));

        auto vSplitter = new QSplitter(Qt::Vertical);
        vSplitter->setHandleWidth(12);

        auto vSplitterLayout = new QVBoxLayout(vSplitter);
        {
            auto hSplitter = new QSplitter(Qt::Horizontal);
            hSplitter->setHandleWidth(12);
           
            auto hSplitterLayout = new QHBoxLayout(hSplitter);
            {
                hSplitterLayout->addWidget(canvas, 4);
                canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                hSplitterLayout->addWidget(powerSpectrum, 1);
                powerSpectrum->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            }
            vSplitterLayout->addWidget(hSplitter, 4);

#ifdef UI_OSCILLOSCOPE
            vSplitterLayout->addWidget(oscilloscope, 1);
            oscilloscope->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif
        }

        layout->addWidget(vSplitter);
    }

    setWindowTitle(WINDOW_TITLE);
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Keybind event filters
    this->installEventFilter(keybinds);
    window()->installEventFilter(keybinds);
    canvas->installEventFilter(keybinds);
    powerSpectrum->installEventFilter(keybinds);
#ifdef UI_DISPLAY_SETTINGS_IN_DIALOG
    displaySettings->installEventFilter(keybinds);
#endif

    // Keybind signal connections
    connect(keybinds, &Keybinds::actionCursor, canvas, &AnalyserCanvas::cursorMove);
    connect(keybinds, &Keybinds::actionClose, this, &MainWindow::pressClose);
    connect(keybinds, &Keybinds::actionPause, this, &MainWindow::toggleAnalyser);
    connect(keybinds, &Keybinds::actionFullscreen, this, &MainWindow::toggleFullscreen);
    connect(keybinds, &Keybinds::actionSineWave, canvas, &AnalyserCanvas::toggleSineWave);
    connect(keybinds, &Keybinds::actionNoiseFilter, this, &MainWindow::toggleNoiseFilter);

#if DO_UPDATE_DEVICES
    updateDevices();
#endif

    connect(this, &MainWindow::newFramesTracks, canvas, &AnalyserCanvas::renderTracks);
    connect(this, &MainWindow::newFramesSpectrum, canvas, &AnalyserCanvas::renderSpectrogram); 
    connect(this, &MainWindow::newFramesUI, canvas, &AnalyserCanvas::renderScaleAndCursor);
   
    connect(this, &MainWindow::newFramesSpectrum, powerSpectrum, &PowerSpectrum::renderSpectrum);
    connect(this, &MainWindow::newFramesLpc, powerSpectrum, &PowerSpectrum::renderLpc);
  
#ifdef UI_OSCILLOSCOPE 
    connect(this, &MainWindow::newFramesWaves, oscilloscope, &Oscilloscope::renderWaves);
#endif
   
    connect(&timer, &QTimer::timeout, [&]() {
        analyser->callIfNewFrames(
                    [this](auto&&... ts) { emit newFramesTracks(std::forward<decltype(ts)>(ts)...); },
                    [this](auto&&... ts) { emit newFramesSpectrum(std::forward<decltype(ts)>(ts)...); },
                    [this](auto&&... ts) { emit newFramesLpc(std::forward<decltype(ts)>(ts)...); },
                    [this](auto&&... ts) { emit newFramesUI(std::forward<decltype(ts)>(ts)...); },
                    [this](auto&&... ts) { emit newFramesWaves(std::forward<decltype(ts)>(ts)...); }
        );
        updateFields();
        repaint();
    });

#if TIMER_SLOWER
    timer.setTimerType(Qt::CoarseTimer);
    timer.start(1000.0 / 30.0);
#else
    timer.setTimerType(Qt::PreciseTimer);
    timer.start(1000.0 / 60.0);
#endif

    analyser->startThread();

    show();
}

MainWindow::~MainWindow()
{
    if (canvas != nullptr) {
        cleanup();
    }
}

void MainWindow::cleanup()
{
    timer.stop();

    while (timer.isActive()) {
        std::this_thread::yield();
    }

    delete canvas;
    delete powerSpectrum;
    delete oscilloscope;

    delete analyser;

#if defined(Q_OS_MAC)
    audioInterface->~AudioInterface();
    free(audioInterfaceMem);
#else
    delete audioInterface;
#endif

    delete sineWave;

    delete keybinds;

    delete devs;
    ma_context_uninit(&maCtx);

    all_fft_cleanup();

    canvas = nullptr;
}

void MainWindow::updateFields() {

    const int frame = canvas->getSelectedFrame();

    const auto & formants = analyser->getFormantFrame(frame);
    const double pitch = analyser->getPitchFrame(frame);
    const double Oq = analyser->getOqFrame(frame);

    QPalette palette = this->palette();

    //fieldOq->setText(QString("Oq = %1").arg(Oq));

    for (int i = 0; i < numFormants; ++i) {
        if (i < formants.nFormants) {
            fieldFormant[i]->setText(QString("F%1 = %2 Hz").arg(i + 1).arg(round(formants.formant[i].frequency)));
        }
        else {
            fieldFormant[i]->setText("");
        }
    }

    if (pitch > 0) {
        fieldPitch->setText(QString("H1 = %1 Hz").arg(pitch, 0, 'f', 1));
    } else {
        fieldPitch->setText("Unvoiced");
    }
    fieldPitch->adjustSize();

}

#if DO_UPDATE_DEVICES

void MainWindow::updateDevices()
{
    const auto & inputs = devs->getInputs();
    const auto & outputs = devs->getOutputs();

    QSignalBlocker blocker(inputDevice);

    inputDevice->clear();
    inputDevice->addItem("Default input device", QVariant::fromValue(std::make_pair(true, (const ma_device_id *) nullptr)));

    for (const auto & dev : inputs) {
        const QString name = QString::fromLocal8Bit(dev.name.c_str());
        inputDevice->addItem(name, QVariant::fromValue(std::make_pair(true, &dev.id)));
    }
   
    if (ma_context_is_loopback_supported(&maCtx)) {
        for (const auto & dev : outputs) { 
            const QString name = QString::fromLocal8Bit(dev.name.c_str());
            inputDevice->addItem("(Loopback) " + name, QVariant::fromValue(std::make_pair(false, &dev.id)));
        }
    }

    inputDevice->setCurrentIndex(0);

    connect(inputDevice, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&](const int index) {
                if (index >= 0) {
                    const auto [ isInput, id ] = inputDevice->itemData(index).value<DevicePair>();

                    if (isInput || id == nullptr) {
                        analyser->setInputDevice(id);
                    }
                    else {
                        analyser->setOutputDevice(id);
                    }
                }
            });
}

#endif

#ifdef UI_SHOW_SETTINGS

void MainWindow::updateColorButtons() {
    static QString css = QStringLiteral(" \
                            QPushButton \
                            { \
                                background-color: %1; \
                                border: 1px solid #32414B; \
                                border-radius: 4px; \
                                padding: 5px; \
                                outline: none; \
                                min-width: 80px; \
                            } \
                            QPushButton:hover \
                            { \
                                border: 1px solid #148CD2; \
                            } \
                        ");

    pitchColor->setStyleSheet(css.arg(canvas->getPitchColor().name()));
    
    for (int i = 0; i < numFormants; ++i) {
        formantColors[i]->setStyleSheet(css.arg(canvas->getFormantColor(i).name()));
    }

}

#endif

void MainWindow::pressClose(QObject * obj, bool toggle)
{
    if (!toggle) return;

#ifdef UI_KEYBIND_SETTINGS_IN_DIALOG
        keybinds->close();
#endif

#ifdef UI_DISPLAY_SETTINGS_IN_DIALOG
    if (obj != displaySettings) {
        close();
    }
    else {
        displaySettings->setVisible(false);
        window()->activateWindow();
        window()->setFocus(Qt::ActiveWindowFocusReason);
    }
#else
    close();
#endif
}

void MainWindow::toggleAnalyser(QObject *, bool toggle) {
    if (!toggle) return;

    bool running = analyser->isAnalysing();

    analyser->toggle(!running);

#if defined(UI_BAR_PAUSE_LEFT) || defined(UI_BAR_PAUSE_RIGHT)
    if (running) {
        pause->setStyleSheet(stylePlay);
    } else {
        pause->setStyleSheet(stylePause);
    }
#endif
}

void MainWindow::toggleFullscreen(QObject *, bool toggle) {
    if (!toggle) return;

    bool isFullscreen = (windowState() == Qt::WindowFullScreen);

    if (isFullscreen) {
        setWindowState(Qt::WindowNoState);
    }
    else {
        setWindowState(Qt::WindowFullScreen);
    }

#ifdef UI_BAR_FULLSCREEN
    fullscreen->setChecked(!isFullscreen);
#endif
}

void MainWindow::toggleNoiseFilter(QObject *obj, bool toggle) {
    noiseFilter->setPlaying(toggle);
}

#ifdef UI_BAR_SETTINGS
void MainWindow::openSettings()
{
    QAndroidIntent intent(QtAndroid::androidActivity().object(), "fr.cloyunhee.speechanalysis.SettingsActivity");
    QtAndroid::startActivity(intent, 0);
}
#endif

void MainWindow::saveSettings() 
{
    analyser->saveSettings();
    canvas->saveSettings();
}
