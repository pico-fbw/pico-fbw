#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QSysInfo>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "picolite/picolite.h"
#include "version.h"

#include "window.h"

enum ButtonRoles {
    CreateButtonRole = QMessageBox::ActionRole + 1,
    SelectButtonRole,
    CancelButtonRole
};

// TODO: change to higher resolution image in resources; looks kind of bad on mac

Window::Window() : settings("pico-fbw", APP_NAME) {
    setWindowTitle(APP_NAME);

    // Create the menu bar
    menuBar = new QMenuBar(this);

    // File menu
    fileMenu = new QMenu("File", this);
    loadAction = new QAction(FIELD_LOAD_PROJECT, this);
    loadAction->setShortcut(QKeySequence::Open);
    loadAction->setToolTip(TIP_LOAD_PROJECT);
    fileMenu->addAction(loadAction);
    fileMenu->addSeparator();
    selectModelSubmenu = new QMenu(FIELD_SELECT_MODEL, this);
    model0Action = new QAction(MODEL0_NAME, this);
    model1Action = new QAction(MODEL1_NAME, this);
    selectModelSubmenu->setToolTip(TIP_SELECT_MODEL);
    selectModelSubmenu->addAction(model0Action);
    selectModelSubmenu->addAction(model1Action);
    fileMenu->addMenu(selectModelSubmenu);
    fileMenu->addSeparator();
    buildUploadAction = new QAction(FIELD_BUILD_UPLOAD, this);
    buildUploadAction->setShortcut(QKeySequence::Save);
    buildUploadAction->setToolTip(TIP_BUILD_UPLOAD);
    fileMenu->addAction(buildUploadAction);
    fileMenu->addSeparator();
    exitAction = new QAction(FIELD_EXIT, this);
    exitAction->setShortcut(QKeySequence::Quit);
    fileMenu->addAction(exitAction);

    // Help menu
    helpMenu = new QMenu(FIELD_HELP, this);
    helpAction = new QAction(FIELD_HELP, this);
    helpAction->setShortcut(QKeySequence::HelpContents);
    helpMenu->addAction(helpAction);
    helpMenu->addSeparator();
    licensesAction = new QAction(FIELD_LICENSES, this);
    helpMenu->addAction(licensesAction);
    helpMenu->addSeparator();
    aboutQtAction = new QAction(FIELD_ABOUTQT, this);
    helpMenu->addAction(aboutQtAction);
    aboutAction = new QAction(FIELD_ABOUT, this);
    helpMenu->addAction(aboutAction);
    helpMenu->addSeparator();

    // Connect actions to their respective methods
    connect(loadAction, &QAction::triggered, this, &Window::loadProject);
    connect(model0Action, &QAction::triggered, this, [this]() { setModel(Model::MODEL0); });
    connect(model1Action, &QAction::triggered, this, [this]() { setModel(Model::MODEL1); });
    connect(buildUploadAction, &QAction::triggered, this, &Window::buildProject);
    connect(exitAction, &QAction::triggered, this, &Window::close);

    connect(helpAction, &QAction::triggered, this, &Window::help);
    connect(licensesAction, &QAction::triggered, this, &Window::licenses);
    connect(aboutQtAction, &QAction::triggered, this, &Window::aboutQt);
    connect(aboutAction, &QAction::triggered, this, &Window::about);

    menuBar->addMenu(fileMenu);
    menuBar->addMenu(helpMenu);
    setMenuBar(menuBar);

    // Create the main layout and add widgets
    QVBoxLayout *mainLayout = new QVBoxLayout();

    // TODO: maybe instead of a text editor, parse the file and have easily changeable widgets
    textEditor = new QTextEdit(this);
    textEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // FIXME: doesn't show up? (although should not be a problem if ^ is done)
    textEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEditor->setLineWrapMode(QTextEdit::NoWrap);
    textEditor->setReadOnly(true); // Set as read-only until the config file is loaded
    mainLayout->addWidget(textEditor);

    loadButton = new QPushButton(FIELD_LOAD_PROJECT, this);
    loadButton->setToolTip(TIP_LOAD_PROJECT);
    mainLayout->addWidget(loadButton);

    picoModelSel = new QComboBox(this);
    picoModelSel->setToolTip(TIP_SELECT_MODEL);
    picoModelSel->setEditable(true);
    picoModelSel->lineEdit()->setReadOnly(true);
    picoModelSel->lineEdit()->setAlignment(Qt::AlignCenter);
    picoModelSel->lineEdit()->setStyleSheet("margin-left: 19px;"); // centering is broken without this for some reason
    picoModelSel->addItem(MODEL0_NAME);
    picoModelSel->addItem(MODEL1_NAME);
    for (int i = 0 ; i < picoModelSel->count(); i++) {
        picoModelSel->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
    }
    mainLayout->addWidget(picoModelSel);

    buildButton = new QPushButton(FIELD_BUILD_UPLOAD, this);
    buildButton->setToolTip(TIP_BUILD_UPLOAD);
    mainLayout->addWidget(buildButton);

    stepLabel = new QLabel(this);
    stepLabel->setAlignment(Qt::AlignCenter);
    stepLabel->setObjectName("stepLabel");
    mainLayout->addWidget(stepLabel);

    progressBar = new QProgressBar(this);
    mainLayout->addWidget(progressBar);

    // Connect buttons to their various methods
    connect(loadButton, &QPushButton::clicked, this, &Window::loadProject);
    connect(buildButton, &QPushButton::clicked, this, &Window::buildProject);
    // Connect dropdown
    connect(picoModelSel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && index < picoModelSel->count()) {
            QString modelName = picoModelSel->itemText(index);
            if (modelName == MODEL0_NAME) {
                setModel(Model::MODEL0);
            } else if (modelName == MODEL1_NAME) {
                setModel(Model::MODEL1);
            }
        }
    });
    // Load in dropdown/model default from settings
    setModel((Model)settings.value(SETTING_MODEL).toInt());

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
}

void Window::loadProject() {
    goodProjectDir = false;
    QFile configFile;
    QString filePathRoot = QDir::currentPath() + "/config.h";
    QString filePathSrc = QDir::currentPath() + "/src/config.h";
    QFile configFileRoot(filePathRoot);
    QFile configFileSrc(filePathSrc);
    // Check if the Shift key is held down; this will bypass all checks and go straight to select/download
    bool shiftPressed = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    if (shiftPressed) {
        goto shiftOverride;
    }
    // Try to find config.h, assuming we are in the root "pico-fbw" directory
    if (configFileRoot.exists()) {
        // Config file exists in root directory
        projectDir = QDir::currentPath();
    } else if (configFileSrc.exists()) {
        // Config file exists in src directory (one down from root)
        projectDir = QDir::currentPath() + "/../";
    } else {
        // Config could not be automatically found
        // First check so see if we have a project directory saved in settings, if not, prompt user to select or download one
        if (settings.contains(SETTING_PROJECTDIR)) {
            projectDir = settings.value(SETTING_PROJECTDIR).toString();
        } else {
            shiftOverride:
                QMessageBox promptCreateSelect(this);
                promptCreateSelect.setWindowTitle(ERR_PROJNOTFOUND);
                promptCreateSelect.setText(TIP_PROJNOTFOUND);
                promptCreateSelect.setIcon(QMessageBox::Question);

                QPushButton *createButton = promptCreateSelect.addButton(tr(TIP_NEWPROJ), QMessageBox::ActionRole);
                QPushButton *selectButton = promptCreateSelect.addButton(tr(TIP_OPENPROJ), QMessageBox::ActionRole);
                QPushButton *cancelButton = promptCreateSelect.addButton(QMessageBox::Cancel);
                createButton->setProperty("customRole", CreateButtonRole);
                selectButton->setProperty("customRole", SelectButtonRole);
                cancelButton->setProperty("customRole", CancelButtonRole);
                promptCreateSelect.setDefaultButton(createButton);
                promptCreateSelect.exec();

                QAbstractButton *clickedButton = promptCreateSelect.clickedButton();
                int clickedButtonRole = clickedButton->property("customRole").toInt();

                switch (clickedButtonRole) {
                    case CreateButtonRole:
                        projectDir = QFileDialog::getExistingDirectory(this, TIP_NEWPROJ);
                        if (projectDir.isEmpty()) {
                            return; // User canceled directory selection
                        } else {
                            downloadProject(projectDir);
                            return; // We will continue with the load once git is finished downloading
                        }
                        break;
                    case SelectButtonRole:
                        projectDir = QFileDialog::getExistingDirectory(this, TIP_OPENPROJ);
                        if (projectDir.isEmpty()) return;
                        break;
                    case CancelButtonRole:
                    default:
                        return;
                }
        }
    }

    // Load in the config
    QString filePath = projectDir + "/src/config.h";
    configFile.setFileName(filePath);

    if (!configFile.exists()) {
        // The config file still doesn't exist (likely not the directory we're expecting)
        QMessageBox::critical(this, MSG_ERR, ERR_CFGNOTFOUND);
        // Clear the project directory from settings to ensure we don't try to load it again (infinite loop)
        settings.remove(SETTING_PROJECTDIR);
        return;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, MSG_ERR, ERR_CFGFAILEDOPEN);
        settings.remove(SETTING_PROJECTDIR);
        return;
    }

    // Read the config file content and send it to the text editor
    QTextStream in(&configFile);
    textEditor->setPlainText(in.readAll());
    // Now we can set the text editor as editable
    textEditor->setReadOnly(false);
    // Set flag to indicate we have a good project directory
    goodProjectDir = true;

    // Save project location to settings (not if there is a shift override though, that is only to load something temp)
    if (!shiftPressed) {
        settings.setValue(SETTING_PROJECTDIR, projectDir);
    }

    configFile.close();
}

void Window::downloadProject(QString filePath) {
    QStringList gitArgs;
    gitArgs << "clone" << "https://github.com/MylesAndMore/pico-fbw.git" << filePath;
    git.start("git", gitArgs);
    stepLabel->setText(STEP_DOWNLOAD);
    // Connect a signal so that we can display feedback once git has finished downloading
    connect(&git, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(handleGitFinished(int)));
}

void Window::handleGitFinished(int exitCode) {
    if (git.exitCode() != 0) {
        QMessageBox eMsg(QMessageBox::Critical, ERR_DOWNLOADFAIL, TIP_DOWNLOADFAIL, QMessageBox::Ok, this);
        eMsg.setDetailedText(git.readAllStandardError());
        eMsg.exec();
        stepLabel->setText("");
    } else {
        stepLabel->setText(STEP_DOWNLOADFINISHED);
        // Git suceeded; load the downloaded project (folder already stored in projectDir from last time so loadProject() should be happy)
        loadProject();
    }
}

void Window::buildProject() {
    if (!goodProjectDir) {
        // Attempt to load the project and try once again
        loadProject();
        if (!goodProjectDir) {
            QMessageBox::critical(this, MSG_ERR, ERR_PROJINVALID);
            return;
        }
    }

    // Save the current content of the text editor into the config file
    QString configFilePath = projectDir + "/src/config.h";
    QFile configFile(configFilePath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << textEditor->toPlainText(); // Save the edited configuration
        configFile.close();
    } else {
        QMessageBox::critical(this, MSG_ERR, ERR_CFGFAILEDWRITE);
        return;
    }

    // Create the correct cmake arguments for the selected model
    QStringList confArgs;
    confArgs << "-B" << "build_pfc" << "-DPICO_SDK_FETCH_FROM_GIT=ON" << "-DCMAKE_BUILD_TYPE=Release"; // Arguments required for all models
    switch (selectedModel) {
        case MODEL0:
            confArgs << MODEL0_ARGS;
            break;
        case MODEL1:
            confArgs << MODEL1_ARGS;
            break;
        default:
            QMessageBox::critical(this, MSG_ERR, ERR_MODELINVALID);
            return;
    }

    // Spawn a process to configure the build directory for building
    buildState = BuildState::Configuring;
    buildButton->setEnabled(false); // Disable the build button while we're building
    cmake.setWorkingDirectory(projectDir);
    // Connect a signal so that we can process the output of CMake to display graphical progress to the user
    connect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
    connect(&cmake, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(handleCMakeFinished(int)));
    cmake.start("cmake", confArgs);
}

void Window::handleCMakeFinished(int exitCode) {
    // Disconnect the signals so we don't get any unnecessary functions triggered
    disconnect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
    disconnect(&cmake, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(handleCMakeFinished(int)));
    buildButton->setEnabled(true); // Re-enable the build button
    switch (buildState) {
        case BuildState::Configuring:
            if (cmake.exitCode() != 0) {
                QMessageBox eMsg(QMessageBox::Critical, ERR_BUILDFAIL, TIP_BUILDFAIL_CONFIG, QMessageBox::Ok, this);
                eMsg.setDetailedText(cmake.readAllStandardError());
                eMsg.exec();
                return;
            } else {
                // CMake's configuration step succeeded, now build the project
                buildState = BuildState::Building;
                buildButton->setEnabled(false);
                QStringList buildArgs;
                buildArgs << "--build" << "build_pfc";
                connect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
                connect(&cmake, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(handleCMakeFinished(int)));
                cmake.start("cmake", buildArgs);
            }
            break;
        case BuildState::Building:
            if (cmake.exitCode() != 0) {
                buildState = BuildState::Configuring;
                QMessageBox eMsg(QMessageBox::Critical, ERR_BUILDFAIL, TIP_BUILDFAIL_BUILD, QMessageBox::Ok, this);
                eMsg.setDetailedText(cmake.readAllStandardError());
                eMsg.exec();
                return;
            } else {
                buildState = BuildState::Finished;
                stepLabel->setText(STEP_BUILDFINISHED);
                // Build was successful, prompt the user if they want to upload the firmware
                QMessageBox::StandardButton reply = QMessageBox::question(this, MSG_BUILDSUCCESS, TIP_BUILDSUCCESS_UPLOAD, QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    handleUpload();
                }
            }
            break;
        default:
            break;
    }
}

void Window::processCMakeOutput() {
    // Fetch the current output from the CMake process
    QByteArray output = cmake.readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);

    int percentIndex = outputStr.indexOf("%");
    if (percentIndex != -1 && percentIndex >= 3) {
        QString progressStr = outputStr.mid(percentIndex - 3, 3);
        bool ok;
        int progress = progressStr.toInt(&ok);
        if (ok) {
            progressBar->setValue(progress);
        }
    }

    // Look for indications of build steps
    QString dependencyPrefix = "Scanning dependencies of target ";
    int dependencyIndex = outputStr.indexOf(dependencyPrefix);
    if (dependencyIndex == -1) {
        // Try another prefix
        dependencyPrefix = "Built target ";
        dependencyIndex = outputStr.indexOf(dependencyPrefix);
        if (dependencyIndex == -1) {
            return;
        }
    }
    // Extract the target name substring
    int targetStartIndex = dependencyIndex + dependencyPrefix.length();
    int targetEndIndex = outputStr.indexOf('\n', targetStartIndex);
    if (targetEndIndex != -1) {
        QString targetName = outputStr.mid(targetStartIndex, targetEndIndex - targetStartIndex).trimmed();
        // Update the step label with the target name
        stepLabel->setText(STEP_BUILD + targetName);
    }
}

void Window::handleUpload() {
    // Check if the build directory contains the uf2 file we need
    QFile uf2(projectDir + "/build_pfc/pico-fbw.uf2");
    if (!uf2.exists()) {
        QMessageBox::critical(this, ERR_UPLOADFAIL, TIP_UPLOADBNOTFOUND);
        return;
    }

    // Load the uf2 onto the Pico and check for errors
    progressBar->setValue(0);
    stepLabel->setText("");
    int status = picolite_load(uf2.fileName().toUtf8().constData());
    const char *errorMsg = nullptr;
    switch (status) {
        case 0:
            progressBar->setValue(100);
            break;
        case PICOLITE_ERROR_FORMAT:
        case PICOLITE_ERROR_INCOMPATIBLE:
            errorMsg = UERR_INVALIDB;
            break;
        case PICOLITE_ERROR_READ_FAILED:
            errorMsg = UERR_READFAIL;
            break;
        case PICOLITE_ERROR_USB:
            errorMsg = UERR_CONNECTFAIL;
            break;
        case PICOLITE_ERROR_NO_DEVICE:
            errorMsg = UERR_NODETECT;
            break;
        case PICOLITE_ERROR_CONNECTION:
            errorMsg = UERR_CONNECT;
            break;
        case PICOLITE_ERROR_VERIFICATION_FAILED:
            errorMsg = UERR_VERIFY;
            break;
        case PICOLITE_ERROR_PERMISSIONS:
            errorMsg = UERR_PERMS;
            break;
        case PICOLITE_ERROR_DRIVER:
            errorMsg = UERR_DRIVER;
            break;
        case PICOLITE_ERROR_BOOTSEL:
            errorMsg = UERR_BOOTSEL;
            break;
        case PICOLITE_ERROR_UNKNOWN:
        default:
            errorMsg = UERR_UNKNOWN;
    }

    if (!errorMsg) {
        QMessageBox::information(this, MSG_UPLOADSUCCESS, MSG_UPLOADSUCCESS);
    } else {
        QMessageBox eMsg(QMessageBox::Critical, ERR_UPLOADFAIL, QString(MSG_ERR) + QString(": %1").arg(errorMsg), QMessageBox::Ok, this);
        eMsg.setDetailedText(PICOLITE_ERROR_msg);
        eMsg.exec();

        if (status != PICOLITE_ERROR_NO_DEVICE) {
            QMessageBox::StandardButton uploadButton =
            QMessageBox::question(this, Q_MANUALUPLOAD, MSG_MANUALUPLOAD, QMessageBox::Yes | QMessageBox::No);
            if (uploadButton == QMessageBox::Yes) {
                QString buildFolderPath = projectDir + "/build_pfc/";
                QDesktopServices::openUrl(QUrl::fromLocalFile(buildFolderPath));
            }
        }
    }
}

void Window::setModel(Model model) {
    // Update internally
    selectedModel = model;
    // Update dropdown
    picoModelSel->setCurrentIndex((int)model);
    // Update cache (settings)
    settings.setValue(SETTING_MODEL, (int)model);
}

void Window::help() {
    QString helpText = QString(MSG_HELP)
                       .arg(FIELD_LOAD_PROJECT)
                       .arg(TIP_LOAD_PROJECT)
                       .arg(FIELD_SELECT_MODEL)
                       .arg(TIP_SELECT_MODEL)
                       .arg(FIELD_BUILD_UPLOAD)
                       .arg(TIP_BUILD_UPLOAD);

    QMessageBox helpPopup(this);
    helpPopup.setWindowTitle(FIELD_HELP);
    helpPopup.setTextFormat(Qt::RichText);
    helpPopup.setText(helpText);
    helpPopup.setIcon(QMessageBox::Information);
    helpPopup.setStandardButtons(QMessageBox::Ok);
    helpPopup.exec();
}

void Window::licenses() {
    QString licenseText = MSG_LICENSE;

    QMessageBox licensePopup(this);
    licensePopup.setWindowTitle(FIELD_LICENSES);
    licensePopup.setTextFormat(Qt::RichText);
    licensePopup.setText(licenseText);
    licensePopup.setIcon(QMessageBox::Information);
    licensePopup.setStandardButtons(QMessageBox::Ok);
    licensePopup.exec();
}

void Window::aboutQt() {
    QMessageBox::aboutQt(this, FIELD_ABOUTQT);
}

void Window::about() {
    QString aboutText = QString(MSG_ABOUT)
                                .arg(APP_NAME)
                                .arg(APP_VERSION)
                                .arg(QSysInfo::productType());

    QMessageBox aboutPopup(this);
    QMessageBox::about(this, QString(FIELD_ABOUT) + QString(" %1").arg(APP_NAME), aboutText);
}