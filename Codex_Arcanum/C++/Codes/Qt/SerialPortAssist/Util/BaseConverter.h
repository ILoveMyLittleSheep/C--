#ifndef BASECONVERTER_H
#define BASECONVERTER_H

#include <QByteArray>
#include <QObject>
#include <QRegularExpression>
#include <QString>

class BaseConverter : public QObject
{
    Q_OBJECT
 public:
    BaseConverter();

    static QByteArray hexQStringToQByteArray(QString& hexString);

 signals:

 public slots:
};

#endif  // BASECONVERTER_H
