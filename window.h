#ifndef __WINDOW_H
#define __WINDOW_H

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QTextEdit>

#include "resources/messages.h"

#define SETTING_MODEL "model"
#define SETTING_PROJECTDIR "projectDir"

class Window : public QMainWindow {
    Q_OBJECT

    public:
        Window();

    private slots:
        /**
         * @brief Loads a pico-fbw project from a pre-existing directory.
         * @note The steps to loading a project are:
         * 1. Check if a project directory has been saved already.
         * 2. Check if the current directory is a project directory.
         * 3. Check if the current directory is the src directory of a project directory.
         * 4. If none of the above, prompt the user to either select a project directory or download one.
         * 5. Once a project directory has been selected, load the project and save it for next time.
        */
        void loadProject();

        /**
         * @brief Downloads a pico-fbw project from GitHub.
         * @param filePath The path to save the project into.
         * @note This function cannot be called standalone by the user, it will be called from loadProject() when needed.
        */
        void downloadProject(QString filePath);

        /**
         * @brief Helper function that is called when the Git process has finished. This will update the GUI.
         * @param exitCode The exit code of the Git process.
        */
        void handleGitFinished(int exitCode);

        /**
         * @brief Builds the currently loaded project.
         * @note This will fail if the project is not loaded.
        */
        void buildProject();

        /**
         * @brief Helper function that is called when the CMake process has finished. This will update the GUI.
         * @param exitCode The exit code of the CMake process.
        */
        void handleCMakeFinished(int exitCode);
        
        /**
         * @brief Helper function to process CMake's output and provide updates.
        */
        void processCMakeOutput();

        /**
         * @brief Handles the uploading of a recently built project to a connected Pico.
         * @note This function cannot be called standalone by the user, it will be called from buildProject() when needed.
        */
        void handleUpload();

        /**
         * @brief Sets the model to build for the currently loaded project.
        */
        void setModel(Model model);

        /* Menu bar functions */

        void help();
        void licenses();
        void aboutQt();
        void about();

    private:
        /* Menu bar */
        QMenuBar *menuBar;
        QMenu *fileMenu;
        QAction *loadAction;
        QMenu *selectModelSubmenu;
        QAction *model0Action;
        QAction *model1Action;
        QAction *buildUploadAction;
        QAction *exitAction;
        QMenu *helpMenu;
        QAction *helpAction;
        QAction *licensesAction;
        QAction *aboutQtAction;
        QAction *aboutAction;

        /* Widgets */
        QTextEdit *textEditor;
        QPushButton *loadButton;
        QPushButton *buildButton;
        QComboBox *picoModelSel;
        QLabel *stepLabel;
        QProgressBar *progressBar;

        /* Settings */
        QSettings settings;

        /* Project variables */
        QString projectDir;
        bool goodProjectDir = false;
        Model selectedModel = Model::MODEL0;
        
        /* Build variables */
        enum BuildState {
            Configuring,
            Building,
            Finished
        };
        BuildState buildState = BuildState::Finished;
        QProcess git;
        QProcess cmake;
};

#endif // __WINDOW_H
