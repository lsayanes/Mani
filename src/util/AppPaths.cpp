#include "util/AppPaths.h"

#include <QDir>
#include <QStandardPaths>

QString appDataDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}

QString databasePath()
{
    return appDataDir() + QStringLiteral("/mani.db");
}

QString backupsDir()
{
    const QString dir = appDataDir() + QStringLiteral("/backups");
    QDir().mkpath(dir);
    return dir;
}
