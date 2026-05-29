#pragma once
#include <QDialog>
#include "preferences.h"

class QCheckBox;
class QComboBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(const Preferences &prefs, QWidget *parent = nullptr);
    Preferences preferences() const;

private:
    QCheckBox *m_enableLogging;
    QComboBox *m_testMode;
};
