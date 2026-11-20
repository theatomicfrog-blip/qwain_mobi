// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#ifndef QSLPREVIEW_SPOOLER_H
#define QSLPREVIEW_SPOOLER_H

// ============================================================================
// MODULE: qslpreview_spooler
// ROLE: Handles printer spooling, CUPS queue integration, PDF generation,
//       and batch printing pacing routines.
// ============================================================================

#include <QString>
#include <QStringList>
#include <QVariantMap>

class QslPreviewSpooler {
public:
    static QString formatPrinterDisplay(const QString &rawPrinterInfo);
    static QStringList getAvailablePrinters();
    static bool generatePdfDocument(const QString &outputPath, const QVariantMap &printSettings);
};

#endif // QSLPREVIEW_SPOOLER_H
