// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#include "qslpreview_spooler.h"
#include <QSettings>
#include <QCoreApplication>
#include <QFile>

// ============================================================================
// MODULE: qslpreview_spooler
// ROLE: Handles printer spooling, CUPS queue integration, PDF generation,
//       and batch printing pacing routines.
// ============================================================================

QString QslPreviewSpooler::formatPrinterDisplay(const QString &rawPrinterInfo) {
    if (rawPrinterInfo.isEmpty()) return "Default Printer";
    QString trimmed = rawPrinterInfo.trimmed();
    if (trimmed.contains(" ")) {
        return trimmed.split(" ").first();
    }
    return trimmed;
}

QStringList QslPreviewSpooler::getAvailablePrinters() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    return settings.value("KnownPrinters").toStringList();
}

bool QslPreviewSpooler::generatePdfDocument(const QString &outputPath, const QVariantMap &printSettings) {
    Q_UNUSED(printSettings);
    if (outputPath.isEmpty()) return false;
    // PDF Document Spooler Stub
    return true;
}
