#ifndef DATAREVANDSEND_H
#define DATAREVANDSEND_H

#include <QDebug>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QThread>
#include <QTimer>

class DataRevAndSend : public QObject
{
    Q_OBJECT
 public:
    DataRevAndSend(QString serialName, qint32 baudrate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                   QSerialPort::Parity parity, QSerialPort::FlowControl flowControl, QSerialPort::OpenMode openMode);

    void threadStart();
    void threadStop();
    bool threadIsRunning();

    bool getOpenStatus();

 signals:
    void receivedData(const QByteArray& datagram);

    void messageNotification(QString msg);

 private slots:
    void threadStarted();
    void threadFinished();

    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);

    void sendData();
    void sendData(const QByteArray& datagram, int sendTime);

    void onStopRepeatSendData();

 private:
    // thread
    QThread m_thread;

    // sender
    QTimer* m_sendTimer = nullptr;

    // serialPort
    QSerialPort* m_serialPort = nullptr;
    QString m_portName;
    qint32 m_baudrate;
    QSerialPort::DataBits m_dataBits;
    QSerialPort::StopBits m_stopBits;
    QSerialPort::Parity m_parity;
    QSerialPort::FlowControl m_flowControl;
    QSerialPort::OpenMode m_openMode;

    QByteArray m_sendDatagram;
    QByteArray m_saveByteArray;
};

#endif  // DATAREVANDSEND_H
