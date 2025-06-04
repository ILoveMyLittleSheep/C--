#include "MainForm.h"
#include "ui_MainForm.h".h "

MainForm::MainForm(QWidget *parent) : QMainWindow(parent), ui(new Ui::SeialPort)
{
    ui->setupUi(this);

    initUI();
    boundUIOperation();

    m_timer = new QTimer();
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onUpdateUI()));
    m_timer->start(50);
}

MainForm::~MainForm()
{
    if (m_dataRevAndSend && m_dataRevAndSend->threadIsRunning())
    {
        m_dataRevAndSend->threadStop();
        delete m_dataRevAndSend;
    }
    delete ui;
}

void MainForm::initUI()
{
    //    this->setWindowFlags(Qt::FramelessWindowHint);
    //    ui->centralWidget->setStyleSheet("background-color: lightblue;");
    ui->cmb_device->setEditable(true);
    QDir realDir("/dev");
    QStringList realFilters;
    realFilters << "tty*";
    QStringList ttyDevices = realDir.entryList(realFilters, QDir::System | QDir::NoDotAndDotDot);
    ui->cmb_device->addItems(ttyDevices);
    ui->txt_receive->setEnabled(true);

    // 设置样式表，其中 'line-height' 属性用于定义行距
    // 这里我们将其设置为1.5倍的字体大小，但你也可以使用固定的值（如20px）
    QString txtRevStyleSheet = "p { line-height: 1.5; }";
    ui->txt_receive->setStyleSheet(txtRevStyleSheet);

    ui->cmb_openMode->setCurrentIndex(2);

    ui->repeatTime->setRange(0, 3600000);
}

void MainForm::boundUIOperation()
{
    // open & close button
    connect(ui->btn_open_close, SIGNAL(clicked(bool)), this, SLOT(onOpenSerialPort(bool)));

    // send message button
    connect(ui->btn_send, SIGNAL(clicked(bool)), this, SLOT(onSendData(bool)));

    // clear receive window
    connect(ui->btn_clear, SIGNAL(clicked(bool)), this, SLOT(onClearView(bool)));

    // baudrate
    connect(ui->cmb_baudrate, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbBaudrateChanged(int)));

    // Receive style
    connect(ui->cmb_revStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbRevStyleChanged(int)));

    // Repeat send
    connect(ui->checkBox_repeat, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxRepeatChanged(bool)));
    connect(ui->repeatTime, SIGNAL(valueChanged(int)), this, SLOT(onRepeatTimeChanged(int)));

    // send style
    connect(ui->cmb_sendStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(onSendStyleChanged(int)));

    // open mode
    connect(ui->cmb_openMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbOpenModeChanged(int)));
}

void MainForm::onUpdateUI()
{
    if (m_dataRevAndSend)
    {
        if (m_dataRevAndSend->getOpenStatus())
        {
            m_serialPortStatus = true;
            ui->btn_open_close->setText("Close");
            ui->cmb_baudrate->setEnabled(false);
            ui->cmb_dataBits->setEnabled(false);
            ui->cmb_flowControl->setEnabled(false);
            ui->cmb_stopBits->setEnabled(false);
            ui->cmb_openMode->setEnabled(false);
            ui->cmb_parity->setEnabled(false);
            ui->cmb_device->setEnabled(false);
        }
        else
        {
            m_serialPortStatus = false;
            ui->btn_open_close->setText("Open");
            ui->cmb_baudrate->setEnabled(true);
            ui->cmb_dataBits->setEnabled(true);
            ui->cmb_flowControl->setEnabled(true);
            ui->cmb_stopBits->setEnabled(true);
            ui->cmb_openMode->setEnabled(true);
            ui->cmb_parity->setEnabled(true);
            ui->cmb_device->setEnabled(true);
        }
    }
    else
    {
        m_serialPortStatus = false;
        ui->btn_open_close->setText("Open");
        ui->cmb_baudrate->setEnabled(true);
        ui->cmb_dataBits->setEnabled(true);
        ui->cmb_flowControl->setEnabled(true);
        ui->cmb_stopBits->setEnabled(true);
        ui->cmb_openMode->setEnabled(true);
        ui->cmb_parity->setEnabled(true);
        ui->cmb_device->setEnabled(true);
    }

    if (m_repeatlySend)
    {
        ui->repeatTime->setReadOnly(false);
    }
    else
    {
        ui->repeatTime->setReadOnly(true);
    }
}

void MainForm::onReceiveData(const QByteArray &data)
{
    // 设置要追加的文字内容和颜色
    QString appendHtml;
    appendHtml += "<span style='color:green;'>[" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") +
                  "]</span><br>";

    switch (m_displayStyle)
    {
        case DisplayStyle::HexRev:
            // 设置要追加的文字内容和颜色
            {
                appendHtml += "<span style='color:blue;'>" + data.toHex(' ').toUpper() + "</span><br>";
            }
            break;

        case DisplayStyle::ASCIIRev:
            // 设置要追加的文字内容和颜色
            {
                appendHtml += "<span style='color:blue;'>" + data + "</span><br>";
            }
            break;
        default:
            break;
    }

    // 获取文本光标并移动到文档末尾
    QTextCursor cursor = ui->txt_receive->textCursor();
    cursor.movePosition(QTextCursor::End);

    // 插入HTML格式的文本
    cursor.insertHtml(appendHtml);

    // 显示QTextBrowser
    ui->txt_receive->show();

    QScrollBar *scrollBar = ui->txt_receive->verticalScrollBar();
    if (scrollBar)
    {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void MainForm::onDisplayMsg(QString msg) { QMessageBox::information(nullptr, "Notice", msg); }

void MainForm::onOpenSerialPort(bool flag)
{
    if (!m_serialPortStatus)
    {
        QString portName               = ui->cmb_device->currentText();
        qint32 baudrate                = ui->cmb_baudrate->currentText().toInt();
        QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(ui->cmb_dataBits->currentIndex() + 5);
        QSerialPort::StopBits stopBits = static_cast<QSerialPort::StopBits>(ui->cmb_stopBits->currentIndex() + 1);
        QSerialPort::Parity parity;
        if (ui->cmb_parity->currentIndex() == 0)
        {
            parity = QSerialPort::NoParity;
        }
        else
        {
            parity = static_cast<QSerialPort::Parity>(ui->cmb_parity->currentIndex() + 1);
        }
        QSerialPort::FlowControl flowControl =
            static_cast<QSerialPort::FlowControl>(ui->cmb_flowControl->currentIndex());
        QSerialPort::OpenMode openMode = static_cast<QSerialPort::OpenMode>(ui->cmb_openMode->currentIndex() + 1);

        m_dataRevAndSend = new DataRevAndSend(portName, baudrate, dataBits, stopBits, parity, flowControl, openMode);

        connect(m_dataRevAndSend, SIGNAL(receivedData(QByteArray)), this, SLOT(onReceiveData(QByteArray)));
        connect(m_dataRevAndSend, SIGNAL(messageNotification(QString)), this, SLOT(onDisplayMsg(QString)));
        connect(this, SIGNAL(readySendDatagram(QByteArray, int)), m_dataRevAndSend, SLOT(sendData(QByteArray, int)));
        connect(this, SIGNAL(stopRepeatSendDatagram()), m_dataRevAndSend, SLOT(onStopRepeatSendData()));

        m_dataRevAndSend->threadStart();
    }
    else
    {
        if (m_dataRevAndSend)
        {
            if (m_dataRevAndSend->threadIsRunning())
            {
                m_dataRevAndSend->threadStop();
            }
            delete m_dataRevAndSend;
            m_dataRevAndSend = nullptr;
        }

        m_serialPortStatus = false;
    }
}

void MainForm::onSendData(bool flag)
{
    if (m_dataRevAndSend && m_serialPortStatus)
    {
        QString data = ui->txt_send->toPlainText();
        QByteArray sendData;
        switch (m_sendStyle)
        {
            case SendStyle::HexSend:
            {
                sendData = BaseConverter::hexQStringToQByteArray(data);
            }
            break;
            default:
            case SendStyle::ASCIISend:
            {
                sendData = QByteArray::fromStdString(data.toStdString());
            }
            break;
        }
        if (m_repeatlySend)
        {
            if (m_repeatTime > 0)
            {
                emit readySendDatagram(sendData, m_repeatTime);
            }
        }
        else
        {
            emit readySendDatagram(sendData, -1);
        }
    }
}

void MainForm::onClearView(bool flag) { ui->txt_receive->clear(); }

void MainForm::onCmbOpenModeChanged(int index)
{
    if (index == 0 || !m_serialPortStatus)
    {
        ui->btn_send->setEnabled(false);
        ui->checkBox_repeat->setEnabled(false);
        ui->cmb_sendStyle->setEnabled(false);
        ui->repeatTime->setEnabled(false);
    }
    else
    {
        ui->btn_send->setEnabled(true);
        ui->checkBox_repeat->setEnabled(true);
        ui->cmb_sendStyle->setEnabled(true);
        ui->repeatTime->setEnabled(true);
    }
}

void MainForm::onCmbBaudrateChanged(int index)
{
    if (index == 8)
    {
        ui->cmb_baudrate->setEditable(true);
    }
    else
    {
        ui->cmb_baudrate->setEditable(false);
    }
}

void MainForm::onCmbRevStyleChanged(int index)
{
    if (index == 0)
    {
        m_displayStyle = DisplayStyle::HexRev;
    }
    else if (index == 1)
    {
        m_displayStyle = DisplayStyle::ASCIIRev;
    }
}

void MainForm::onCheckBoxRepeatChanged(bool flag)
{
    m_repeatlySend = flag;
    if (m_serialPortStatus && !flag)
    {
        emit stopRepeatSendDatagram();
    }
}

void MainForm::onRepeatTimeChanged(int mSec) { m_repeatTime = mSec; }

void MainForm::onSendStyleChanged(int index) { m_sendStyle = static_cast<SendStyle>(index); }
