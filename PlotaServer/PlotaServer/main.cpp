#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>

// hashing
#include <QHash>
#include <QRegularExpression>
#include <QUuid>
#include <QCryptographicHash>

// json file send and recive
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// sql stuff
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>

// validation for signup
static bool isValidUsername(const QString &u) {
    // 3..20 chars, letters/digits/underscore, must start with letter
    static QRegularExpression re(R"(^[A-Za-z][A-Za-z0-9_]{2,19}$)");
    return re.match(u).hasMatch();
}

static bool isValidEmail(const QString &email) {
    static QRegularExpression re(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
    return re.match(email).hasMatch();
}

static bool isValidPhone(const QString &phone) {
    // digits only, length 10..15 (simple + practical)
    static QRegularExpression re(R"(^\d{10,15}$)");
    return re.match(phone).hasMatch();
}

static bool isStrongPasswordPlain(const QString &pw) {
    if (pw.size() < 8) return false; // required by spec
    bool hasUpper=false, hasLower=false, hasDigit=false, hasSpecial=false;
    for (QChar c : pw) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

static bool looksLikeSha256Hex(const QString &s) {
    static QRegularExpression re(R"(^[0-9a-fA-F]{64}$)");
    return re.match(s).hasMatch();
}


static bool isStrongPassword(const QString &pw) {
    if (pw.size() < 8) return false; // required
    bool hasUpper=false, hasLower=false, hasDigit=false, hasSpecial=false;
    for (QChar c : pw) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}


// salt hash
static QString makeSalt() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

static QString hashPassword(const QString &salt, const QString &password) {
    QByteArray in = (salt + password).toUtf8();
    QByteArray out = QCryptographicHash::hash(in, QCryptographicHash::Sha256);
    return QString::fromUtf8(out.toHex());
}


static bool columnExists(QSqlDatabase &db, const QString &table, const QString &column)
{
    QSqlQuery q(db);
    q.prepare("PRAGMA table_info(" + table + ");");
    if (!q.exec()) return false;

    while (q.next()) {
        // PRAGMA table_info columns: cid, name, type, notnull, dflt_value, pk
        if (q.value(1).toString().compare(column, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

static QSqlDatabase initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QString dbPath = QCoreApplication::applicationDirPath() + "/plota.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "DB open failed:" << db.lastError().text();
        return db;
    }

    QSqlQuery q(db);

    // Create table with salt included (fresh DB)
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            username TEXT NOT NULL UNIQUE,
            phone TEXT NOT NULL,
            email TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        );
    )SQL")) {
        qDebug() << "Create users table failed:" << q.lastError().text();
    }

    // Migration for old DBs without salt
    if (!columnExists(db, "users", "salt")) {
        if (!q.exec("ALTER TABLE users ADD COLUMN salt TEXT;")) {
            qDebug() << "Add salt column failed:" << q.lastError().text();
        } else {
            // Fill salt for existing rows (weak, but avoids NULL). Ideally force reset.
            // We'll set random salt and recompute hash only if we had plain (we didn't).
            // For now: set salt to random and keep existing hash as-is.
            QSqlQuery fill(db);
            if (!fill.exec("UPDATE users SET salt = 'legacy' WHERE salt IS NULL OR salt = '';")) {
                qDebug() << "Fill legacy salt failed:" << fill.lastError().text();
            }
        }
    }

    return db;
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QSqlDatabase db = initDatabase();
    if (!db.isOpen()) {
        qDebug() << "Database is not open. Exiting.";
        return -1;
    }


    QTcpServer server;
    QHash<QTcpSocket*, QString> socketUser; // empty = not logged in


    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        socketUser[clientSocket] = "";
        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [&socketUser, clientSocket, &db]() {
            while (clientSocket->canReadLine()) {
                QByteArray line = clientSocket->readLine().trimmed();

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(line, &err);

                if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                    qDebug() << "Invalid JSON:" << line;
                    continue;
                }

                QJsonObject msg = doc.object();
                QString type = msg.value("type").toString();
                QJsonObject payload = msg.value("payload").toObject();

                qDebug() << "JSON type =" << type << "payload =" << payload;

                // reply example
                // ---- handle message types here ----
                QJsonObject reply;
                QJsonObject replyPayload;

                if (type == "hello") {
                    reply["type"] = "hello_ack";
                    replyPayload["ok"] = true;
                }
                else if (type == "login") {
                    reply["type"] = "login_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString passwordPlain = payload.value("password").toString();
                    QString legacyPassHash = payload.value("passwordHash").toString(); // TEMP compatibility

                    if (username.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "EMPTY_USERNAME";
                    } else {
                        // Protocol compatibility: if password is not present, treat passwordHash as plain if not sha256
                        if (passwordPlain.isEmpty()) {
                            if (!legacyPassHash.isEmpty() && !looksLikeSha256Hex(legacyPassHash)) {
                                passwordPlain = legacyPassHash; // TEMP
                            }
                        }

                        if (passwordPlain.isEmpty()) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "EMPTY_PASSWORD";
                        } else {
                            QSqlQuery q(db);
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
                            } else if (!q.next()) {
                                replyPayload["ok"] = false;
                                replyPayload["error"] = "NO_SUCH_USER";
                            } else {
                                QString name = q.value(0).toString();
                                QString storedHash = q.value(1).toString();
                                QString salt = q.value(2).toString();

                                QString computed = hashPassword(salt, passwordPlain);

                                if (computed != storedHash) {
                                    replyPayload["ok"] = false;
                                    replyPayload["error"] = "WRONG_PASSWORD";
                                } else {
                                    replyPayload["ok"] = true;
                                    replyPayload["name"] = name;

                                    // ✅ socket-binding (no tokens)
                                    socketUser[clientSocket] = username;
                                }
                            }
                        }
                    }
                }

                else if (type == "signup") {
                    reply["type"] = "signup_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString name     = payload.value("name").toString().trimmed();
                    QString phone    = payload.value("phone").toString().trimmed();
                    QString email    = payload.value("email").toString().trimmed();

                    QString passwordPlain = payload.value("password").toString();
                    QString legacyPassHash = payload.value("passwordHash").toString(); // TEMP compatibility

                    // ---- validations ----
                    if (name.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "EMPTY_NAME";
                    }
                    else if (!isValidUsername(username)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "INVALID_USERNAME";
                    }
                    else if (!isValidPhone(phone)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "INVALID_PHONE";
                    }
                    else if (!isValidEmail(email)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "INVALID_EMAIL";
                    }
                    else {
                        // Require plain password for proper strength validation + server hashing
                        if (passwordPlain.isEmpty()) {
                            // If you haven't updated client yet, it may still send the password in passwordHash.
                            // We treat legacyPassHash as a plain password if it doesn't look like sha256.
                            if (legacyPassHash.isEmpty()) {
                                replyPayload["ok"] = false;
                                replyPayload["error"] = "EMPTY_PASSWORD";
                            } else if (looksLikeSha256Hex(legacyPassHash)) {
                                replyPayload["ok"] = false;
                                replyPayload["error"] = "PASSWORD_PROTOCOL_MISMATCH"; // force client to send plain password
                            } else {
                                passwordPlain = legacyPassHash; // treat as plain (TEMP)
                            }
                        }

                        if (!replyPayload.contains("error")) {
                            if (!isStrongPasswordPlain(passwordPlain)) {
                                replyPayload["ok"] = false;
                                replyPayload["error"] = "WEAK_PASSWORD";
                            } else {
                                // ---- salted hash ----
                                QString salt = makeSalt();
                                QString passHash = hashPassword(salt, passwordPlain);

                                QSqlQuery q(db);
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
                                } else {
                                    replyPayload["ok"] = true;
                                }
                            }
                        }
                    }
                }
                else if (type == "update_profile") {
                    reply["type"] = "update_profile_result";

                    QString username = socketUser.value(clientSocket);
                    if (username.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "NOT_LOGGED_IN";
                    } else {
                        QString newName  = payload.value("newName").toString().trimmed();
                        QString newPhone = payload.value("newPhone").toString().trimmed();
                        QString newEmail = payload.value("newEmail").toString().trimmed();

                        QString newPasswordPlain = payload.value("newPassword").toString();
                        QString legacyNewHash = payload.value("newPasswordHash").toString(); // TEMP

                        // Basic validations (allow empty -> "keep old", BUT client should normally send full profile)
                        if (!newName.isEmpty() && newName.size() > 80) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "INVALID_NAME";
                        }
                        else if (!newPhone.isEmpty() && !isValidPhone(newPhone)) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "INVALID_PHONE";
                        }
                        else if (!newEmail.isEmpty() && !isValidEmail(newEmail)) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "INVALID_EMAIL";
                        }
                        else {
                            // password optional
                            bool changePassword = false;
                            QString newSalt, newHash;

                            if (newPasswordPlain.isEmpty()) {
                                if (!legacyNewHash.isEmpty() && !looksLikeSha256Hex(legacyNewHash)) {
                                    newPasswordPlain = legacyNewHash; // TEMP treat as plain
                                }
                            }

                            if (!newPasswordPlain.isEmpty()) {
                                if (!isStrongPasswordPlain(newPasswordPlain)) {
                                    replyPayload["ok"] = false;
                                    replyPayload["error"] = "WEAK_PASSWORD";
                                } else {
                                    changePassword = true;
                                    newSalt = makeSalt();
                                    newHash = hashPassword(newSalt, newPasswordPlain);
                                }
                            }

                            if (!replyPayload.contains("error")) {
                                // Build update query dynamically-ish:
                                // If a field is empty, keep existing (COALESCE / CASE approach)
                                QSqlQuery q(db);

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
                                } else if (q.numRowsAffected() == 0) {
                                    replyPayload["ok"] = false;
                                    replyPayload["error"] = "NO_SUCH_USER";
                                } else {
                                    replyPayload["ok"] = true;
                                    replyPayload["username"] = username; // immutable
                                }
                            }
                        }
                    }
                }
                else if (type == "forgot_password") {
                    reply["type"] = "forgot_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString phone    = payload.value("phone").toString().trimmed();
                    QString newHash  = payload.value("newPasswordHash").toString();

                    // 1) check user + phone
                    QSqlQuery q1(db);
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
                    } else if (!q1.next()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "NO_SUCH_USER";
                    } else {
                        QString storedPhone = q1.value(0).toString();
                        if (storedPhone != phone) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "PHONE_MISMATCH";
                        } else {
                            // 2) update password
                            QSqlQuery q2(db);
                            q2.prepare(R"SQL(
                UPDATE users
                SET password_hash = :new_hash
                WHERE username = :username;
            )SQL");
                            q2.bindValue(":new_hash", newHash);
                            q2.bindValue(":username", username);

                            if (!q2.exec()) {
                                replyPayload["ok"] = false;
                                replyPayload["error"] = "DB_ERROR";
                                replyPayload["detail"] = q2.lastError().text();
                            } else {
                                replyPayload["ok"] = true;
                            }
                        }
                    }
                }
                else if (type == "get_profile") {
                    reply["type"] = "profile_result";

                    QString username = socketUser.value(clientSocket);
                    if (username.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "NOT_LOGGED_IN";
                    } else {
                        QSqlQuery q(db);
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
                        } else if (!q.next()) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "NO_SUCH_USER";
                        } else {
                            replyPayload["ok"] = true;
                            replyPayload["name"] = q.value(0).toString();
                            replyPayload["username"] = q.value(1).toString();
                            replyPayload["phone"] = q.value(2).toString();
                            replyPayload["email"] = q.value(3).toString();
                        }
                    }
                }


                else {
                    reply["type"] = "error";
                    replyPayload["ok"] = false;
                    replyPayload["error"] = "UNKNOWN_TYPE";
                }

                reply["payload"] = replyPayload;

                QByteArray out = QJsonDocument(reply).toJson(QJsonDocument::Compact);
                out.append('\n');
                clientSocket->write(out);
            }
        });

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [&socketUser, clientSocket]() {
            qDebug() << "Client disconnected";
            socketUser.remove(clientSocket);
            clientSocket->deleteLater();
        });
    });

    if (!server.listen(QHostAddress::Any, 45454)) {
        qDebug() << "Server failed to start!";
        return -1;
    }

    qDebug() << "Server listening on port 45454...";

    return a.exec();
}
