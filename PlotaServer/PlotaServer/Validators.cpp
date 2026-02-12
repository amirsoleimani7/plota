#include "Validators.h"
#include <QRegularExpression>

namespace Validators {

bool isValidUsername(const QString &u) {
    static QRegularExpression re(R"(^[A-Za-z][A-Za-z0-9_]{2,19}$)");
    return re.match(u).hasMatch();
}

bool isValidEmail(const QString &email) {
    static QRegularExpression re(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
    return re.match(email).hasMatch();
}

bool isValidPhone(const QString &phone) {
    static QRegularExpression re(R"(^\d{10,15}$)");
    return re.match(phone).hasMatch();
}

bool isStrongPasswordPlain(const QString &pw) {
    if (pw.size() < 8) return false;
    bool hasUpper=false, hasLower=false, hasDigit=false, hasSpecial=false;
    for (QChar c : pw) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

bool looksLikeSha256Hex(const QString &s) {
    static QRegularExpression re(R"(^[0-9a-fA-F]{64}$)");
    return re.match(s).hasMatch();
}

} // namespace Validators
