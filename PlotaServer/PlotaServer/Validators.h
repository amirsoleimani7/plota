#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <QString>

namespace Validators {

// 3..20 chars, letters/digits/underscore, must start with letter
bool isValidUsername(const QString &u);

// Simple, practical email format check
bool isValidEmail(const QString &email);

// Digits only, length 10..15
bool isValidPhone(const QString &phone);

// Min 8, has upper+lower+digit+special
bool isStrongPasswordPlain(const QString &pw);

// 64 hex chars (sha256)
bool looksLikeSha256Hex(const QString &s);

} // namespace Validators

#endif // VALIDATORS_H
