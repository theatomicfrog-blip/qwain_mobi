// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#include "mw_lookups.h"
#include <QRegularExpression>

// ============================================================================
// MODULE: mw_lookups
// ROLE: Handles QRZ.com XML session authentication, HamQTH callsign queries,
//       XML tag parsing, and QSL info data extraction.
// ============================================================================

QString MwLookupEngine::parseXmlTag(const QString &xml, const QString &tag) {
    if (xml.isEmpty() || tag.isEmpty()) return "";
    
    QRegularExpression re(QString("<%1>(.*?)</%1>").arg(tag), QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = re.match(xml);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return "";
}

QVariantMap MwLookupEngine::parseQrzResponse(const QString &xmlData) {
    QVariantMap res;
    if (xmlData.isEmpty()) return res;

    res["call"] = parseXmlTag(xmlData, "call");
    res["fname"] = parseXmlTag(xmlData, "fname");
    res["name"] = parseXmlTag(xmlData, "name");
    res["addr1"] = parseXmlTag(xmlData, "addr1");
    res["addr2"] = parseXmlTag(xmlData, "addr2");
    res["state"] = parseXmlTag(xmlData, "state");
    res["zip"] = parseXmlTag(xmlData, "zip");
    res["country"] = parseXmlTag(xmlData, "country");
    res["grid"] = parseXmlTag(xmlData, "grid");
    res["lat"] = parseXmlTag(xmlData, "lat");
    res["lon"] = parseXmlTag(xmlData, "lon");
    res["image"] = parseXmlTag(xmlData, "image");

    return res;
}
