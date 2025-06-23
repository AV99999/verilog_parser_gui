// File: src/LicenseChecker.cpp
#include "LicenseChecker.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

bool LicenseChecker::isLicenseValid(const QString& licensePath) {
    QFile file(licensePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[LICENSE] Could not open license file.";
        return false;
    }

    QTextStream in(&file);
    QString expires;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("LICENSE_EXPIRES=")) {
            expires = line.section('=', 1, 1);
            break;
        }
    }

    file.close();

    if (expires.isEmpty()) {
        qWarning() << "[LICENSE] Expiry not found.";
        return false;
    }

    QDateTime expiryTime = QDateTime::fromString(expires, Qt::ISODate);
    if (!expiryTime.isValid()) {
        qWarning() << "[LICENSE] Invalid date format.";
        return false;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    if (now > expiryTime) {
        qWarning() << "[LICENSE] License expired at:" << expiryTime.toString();
        return false;
    }

    qDebug() << "[LICENSE] Valid until:" << expiryTime.toString();
    return true;
}
