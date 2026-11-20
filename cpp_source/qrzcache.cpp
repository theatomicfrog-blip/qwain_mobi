// ============================================================================
// COMPONENT: QrzCache
// ROLE: Multi-database SQLite transactional storage and profile caching layer.
//       This component operates as an INI-compatible backend proxy, splitting callsign
//       lookup metadata and heavy flag graphics/icons into separate SQLite stores,
//       supporting transactions, automatic legacy INI-to-SQL migration, and self-healing DB scrubs.
// INVOKED BY: MainWindow, QslManagerDialog, DxClusterWidget, PrefetchFlagsDialog.
// PRODUCES: Highly efficient metadata reads/writes and cached graphics blobs.
// ============================================================================

#include "qrzcache.h"
#include "paths.h"
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDataStream>

static inline QString getDbConnectionName(const QString &group) {
    if (group.trimmed().toUpper() == "ICONS") {
        return "qrz_graphics_db";
    }
    return "qrz_cache_db";
}

// [ENTRY POINT: QrzCache Constructor]
// Acts as a proxy interface; fetches the standardized cache database file path 
// and triggers the initial database structures bootstrap process.
QrzCache::QrzCache(const QString &filePath, int format) {
    Q_UNUSED(filePath);
    Q_UNUSED(format);
    
    QString dbPath = getAppDataDir() + "/qrz_cache.db";
    initDatabase(dbPath);
}

// [ENTRY POINT: SQLite Database Bootstrap & Migration Engine]
// Configures the SQLite graphics database, registers schemas, auto-seeds default icons, 
// and executes transactional migrations if legacy INI files are found on disk.
void QrzCache::initDatabase(const QString &dbPath) {
    // 1. Initialize Graphics Cache DB first
    QSqlDatabase gdb = QSqlDatabase::database("qrz_graphics_db", false);
    if (!gdb.isValid()) {
        gdb = QSqlDatabase::addDatabase("QSQLITE", "qrz_graphics_db");
        gdb.setDatabaseName(getAppDataDir() + "/qrz_graphics.db");
    }
    if (!gdb.isOpen()) {
        if (!gdb.open()) {
            qWarning() << "Failed to open Graphics Cache DB:" << gdb.lastError().text();
            return;
        }
        QSqlQuery gq(gdb);
        gq.exec("CREATE TABLE IF NOT EXISTS qrz_cache ("
                "group_name TEXT, "
                "key_name TEXT, "
                "value_data BLOB, "
                "PRIMARY KEY (group_name, key_name))");
        gq.exec("CREATE INDEX IF NOT EXISTS idx_qrz_group ON qrz_cache(group_name)");
    }

    // Auto-seed native icons if missing in the graphics database
    {
        QSqlQuery qCheck(gdb);
        qCheck.prepare("SELECT COUNT(*) FROM qrz_cache WHERE group_name = 'ICONS' AND key_name = ?");
        
        auto ensureIcon = [&](const QString &key, const QString &resPath) {
            qCheck.bindValue(0, key);
            if (qCheck.exec() && qCheck.next() && qCheck.value(0).toInt() == 0) {
                QFile file(resPath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray valBytes;
                    QDataStream out(&valBytes, QIODevice::WriteOnly);
                    out << QVariant(file.readAll());
                    
                    QSqlQuery ins(gdb);
                    ins.prepare("INSERT INTO qrz_cache (group_name, key_name, value_data) VALUES ('ICONS', ?, ?)");
                    ins.bindValue(0, key);
                    ins.bindValue(1, valBytes);
                    ins.exec();
                }
            }
        };
        ensureIcon("arrl_icon", ":/arrl_small.png");
        ensureIcon("env_icon", ":/env.png");
    }

    // 2. Initialize Callsign Cache DB
    QSqlDatabase db = QSqlDatabase::database("qrz_cache_db", false);
    bool isNewDbConnection = !db.isValid();
    if (isNewDbConnection) {
        db = QSqlDatabase::addDatabase("QSQLITE", "qrz_cache_db");
        db.setDatabaseName(dbPath);
    }
    if (!db.isOpen()) {
        if (!db.open()) {
            qWarning() << "Failed to open QrzCache DB:" << db.lastError().text();
            return;
        }
    }

    if (isNewDbConnection) {
        QSqlQuery q(db);
        q.exec("CREATE TABLE IF NOT EXISTS qrz_cache ("
               "group_name TEXT, "
               "key_name TEXT, "
               "value_data BLOB, "
               "PRIMARY KEY (group_name, key_name))");
        q.exec("CREATE INDEX IF NOT EXISTS idx_qrz_group ON qrz_cache(group_name)");
        
        // Migrate old INI cache to SQLite databases (routing ICONS to graphics db)
        QString iniPath = getAppDataDir() + "/qrz_cache.ini";
        if (QFile::exists(iniPath)) {
            qDebug() << "Migrating qrz_cache.ini to SQLite databases...";
            QSettings oldCache(iniPath, QSettings::IniFormat);
            
            QStringList groups = oldCache.childGroups();
            for (const QString &grp : groups) {
                QString upperGrp = grp.toUpper();
                QSqlDatabase targetDb = (upperGrp == "ICONS") ? gdb : db;
                
                targetDb.transaction();
                QSqlQuery ins(targetDb);
                ins.prepare("INSERT OR REPLACE INTO qrz_cache (group_name, key_name, value_data) VALUES (?, ?, ?)");
                
                oldCache.beginGroup(grp);
                QStringList keys = oldCache.allKeys();
                for (const QString &k : keys) {
                    QVariant val = oldCache.value(k);
                    QByteArray valBytes;
                    QDataStream out(&valBytes, QIODevice::WriteOnly);
                    out << val;
                    
                    ins.bindValue(0, upperGrp);
                    ins.bindValue(1, k);
                    ins.bindValue(2, valBytes);
                    ins.exec();
                }
                oldCache.endGroup();
                targetDb.commit();
            }
            qDebug() << "Migration completed successfully!";
            
            // Rename to prevent re-migration
            QFile::rename(iniPath, iniPath + ".migrated");
        }
        
        // Automated self-healing database scrub for corrupted city cache entries
        QSqlQuery scrub(db);
        if (scrub.exec("SELECT group_name, key_name, value_data FROM qrz_cache WHERE key_name IN ('qrz_city', 'qrz_addr2')")) {
            QHash<QString, QString> groupCities;
            QHash<QString, QString> groupAddr2s;
            while (scrub.next()) {
                QString grp = scrub.value(0).toString().trimmed().toUpper();
                QString key = scrub.value(1).toString().trimmed();
                QByteArray valBytes = scrub.value(2).toByteArray();
                QDataStream in(&valBytes, QIODevice::ReadOnly);
                QVariant val;
                in >> val;
                QString str = val.toString().trimmed();
                
                if (key == "qrz_city") {
                    groupCities[grp] = str;
                } else if (key == "qrz_addr2") {
                    groupAddr2s[grp] = str;
                }
            }
            
            QStringList corruptedGroups;
            static const QRegularExpression zipRegex("\\b\\d{5}\\b");
            
            // Gather all group names to analyze (using modern QSet range constructor to avoid deprecated QSet::fromList)
            const auto cityKeys = groupCities.keys();
            const auto addr2Keys = groupAddr2s.keys();
            QSet<QString> allGroups = QSet<QString>(cityKeys.begin(), cityKeys.end()) + QSet<QString>(addr2Keys.begin(), addr2Keys.end());
            for (const QString &grp : allGroups) {
                if (grp == "ICONS") continue; // Skip icon groups from scrubbing
                QString city = groupCities.value(grp);
                QString addr2 = groupAddr2s.value(grp);
                
                bool isCorrupted = false;
                if (!city.isEmpty()) {
                    if (city.contains(zipRegex)) {
                        isCorrupted = true;
                    }
                } else if (!addr2.isEmpty()) {
                    if (addr2.contains(zipRegex)) {
                        isCorrupted = true;
                    }
                }
                
                if (isCorrupted) {
                    corruptedGroups.append(grp);
                }
            }
            
            if (!corruptedGroups.isEmpty()) {
                db.transaction();
                QSqlQuery del(db);
                del.prepare("DELETE FROM qrz_cache WHERE group_name = ?");
                for (const QString &grp : corruptedGroups) {
                    del.bindValue(0, grp);
                    del.exec();
                }
                db.commit();
                qDebug() << "Auto-scrubbed" << corruptedGroups.size() << "corrupted cache entries containing ZIP codes in the city or fallback field.";
            }
        }
    }
}

void QrzCache::beginGroup(const QString &prefix) {
    m_currentGroup = prefix.toUpper();
}

void QrzCache::endGroup() {
    m_currentGroup.clear();
}

void QrzCache::setValue(const QString &key, const QVariant &value) {
    QString group = m_currentGroup;
    QString realKey = key;
    
    if (realKey.contains('/')) {
        int idx = realKey.lastIndexOf('/');
        group = realKey.left(idx).toUpper();
        realKey = realKey.mid(idx + 1);
    }
    
    QSqlDatabase db = QSqlDatabase::database(getDbConnectionName(group));
    if (!db.isOpen()) return;
    
    QByteArray valBytes;
    QDataStream out(&valBytes, QIODevice::WriteOnly);
    out << value;
    
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO qrz_cache (group_name, key_name, value_data) VALUES (?, ?, ?)");
    q.bindValue(0, group);
    q.bindValue(1, realKey);
    q.bindValue(2, valBytes);
    if (!q.exec()) {
        qWarning() << "QrzCache::setValue failed:" << q.lastError().text();
    }
}

QVariant QrzCache::value(const QString &key, const QVariant &defaultValue) const {
    QString group = m_currentGroup;
    QString realKey = key;
    
    if (realKey.contains('/')) {
        int idx = realKey.lastIndexOf('/');
        group = realKey.left(idx).toUpper();
        realKey = realKey.mid(idx + 1);
    }
    
    QSqlDatabase db = QSqlDatabase::database(getDbConnectionName(group));
    if (!db.isOpen()) return defaultValue;
    
    QSqlQuery q(db);
    q.prepare("SELECT value_data FROM qrz_cache WHERE group_name = ? AND key_name = ?");
    q.bindValue(0, group);
    q.bindValue(1, realKey);
    
    if (q.exec() && q.next()) {
        QByteArray valBytes = q.value(0).toByteArray();
        QDataStream in(&valBytes, QIODevice::ReadOnly);
        QVariant val;
        in >> val;
        return val;
    }
    return defaultValue;
}

bool QrzCache::contains(const QString &key) const {
    QString group = m_currentGroup;
    QString realKey = key;
    
    if (realKey.contains('/')) {
        int idx = realKey.lastIndexOf('/');
        group = realKey.left(idx).toUpper();
        realKey = realKey.mid(idx + 1);
    }
    
    QSqlDatabase db = QSqlDatabase::database(getDbConnectionName(group));
    if (!db.isOpen()) return false;
    
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM qrz_cache WHERE group_name = ? AND key_name = ?");
    q.bindValue(0, group);
    q.bindValue(1, realKey);
    
    if (q.exec() && q.next()) {
        return q.value(0).toInt() > 0;
    }
    return false;
}

void QrzCache::remove(const QString &key) {
    QString group = m_currentGroup;
    QString realKey = key;
    
    if (realKey.contains('/')) {
        int idx = realKey.lastIndexOf('/');
        group = realKey.left(idx).toUpper();
        realKey = realKey.mid(idx + 1);
    }
    
    QSqlDatabase db = QSqlDatabase::database(getDbConnectionName(group));
    if (!db.isOpen()) return;
    
    QSqlQuery q(db);
    if (realKey.isEmpty()) {
        q.prepare("DELETE FROM qrz_cache WHERE group_name = ?");
        q.bindValue(0, group);
    } else {
        q.prepare("DELETE FROM qrz_cache WHERE group_name = ? AND key_name = ?");
        q.bindValue(0, group);
        q.bindValue(1, realKey);
    }
    q.exec();
}

QStringList QrzCache::childGroups() const {
    QStringList list;
    
    // 1. Fetch from Callsign DB
    QSqlDatabase db = QSqlDatabase::database("qrz_cache_db");
    if (db.isOpen()) {
        QSqlQuery q(db);
        q.prepare("SELECT DISTINCT group_name FROM qrz_cache");
        if (q.exec()) {
            while (q.next()) {
                list << q.value(0).toString();
            }
        }
    }
    
    // 2. Fetch from Graphics DB
    QSqlDatabase gdb = QSqlDatabase::database("qrz_graphics_db");
    if (gdb.isOpen()) {
        QSqlQuery gq(gdb);
        gq.prepare("SELECT DISTINCT group_name FROM qrz_cache");
        if (gq.exec()) {
            while (gq.next()) {
                QString grp = gq.value(0).toString();
                if (!list.contains(grp)) {
                    list << grp;
                }
            }
        }
    }
    return list;
}

QStringList QrzCache::childKeys() const {
    QSqlDatabase db = QSqlDatabase::database(getDbConnectionName(m_currentGroup));
    QStringList list;
    if (!db.isOpen()) return list;
    
    QSqlQuery q(db);
    q.prepare("SELECT key_name FROM qrz_cache WHERE group_name = ?");
    q.bindValue(0, m_currentGroup);
    if (q.exec()) {
        while (q.next()) {
            list << q.value(0).toString();
        }
    }
    return list;
}

void QrzCache::sync() {
}
