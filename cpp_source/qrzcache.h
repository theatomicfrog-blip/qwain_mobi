#ifndef QRZCACHE_H
#define QRZCACHE_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDataStream>
#include <QByteArray>

class QrzCache {
public:
    enum Format { IniFormat = 0 }; // Compatibility with QSettings

    QrzCache(const QString &filePath = QString(), int format = 0);
    ~QrzCache() {}

    void beginGroup(const QString &prefix);
    void endGroup();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    bool contains(const QString &key) const;
    void remove(const QString &key);
    QStringList childGroups() const;
    QStringList childKeys() const;
    void sync();

private:
    QString m_currentGroup;
    static void initDatabase(const QString &dbPath);
};

#endif // QRZCACHE_H
