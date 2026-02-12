#include "Database.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace {

// helper: check if a column exists
bool columnExists(QSqlDatabase &db, const QString &table, const QString &column)
{
    QSqlQuery q(db);
    if (!q.exec("PRAGMA table_info(" + table + ");"))
        return false;

    while (q.next()) {
        if (q.value(1).toString().compare(column, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

} // namespace

namespace Database {

QSqlDatabase initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    const QString dbPath = QCoreApplication::applicationDirPath() + "/plota.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "DB open failed:" << db.lastError().text();
        return db;
    }

    QSqlQuery q(db);

    // Fresh DB schema (salt included)
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

    // Migration for older DBs without salt column
    if (!columnExists(db, "users", "salt")) {
        if (!q.exec("ALTER TABLE users ADD COLUMN salt TEXT;")) {
            qDebug() << "Add salt column failed:" << q.lastError().text();
        } else {
            // We can't recompute hashes safely; mark legacy salts so schema is non-null-ish.
            QSqlQuery fill(db);
            if (!fill.exec("UPDATE users SET salt = 'legacy' WHERE salt IS NULL OR salt = '';")) {
                qDebug() << "Fill legacy salt failed:" << fill.lastError().text();
            }
        }
    }

    return db;
}

} // namespace Database
