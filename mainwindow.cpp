#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPortInfo>
#include <QStatusBar>
#include <QDateTime>
#include <QTextStream>
#include <QCoreApplication>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include "preferencesdialog.h"
#include "loadtemplatedialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_device(new ValveTesterDevice(this))
    , m_sweepRunner(new SweepRunner(m_device, this))
{
    ui->setupUi(this);

    ui->deviceType->addItem("Triode");
    ui->deviceType->addItem("Double Triode");
    ui->deviceType->addItem("Pentode");

    connect(ui->deviceType, &QComboBox::currentIndexChanged,
            this, &MainWindow::onDeviceTypeChanged);
    connect(ui->testType, &QComboBox::currentIndexChanged,
            this, &MainWindow::updateSweepFields);
    onDeviceTypeChanged(); // initialise testType and field states for default selection

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(ui->connectButton,      &QPushButton::clicked, this, &MainWindow::onConnectToggled);
    connect(ui->portCombo, &QComboBox::currentIndexChanged, this, [this]() {
        ui->connectButton->setEnabled(ui->portCombo->count() > 0 && !m_device->isConnected());
    });

    connect(m_device, &ValveTesterDevice::connected,        this, &MainWindow::onDeviceConnected);
    connect(m_device, &ValveTesterDevice::disconnected,     this, &MainWindow::onDeviceDisconnected);
    connect(m_device, &ValveTesterDevice::hardwareVersion,  this, &MainWindow::onHardwareVersion);
    connect(m_device, &ValveTesterDevice::firmwareVersion,  this, &MainWindow::onFirmwareVersion);
    connect(m_device, &ValveTesterDevice::lineReceived,     this, &MainWindow::onLineReceived);
    connect(m_device, &ValveTesterDevice::lineSent,         this, &MainWindow::onLineSent);
    connect(m_device, &ValveTesterDevice::errorOccurred,    this, &MainWindow::onDeviceError);

    connect(m_sweepRunner, &SweepRunner::progressChanged,      this, &MainWindow::onSweepProgress);
    connect(m_sweepRunner, &SweepRunner::sampleAcquired,       this, &MainWindow::onSampleAcquired);
    connect(m_sweepRunner, &SweepRunner::measurementComplete,  this, &MainWindow::onMeasurementComplete);
    connect(m_sweepRunner, &SweepRunner::sweepAborted,         this, &MainWindow::onSweepAborted);

    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::onRunToggled);

    refreshPorts();
    m_connLabel = new QLabel(" \u25cf Disconnected ");
    m_connLabel->setStyleSheet("color: red; font-weight: bold;");
    statusBar()->addPermanentWidget(m_connLabel);
    m_infoLabel = new QLabel(" Ready ");
    statusBar()->addWidget(m_infoLabel);
    statusBar()->setSizeGripEnabled(false);

    m_prefsPath = QCoreApplication::applicationDirPath() + "/ValveTester.json";
    m_prefs.load(m_prefsPath);
    applyPreferences();

    m_templatesPath = QCoreApplication::applicationDirPath() + "/templates.json";
    m_templates.load(m_templatesPath);

    connect(ui->actionOptions, &QAction::triggered,   this, &MainWindow::onEditPreferences);
    connect(ui->pushButton_3,  &QPushButton::clicked, this, &MainWindow::onLoadTemplate);
    connect(ui->pushButton_4,  &QPushButton::clicked, this, &MainWindow::onSaveTemplate);

    if (ui->portCombo->count() == 1)
        onConnectToggled();

    // ── Chart setup ──────────────────────────────────────────────────────────
    m_chart = new QChart();
    m_chart->setTitle("No measurement");
    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    ui->graphicsView->setChart(m_chart);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);

    connect(ui->measureCheck, &QCheckBox::toggled, this, &MainWindow::onPlotVisibilityChanged);
    connect(ui->screenCheck,  &QCheckBox::toggled, this, &MainWindow::onPlotVisibilityChanged);
}

MainWindow::~MainWindow()
{
    m_device->disconnect();
    delete ui;
}

// ─── Device signal handlers ───────────────────────────────────────────────────

void MainWindow::onDeviceConnected()
{
    ui->connectButton->setText("Disconnect");
    m_connLabel->setText(" \u25cf Connected ");
    m_connLabel->setStyleSheet("color: green; font-weight: bold;");
    ui->portCombo->setEnabled(false);
    ui->connectButton->setEnabled(true);
}

void MainWindow::onDeviceDisconnected()
{
    ui->connectButton->setText("Connect");
    m_connLabel->setText(" \u25cf Disconnected ");
    m_connLabel->setStyleSheet("color: red; font-weight: bold;");
    m_hwVersion.clear();
    m_fwVersion.clear();
    m_infoLabel->setText(" Ready ");
    ui->portCombo->setEnabled(true);
    ui->connectButton->setEnabled(ui->portCombo->count() > 0);
}

void MainWindow::onHardwareVersion(const QString &version)
{
    m_hwVersion = version;
    updateInfoLabel();
}

void MainWindow::onFirmwareVersion(const QString &version)
{
    m_fwVersion = version;
    updateInfoLabel();
}

void MainWindow::onLineReceived(const QString &line)
{
    logLine("RX", line);
    // Show lines that aren't already handled as version strings
    if (!line.startsWith("OK: Info("))
        statusBar()->showMessage(line, 3000);
}

void MainWindow::onLineSent(const QString &line)
{
    logLine("TX", line);
}

void MainWindow::onDeviceError(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}

// ─── UI handlers ─────────────────────────────────────────────────────────────

void MainWindow::onDeviceTypeChanged()
{
    const QString device  = ui->deviceType->currentText();
    const QString current = ui->testType->currentText();

    ui->testType->blockSignals(true);
    ui->testType->clear();
    ui->testType->addItem("Anode Characteristic");
    if (device == "Pentode") {
        ui->testType->addItem("Transfer Characteristic");
    }
    const int idx = ui->testType->findText(current);
    ui->testType->setCurrentIndex(idx >= 0 ? idx : 0);
    ui->testType->blockSignals(false);
    updateSweepFields();
}

void MainWindow::refreshPorts()
{
    ui->portCombo->clear();
    for (const auto &info : QSerialPortInfo::availablePorts())
        ui->portCombo->addItem(info.portName());
}

void MainWindow::onConnectToggled()
{
    if (m_device->isConnected()) {
        m_device->disconnect();
    } else {
        m_device->connectTo(ui->portCombo->currentText(), m_prefs.testMode);
    }
}

void MainWindow::updateInfoLabel()
{
    if (!m_hwVersion.isEmpty() && !m_fwVersion.isEmpty())
        m_infoLabel->setText(QString(" HW: %1   SW: %2 ").arg(m_hwVersion, m_fwVersion));
    else if (!m_hwVersion.isEmpty())
        m_infoLabel->setText(QString(" HW: %1 ").arg(m_hwVersion));
    else if (!m_fwVersion.isEmpty())
        m_infoLabel->setText(QString(" SW: %1 ").arg(m_fwVersion));
}

void MainWindow::logLine(const QString &prefix, const QString &text)
{
    if (!m_logFile.isOpen()) return;
    QTextStream out(&m_logFile);
    out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
        << " [" << prefix << "] " << text << "\n";
    m_logFile.flush();
}

void MainWindow::applyPreferences()
{
    const QString logPath = QCoreApplication::applicationDirPath() + "/ValveTester.log";
    if (m_prefs.enableLogging) {
        if (!m_logFile.isOpen()) {
            m_logFile.setFileName(logPath);
            if (!m_logFile.open(QIODevice::Append | QIODevice::Text))
                statusBar()->showMessage("Warning: could not open log file " + logPath, 5000);
            else
                logLine("---", QString("Session started %1")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        }
    } else {
        if (m_logFile.isOpen())
            m_logFile.close();
    }

    if (m_device->isConnected())
        m_device->setTestMode(m_prefs.testMode);
}

void MainWindow::updateSweepFields()
{
    const QString device = ui->deviceType->currentText();
    const QString test   = ui->testType->currentText();
    const bool isPentode      = (device == "Pentode");
    const bool isDoubleTri    = (device == "Double Triode");
    const bool isAnodeChar    = (test == "Anode Characteristic");
    const bool isTransfer     = !isAnodeChar;

    // Anode row: stop/step hidden for transfer char where Va is a fixed value
    ui->anodeLabel->setText(isTransfer ? "Anode Voltage (fixed):" : "Anode Voltage:");
    ui->anodeStart->setVisible(true);
    ui->anodeStop ->setVisible(!isTransfer);
    ui->anodeStep ->setVisible(!isTransfer);

    // Grid row: always fully visible; label describes role in each test type
    ui->gridLabel->setText(isTransfer ? "Grid 1 (Vg1) sweep:" : "-ve Grid Voltage:");
    ui->gridStart->setVisible(true);
    ui->gridStop ->setVisible(true);
    ui->gridStep ->setVisible(true);

    // Screen row: hidden for non-pentodes; all 3 fields shown for Transfer (Vg2 stepped)
    //   Anode char  → 1 field only (fixed Vg2)
    //   Transfer char → 3 fields  (Vg2 sweep: from / to / step)
    ui->screenLabel->setVisible(isPentode);
    if (isPentode)
        ui->screenLabel->setText(isTransfer ? "Screen (Vg2) sweep:" : "Screen Voltage:");
    ui->screenStart->setVisible(isPentode);
    ui->screenStop ->setVisible(isPentode && isTransfer);
    ui->screenStep ->setVisible(isPentode && isTransfer);

    // Re-populate field values from the active template whenever the test type changes
    applyTemplateFields();

    // Checkbox labels and visibility
    if (isDoubleTri) {
        ui->measureCheck->setText("Show Triode 1");
        ui->screenCheck->setText("Show Triode 2");
        ui->screenCheck->setVisible(true);
    } else if (isPentode) {
        ui->measureCheck->setText("Show Anode Current");
        ui->screenCheck->setText("Show Screen Current");
        ui->screenCheck->setVisible(true);
    } else {
        // Single triode
        ui->measureCheck->setText("Show Anode Current");
        ui->screenCheck->setVisible(false);
    }
}

void MainWindow::onEditPreferences()
{
    PreferencesDialog dlg(m_prefs, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_prefs = dlg.preferences();
        m_prefs.save(m_prefsPath);
        applyPreferences();
    }
}

void MainWindow::applyTemplate(const DeviceTemplate &tmpl)
{
    m_activeTemplate    = tmpl;
    m_hasActiveTemplate = true;

    const int dtIdx = ui->deviceType->findText(tmpl.deviceType);
    if (dtIdx >= 0) ui->deviceType->setCurrentIndex(dtIdx);

    onDeviceTypeChanged();  // rebuilds testType combo
    const int ttIdx = ui->testType->findText(tmpl.testType);
    if (ttIdx >= 0) ui->testType->setCurrentIndex(ttIdx);
    // updateSweepFields is called by the above, which also calls applyTemplateFields
}

void MainWindow::applyTemplateFields()
{
    if (!m_hasActiveTemplate) return;
    const DeviceTemplate &t = m_activeTemplate;
    const bool isTransfer = (ui->testType->currentText() == "Transfer Characteristic");

    if (isTransfer) {
        // Fixed Va from vaFixed; Vg1 inner sweep; Vg2 outer sweep from screen fields
        ui->anodeStart ->setText(t.vaFixed.isEmpty() ? t.anodeStart : t.vaFixed);
        ui->screenStart->setText(t.screenStart);
        ui->screenStop ->setText(t.screenStop);
        ui->screenStep ->setText(t.screenStep);
    } else {
        // Full Va sweep; fixed screen voltage from screenFixed (falls back to screenStart)
        ui->anodeStart ->setText(t.anodeStart);
        ui->anodeStop  ->setText(t.anodeStop);
        ui->anodeStep  ->setText(t.anodeStep);
        ui->screenStart->setText(t.screenFixed.isEmpty() ? t.screenStart : t.screenFixed);
    }
    // Grid range is shared between both test types; Transfer uses finer step if specified
    ui->gridStart->setText(t.gridStart);
    ui->gridStop ->setText(t.gridStop);
    ui->gridStep ->setText(isTransfer && !t.gridStepTransfer.isEmpty() ? t.gridStepTransfer : t.gridStep);
    ui->iaMax    ->setText(isTransfer && !t.iaMaxTransfer.isEmpty() ? t.iaMaxTransfer : t.iaMax);
    ui->pMax     ->setText(t.pMax);
}

void MainWindow::onLoadTemplate()
{
    LoadTemplateDialog dlg(m_templates, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString name = dlg.selectedName();
        if (name.isEmpty()) return;
        ui->deviceName->setText(name);
        applyTemplate(m_templates.get(name));
    }
}

void MainWindow::onSaveTemplate()
{
    const QString name = ui->deviceName->text().trimmed().toUpper();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Save Template",
            "Please enter a device name before saving a template.");
        return;
    }

    if (m_templates.hasAny(name)) {
        const QString source = m_templates.hasSystem(name) ? "built-in" : "user";
        const int ret = QMessageBox::question(this, "Save Template",
            QString("A %1 template named \"%2\" already exists.\nOverwrite it?")
                .arg(source, name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    DeviceTemplate tmpl;
    tmpl.deviceType  = ui->deviceType->currentText();
    tmpl.testType    = ui->testType->currentText();
    tmpl.anodeStart  = ui->anodeStart->text();
    tmpl.anodeStop   = ui->anodeStop->text();
    tmpl.anodeStep   = ui->anodeStep->text();
    tmpl.gridStart   = ui->gridStart->text();
    tmpl.gridStop    = ui->gridStop->text();
    tmpl.gridStep    = ui->gridStep->text();
    tmpl.screenStart = ui->screenStart->text();
    tmpl.screenStop  = ui->screenStop->text();
    tmpl.screenStep  = ui->screenStep->text();
    tmpl.iaMax       = ui->iaMax->text();
    tmpl.pMax        = ui->pMax->text();

    m_templates.saveUser(name, tmpl);
    statusBar()->showMessage(QString("Template \"%1\" saved.").arg(name), 3000);
}

// ─── Sweep ────────────────────────────────────────────────────────────────────

void MainWindow::onRunToggled()
{
    if (m_sweepRunner->isRunning()) {
        m_sweepRunner->abort();
        return;
    }

    SweepParams p;
    p.deviceName = ui->deviceName->text().trimmed();
    p.deviceType = ui->deviceType->currentText();
    p.testType   = ui->testType->currentText();

    p.sweepType = (p.testType == "Transfer Characteristic")
                  ? SweepType::Transfer
                  : SweepType::Anode;

    p.vaStart    = ui->anodeStart->text().toDouble();
    p.vaStop     = ui->anodeStop->text().toDouble();
    p.vaStep     = ui->anodeStep->text().toDouble();
    p.vgStart    = ui->gridStart->text().toDouble();
    p.vgStop     = ui->gridStop->text().toDouble();
    p.vgStep     = ui->gridStep->text().toDouble();
    p.vg2        = ui->screenStart->text().toDouble();  // fixed screen V for Anode char
    p.vg2Start   = ui->screenStart->text().toDouble();
    p.vg2Stop    = ui->screenStop->text().toDouble();
    p.vg2Step    = ui->screenStep->text().toDouble();
    p.iaMax      = ui->iaMax->text().toDouble();
    p.pMax       = ui->pMax->text().toDouble();
    // Suppress end-of-measurement M0 discharge when a hardware test mode is
    // active — the supply would only need to recharge immediately for the next run.
    p.dischargeOnComplete = (m_prefs.testMode == 0);

    // Validate the inner (swept) axis
    if (p.sweepType == SweepType::Anode) {
        if (p.vaStep <= 0.0 || p.vaStop < p.vaStart) {
            statusBar()->showMessage("Invalid anode sweep range", 3000);
            return;
        }
    } else {
        if (p.vgStep <= 0.0 || p.vgStop < p.vgStart) {
            statusBar()->showMessage("Invalid grid sweep range", 3000);
            return;
        }
    }

    ui->runButton->setText("Abort");
    ui->progressBar->setValue(0);
    m_sweepParams = p;
    setupChart(p);
    m_sweepRunner->start(p);
}

void MainWindow::onSweepProgress(int percent)
{
    ui->progressBar->setValue(percent);
}

void MainWindow::onMeasurementComplete(const Measurement &measurement)
{
    m_lastMeasurement = measurement;
    ui->runButton->setText("Run Test");
    ui->runButton->setChecked(false);
    ui->progressBar->setValue(100);
    statusBar()->showMessage(
        QString("Measurement complete — %1 sweeps, %2 points each")
            .arg(measurement.sweeps.size())
            .arg(measurement.sweeps.isEmpty() ? 0 : measurement.sweeps.first().samples.size()),
        5000);
}

void MainWindow::onSweepAborted(const QString &reason)
{
    ui->runButton->setText("Run Test");
    ui->runButton->setChecked(false);
    ui->progressBar->setValue(0);
    statusBar()->showMessage("Sweep aborted: " + reason, 5000);
}

// ─── Chart ────────────────────────────────────────────────────────────────────

void MainWindow::setupChart(const SweepParams &p)
{
    m_chart->removeAllSeries();
    const auto oldAxes = m_chart->axes();
    for (auto *ax : oldAxes)
        m_chart->removeAxis(ax);
    m_iaSeries.clear();
    m_ig2Series.clear();

    const bool isTransfer = (p.sweepType == SweepType::Transfer);
    m_chart->legend()->setVisible(false);

    m_chart->setTitle(QString("%1 — %2")
                          .arg(p.deviceName.isEmpty() ? "Unknown" : p.deviceName,
                               p.testType));

    // X axis
    auto *axisX = new QValueAxis(m_chart);
    if (isTransfer) {
        axisX->setTitleText("Grid 1 Voltage (V)");
        axisX->setRange(-p.vgStop, 0.0);
    } else {
        axisX->setTitleText("Anode Voltage (V)");
        axisX->setRange(p.vaStart, p.vaStop);
    }
    axisX->setTickCount(11);
    axisX->setLabelFormat("%.0f");
    m_chart->addAxis(axisX, Qt::AlignBottom);

    // Y axis
    auto *axisY = new QValueAxis(m_chart);
    axisY->setTitleText("Current (mA)");
    axisY->setRange(0.0, p.iaMax > 0.0 ? p.iaMax : 20.0);
    axisY->setTickCount(6);
    axisY->setLabelFormat("%.1f");
    m_chart->addAxis(axisY, Qt::AlignLeft);
}

QLineSeries *MainWindow::getOrCreateSeries(const QString &label, bool dashed)
{
    // Fixed two-colour palette: all Ia curves steel blue, all Ig2 curves burnt red (dashed).
    // In Transfer mode the legend distinguishes Vg2 values; colour identifies current type.
    static const QColor kIaColor (31, 119, 180);
    static const QColor kIg2Color(214,  39,  40);

    auto &map = dashed ? m_ig2Series : m_iaSeries;
    if (!map.contains(label)) {
        auto *series = new QLineSeries(m_chart);
        series->setName(label);
        m_chart->addSeries(series);

        QPen pen = series->pen();
        pen.setColor(dashed ? kIg2Color : kIaColor);
        pen.setWidth(2);
        if (dashed) pen.setStyle(Qt::DashLine);
        series->setPen(pen);

        series->attachAxis(m_chart->axes(Qt::Horizontal).first());
        series->attachAxis(m_chart->axes(Qt::Vertical).first());

        // Respect current checkbox state for newly created series.
        series->setVisible(dashed ? ui->screenCheck->isChecked()
                                  : ui->measureCheck->isChecked());
        map[label] = series;
    }
    return map[label];
}

void MainWindow::onSampleAcquired(const Sweep &sweep, const SweepSample &sample)
{
    const bool isTransfer = (m_sweepParams.sweepType == SweepType::Transfer);
    const double xVal = isTransfer ? sample.vg1Volts : sample.vaVolts;

    const QString label = isTransfer
        ? QString("Vg2 = %1 V").arg(sweep.vg2Volts, 0, 'f', 0)
        : QString("Vg = %1 V").arg(sweep.vgVolts, 0, 'f', 1);

    // Always populate both series so the data is retained regardless of the
    // current checkbox state.  Visibility is controlled separately.
    getOrCreateSeries(label, false)->append(xVal, sample.iaMa);
    getOrCreateSeries(label + " (Ig2)", true)->append(xVal, sample.ig2Ma);
}

void MainWindow::onPlotVisibilityChanged()
{
    const bool showIa  = ui->measureCheck->isChecked();
    const bool showIg2 = ui->screenCheck->isChecked();
    for (auto *s : m_iaSeries)  s->setVisible(showIa);
    for (auto *s : m_ig2Series) s->setVisible(showIg2);
}
