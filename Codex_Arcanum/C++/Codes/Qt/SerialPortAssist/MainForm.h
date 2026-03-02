#ifndef MAINFORM_H
#define MAINFORM_H

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMainWindow>
#include <QMessageBox>
#include <QScrollBar>
#include <QSerialPort>
#include <QTimer>
#include <string>
using namespace std;
#include "Business/DataRevAndSend.h"
#include "Util/BaseConverter.h"

namespace Ui
{
class SeialPort;
}

class MainForm : public QMainWindow
{
    Q_OBJECT

 public:
    explicit MainForm(QWidget *parent = 0);
    ~MainForm();

    void boundUIOperation();
    void initUI();

    enum SendStyle
    {
        HexSend,
        ASCIISend
    };

    enum DisplayStyle
    {
        HexRev,
        ASCIIRev
    };

 private:
    Ui::SeialPort *ui;

    // serialport
    DataRevAndSend *m_dataRevAndSend = nullptr;

    // serialport status
    bool m_serialPortStatus = false;

    // display style
    DisplayStyle m_displayStyle = DisplayStyle::HexRev;

    // send style
    SendStyle m_sendStyle = SendStyle::HexSend;
    bool m_repeatlySend   = false;
    uint m_repeatTime     = 0;

    // udpate timer
    QTimer *m_timer = nullptr;
 signals:
    void readySendDatagram(const QByteArray &datagram, int m_repeatTime);
    void stopRepeatSendDatagram();
 private slots:
    void onReceiveData(const QByteArray &data);
    void onOpenSerialPort(bool flag);
    void onSendData(bool flag);
    void onDisplayMsg(QString msg);

    void onCmbBaudrateChanged(int index);
    void onCmbRevStyleChanged(int index);
    void onCheckBoxRepeatChanged(bool flag);
    void onRepeatTimeChanged(int mSec);
    void onSendStyleChanged(int index);
    void onClearView(bool flag);
    void onCmbOpenModeChanged(int index);

    void onUpdateUI();
};

#endif  // MAINFORM_H
