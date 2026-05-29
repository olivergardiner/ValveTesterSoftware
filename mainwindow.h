#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QLabel>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "preferences.h"
#include "templatestore.h"
#include "valvetesterdevice.h"
#include "sweeprunner.h"
#include "measurement.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void refreshPorts();
    void onConnectToggled();
    void onDeviceTypeChanged();
    void onEditPreferences();
    void onLoadTemplate();
    void onSaveTemplate();

    void onRunToggled();
    void onSampleAcquired(const Sweep &sweep, const SweepSample &sample);
    void onSweepProgress(int percent);
    void onMeasurementComplete(const Measurement &measurement);
    void onSweepAborted(const QString &reason);
    void onPlotVisibilityChanged();

    // Slots wired to ValveTesterDevice signals
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onHardwareVersion(const QString &version);
    void onFirmwareVersion(const QString &version);
    void onLineReceived(const QString &line);
    void onLineSent(const QString &line);
    void onDeviceError(const QString &message);

private:
    void updateInfoLabel();
    void logLine(const QString &prefix, const QString &text);
    void applyPreferences();
    void applyTemplate(const DeviceTemplate &tmpl);
    void applyTemplateFields();
    void updateSweepFields();
    void setupChart(const SweepParams &params);
    QLineSeries *getOrCreateSeries(const QString &label, bool dashed);

    Ui::MainWindow    *ui;
    ValveTesterDevice *m_device;
    SweepRunner       *m_sweepRunner;
    QLabel            *m_connLabel;
    QLabel            *m_infoLabel;
    QFile              m_logFile;
    Preferences        m_prefs;
    QString            m_prefsPath;
    TemplateStore      m_templates;
    QString            m_templatesPath;
    QString            m_hwVersion;
    QString            m_fwVersion;
    DeviceTemplate     m_activeTemplate;
    bool               m_hasActiveTemplate = false;
    // Chart state
    QChart            *m_chart = nullptr;
    SweepParams        m_sweepParams;
    Measurement        m_lastMeasurement;
    QMap<QString, QLineSeries*> m_iaSeries;
    QMap<QString, QLineSeries*> m_ig2Series;
};

#endif // MAINWINDOW_H
