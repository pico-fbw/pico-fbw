#include <QByteArray>

#include "worker.h"

Worker::Worker() {}

void Worker::buildProject(const QStringList &confArgs,  const QString &projectDir) {
    // Spawn a process to configure the build directory for building (aka generate Makefiles)
    cmake.setWorkingDirectory(projectDir);
    cmake.start("cmake", confArgs);
    connect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));
    bool timedOut = !cmake.waitForFinished(60 * 1000); // 60 second timeout for configuration step; keep in mind it may have to download pico sdk at this step
    disconnect(&cmake, SIGNAL(readyReadStandardOutput()), this, SLOT(processCMakeOutput()));

    if (cmake.exitCode() != 0) {
        buildFinished(false, "Build process failed during configuration step!<br>"
                             "Ensure you have all necessary components installed and have selected the correct directory.<br>br>" + 
                             cmake.readAllStandardError());
        return;
    } else if (timedOut) {
        buildFinished(false, "Build process failed during configuration step!<br><br>Timed out after 60 seconds.");
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
        buildFinished(false, "Build process failed during build step!<br><br>" + cmake.readAllStandardError());
        return;
    } else if (timedOut) {
        buildFinished(false, "Build process failed during build step!<br><br>Timed out after 100 seconds.");
        return;
    }
}

void Worker::processCMakeOutput() {
    // Fetch the current output from the CMake process
    QByteArray output = cmake.readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);
    // printf("%s", outputStr.toStdString().c_str());

    int percentIndex = outputStr.indexOf("%");
    if (percentIndex != -1 && percentIndex >= 3) {
        QString progressStr = outputStr.mid(percentIndex - 3, 3);
        bool ok;
        int progress = progressStr.toInt(&ok);
        if (ok) {
            buildProgress(progress);
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
            buildStep(targetName);
            return;
        }
    }
}