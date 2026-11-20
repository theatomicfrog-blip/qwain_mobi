// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#ifndef QSLDESIGNER_CANVAS_H
#define QSLDESIGNER_CANVAS_H

// ============================================================================
// MODULE: qsldesigner_canvas
// ROLE: Encapsulates WYSIWYG canvas graphics rulers, crosshair overlays,
//       mouse telemetry coordinate tracking, and unit conversions.
// ============================================================================

#include <QPointF>
#include <QString>

class QslCanvasUtils {
public:
    static QString formatCoordinates(QPointF pos, bool isCm);
    static double convertInchesToCm(double inches);
    static double convertCmToInches(double cm);
};

#endif // QSLDESIGNER_CANVAS_H
