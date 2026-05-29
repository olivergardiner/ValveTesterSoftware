#include "valvetesterdevice.h"
#include "hw_scaling.h"

#include <QSerialPort>
#include <QTimer>

ValveTesterDevice::ValveTesterDevice(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_responseTimer(new QTimer(this))
{
    connect(m_serial, &QSerialPort::readyRead, this, &ValveTesterDevice::onSerialDataReady);
    m_responseTimer->setSingleShot(true);
    connect(m_responseTimer, &QTimer::timeout, this, &ValveTesterDevice::onResponseTimeout);
}

ValveTesterDevice::~ValveTesterDevice()
{
    if (m_serial->isOpen()) {
        m_serial->write("M0\n");
        m_serial->flush();
        m_serial->close();
    }
}

void ValveTesterDevice::connectTo(const QString &portName, int testMode)
{
    if (m_serial->isOpen())
        return;

    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Could not open %1: %2")
                               .arg(portName, m_serial->errorString()));
        return;
    }

    emit connected();

    // Arduino resets on DTR — wait 2 s for it to boot before sending commands.
    QTimer::singleShot(2000, this, [this, testMode]() {
        if (m_serial->isOpen()) {
            enqueueCommand("I0");
            enqueueCommand("I1");
            enqueueCommand(QString("S4 %1").arg(testMode));
        }
    });
}

void ValveTesterDevice::disconnect()
{
    if (!m_serial->isOpen())
        return;

    m_responseTimer->stop();
    m_commandQueue.clear();
    m_waitingForResponse = false;

    // Send M0 directly — queue is now empty and we're closing immediately.
    m_serial->write("M0\n");
    m_serial->flush();
    m_serial->close();
    m_receiveBuffer.clear();
    emit disconnected();
}

void ValveTesterDevice::setTestMode(int testMode)
{
    enqueueCommand(QString("S4 %1").arg(testMode));
}

void ValveTesterDevice::setGrid(int dacCode)
{
    enqueueCommand(QString("S0 %1").arg(dacCode));
}

void ValveTesterDevice::setGrid2(int dacCode)
{
    enqueueCommand(QString("S1 %1").arg(dacCode));
}

void ValveTesterDevice::setScreen(int htAdcCode)
{
    enqueueCommand(QString("S3 %1").arg(htAdcCode));
}

void ValveTesterDevice::setHtTarget(int htAdcCode)
{
    enqueueCommand(QString("S2 %1").arg(htAdcCode));
}

void ValveTesterDevice::runTest()
{
    enqueueCommand("M1");
}

void ValveTesterDevice::safeMode()
{
    enqueueCommand("M0");
}

void ValveTesterDevice::enqueueCommand(const QString &cmd)
{
    if (!m_serial->isOpen())
        return;
    m_commandQueue.enqueue(cmd);
    if (!m_waitingForResponse)
        dispatchNext();
}

void ValveTesterDevice::dispatchNext()
{
    if (m_commandQueue.isEmpty()) {
        m_waitingForResponse = false;
        return;
    }
    const QString cmd = m_commandQueue.dequeue();
    m_lastDispatchedCommand = cmd;
    emit lineSent(cmd);
    m_serial->write((cmd + "\n").toLatin1());
    m_waitingForResponse = true;
    // M0 (safe/discharge): firmware does not send a response — the timeout acts
    // as a minimum discharge wait.  5 s is enough for real hardware; in
    // simulator mode (S4=1) M0 is a no-op so the wait is the only delay.
    // M1 needs time to charge and stabilise the HT supply (up to 10 s).
    // All other commands get a 2 s guard.
    int timeoutMs = 2000;
    if (cmd == "M0")               timeoutMs = 5000;
    else if (cmd.startsWith('M'))  timeoutMs = 10000;
    m_responseTimer->start(timeoutMs);
}

void ValveTesterDevice::onResponseTimeout()
{
    m_waitingForResponse = false;
    emit errorOccurred("Command timed out — no response from device");
    dispatchNext();
}

void ValveTesterDevice::onSerialDataReady()
{
    m_receiveBuffer += QString::fromLatin1(m_serial->readAll());

    int pos;
    while ((pos = m_receiveBuffer.indexOf('\n')) != -1) {
        const QString line = m_receiveBuffer.left(pos).trimmed();
        m_receiveBuffer.remove(0, pos + 1);
        processLine(line);
    }
}

void ValveTesterDevice::processLine(const QString &line)
{
    // A response is valid for the in-flight command only if the type matches:
    //   S commands → "OK: Set(...)"  or "ERR:"
    //   M0         → "OK: Mode(0)"  or "ERR:"  (firmware may not respond at all)
    //   M1         → "OK: Mode(1)"  or "ERR:"
    //   I commands → "OK: Info(...)" or "ERR:"
    if (m_waitingForResponse) {
        bool isExpected = false;
        if (line.startsWith("ERR:")) {
            isExpected = true;
        } else if (line.startsWith("OK:")) {
            if (m_lastDispatchedCommand.startsWith('S'))
                isExpected = line.startsWith("OK: Set(");
            else if (m_lastDispatchedCommand == "M0")
                isExpected = line.startsWith("OK: Mode(0)");
            else if (m_lastDispatchedCommand == "M1")
                isExpected = line.startsWith("OK: Mode(1)");
            else if (m_lastDispatchedCommand.startsWith('I'))
                isExpected = line.startsWith("OK: Info(");
            else
                isExpected = true; // fallback for unknown commands
        }
        if (isExpected) {
            m_responseTimer->stop();
            m_waitingForResponse = false;
            dispatchNext();
        }
    }

    emit lineReceived(line);

    if (line.startsWith("OK: Info(0) = "))
        emit hardwareVersion(line.mid(14).trimmed());
    else if (line.startsWith("OK: Info(1) = "))
        emit firmwareVersion(line.mid(14).trimmed());
    else if (line.startsWith("OK: Mode(1) ")) {
        // Parse 12 CSV values: grid1, grid2, targetHT1, targetHT2,
        //   measuredHT1, measuredHT2, currentLo1, currentMid1, currentHi1,
        //   currentLo2, currentMid2, currentHi2
        const QStringList parts = line.mid(12).split(',');
        if (parts.size() >= 9) {
            const int measuredHT1 = parts[4].trimmed().toInt();
            const int loAdc1      = parts[6].trimmed().toInt();
            const int midAdc1     = parts[7].trimmed().toInt();
            const int hiAdc1      = parts[8].trimmed().toInt();

            const double vaVolts = Hw::htCountToVolts(measuredHT1);
            const double iaMa    = Hw::adcToCurrentMa(hiAdc1, midAdc1, loAdc1);

            double ig2Ma = 0.0;
            if (parts.size() >= 12) {
                const int loAdc2  = parts[9].trimmed().toInt();
                const int midAdc2 = parts[10].trimmed().toInt();
                const int hiAdc2  = parts[11].trimmed().toInt();
                ig2Ma = Hw::adcToCurrentMa(hiAdc2, midAdc2, loAdc2);
            }

            emit measurementReady(vaVolts, iaMa, ig2Ma);
        }
    }
    else if (line.startsWith("ERR:") && m_lastDispatchedCommand.startsWith('M')) {
        // The firmware could not complete the test (e.g. HT voltage could not be
        // regulated to the requested target).  Notify the sweep engine so it can
        // skip this point and continue rather than stalling indefinitely.
        emit measurementFailed();
    }
}
