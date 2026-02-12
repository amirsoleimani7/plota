#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <QString>

namespace PasswordHasher {

// Random salt (UUID-based)
QString makeSalt();

// SHA-256 hex of (salt + password)
QString hashPassword(const QString &salt, const QString &password);

} // namespace PasswordHasher

#endif // PASSWORDHASHER_H
