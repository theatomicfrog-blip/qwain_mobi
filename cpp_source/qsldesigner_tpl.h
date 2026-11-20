// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#ifndef QSLDESIGNER_TPL_H
#define QSLDESIGNER_TPL_H

// ============================================================================
// MODULE: qsldesigner_tpl
// ROLE: Encapsulates all-inclusive Qwain QSL template JSON export options,
//       explicit .tpl signature encoding ("QWAIN-TPL-V1"), base64 image asset
//       embedding/restoration, and template import validation.
// ============================================================================

#include <QWidget>
#include <QString>

class QslDesigner;

class QslTemplateIO {
public:
    static void exportTemplate(QslDesigner *designer, QWidget *parent);
    static void importTemplate(QslDesigner *designer, QWidget *parent);
};

#endif // QSLDESIGNER_TPL_H
