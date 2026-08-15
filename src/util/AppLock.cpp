#include "util/AppLock.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSettings>

namespace {

QString hashPassword(const QString &password, const QString &salt)
{
    const QByteArray data = (salt + password).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

} // namespace

namespace AppLock {

bool isEnabled()
{
    QSettings settings;
    return settings.value(QStringLiteral("lock/enabled"), false).toBool();
}

bool setPassword(const QString &password)
{
    if (password.isEmpty()) {
        return false;
    }

    const QString salt =
        QString::number(QRandomGenerator::global()->generate64(), 16) +
        QString::number(QRandomGenerator::global()->generate64(), 16);

    QSettings settings;
    settings.setValue(QStringLiteral("lock/enabled"), true);
    settings.setValue(QStringLiteral("lock/salt"), salt);
    settings.setValue(QStringLiteral("lock/hash"), hashPassword(password, salt));
    return true;
}

bool verifyPassword(const QString &password)
{
    QSettings settings;
    if (!settings.value(QStringLiteral("lock/enabled"), false).toBool()) {
        return true;
    }

    const QString salt = settings.value(QStringLiteral("lock/salt")).toString();
    const QString storedHash = settings.value(QStringLiteral("lock/hash")).toString();
    if (salt.isEmpty() || storedHash.isEmpty()) {
        return false;
    }

    return hashPassword(password, salt) == storedHash;
}

void clearPassword()
{
    QSettings settings;
    settings.remove(QStringLiteral("lock/enabled"));
    settings.remove(QStringLiteral("lock/salt"));
    settings.remove(QStringLiteral("lock/hash"));
}

} // namespace AppLock
