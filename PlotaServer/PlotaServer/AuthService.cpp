#include "AuthService.h"

#include "SessionManager.h"
#include "Validators.h"
#include "PasswordHasher.h"

#include <QTcpSocket>
#include <QSqlError>
#include <QSqlQuery>

AuthService::AuthService(QSqlDatabase &db, SessionManager &sessions)
    : m_db(db), m_sessions(sessions)
{
}

QJsonObject AuthService::handleLogin(QTcpSocket *socket, const QJsonObject &payload)
{
    QJsonObject replyPayload;

    QString username = payload.value("username").toString().trimmed();
    QString passwordPlain = payload.value("password").toString();
    QString legacyPassHash = payload.value("passwordHash").toString(); // TEMP compatibility

    if (username.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "EMPTY_USERNAME";
        return replyPayload;
    }

    // TEMP compatibility
    if (passwordPlain.isEmpty()) {
        if (!legacyPassHash.isEmpty() && !Validators::looksLikeSha256Hex(legacyPassHash)) {
            passwordPlain = legacyPassHash;
        }
    }

    if (passwordPlain.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "EMPTY_PASSWORD";
        return replyPayload;
    }

    QSqlQuery q(m_db);
    q.prepare(R"SQL(
        SELECT name, password_hash, salt
        FROM users
        WHERE username = :username;
    )SQL");
    q.bindValue(":username", username);

    if (!q.exec()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "DB_ERROR";
        replyPayload["detail"] = q.lastError().text();
        return replyPayload;
    }
    if (!q.next()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NO_SUCH_USER";
        return replyPayload;
    }

    QString name = q.value(0).toString();
    QString storedHash = q.value(1).toString();
    QString salt = q.value(2).toString();

    QString computed = PasswordHasher::hashPassword(salt, passwordPlain);

    if (computed != storedHash) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "WRONG_PASSWORD";
        return replyPayload;
    }

    // ✅ socket-binding
    m_sessions.bind(socket, username);

    replyPayload["ok"] = true;
    replyPayload["name"] = name;
    return replyPayload;
}

QJsonObject AuthService::handleSignup(const QJsonObject &payload)
{
    QJsonObject replyPayload;

    QString username = payload.value("username").toString().trimmed();
    QString name     = payload.value("name").toString().trimmed();
    QString phone    = payload.value("phone").toString().trimmed();
    QString email    = payload.value("email").toString().trimmed();

    QString passwordPlain = payload.value("password").toString();
    QString legacyPassHash = payload.value("passwordHash").toString(); // TEMP compatibility

    if (name.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "EMPTY_NAME";
        return replyPayload;
    }
    if (!Validators::isValidUsername(username)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_USERNAME";
        return replyPayload;
    }
    if (!Validators::isValidPhone(phone)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_PHONE";
        return replyPayload;
    }
    if (!Validators::isValidEmail(email)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_EMAIL";
        return replyPayload;
    }

    // require plain password (or TEMP accept legacy)
    if (passwordPlain.isEmpty()) {
        if (legacyPassHash.isEmpty()) {
            replyPayload["ok"] = false;
            replyPayload["error"] = "EMPTY_PASSWORD";
            return replyPayload;
        }
        if (Validators::looksLikeSha256Hex(legacyPassHash)) {
            replyPayload["ok"] = false;
            replyPayload["error"] = "PASSWORD_PROTOCOL_MISMATCH";
            return replyPayload;
        }
        passwordPlain = legacyPassHash; // TEMP treat as plain
    }

    if (!Validators::isStrongPasswordPlain(passwordPlain)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "WEAK_PASSWORD";
        return replyPayload;
    }

    QString salt = PasswordHasher::makeSalt();
    QString passHash = PasswordHasher::hashPassword(salt, passwordPlain);

    QSqlQuery q(m_db);
    q.prepare(R"SQL(
        INSERT INTO users (name, username, phone, email, password_hash, salt)
        VALUES (:name, :username, :phone, :email, :password_hash, :salt);
    )SQL");
    q.bindValue(":name", name);
    q.bindValue(":username", username);
    q.bindValue(":phone", phone);
    q.bindValue(":email", email);
    q.bindValue(":password_hash", passHash);
    q.bindValue(":salt", salt);

    if (!q.exec()) {
        if (q.lastError().text().contains("UNIQUE", Qt::CaseInsensitive)) {
            replyPayload["ok"] = false;
            replyPayload["error"] = "USERNAME_TAKEN";
        } else {
            replyPayload["ok"] = false;
            replyPayload["error"] = "DB_ERROR";
            replyPayload["detail"] = q.lastError().text();
        }
        return replyPayload;
    }

    replyPayload["ok"] = true;
    return replyPayload;
}

QJsonObject AuthService::handleGetProfile(QTcpSocket *socket)
{
    QJsonObject replyPayload;

    QString username = m_sessions.username(socket);
    if (username.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NOT_LOGGED_IN";
        return replyPayload;
    }

    QSqlQuery q(m_db);
    q.prepare(R"SQL(
        SELECT name, username, phone, email
        FROM users
        WHERE username = :username;
    )SQL");
    q.bindValue(":username", username);

    if (!q.exec()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "DB_ERROR";
        replyPayload["detail"] = q.lastError().text();
        return replyPayload;
    }
    if (!q.next()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NO_SUCH_USER";
        return replyPayload;
    }

    replyPayload["ok"] = true;
    replyPayload["name"] = q.value(0).toString();
    replyPayload["username"] = q.value(1).toString();
    replyPayload["phone"] = q.value(2).toString();
    replyPayload["email"] = q.value(3).toString();
    return replyPayload;
}

QJsonObject AuthService::handleUpdateProfile(QTcpSocket *socket, const QJsonObject &payload)
{
    QJsonObject replyPayload;

    QString username = m_sessions.username(socket);
    if (username.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NOT_LOGGED_IN";
        return replyPayload;
    }

    QString newName  = payload.value("newName").toString().trimmed();
    QString newPhone = payload.value("newPhone").toString().trimmed();
    QString newEmail = payload.value("newEmail").toString().trimmed();

    QString newPasswordPlain = payload.value("newPassword").toString();
    QString legacyNewHash = payload.value("newPasswordHash").toString(); // TEMP

    if (!newName.isEmpty() && newName.size() > 80) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_NAME";
        return replyPayload;
    }
    if (!newPhone.isEmpty() && !Validators::isValidPhone(newPhone)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_PHONE";
        return replyPayload;
    }
    if (!newEmail.isEmpty() && !Validators::isValidEmail(newEmail)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_EMAIL";
        return replyPayload;
    }

    // Password optional
    bool changePassword = false;
    QString newSalt, newHash;

    if (newPasswordPlain.isEmpty()) {
        if (!legacyNewHash.isEmpty() && !Validators::looksLikeSha256Hex(legacyNewHash)) {
            newPasswordPlain = legacyNewHash; // TEMP treat as plain
        }
    }

    if (!newPasswordPlain.isEmpty()) {
        if (!Validators::isStrongPasswordPlain(newPasswordPlain)) {
            replyPayload["ok"] = false;
            replyPayload["error"] = "WEAK_PASSWORD";
            return replyPayload;
        }
        changePassword = true;
        newSalt = PasswordHasher::makeSalt();
        newHash = PasswordHasher::hashPassword(newSalt, newPasswordPlain);
    }

    QSqlQuery q(m_db);

    if (!changePassword) {
        q.prepare(R"SQL(
            UPDATE users
            SET
                name  = CASE WHEN :name  = '' THEN name  ELSE :name  END,
                phone = CASE WHEN :phone = '' THEN phone ELSE :phone END,
                email = CASE WHEN :email = '' THEN email ELSE :email END
            WHERE username = :username;
        )SQL");
        q.bindValue(":name", newName);
        q.bindValue(":phone", newPhone);
        q.bindValue(":email", newEmail);
        q.bindValue(":username", username);
    } else {
        q.prepare(R"SQL(
            UPDATE users
            SET
                name  = CASE WHEN :name  = '' THEN name  ELSE :name  END,
                phone = CASE WHEN :phone = '' THEN phone ELSE :phone END,
                email = CASE WHEN :email = '' THEN email ELSE :email END,
                password_hash = :pass,
                salt = :salt
            WHERE username = :username;
        )SQL");
        q.bindValue(":name", newName);
        q.bindValue(":phone", newPhone);
        q.bindValue(":email", newEmail);
        q.bindValue(":pass", newHash);
        q.bindValue(":salt", newSalt);
        q.bindValue(":username", username);
    }

    if (!q.exec()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "DB_ERROR";
        replyPayload["detail"] = q.lastError().text();
        return replyPayload;
    }
    if (q.numRowsAffected() == 0) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NO_SUCH_USER";
        return replyPayload;
    }

    replyPayload["ok"] = true;
    replyPayload["username"] = username; // immutable
    return replyPayload;
}

QJsonObject AuthService::handleForgotPassword(const QJsonObject &payload)
{
    QJsonObject replyPayload;

    // NOTE: we'll update this after your client switches to newPassword.
    QString username = payload.value("username").toString().trimmed();
    QString phone    = payload.value("phone").toString().trimmed();

    QString newPasswordPlain = payload.value("newPassword").toString();
    QString legacyNewHash = payload.value("newPasswordHash").toString(); // TEMP

    if (username.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "EMPTY_USERNAME";
        return replyPayload;
    }
    if (!Validators::isValidPhone(phone)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "INVALID_PHONE";
        return replyPayload;
    }

    // TEMP compatibility
    if (newPasswordPlain.isEmpty()) {
        if (!legacyNewHash.isEmpty() && !Validators::looksLikeSha256Hex(legacyNewHash)) {
            newPasswordPlain = legacyNewHash;
        }
    }
    if (newPasswordPlain.isEmpty()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "EMPTY_PASSWORD";
        return replyPayload;
    }
    if (!Validators::isStrongPasswordPlain(newPasswordPlain)) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "WEAK_PASSWORD";
        return replyPayload;
    }

    // check phone matches
    QSqlQuery q1(m_db);
    q1.prepare(R"SQL(
        SELECT phone
        FROM users
        WHERE username = :username;
    )SQL");
    q1.bindValue(":username", username);

    if (!q1.exec()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "DB_ERROR";
        replyPayload["detail"] = q1.lastError().text();
        return replyPayload;
    }
    if (!q1.next()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "NO_SUCH_USER";
        return replyPayload;
    }

    QString storedPhone = q1.value(0).toString();
    if (storedPhone != phone) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "PHONE_MISMATCH";
        return replyPayload;
    }

    // update password with new salt/hash
    QString salt = PasswordHasher::makeSalt();
    QString passHash = PasswordHasher::hashPassword(salt, newPasswordPlain);

    QSqlQuery q2(m_db);
    q2.prepare(R"SQL(
        UPDATE users
        SET password_hash = :hash, salt = :salt
        WHERE username = :username;
    )SQL");
    q2.bindValue(":hash", passHash);
    q2.bindValue(":salt", salt);
    q2.bindValue(":username", username);

    if (!q2.exec()) {
        replyPayload["ok"] = false;
        replyPayload["error"] = "DB_ERROR";
        replyPayload["detail"] = q2.lastError().text();
        return replyPayload;
    }

    replyPayload["ok"] = true;
    return replyPayload;
}
