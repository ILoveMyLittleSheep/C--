#include "DataRevAndSend.h"

DataRevAndSend::DataRevAndSend(QString serialName, qint32 baudrate, QSerialPort::DataBits dataBits,
                               QSerialPort::StopBits stopBits, QSerialPort::Parity parity,
                               QSerialPort::FlowControl flowControl, QSerialPort::OpenMode openMode)
    : m_portName(serialName),
      m_baudrate(baudrate),
      m_dataBits(dataBits),
      m_stopBits(stopBits),
      m_parity(parity),
      m_flowControl(flowControl),
      m_openMode(openMode)
{
    this->moveToThread(&m_thread);
    connect(&m_thread, SIGNAL(started()), this, SLOT(threadStarted()));
    connect(&m_thread, SIGNAL(finished()), this, SLOT(threadFinished()));
}

bool DataRevAndSend::threadIsRunning() { return m_thread.isRunning(); }

void DataRevAndSend::threadStart()
{
    if (!m_thread.isRunning())
    {
        m_thread.start();
    }
}

void DataRevAndSend::threadStop()
{
    if (m_thread.isRunning())
    {
        m_thread.quit();
        m_thread.wait();
    }
}

void DataRevAndSend::threadStarted()
{
    m_thread.setPriority(QThread::HighPriority);

    // 设置波特率等参数
    m_serialPort = new QSerialPort();
    m_serialPort->setPortName(m_portName);
    m_serialPort->setBaudRate(m_baudrate);
    m_serialPort->setDataBits(m_dataBits);
    m_serialPort->setStopBits(m_stopBits);
    m_serialPort->setParity(m_parity);
    m_serialPort->setFlowControl(m_flowControl);

    connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(m_serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this,
            SLOT(onError(QSerialPort::SerialPortError)));

    if (m_serialPort->open(m_openMode))
    {
        emit messageNotification("[Successfully open: " + m_portName + "]");

        m_sendTimer = new QTimer();
        m_sendTimer->setTimerType(Qt::PreciseTimer);
    }
    //    else
    //    {
    //        emit messageNotification("[Failed to open: " + m_portName + "! | Error: " + m_serialPort->errorString() +
    //        "]");
    //    }
}

void DataRevAndSend::threadFinished()
{
    if (m_sendTimer)
    {
        if (m_sendTimer->isActive())
        {
            m_sendTimer->stop();
        }
        delete m_sendTimer;
        m_sendTimer = nullptr;
    }

    if (m_serialPort)
    {
        if (m_serialPort->isOpen())
        {
            m_serialPort->close();
        }
        emit messageNotification("[Successfully close: " + m_portName + "]");

        delete m_serialPort;
        m_serialPort = nullptr;
    }
}

void DataRevAndSend::sendData() { m_serialPort->write(m_sendDatagram); }

void DataRevAndSend::sendData(const QByteArray &datagram, int sendTime)
{
    if (m_serialPort && m_serialPort->isOpen())
    {
        m_sendDatagram = datagram;
        if (sendTime <= 0)
        {
            sendData();
        }
        else
        {
            m_sendTimer->setInterval(sendTime);
            connect(m_sendTimer, SIGNAL(timeout()), this, SLOT(sendData()));
            m_sendTimer->start();
        }
    }
}

void DataRevAndSend::onStopRepeatSendData()
{
    if (m_sendTimer)
    {
        m_sendTimer->disconnect();
        m_sendTimer->stop();
    }
}

bool DataRevAndSend::getOpenStatus()
{
    if (m_serialPort)
    {
        return m_serialPort->isOpen();
    }
    else
    {
        return false;
    }
}

void DataRevAndSend::onReadyRead() { emit receivedData(m_serialPort->readAll()); }

void DataRevAndSend::onError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        emit messageNotification("[Error: " + m_serialPort->errorString() + "]");
    }
}
