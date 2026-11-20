// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "waja_mapping.h"
#include <QString>
#include <QRegularExpression>
#include <cmath>

struct WajaCenter {
    int id;
    double lat;
    double lon;
    QString name;
};

static const WajaCenter wajaCenters[] = {
    {1, 43.064, 141.346, "hokkaido"},
    {2, 40.824, 140.740, "aomori"},
    {3, 39.703, 141.152, "iwate"},
    {4, 38.268, 140.869, "miyagi"},
    {5, 39.718, 140.102, "akita"},
    {6, 38.240, 140.363, "yamagata"},
    {7, 37.760, 140.467, "fukushima"},
    {8, 36.365, 140.471, "ibaraki"},
    {9, 36.565, 139.883, "tochigi"},
    {10, 36.389, 139.063, "gunma"},
    {11, 35.861, 139.645, "saitama"},
    {12, 35.604, 140.123, "chiba"},
    {13, 35.689, 139.691, "tokyo"},
    {14, 35.447, 139.638, "kanagawa"},
    {15, 37.916, 139.036, "niigata"},
    {16, 36.695, 137.211, "toyama"},
    {17, 36.594, 136.625, "ishikawa"},
    {18, 36.064, 136.219, "fukui"},
    {19, 35.663, 138.568, "yamanashi"},
    {20, 36.648, 138.194, "nagano"},
    {21, 35.423, 136.760, "gifu"},
    {22, 34.975, 138.382, "shizuoka"},
    {23, 35.181, 136.906, "aichi"},
    {24, 34.718, 136.505, "mie"},
    {25, 35.014, 135.858, "shiga"},
    {26, 35.011, 135.768, "kyoto"},
    {27, 34.693, 135.502, "osaka"},
    {28, 34.691, 135.183, "hyogo"},
    {29, 34.685, 135.804, "nara"},
    {30, 34.230, 135.170, "wakayama"},
    {31, 35.501, 134.235, "tottori"},
    {32, 35.468, 133.050, "shimane"},
    {33, 34.661, 133.934, "okayama"},
    {34, 34.385, 132.455, "hiroshima"},
    {35, 34.178, 131.473, "yamaguchi"},
    {36, 34.070, 134.554, "tokushima"},
    {37, 34.340, 134.043, "kagawa"},
    {38, 33.839, 132.766, "ehime"},
    {39, 33.559, 133.531, "kochi"},
    {40, 33.590, 130.401, "fukuoka"},
    {41, 33.263, 130.300, "saga"},
    {42, 32.750, 129.877, "nagasaki"},
    {43, 32.803, 130.707, "kumamoto"},
    {44, 33.239, 131.612, "oita"},
    {45, 31.907, 131.420, "miyazaki"},
    {46, 31.596, 130.557, "kagoshima"},
    {47, 26.212, 127.679, "okinawa"}
};

static double haversineSq(double lat1, double lon1, double lat2, double lon2) {
    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;
    // Using simple pythagorean distance for fast lookup over small area (Japan)
    return dLat*dLat + dLon*dLon;
}

QString getJapanPrefectureFromLatLon(double lat, double lon) {
    // If not vaguely in Japan bounds, ignore
    if (lat < 24.0 || lat > 46.0 || lon < 122.0 || lon > 146.0) return QString();

    double minD = 99999999.0;
    QString bestCode = "";
    
    for (size_t i = 0; i < sizeof(wajaCenters)/sizeof(wajaCenters[0]); ++i) {
        double d = haversineSq(lat, lon, wajaCenters[i].lat, wajaCenters[i].lon);
        if (d < minD) {
            minD = d;
            bestCode = QString::number(wajaCenters[i].id);
        }
    }
    
    // Only return if it's reasonably close (e.g. sq-dist < 4.0 degrees)
    if (minD < 5.0) return bestCode;
    return QString();
}

QString getJapanPrefectureFromGrid(const QString &gridStr) {
    QString grid = gridStr.toUpper();
    QRegularExpression gridRe("^[A-Z]{2}\\d{2}");
    if (grid.length() >= 4 && gridRe.match(grid).hasMatch()) {
        int fLon = grid[0].unicode() - 'A';
        int fLat = grid[1].unicode() - 'A';
        int sLon = grid[2].unicode() - '0';
        int sLat = grid[3].unicode() - '0';
        double lon = -180.0 + (fLon * 20.0) + (sLon * 2.0) + 1.0;
        double lat = -90.0 + (fLat * 10.0) + (sLat * 1.0) + 0.5;
        return getJapanPrefectureFromLatLon(lat, lon);
    }
    return QString();
}
