// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef GRAYLINEWIDGET_H
#define GRAYLINEWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class GraylineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraylineWidget(QWidget *parent = nullptr);
    ~GraylineWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QImage m_earthMap;
    QTimer *m_updateTimer;
    QNetworkAccessManager *m_netManager;
    
    void loadEarthMap();
    
    // For drawing the grayline (day/night shadow)
    void calculateSunPosition(double &subSolarLat, double &subSolarLon);
    QImage m_shadowLayer;
    void updateShadow();
};

#endif
