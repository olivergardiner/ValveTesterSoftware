#include "preferencesdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(const Preferences &prefs, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setMinimumWidth(320);
    setSizeGripEnabled(false);

    m_enableLogging = new QCheckBox("Enable serial communication logging", this);
    m_enableLogging->setChecked(prefs.enableLogging);

    auto *loggingGroup = new QGroupBox("Logging", this);
    auto *groupLayout = new QVBoxLayout(loggingGroup);
    groupLayout->addWidget(m_enableLogging);

    m_testMode = new QComboBox(this);
    m_testMode->addItem("None",  0);
    m_testMode->addItem("ECC83", 1);
    m_testMode->addItem("EL84",  2);
    m_testMode->setCurrentIndex(m_testMode->findData(prefs.testMode));

    auto *hwGroup  = new QGroupBox("Hardware", this);
    auto *formLayout = new QFormLayout(hwGroup);
    formLayout->addRow("Test Mode:", m_testMode);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(loggingGroup);
    layout->addWidget(hwGroup);
    layout->addStretch();
    layout->addWidget(buttons);
}

Preferences PreferencesDialog::preferences() const
{
    Preferences p;
    p.enableLogging = m_enableLogging->isChecked();
    p.testMode      = m_testMode->currentData().toInt();
    return p;
}
