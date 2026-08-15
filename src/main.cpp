#include "MainWindow.h"
#include "database/Database.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>

#include "util/AppPaths.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLocale::setDefault(QLocale(QStringLiteral("es_AR")));
    app.setApplicationName(QStringLiteral("Mani"));
    app.setOrganizationName(QStringLiteral("Mani"));

    QCoreApplication::addLibraryPath(QLibraryInfo::path(QLibraryInfo::PluginsPath));

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        QMessageBox::critical(
            nullptr, QObject::tr("Error de base de datos"),
            QObject::tr("No se encontro el driver SQLite de Qt.\n"
                        "Recompila la app o verifica la instalacion de Qt."));
        return 1;
    }

    Database database;
    if (!database.open(databasePath())) {
        QMessageBox::critical(
            nullptr, QObject::tr("Error de base de datos"),
            QObject::tr("No se pudo abrir la base de datos:\n%1").arg(database.lastError()));
        return 1;
    }

    MainWindow window(&database);
    window.show();

    return app.exec();
}
