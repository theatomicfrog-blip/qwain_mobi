// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#ifndef MW_LOOKUPS_H
#define MW_LOOKUPS_H

// ============================================================================
// MODULE: mw_lookups
// ROLE: Handles QRZ.com XML session authentication, HamQTH callsign queries,
//       XML tag parsing, and QSL info data extraction.
// ============================================================================

#include <QString>
#include <QVariantMap>

class MwLookupEngine {
public:
    static QString parseXmlTag(const QString &xml, const QString &tag);
    static QVariantMap parseQrzResponse(const QString &xmlData);
};

#endif // MW_LOOKUPS_H
