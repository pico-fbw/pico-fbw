#ifndef WINDOW_H
#define WINDOW_H

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
#include <QThread>

#include "messages.h"
#include "worker.h"

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
     * @return true if the download was a sucessful, false if it failed.
     * @note This function cannot be called standalone by the user, it will be called from loadProject() when needed.
    */
    bool downloadProject(QString filePath);

    void startBuild();
    void updateBuildProgress(int progress);
    void updateBuildStep(QString step);
    void finishBuild(bool success, QString error);\

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
    
    Worker *worker;
    QThread *workerThread;
};

#endif // WINDOW_H
