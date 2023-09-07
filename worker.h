#ifndef __WORKER_H
#define __WORKER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class Worker : public QObject {
    Q_OBJECT

public:
    Worker();

public slots:
    /**
     * @brief Builds the currently loaded project.
     * @param confArgs Arguments to pass to CMake for the configuration step (NOT build step!!).
     * @param projectDir Directory of the project to build.
     * @note This will fail if the project is not loaded.
    */
    void buildProject(const QStringList &confArgs, const QString &projectDir);

    /**
     * @brief Helper function to process CMake's output and provide updates.
    */
    void processCMakeOutput();

private:
    QProcess cmake;

signals:
    void buildProgress(int progress);
    void buildStep(QString step);
    void buildFinished(bool success, QString error);
};

#endif // __WORKER_H
