#include "PasswordHasher.h"

#include <QUuid>
#include <QCryptographicHash>

namespace PasswordHasher {

QString makeSalt() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString hashPassword(const QString &salt, const QString &password) {
    QByteArray in = (salt + password).toUtf8();
    QByteArray out = QCryptographicHash::hash(in, QCryptographicHash::Sha256);
    return QString::fromUtf8(out.toHex());
}

} // namespace PasswordHasher
