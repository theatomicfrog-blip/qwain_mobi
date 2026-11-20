// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#include "qsldesigner_canvas.h"

// ============================================================================
// MODULE: qsldesigner_canvas
// ROLE: Encapsulates WYSIWYG canvas graphics rulers, crosshair overlays,
//       mouse telemetry coordinate tracking, and unit conversions.
// ============================================================================

QString QslCanvasUtils::formatCoordinates(QPointF pos, bool isCm) {
    double xIn = pos.x() / 96.0;
    double yIn = pos.y() / 96.0;
    if (isCm) {
        return QString("<b>X: %1 cm &nbsp; Y: %2 cm</b>")
            .arg(xIn * 2.54, 0, 'f', 2)
            .arg(yIn * 2.54, 0, 'f', 2);
    }
    return QString("<b>X: %1 in &nbsp; Y: %2 in</b>")
        .arg(xIn, 0, 'f', 2)
        .arg(yIn, 0, 'f', 2);
}

double QslCanvasUtils::convertInchesToCm(double inches) {
    return inches * 2.54;
}

double QslCanvasUtils::convertCmToInches(double cm) {
    return cm / 2.54;
}
