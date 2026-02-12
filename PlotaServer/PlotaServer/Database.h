#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>

namespace Database {

// Open SQLite DB and ensure schema is ready. Returns an opened QSqlDatabase.
// If it fails, returned db may be not-open; caller should check db.isOpen().
QSqlDatabase initDatabase();

} // namespace Database

#endif // DATABASE_H
