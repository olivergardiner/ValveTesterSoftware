#pragma once
#include <QString>

struct Preferences
{
    bool enableLogging = false;

    // S4 test mode: 0 = None, 1 = ECC83, 2 = EL84
    int testMode = 0;

    void load(const QString &filePath);
    void save(const QString &filePath) const;
};
