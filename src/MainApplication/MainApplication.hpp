#include <chrono>
#include <memory>
#include <vector>
#include <QApplication>

#include <Core/Time/Timer.hpp>
#include <MainApplication/TimerData/FrameTimerData.hpp>
#include <MainApplication/Viewer/Viewer.hpp>

class QTimer;
namespace Ra
{
    namespace Core
    {
        class TaskQueue;
    }
}

namespace Ra
{
    namespace Engine
    {
        class RadiumEngine;
    }
}

namespace Ra
{
    namespace Gui
    {
        class Viewer;
        class MainWindow;
    }
}

/// Allow singleton-like access to the main app à la qApp.
#if defined(mainApp)
#undef mainApp
#endif
#define mainApp (static_cast<Ra::MainApplication*>(qApp))

namespace Ra
{
    /// This class contains the main application logic. It owns the engine and the GUI.
    class MainApplication : public QApplication
    {
        Q_OBJECT

    public:
        MainApplication( int argc, char** argv );
        ~MainApplication();

        /// Advance the engine for one frame.
        void radiumFrame();

        bool isRunning() const { return !m_isAboutToQuit; }

    signals:
        /// Fired when the engine has just started, before the frame timer is set.
        void starting();

        /// Fired when the engine is about to stop.
        void stopping();

        /// Fired when the scene has changed.
        void sceneChanged( const Core::Aabb& );

        void updateFrameStats( const std::vector<FrameTimerData>& );

        void endFrame();

    public slots:

        void loadFile( QString path );
        void framesCountForStatsChanged( int count );
        void appNeedsToQuit();

    private:
        /// Create signal / slots connections
        void createConnections();

        /// Temporary methods ?
        void registerSystems();
        void setupScene();


        // Public variables, accessible through the mainApp singleton.
    public:
        /// Application main window and GUI root class.
        std::unique_ptr<Gui::MainWindow> m_mainWindow;

        /// Instance of the radium engine.
        std::unique_ptr<Engine::RadiumEngine> m_engine;

        /// Task queue for processing tasks.
        std::unique_ptr<Core::TaskQueue> m_taskQueue;

    private:
        /// Pointer to OpenGL Viewer for render call (belongs to MainWindow).
        Gui::Viewer* m_viewer;

        /// Timer to wake us up at every frame start.
        QTimer* m_frameTimer;

        /// Time since the last frame start.
        Core::Timer::TimePoint m_lastFrameStart;

        int m_frameCounter;
        int m_frameCountBeforeUpdate;
        std::vector<FrameTimerData> m_timerData;

        bool m_isAboutToQuit;
    };
}