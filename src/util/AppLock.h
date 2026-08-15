#pragma once

#include <QString>

namespace AppLock {

bool isEnabled();
bool setPassword(const QString &password);
bool verifyPassword(const QString &password);
void clearPassword();

} // namespace AppLock
