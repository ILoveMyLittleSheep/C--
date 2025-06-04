#include "BaseConverter.h"

BaseConverter::BaseConverter() {}

QByteArray BaseConverter::hexQStringToQByteArray(QString &hexString)
{
    QRegularExpression re("\\s+");  // 此正则表达式匹配一个或多个空白字符
    hexString.remove(re);

    QByteArray bytes;
    bool ok;
    int i      = 0;
    int length = hexString.length();

    while (i < length)
    {
        if (length - i < 2)
        {
            break;
        }
        bytes.append(static_cast<char>((hexString.mid(i, 2).toInt(&ok, 16) & 0xFF)));
        if (!ok)
        {
            break;
        }
        i += 2;
    }
    if (i != length || !ok)
    {
        return QByteArray();
    }
    return bytes;
}
