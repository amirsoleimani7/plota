#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>


// json file send and recive
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// sql stuff
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>






static QSqlDatabase initDatabase()
{
    // SQLite driver
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    // Create DB next to the server executable
    QString dbPath = QCoreApplication::applicationDirPath() + "/plota.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "DB open failed:" << db.lastError().text();
        return db;
    }

    QSqlQuery q(db);

    // Users table
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            username TEXT NOT NULL UNIQUE,
            phone TEXT NOT NULL,
            email TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        );
    )SQL")) {
        qDebug() << "Create users table failed:" << q.lastError().text();
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

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [=]() {
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
                else if (type == "signup") {
                    reply["type"] = "signup_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString name     = payload.value("name").toString().trimmed();
                    QString phone    = payload.value("phone").toString().trimmed();
                    QString email    = payload.value("email").toString().trimmed();
                    QString passHash = payload.value("passwordHash").toString();

                    if (username.isEmpty() || passHash.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "EMPTY_USERNAME_OR_PASSWORD";
                    } else {
                        QSqlQuery q(db);
                        q.prepare(R"SQL(
            INSERT INTO users (name, username, phone, email, password_hash)
            VALUES (:name, :username, :phone, :email, :password_hash);
        )SQL");
                        q.bindValue(":name", name);
                        q.bindValue(":username", username);
                        q.bindValue(":phone", phone);
                        q.bindValue(":email", email);
                        q.bindValue(":password_hash", passHash);

                        if (!q.exec()) {
                            // UNIQUE constraint hit => username taken
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

                else if (type == "login") {
                    reply["type"] = "login_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString passHash = payload.value("passwordHash").toString();

                    QSqlQuery q(db);
                    q.prepare(R"SQL(
        SELECT name, password_hash
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

                        if (storedHash != passHash) {
                            replyPayload["ok"] = false;
                            replyPayload["error"] = "WRONG_PASSWORD";
                        } else {
                            replyPayload["ok"] = true;
                            replyPayload["name"] = name;
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

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [=]() {
            qDebug() << "Client disconnected";
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
