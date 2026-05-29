#ifndef VALVETESTERDEVICE_H
#define VALVETESTERDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QQueue>
#include <QTimer>
#include <QString>

// Abstracts all serial communication with the Arduino valve-tester firmware.
// Emits typed signals; callers never touch raw protocol strings.
class ValveTesterDevice : public QObject
{
    Q_OBJECT

public:
    explicit ValveTesterDevice(QObject *parent = nullptr);
    ~ValveTesterDevice() override;

    bool isConnected() const { return m_serial->isOpen(); }

public slots:
    // Open the named port and begin handshake (I0, I1, S4) after DTR delay.
    void connectTo(const QString &portName, int testMode);

    // Send M0 (safe/discharge) then close the port.
    void disconnect();

    // Send the test-mode command immediately (if connected).
    void setTestMode(int testMode);

    // Sweep control — set hardware parameters then trigger a single measurement.
    void setGrid(int dacCode);           // S0
    void setGrid2(int dacCode);          // S1 (double triode second section)
    void setScreen(int htAdcCode);       // S3 (pentode screen via HT2)
    void setHtTarget(int htAdcCode);     // S2
    void runTest();                      // M1
    void safeMode();                     // M0 — discharge / safe state

signals:
    // Emitted once the port is open (before DTR delay completes).
    void connected();

    // Emitted after the port is closed.
    void disconnected();

    // Hardware and firmware version strings from I0 / I1 responses.
    void hardwareVersion(const QString &version);
    void firmwareVersion(const QString &version);

    // A raw line was received (for logging / status-bar display).
    void lineReceived(const QString &line);

    // A raw line was sent (for logging).
    void lineSent(const QString &line);

    // Emitted when M1 response is parsed. Values are in engineering units (V, mA).
    void measurementReady(double vaVolts, double iaMa, double ig2Ma);

    // Emitted when an M command (M1) receives an ERR: response from the firmware.
    void measurementFailed();

    // Emitted when the port cannot be opened.
    void errorOccurred(const QString &message);

private slots:
    void onSerialDataReady();
    void onResponseTimeout();

private:
    void enqueueCommand(const QString &cmd);
    void dispatchNext();
    void processLine(const QString &line);

    QSerialPort   *m_serial;
    QString        m_receiveBuffer;
    QQueue<QString> m_commandQueue;
    bool           m_waitingForResponse = false;
    QString        m_lastDispatchedCommand;
    QTimer        *m_responseTimer;
};

#endif // VALVETESTERDEVICE_H
