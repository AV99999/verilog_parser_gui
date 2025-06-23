// File: src/LicenseChecker.h
#pragma once
#include <QString>

class LicenseChecker {
public:
    static bool isLicenseValid(const QString& licensePath);
};
