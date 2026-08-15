#pragma once

#include <QDate>

QString mesActivoActual();
QString mesAnterior(const QString &mes);
QString mesSiguiente(const QString &mes);
bool esMesValido(const QString &mes);
int compararMeses(const QString &a, const QString &b);
QDate fechaDefaultParaMes(const QString &mes);

