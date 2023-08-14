#include <QAbstractButton>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
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

Window::Window() : settings("pico-fbw", APP_NAME) {
    setWindowTitle(APP_NAME);

    // Create the menu bar
    menuBar = new QMenuBar(this);

    // File menu
    fileMenu = new QMenu("File", this);
    loadAction = new QAction(BTN_LOAD_PROJECT, this);
    loadAction->setShortcut(QKeySequence::Open);
    loadAction->setToolTip(TOOLTIP_LOAD_PROJECT);
    fileMenu->addAction(loadAction);
    fileMenu->addSeparator();
    selectModelSubmenu = new QMenu(FIELD_SELECT_MODEL, this);
    model0Action = new QAction(MODEL0_NAME, this);
    model1Action = new QAction(MODEL1_NAME, this);
    selectModelSubmenu->setToolTip(TOOLTIP_SELECT_MODEL);
    selectModelSubmenu->addAction(model0Action);
    selectModelSubmenu->addAction(model1Action);
    fileMenu->addMenu(selectModelSubmenu);
    fileMenu->addSeparator();
    buildUploadAction = new QAction(BTN_BUILD_UPLOAD, this);
    buildUploadAction->setShortcut(QKeySequence::Save);
    buildUploadAction->setToolTip(TOOLTIP_BUILD_UPLOAD);
    fileMenu->addAction(buildUploadAction);
    fileMenu->addSeparator();
    exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    fileMenu->addAction(exitAction);

    // Help menu
    helpMenu = new QMenu("Help", this);
    helpAction = new QAction("Help", this);
    helpAction->setShortcut(QKeySequence::HelpContents);
    helpMenu->addAction(helpAction);
    helpMenu->addSeparator();
    licensesAction = new QAction("Licenses", this);
    helpMenu->addAction(licensesAction);
    helpMenu->addSeparator();
    aboutQtAction = new QAction("About Qt", this);
    helpMenu->addAction(aboutQtAction);
    aboutAction = new QAction("About", this);
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
    textEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // FIXME: doesn't show up?
    textEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEditor->setLineWrapMode(QTextEdit::NoWrap);
    textEditor->setReadOnly(true); // Set as read-only until the config file is loaded
    mainLayout->addWidget(textEditor);

    loadButton = new QPushButton(BTN_LOAD_PROJECT, this);
    loadButton->setToolTip(TOOLTIP_LOAD_PROJECT);
    mainLayout->addWidget(loadButton);

    picoModelSel = new QComboBox(this);
    picoModelSel->setToolTip(TOOLTIP_SELECT_MODEL);
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

    buildButton = new QPushButton(BTN_BUILD_UPLOAD, this);
    buildButton->setToolTip(TOOLTIP_BUILD_UPLOAD);
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
    setModel((Model)settings.value("model").toInt());

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
}

void Window::loadProject() {
    goodProjectDir = false;
    QFile configFile;

    // Try to find config.h, assuming we are in the root "pico-fbw" directory
    QString filePathRoot = QDir::currentPath() + "/config.h";
    QString filePathSrc = QDir::currentPath() + "/src/config.h";
    QFile configFileRoot(filePathRoot);
    QFile configFileSrc(filePathSrc);
    if (configFileRoot.exists()) {
        // Config file exists in root directory
        projectDir = QDir::currentPath();
    } else if (configFileSrc.exists()) {
        // Config file exists in src directory (one down from root)
        projectDir = QDir::currentPath() + "/../";
    } else {
        // Config could not be automatically found
        // First check so see if we have a project directory saved in settings, if not, prompt user to select or download one
        if (settings.contains("projectDir")) {
            projectDir = settings.value("projectDir").toString();
        } else {
            QMessageBox promptCreateSelect(this);
            promptCreateSelect.setWindowTitle("Project Not Found");
            promptCreateSelect.setText("A valid pico-fbw project could not be found.<br><br>What would you like to do?");
            promptCreateSelect.setIcon(QMessageBox::Question);

            QPushButton *createButton = promptCreateSelect.addButton(tr("Create New Project"), QMessageBox::ActionRole);
            QPushButton *selectButton = promptCreateSelect.addButton(tr("Select Existing Project"), QMessageBox::ActionRole);
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
                    projectDir = QFileDialog::getExistingDirectory(this, "Create a project directory");
                    if (projectDir.isEmpty()) {
                        // User canceled directory selection
                        return;
                    } else {
                        if (!downloadProject(projectDir)) {
                            return;
                        }
                    }
                    break;
                case SelectButtonRole:
                    projectDir = QFileDialog::getExistingDirectory(this, "Select existing pico-fbw directory");
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
        QMessageBox::critical(this, "Error", "Config file not found!");
        // Clear the project directory from settings to ensure we don't try to load it again (infinite loop)
        settings.remove("projectDir");
        return;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to open config file!");
        settings.remove("projectDir");
        return;
    }

    // Read the config file content and send it to the text editor
    QTextStream in(&configFile);
    textEditor->setPlainText(in.readAll());
    // Now we can set the text editor as editable
    textEditor->setReadOnly(false);
    // Set flag to indicate we have a good project directory
    goodProjectDir = true;

    // Save project location to settings
    settings.setValue("projectDir", projectDir);

    configFile.close();
}

bool Window::downloadProject(QString filePath) {
    QStringList gitArgs;
    gitArgs << "clone" << "https://github.com/MylesAndMore/pico-fbw.git" << filePath << "--branch" << "alpha";
    QProcess git;
    git.start("git", gitArgs);
    stepLabel->setText("<b>Downloading...</b>");
    // Connect to the process's output so we can have a progress bar
    connect(&git, SIGNAL(readyReadStandardOutput()), this, SLOT(processGitOutput()));
    // Wait until either the process finishes or we timeout (20 second timeout for configuration step)
    bool timedOut = !git.waitForFinished(30 * 1000);
    // Disconnect from the output so we don't parse any unnecessary lines
    disconnect(&git, SIGNAL(readyReadStandardOutput()), this, SLOT(processGitOutput()));
    stepLabel->setText("");

    if (git.exitCode() != 0) {
        QMessageBox eMsg(QMessageBox::Critical, "Download Failed", "Failed to download project!<br>"
                                                                   "Ensure you have all necessary components installed and the directory you selected is empty.",
                                                                   QMessageBox::Ok, this);
        eMsg.setDetailedText(git.readAllStandardError());
        eMsg.exec();
        return false;
    } else if (timedOut) {
        QMessageBox::critical(this, "Download Failed", "Failed to download project!<br><br>Timed out after 30 seconds.");
        return false;
    } else {
        return true;
    }
}

void Window::buildProject() {
    if (!goodProjectDir) {
        QMessageBox::critical(this, "Error", "Invalid project!");
        return;
    }

    QStringList confArgs;
    confArgs << "-B" << "build_pfc" << "-DPICO_SDK_FETCH_FROM_GIT=ON" << "-DCMAKE_BUILD_TYPE=Release"; // Arguments required for all models

    // Create the correct cmake arguments for the selected model
    switch (selectedModel) {
        case MODEL0:
            confArgs << MODEL0_ARGS;
            break;
        case MODEL1:
            confArgs << MODEL1_ARGS;
            break;
        default:
            QMessageBox::critical(this, "Error", "Invalid model!");
            return;
    }

    // Save the current content of the text editor into the config file
    QString configFilePath = projectDir + "/src/config.h";
    QFile configFile(configFilePath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << textEditor->toPlainText(); // Save the edited configuration
        configFile.close();
    } else {
        QMessageBox::critical(this, "Error", "Failed to save config file!");
        return;
    }

    // Spawn a process to configure the build directory for building (aka generate Makefiles)
    cmake.setWorkingDirectory(projectDir);
    cmake.start("cmake", confArgs);
    connect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
    bool timedOut = !cmake.waitForFinished(20 * 1000); // 20 second timeout for configuration step
    disconnect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));

    if (cmake.exitCode() != 0) {
        QMessageBox eMsg(QMessageBox::Critical, "Configuration Failed", "Build process failed during configuration step!<br>"
                                                                        "Ensure you have all necessary components installed and have selected the correct directory.",
                                                                        QMessageBox::Ok, this);
        eMsg.setDetailedText(cmake.readAllStandardError());
        eMsg.exec();
        return;
    } else if (timedOut) {
        QMessageBox::critical(this, "Configuration Failed", "Build process failed during configuration step!<br><br>Timed out after 20 seconds.");
        return;
    }

    // CMake's configuration step succeeded, now build the project
    QStringList buildArgs;
    buildArgs << "--build" << "build_pfc";
    cmake.start("cmake", buildArgs);
    connect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
    timedOut = !cmake.waitForFinished(100 * 1000); // 100 second timeout for build step
    disconnect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));

    if (cmake.exitCode() != 0) {
        QMessageBox eMsg(QMessageBox::Critical, "Build Failed", "Build process failed during build step!", QMessageBox::Ok, this);
        eMsg.setDetailedText(cmake.readAllStandardError());
        eMsg.exec();
        return;
    } else if (timedOut) {
        QMessageBox::critical(this, "Build Failed", "Build process failed during build step!<br><br>Timed out after 80 seconds.");
        return;
    }

    // Build was also successful, prompt the user if they want to upload the firmware
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Build Successful", "Build process was successful!<br><br>"
                                                                    "Would you like to upload the firmware now?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        handleUpload();
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
            return;
        }
    }

    // Look for indications of build steps
    QString dependencyPrefix = "Scanning dependencies of target ";
    int dependencyIndex = outputStr.indexOf(dependencyPrefix);
    if (dependencyIndex != -1) {
        // Extract the target name substring
        int targetStartIndex = dependencyIndex + dependencyPrefix.length();
        int targetEndIndex = outputStr.indexOf('\n', targetStartIndex);
        if (targetEndIndex != -1) {
            QString targetName = outputStr.mid(targetStartIndex, targetEndIndex - targetStartIndex).trimmed();
            // Update the step label with the target name
            stepLabel->setText("<b>Building: </b>" + targetName);
            return;
        }
    }
}

void Window::handleUpload() {
    // Check if the build directory contains the uf2 file we need
    QFile uf2(projectDir + "/build_pfc/pico-fbw.uf2");
    if (!uf2.exists()) {
        QMessageBox::critical(this, "Upload Failed", "Binary file not found!");
        return;
    }

    // Load the uf2 onto the Pico and check for errors
    progressBar->setValue(0);
    int status = picolite_load(uf2.fileName().toUtf8().constData());
    const char *errorMsg = nullptr;
    switch (status) {
        case 0:
            progressBar->setValue(100);
            break;
        case PICOLITE_ERROR_FORMAT:
        case PICOLITE_ERROR_INCOMPATIBLE:
            errorMsg = "Invalid binary file!";
            break;
        case PICOLITE_ERROR_READ_FAILED:
            errorMsg = "Failed to read binary file!";
            break;
        case PICOLITE_ERROR_USB:
            errorMsg = "Failed to connect to Pico!";
            break;
        case PICOLITE_ERROR_NO_DEVICE:
            errorMsg = "No Pico was detected.";
            break;
        case PICOLITE_ERROR_CONNECTION:
            errorMsg = "Communication with Pico failed.";
            break;
        case PICOLITE_ERROR_VERIFICATION_FAILED:
            errorMsg = "Verification of binary failed.";
            break;
        case PICOLITE_ERROR_PERMISSIONS:
            errorMsg = "A Pico was detected but the application was unable to communicate with it. This is likely a permission issue.";
            break;
        case PICOLITE_ERROR_DRIVER:
            errorMsg = "A Pico was detected but the application was unable to communicate with it. This is likely a driver issue.";
            break;
        case PICOLITE_ERROR_BOOTSEL:
            errorMsg = "A Pico was detected but the application was unable to set it into BOOTSEL mode. Reboot the Pico with the BOOTSEL button pressed and try again.";
            break;
        case PICOLITE_ERROR_UNKNOWN:
        default:
            errorMsg = "Unknown error.";
    }

    if (!errorMsg) {
        QMessageBox::information(this, "Upload Successful!", "Upload was successful!");
    } else {
        QMessageBox eMsg(QMessageBox::Critical, "Upload Failed!", QString("Error: %1").arg(errorMsg), QMessageBox::Ok, this);
        eMsg.setDetailedText(PICOLITE_ERROR_msg);
        eMsg.exec();

        if (status != PICOLITE_ERROR_NO_DEVICE) {
            QMessageBox::StandardButton uploadButton =
            QMessageBox::question(this, "Manual Upload", "Would you like to upload the file manually?",
                                QMessageBox::Yes | QMessageBox::No);

            if (uploadButton == QMessageBox::Yes) {
                QString buildFolderPath = projectDir + "/build_pfc/";
                QDesktopServices::openUrl(QUrl::fromLocalFile(buildFolderPath));
            }
        }
    }

    // TODO: privelage escelation for picolite_load?
}

void Window::setModel(Model model) {
    // Update internally
    selectedModel = model;
    // Update dropdown
    picoModelSel->setCurrentIndex((int)model);
    // Update cache (settings)
    settings.setValue("model", (int)model);
}


void Window::help() {
    QString helpText = QString("This application allows you to easily configure, build, and upload the pico-fbw firmware.<br><br>"
                       "<b>%1</b>: %2<br><br>"
                       "<b>%3</b>: %4<br><br>"
                       "<b>%5</b>: %6<br><br>"
                       "<br>For more information, refer to the online documentation at <a href=\"https://pico-fbw.org\">pico-fbw.org</a>.")
                       .arg(BTN_LOAD_PROJECT)
                       .arg(TOOLTIP_LOAD_PROJECT)
                       .arg(FIELD_SELECT_MODEL)
                       .arg(TOOLTIP_SELECT_MODEL)
                       .arg(BTN_BUILD_UPLOAD)
                       .arg(TOOLTIP_BUILD_UPLOAD);

    QMessageBox helpPopup(this);
    helpPopup.setWindowTitle("Help");
    helpPopup.setTextFormat(Qt::RichText);
    helpPopup.setText(helpText);
    helpPopup.setIcon(QMessageBox::Information);
    helpPopup.setStandardButtons(QMessageBox::Ok);
    helpPopup.exec();
}

void Window::licenses() {
    QString licenseText = "This application uses the Qt framework, which is licensed under the GNU GPL-3.0 license.<br><br>"
                          "The application itself is also licensed under the GNU GPL-3.0 license.<br><br>"
                          "Parts of this application utilize code from picotool by Raspberry Pi, which is licensed under the BSD 3-Clause license.<br><br>"
                          "For more details, you can review the license texts:<br>"
                          "<a href=\"https://www.gnu.org/licenses/gpl-3.0.en.html\">GNU GPL-3.0 License</a><br>"
                          "<a href=\"https://opensource.org/licenses/BSD-3-Clause\">BSD 3-Clause License</a>";

    QMessageBox licensePopup(this);
    licensePopup.setWindowTitle("Licenses");
    licensePopup.setTextFormat(Qt::RichText);
    licensePopup.setText(licenseText);
    licensePopup.setIcon(QMessageBox::Information);
    licensePopup.setStandardButtons(QMessageBox::Ok);
    licensePopup.exec();
}

void Window::aboutQt() {
    QMessageBox::aboutQt(this, "About Qt");
}

void Window::about() {
    QString aboutText = QString("<b>%1</b> version %2_%3<br><br>"
                                "This application allows you to easily configure, build, and upload the pico-fbw firmware.<br><br>"
                                "pico-fbw and this application are created by MylesAndMore and are licensed under the GNU GPL-3.0 license.<br><br>"
                                "For more information and source code, visit <a href=\"https://pico-fbw.org\">pico-fbw.org</a> or "
                                "the <a href=\"https://github.com/MylesAndMore/pico-fbw\">GitHub repository</a>.")
                                .arg(APP_NAME)
                                .arg(APP_VERSION)
                                .arg(QSysInfo::productType());

    QMessageBox aboutPopup(this);
    QMessageBox::about(this, QString("About %1").arg(APP_NAME), aboutText);
}
