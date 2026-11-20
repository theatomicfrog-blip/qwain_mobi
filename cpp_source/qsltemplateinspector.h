// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#ifndef QSLTEMPLATEINSPECTOR_H
#define QSLTEMPLATEINSPECTOR_H

// ============================================================================
// MODULE: qsltemplateinspector
// ROLE: Real-time side-by-side forensic inspection window monitoring template
//       creation on canvas, QSettings disk serialization, and preview population.
// ============================================================================

#include <QDialog>
#include <QString>

class QTextBrowser;
class QTabWidget;
class QslDesigner;

class QslTemplateInspector : public QDialog {
    Q_OBJECT
public:
    explicit QslTemplateInspector(const QString &templateName, QslDesigner *designer = nullptr, QWidget *parent = nullptr);
    ~QslTemplateInspector() override = default;

    void refreshDiagnostics();

private:
    QString m_templateName;
    QslDesigner *m_designer = nullptr;

    QTabWidget *m_tabs = nullptr;
    QTextBrowser *m_canvasBrowser = nullptr;
    QTextBrowser *m_diskBrowser = nullptr;
    QTextBrowser *m_previewBrowser = nullptr;

    void inspectCanvasState();
    void inspectDiskSettings();
    void inspectPreviewPopulation();
};

#endif // QSLTEMPLATEINSPECTOR_H
