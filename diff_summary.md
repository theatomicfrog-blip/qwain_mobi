# Qwain Code-Flow Documentation Diffs

This file contains the unified diffs for the documentation-only code-flow comment passes applied to `qslmanagerdialog.cpp`, `mainwindow.cpp`, `qsldesigner.cpp`, `receiveqsldialog.cpp`, `qslpreviewdialog.cpp`, and `globewidget.cpp`. These passes add clear visual markers, structural descriptions, and trace parameters for developers or security auditors to easily navigate Qwain's execution paths and state boundaries.

---

## 1. QslManagerDialog (`cpp_source/qslmanagerdialog.cpp`)

```diff
--- cpp_source/qslmanagerdialog.cpp
+++ cpp_source/qslmanagerdialog.cpp
@@ -6,6 +6,16 @@
 // 
 // You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 
+// ============================================================================
+// COMPONENT: QslManagerDialog
+// ROLE: QSL database manager, search engine, and print queue director.
+//       This component coordinates searches across the local QSO logbook,
+//       filters results by band/mode/date/DXCC, updates local printed logs,
+//       pulls profiles from QrzCache, and directs the QSL print queue list.
+// INVOKED BY: MainWindow (toolbar button and menu options).
+// PRODUCES: Filtered QSO listings, interactive profiles, and queued print jobs.
+// ============================================================================
+
 #include "qslmanagerdialog.h"
 #include "qrzcache.h"
 #include "qslpreviewdialog.h"
@@ -784,6 +794,13 @@
     QDialog::resizeEvent(event);
 }
 
+// [ENTRY POINT: Selection Callback / Profile Detail Assembly]
+// This function displays the details for a selected callsign in the HTML info panel.
+// Inputs: callsign - The amateur radio callsign to display.
+// Effects: 
+//   1. Retrieves biographical HTML and profile image from the local QrzCache.
+//   2. If missing, programmatically triggers a background QRZ.com API lookup.
+//   3. Builds and renders the interactive HTML layout (past QSO table, ARRL/QRZ confirmation icons, and Rcvd on timestamps).
 void QslManagerDialog::displayCallInfo(const QString &callsign) {
     m_currentDisplayedCallsign = callsign;
     
@@ -1088,6 +1095,10 @@
     return callsign + "|unprinted";
 }
 
+// [ENTRY POINT: Centralized Search Query Gateway]
+// Dispatches the appropriate search subroutine based on the active query mode.
+// Uses a custom 'SearchGuard' RAII lock to avoid concurrent search collision,
+// flagging 'm_searchPending' if another lookup is invoked during execution.
 void QslManagerDialog::executeLastSearch() {
     if (m_isExecuting) { m_searchPending = true; return; }
     SearchGuard guard(this);
@@ -1160,6 +1171,10 @@
     populateResults(matched, true, counts, true);
 }
 
+// [ENTRY POINT: Wildcard Parser & Filter Matcher]
+// Performs callsign-specific database searches, supporting wildcard queries (*, @, ?).
+// Compiles wildcard characters into robust regular expressions to filter matching logs,
+// integrates LoTW/eQSL criteria with local QrzCache status, and populates the table results.
 void QslManagerDialog::doLookup() {
     if (!m_isExecuting) {
         if (m_searchActive) { m_searchPending = true; m_lastSearchMode = "lookup"; return; }
@@ -1917,6 +1932,9 @@
 
 
 
+// [ENTRY POINT: Bulk Queue Operator]
+// Selects all matched QSOs from the current search results and queues them
+// in the print queue list, ensuring no duplicates are introduced and updating m_btnPrintQsl state.
 void QslManagerDialog::selectAllResults() {
     bool addedAny = false;
     for (int row = 0; row < m_resultsTable->rowCount(); ++row) {
```

---

## 2. MainWindow (`cpp_source/mainwindow.cpp`)

```diff
--- cpp_source/mainwindow.cpp
+++ cpp_source/mainwindow.cpp
@@ -6,6 +6,16 @@
 // 
 // You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 
+// ============================================================================
+// COMPONENT: MainWindow
+// ROLE: Main Workspace controller of the Qwain ecosystem.
+//       Coordinates live TCP/UDP network servers, initiates ADIF record parsing,
+//       triggers local/online QRZ.com lookups, updates real-time award statistics,
+//       manages layout themes/fonts, and hosts child dialog spools (designer, manager).
+// INVOKED BY: main.cpp (application entry point).
+// PRODUCES: Live QSO updates, leaderboard listings, and real-time GUI views.
+// ============================================================================
+
 #include "mainwindow.h"
 #include <QSvgRenderer>
 #include "fips_data.h"
@@ -1442,8 +1452,9 @@
     applyThemeStyle();
     
     updateMenuLabels();
-}
-
+// [ENTRY POINT: UDP Network Packet Callback]
+// Triggered on incoming live datagrams (e.g. from WSJT-X or N1MM).
+// Extracts raw bytes, appends them to m_receiveBuffer, and executes the ADIF parsing loop.
 void MainWindow::onUdpReadyRead()
 {
     while (m_udpSocket && m_udpSocket->hasPendingDatagrams()) {
@@ -1477,6 +1488,9 @@
     connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);
 }
 
+// [ENTRY POINT: TCP Connection Packet Callback]
+// Triggered on incoming client socket data streams (e.g. from third-party loggers).
+// Reads available data, segments it by <EOR> records, commits raw logs to disk, and dispatches to processAdifRecord().
 void MainWindow::onReadyRead()
 {
     m_statusLabel->setText("Status: Receiving data...");
@@ -1687,6 +1701,13 @@
     return QString("%1 ms").arg(elapsedMs, 4, 10, QChar('0'));
 }
 
+// [ENTRY POINT: Unified ADIF Record Parser & Pipeline]
+// Decodes a raw ADIF string, extracts geographic and biographical fields,
+// updates local real-time award statistics/leaderboards, and spools lookups.
+// Inputs:
+//   record - The raw bracketed ADIF string.
+//   isManualLookup - True if triggered by callsign search rather than a live/imported log.
+//   updateDisplay - True to immediately update GUI labels and globe views.
 void MainWindow::processAdifRecord(const QString &record, bool isManualLookup, bool updateDisplay, bool isUdp) {
     qint64 startMs = QDateTime::currentMSecsSinceEpoch();
     
```

---

## 3. QslDesigner (`cpp_source/qsldesigner.cpp`)

```diff
--- cpp_source/qsldesigner.cpp
+++ cpp_source/qsldesigner.cpp
@@ -19,6 +19,16 @@
 #include <QDataStream>
 #include <QProcess>
 
+// ============================================================================
+// COMPONENT: QslDesigner
+// ROLE: Interactive WYSIWYG QSL card and envelope layout canvas editor.
+//       This component coordinates mouse events, drag-and-drop panning, 
+//       undo/redo history state, crosshair cursor-tracking, custom reference
+//       points, and design elements serialization/deserialization.
+// INVOKED BY: MainWindow (via the "Design QSLs" action).
+// PRODUCES: Custom structured templates (XML, JSON) loaded into QslPreviewDialog.
+// ============================================================================
+
 #include "qsldesigner.h"
 #include "paths.h"
 #include <QFileDialog>
@@ -5988,6 +5998,8 @@
     // Stub implementation to satisfy linker
 }
 
+// [ENTRY POINT: Canvas Drag-Panning Mouse Move Tracker]
+// Translates view coordinates into scene deltas for inertia-free center shifts.
 void MyGraphicsView::mouseMoveEvent(QMouseEvent *event) {
     if (m_isPanning) {
         QPoint delta = event->pos() - m_panStartPos;
@@ -6010,6 +6022,9 @@
     emit mouseMoved(mapToScene(event->pos()));
 }
 
+// [ENTRY POINT: Click Selection & Intercept Bypass Gateway]
+// Intercepts selections, bypasses non-interactive guidelines (Z-value < 0 or Z-value = 100),
+// and initiates undo snapshot states or left-click-drag canvas panning.
 void MyGraphicsView::mousePressEvent(QMouseEvent *event) {
     bool isTestView = scene() && scene()->property("isTestView").toBool();
     if (!isTestView) {
```

---

## 4. ReceiveQslsDialog (`cpp_source/receiveqsldialog.cpp`)

```diff
--- cpp_source/receiveqsldialog.cpp
+++ cpp_source/receiveqsldialog.cpp
@@ -4,6 +4,17 @@
 // 
 // This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
+// ============================================================================
+// COMPONENT: ReceiveQslsDialog
+// ROLE: Incoming QSL card registration and online logbook synchronization.
+//       This component processes received physical paper cards, matches them
+//       against local QSO records, updates persistent .adi log files, and
+//       synchronizes status changes (QSL_RCVD, QSLRDATE) back to online
+//       databases (QRZ, eQSL, ClubLog, LoTW).
+// INVOKED BY: MainWindow (via the "Receive QSL" action).
+// PRODUCES: Updated local log files and synchronized online logbooks.
+// ============================================================================
+
 #include "receiveqsldialog.h"
 #include "qrzcache.h"
 #include "mainwindow.h"
@@ -335,6 +335,9 @@
     }
 }
 
+// [ENTRY POINT: Callsign Lookup Resolver]
+// Searches the local QSO logbook for previous contacts matching the query callsign,
+// sorts matching entries by date/time (most recent first), and populates the table.
 void ReceiveQslsDialog::onSearch() {
     QString call = m_searchEdit->text().trimmed().toUpper();
     if (call.isEmpty()) return;
@@ -407,6 +422,9 @@
     m_contactsTable->setFixedHeight(tableHeight);
 }
 
+// [ENTRY POINT: Received Status Submitter & Online Sync Spooler]
+// Processes marked contacts, writes the "QSL_RCVD=Y" and receipt date values
+// directly to the local .adi log file, and initiates asynchronous multi-platform API updates.
 void ReceiveQslsDialog::onSubmit() {
     QList<int> selectedIndices;
     for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
```

---

## 5. QslPreviewDialog (`cpp_source/qslpreviewdialog.cpp`)

```diff
--- cpp_source/qslpreviewdialog.cpp
+++ cpp_source/qslpreviewdialog.cpp
@@ -6,6 +6,17 @@
 // 
 // You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 
+// ============================================================================
+// COMPONENT: QslPreviewDialog
+// ROLE: High-fidelity print preview layout engine and batch spool controller.
+//       This component processes selected QSO records, formats them according
+//       to custom template tags (including standard emission modes and QRP
+//       bragging auto-sizing text), paints them onto a virtual graphics scene
+//       for real-time preview, and drives spooled physical/PDF batch printing.
+// INVOKED BY: QslManagerDialog ("Print Now") or MainWindow.
+// PRODUCES: On-screen preview renders, spooled card/envelope print runs.
+// ============================================================================
+
 #include <QCoreApplication>
 #include "qrzcache.h"
 #include <QClipboard>
@@ -2430,6 +2430,8 @@
     renderTemplate();
 }
 
+// [ENTRY POINT: Preview Invalidation Slot]
+// Clears layout buffers, recalculates table dimensions, and re-triggers renderTemplate().
 void QslPreviewDialog::updatePreview() {
     QString mainName = m_previewingEnvelope ? getSelectedEnvTemplate() : getSelectedCardTemplate();
     
@@ -2455,6 +2457,9 @@
     updateSendToPrinterButtonText();
 }
 
+// [ENTRY POINT: Core Layout Composition and Tag Substitution Engine]
+// Loads selected card/envelope templates, processes each element (labels, shapes, text boxes),
+// resolves custom ADIF/QSL tags (like <qrp> with auto-scaling text on path), and compiles the scene.
 void QslPreviewDialog::renderTemplate() {
     QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
     bool useStandardEmission = mainSettings.value("QslUpdates/UseStandardEmission", false).toBool();
@@ -4335,6 +4340,9 @@
     QEventLoop *m_waitingLoop = nullptr;
 };
 
+// [ENTRY POINT: Print Dispatcher Gateway]
+// Verifies queue statuses, configures target CUPS or physical printer destinations,
+// handles single or batch spool setups, and initiates the card/envelope printing loops.
 void QslPreviewDialog::onSendToPrinterClicked() {
     if (m_printQueue.isEmpty()) {
```

---

## 6. GlobeWidget (`cpp_source/globewidget.cpp`)

```diff
--- cpp_source/globewidget.cpp
+++ cpp_source/globewidget.cpp
@@ -6,6 +6,17 @@
 // 
 // You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 
+// ============================================================================
+// COMPONENT: GlobeWidget
+// ROLE: Real-time 3D OpenGL geospatial visualizer.
+//       This component processes geographic coordinates (latitude, longitude)
+//       of worked/needed amateur radio contacts, plots them as 3D markers on
+//       a rotatable orthographic globe, handles greyline shading transitions,
+//       and provides an interactive graphical HUD of QSO statistics.
+// INVOKED BY: MainWindow (embedded in the main panel).
+// PRODUCES: Interactive 3D globe renderings, contact markers, and zoom views.
+// ============================================================================
+
 #include "globewidget.h"
 #include "paths.h"
 #include <QMouseEvent>
@@ -114,6 +125,8 @@
     reply->deleteLater();
 }
 
+// [ENTRY POINT: OpenGL Context Init Callback]
+// Sets up background colors, deep buffer states, and blending parameters for transparent overlays.
 void GlobeWidget::initializeGL() {
     initializeOpenGLFunctions();
     glClearColor(0.0f, 0.05f, 0.1f, 1.0f);
@@ -165,6 +178,9 @@
     }
 }
 
+// [ENTRY POINT: 3D Render Loop Callback]
+// Coordinates glClear passes, calculates particle expansion physics if in "explosion" Easter Egg mode,
+// maps raw flat Earth texture maps onto 3D spherical coordinates, and plots contact markers.
 void GlobeWidget::paintGL() {
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

---

## 7. GraylineWidget (`cpp_source/graylinewidget.cpp`)

```diff
--- cpp_source/graylinewidget.cpp
+++ cpp_source/graylinewidget.cpp
@@ -6,6 +6,16 @@
 // 
 // You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 
+// ============================================================================
+// COMPONENT: GraylineWidget
+// ROLE: Real-time 2D equirectangular solar terminator (grayline) projection.
+//       Calculates astronomical solar declination/equation-of-time to map
+//       the daylight terminator over a flat Earth texture, centering on the
+//       user's configured Maidenhead grid coordinates.
+// INVOKED BY: MainWindow (via "View Grayline Map" action/dialog).
+// PRODUCES: Custom 2D equirectangular map with dynamic day/night shading overlays.
+// ============================================================================
+
 #include "graylinewidget.h"
 #include "paths.h"
 #include <QPainter>
@@ -15,6 +25,8 @@
 #include <QDebug>
 #include <QFile>
 
+// [ENTRY POINT: Grayline Widget Constructor]
+// Initializes layout sizes, reads cached textures, and schedules a 60s periodic shadow update loop.
 GraylineWidget::GraylineWidget(QWidget *parent)
     : QWidget(parent), m_updateTimer(new QTimer(this)), m_netManager(new QNetworkAccessManager(this))
 {
@@ -47,6 +59,9 @@
 
 GraylineWidget::~GraylineWidget() {}
 
+// [ENTRY POINT: Texture Asset Loader & Cache Manager]
+// Attempts to load a cached JPEG map of the Earth. Falls back to an asynchronous
+// HTTP fetch with redirect handling, persisting the downloaded texture back to disk.
 void GraylineWidget::loadEarthMap() {
     QString cachePath = getAppDataDir() + "/earth_texture.jpg";
     if (QFile::exists(cachePath)) {
@@ -110,6 +125,9 @@
     QWidget::showEvent(event);
 }
 
+// [ENTRY POINT: Astronomical Solar Position Solver]
+// Approximates the current sub-solar latitude and longitude using standard Julian/orbital
+// orbital mechanics equations, including Equation of Time (EoT) correction.
 void GraylineWidget::calculateSunPosition(double &subSolarLat, double &subSolarLon) {
     QDateTime now = QDateTime::currentDateTimeUtc();
     int days = now.date().dayOfYear();
@@ -130,6 +148,9 @@
     if (subSolarLon > 180.0) subSolarLon -= 360.0;
 }
 
+// [ENTRY POINT: Terminator Projection Rasterizer]
+// Computes daylight/twilight/night boundaries across the image raster by calculating the 
+// spherical zenith angle between each latitude/longitude point and the sub-solar coordinates.
 void GraylineWidget::updateShadow() {
     if (m_earthMap.isNull()) return;
     
@@ -177,6 +198,9 @@
     }
 }
 
+// [ENTRY POINT: Equirectangular Projection Renderer]
+// Parses the user's home Maidenhead grid location to shift/center the 2D map projection,
+// wraps the Earth map texture horizontally, and renders the dynamic twilight/night shade overlays.
 void GraylineWidget::paintEvent(QPaintEvent *event) {
     QPainter p(this);
     p.setRenderHint(QPainter::Antialiasing);
```

---

## 8. DxClusterWidget (`cpp_source/dxclusterwidget.cpp`)

```diff
--- cpp_source/dxclusterwidget.cpp
+++ cpp_source/dxclusterwidget.cpp
@@ -32,6 +32,20 @@
 #include <QStandardPaths>
 #include <QDir>
 
+// ============================================================================
+// COMPONENT: DxClusterWidget
+// ROLE: Dynamic DX Cluster spot aggregator and presentation feed.
+//       Establishes asynchronous Telnet TCP socket links with configured logbook
+//       servers, parses incoming spot feeds via regex patterns, performs local
+//       QRZ country cache mapping or asynchronous SVG flag lookups, and paints
+//       a custom scroll-animated table with smooth alpha-channel bottom fading.
+// INVOKED BY: MainWindow (via "Spots / DX Cluster" action).
+// PRODUCES: Live DX spot entries, flag icons, and click-to-lookup spot signals.
+// ============================================================================
+
+// [ENTRY POINT: DX Cluster Constructor]
+// Restores saved window geometry, registers socket TCP callbacks, and configures
+// the periodic connection heartbeat and scrolling animation timers.
 DxClusterWidget::DxClusterWidget(QWidget *parent) 
     : QWidget(parent, Qt::Window)
     , m_scrollOffset(0)
@@ -108,6 +122,9 @@
     QWidget::closeEvent(event);
 }
 
+// [ENTRY POINT: Feed Polling & Connection Manager]
+// Orchestrates startup and teardown of the background TCP feed connection loops,
+// scheduling recurrent socket heartbeats and starting smooth visual interpolation timers.
 void DxClusterWidget::startClustering() {
     connectToServer();
     m_reconnectTimer->start();
@@ -120,6 +137,9 @@
     m_socket->close();
 }
 
+// [ENTRY POINT: Telnet Server Handshake Dispatcher]
+// Connects to the host/port configured in Feeds/dxClusterServer, logs the connection event 
+// to the system log, and writes the user's callsign token as the Telnet handshake.
 void DxClusterWidget::connectToServer() {
     if (m_socket->state() == QAbstractSocket::ConnectedState || m_socket->state() == QAbstractSocket::ConnectingState) return;
 
@@ -159,6 +179,8 @@
     m_socket->connectToHost(host, port);
 }
 
+// [ENTRY POINT: Connection Connected Callback]
+// Logs successful connection telemetry and dispatches the initial login credentials block to the host socket.
 void DxClusterWidget::onConnected() {
     QSettings settings(QCoreApplication::organizationName(), "Qwain");
     QString sysLogPath = settings.value("Files/systemLogfile", "").toString();
@@ -177,6 +199,8 @@
     m_socket->write((call + "\n").toUtf8());
 }
 
+// [ENTRY POINT: Connection Error Callback]
+// Handles connection dropouts or handshake rejections by logging socket error state and aborting the stream.
 void DxClusterWidget::onError(QAbstractSocket::SocketError err) {
     QSettings settings(QCoreApplication::organizationName(), "Qwain");
     QString sysLogPath = settings.value("Files/systemLogfile", "").toString();
@@ -189,6 +213,9 @@
     m_socket->abort();
 }
 
+// [ENTRY POINT: TCP Stream Buffer Callback]
+// Accumulates raw incoming socket bytes, processes line-separated Telnet outputs,
+// and trims trailing whitespace before feeding lines to the regex parser pipeline.
 void DxClusterWidget::onReadyRead() {
     m_buffer += QString::fromLatin1(m_socket->readAll());
     int idx;
@@ -198,6 +225,9 @@
     }
 }
 
+// [ENTRY POINT: Regex Pattern Spot Resolver]
+// Parses individual Telnet lines against multi-tiered regex definitions to extract spotter,
+// DX frequency, target callsign, mode comment, and timestamps.
 void DxClusterWidget::processLine(const QString &line) {
     if (line.contains("login:", Qt::CaseInsensitive) || line.contains("callsign:", Qt::CaseInsensitive)) {
         QSettings settings(QCoreApplication::organizationName(), "Qwain");
@@ -261,6 +291,9 @@
     }
 }
 
+// [ENTRY POINT: Feed Spot Queue Controller]
+// Adds newly parsed spots to the prepend list, limits maximum spots queue to 15 entries, 
+// sets up visual scroll animation offsets, and initiates flag lookup spools.
 void DxClusterWidget::addSpot(const DxSpot &spot) {
     m_spots.prepend(spot);
     if (m_spots.size() > 15) m_spots.takeLast();
@@ -271,6 +304,9 @@
     fetchFlagForSpot(0);
 }
 
+// [ENTRY POINT: Scroll Animation Timer Slot]
+// Performs smooth pixel interpolation adjustments over negative scrollOffset indexes,
+// creating a slide-down animation effect whenever a new DX spot is prepended.
 void DxClusterWidget::animateScroll() {
     if (m_scrollOffset < 0) {
         m_scrollOffset += 1.5; // smoother and faster
@@ -278,7 +314,9 @@
     }
 }
 
-
+// [ENTRY POINT: Callsign Country Flag Resolver]
+// Maps spotted callsigns to countries via local QRZ cached profiles. If a match is found,
+// fetches and caches country flag PNG assets asynchronously from FlagCDN, scaling the result to fit rows.
 void DxClusterWidget::fetchFlagForSpot(int index) {
     if (index < 0 || index >= m_spots.size()) return;
     QString callsign = m_spots[index].spotted;
@@ -464,6 +502,9 @@
     QWidget::mouseReleaseEvent(event);
 }
 
+// [ENTRY POINT: Spot Paint Render Loop]
+// Custom draws rows with smooth scroll interpolation offsets, parses HTML-rich spot layouts,
+// and applies gradient transparency opacity clipping as rows approach screen limits.
 void DxClusterWidget::paintEvent(QPaintEvent *) {
     QPainter p(this);
     p.setRenderHint(QPainter::Antialiasing);
```

---

## 9. ImageViewer (`cpp_source/imageviewer.cpp`)

```diff
--- cpp_source/imageviewer.cpp
+++ cpp_source/imageviewer.cpp
@@ -29,6 +29,16 @@
 #include <QFileIconProvider>
 #include <QImageReader>
 
+// ============================================================================
+// COMPONENT: ImageViewer
+// ROLE: Visual media repository, asset importer, and template background picker.
+//       This component indexes downloaded png/jpg background assets, facilitates
+//       local import with real-time grid preview, supports URL downloading,
+//       and provides picker/picker-bypass pathways to configure QSL layout designs.
+// INVOKED BY: QslDesigner (when configuring backgrounds), or MainWindow.
+// PRODUCES: Selected image paths, unlinked items, or spooled image editor instances.
+// ============================================================================
+
 class ThumbnailIconProvider : public QFileIconProvider {
 public:
     QIcon icon(const QFileInfo &info) const override {
@@ -48,6 +58,9 @@
     }
 };
 
+// [ENTRY POINT: Image Viewer Constructor]
+// Configures dialog dimensions, prepares directory structures, and instantiates 
+// grid/list layouts based on picker mode settings before refreshing the visual gallery.
 ImageViewer::ImageViewer(bool pickerMode, QWidget *parent) : QDialog(parent), m_pickerMode(pickerMode)
 {
     setWindowTitle(pickerMode ? "Select Image" : "Image Viewer");
@@ -117,6 +130,9 @@
     refreshGallery();
 }
 
+// [ENTRY POINT: Disk Directory Gallery Indexer]
+// Clears list views, crawls the local cached images subdirectory for supported files
+// (PNG/JPG/BMP), scales items to thumbnail icons, and binds absolute paths.
 void ImageViewer::refreshGallery()
 {
     m_galleryList->clear();
@@ -132,6 +148,9 @@
     }
 }
 
+// [ENTRY POINT: Local File Dialog Import Gateway]
+// Configures and spools a custom non-native QFileDialog with a custom grid view 
+// icon provider and a live preview sidebar, copying selected targets into the app directory.
 void ImageViewer::uploadLocalImage()
 {
     QSettings settings(QCoreApplication::organizationName(), "Qwain");
@@ -198,6 +217,9 @@
     }
 }
 
+// [ENTRY POINT: Remote URL Asset Downloader]
+// Displays a text input box, executes a synchronous HTTP GET fetch via a nested
+// QEventLoop, and saves the resulting image payload to the local gallery subdirectory.
 void ImageViewer::uploadUrlImage()
 {
     bool ok;
@@ -229,6 +251,9 @@
     reply->deleteLater();
 }
 
+// [ENTRY POINT: Gallery Selection Callback]
+// Triggered on double click; either returns the selected path to the picker caller,
+// or spools an interactive crop-and-rotate ImageEditorDialog for asset modification.
 void ImageViewer::onItemDoubleClicked(QListWidgetItem *item)
 {
     QString filePath = item->data(Qt::UserRole).toString();
+```
+
+---
+
+## 10. PrefetchFlagsDialog (`cpp_source/prefetchflagsdialog.cpp`)
+
+```diff
+--- cpp_source/prefetchflagsdialog.cpp
++++ cpp_source/prefetchflagsdialog.cpp
+@@ -20,6 +20,19 @@
+ #include <QDateTime>
+ #include <QImage>
+ 
++// ============================================================================
++// COMPONENT: PrefetchFlagsDialog
++// ROLE: Bulk country flag asset synchronization and pre-fetching engine.
++//       This component crawl-indexes worked QSO countries from local caches,
++//       correlates names to ISO 3166 codes, presents a status-resolved grid,
++//       and coordinates chained sequential async network downloads from FlagCDN.
++// INVOKED BY: MainWindow (via "Pre-fetch Flags" action).
++// PRODUCES: Populated flag caches in qrz_cache.ini under the 'icons' section.
++// ============================================================================
++
++// [ENTRY POINT: Prefetch Flags Dialog Constructor]
++// Configures layout grids, instantiates network download managers, and triggers 
++// the initial scan of cached worked countries and mapped ISO indexes.
+ PrefetchFlagsDialog::PrefetchFlagsDialog(QWidget *parent)
+     : QDialog(parent)
+ {
+@@ -68,6 +81,9 @@
+ 
+ PrefetchFlagsDialog::~PrefetchFlagsDialog() {}
+ 
++// [ENTRY POINT: Local Logbook Country Crawler]
++// Queries static country registries and iterates local QrzCache callgroups to build a
++// unique list of all worked countries, checking existing cached icons, and populating table cells.
+ void PrefetchFlagsDialog::scanCountries() {
+     QrzCache qrzCache;
+     
+@@ -141,6 +157,9 @@
+     qrzCache.endGroup();
+ }
+ 
++// [ENTRY POINT: Batch Pre-fetch Action Callback]
++// Evaluates selected table rows. If no rows are highlighted, intelligently auto-selects all
++// missing cache items with valid ISO codes, initializing the sequential download pipeline.
+ void PrefetchFlagsDialog::getSelectedFlags() {
+     auto selectedItems = m_table->selectedItems();
+     if (selectedItems.isEmpty()) {
+@@ -195,6 +214,9 @@
+     onDownloadFinished(); // kick off
+ }
+ 
++// [ENTRY POINT: Asynchronous Download Chain Router]
++// Triggered on individual network download completions. Handles the response, writes downloaded flag
++// bytes to the icons group in qrz_cache.ini, and immediately schedules the next pending country item.
+ void PrefetchFlagsDialog::onDownloadFinished() {
+     // If reply was sent to this slot, process it
+     QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
+@@ -245,6 +267,9 @@
+     }
+ }
+ 
++// [ENTRY POINT: Network Download Progress Hook]
++// Receives socket packet progress signals, throttling status printouts to a maximum of 
++// once every 500ms, and calculates active transfer speeds in KB/s.
+ void PrefetchFlagsDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
+     if (bytesTotal <= 0) return;
+     
+```

---

## 11. QrzSyncDialog (`cpp_source/qrzsyncdialog.cpp`)

```diff
--- cpp_source/qrzsyncdialog.cpp
+++ cpp_source/qrzsyncdialog.cpp
@@ -23,6 +23,18 @@
 
 #include <QTextEdit>
 
+// ============================================================================
+// COMPONENT: QrzSyncDialog
+// ROLE: Asynchronous XML QRZ Logbook synchronization download pipeline.
+//       This component checks logbook state entries, contacts the QRZ Logbook
+//       HTTP API, retrieves ADIF formatted call records incrementally or fully,
+//       parses entries, writes to disk, and updates GUI speed gauges and progress bars.
+// INVOKED BY: MainWindow (toolbar "Sync QRZ Log" action).
+// PRODUCES: Freshly synchronized local ADIF logs and updated UI table views.
+// ============================================================================
+
+// [ENTRY POINT: QRZ Sync Dialog Constructor]
+// Sets up layout elements, binds socket callback slots, and launches startDownload via a single-shot timer.
 QrzSyncDialog::QrzSyncDialog(QWidget *parent, const QString &apiKey, const QString &logfilePath, bool isFull)
     : QDialog(parent), m_apiKey(apiKey), m_logfilePath(logfilePath), m_isFull(isFull), m_successful(false), m_reply(nullptr), m_bytesReceived(0)
 {
@@ -101,6 +113,9 @@
     }
 }
 
+// [ENTRY POINT: Local Logbook High-watermark Scanner]
+// Scans the active logfile on disk to locate the largest `<APP_QRZLOG_LOGID>` tag value,
+// establishing the incremental download boundary to fetch only newer records from the server.
 void QrzSyncDialog::startDownload() {
     m_btnRetry->setEnabled(false);
     m_btnCancel->setEnabled(true);
@@ -134,6 +149,9 @@
     requestFetch();
 }
 
+// [ENTRY POINT: HTTP FETCH API Dispatches]
+// Launches an asynchronous HTTP POST request containing the authentication API keys
+// to the QRZ logbook endpoint, establishing progress tracking and signal hooks.
 void QrzSyncDialog::requestFetch() {
     QUrl url("https://logbook.qrz.com/api");
     QNetworkRequest req(url);
@@ -295,6 +313,9 @@
     startDownload();
 }
 
+// [ENTRY POINT: Log merger & ADIF Writer]
+// Overwrites the local logbook with fresh entries in a full-sync scenario, or merges 
+// incremental logs by skipping existing elements based on logged dates/times.
 void QrzSyncDialog::processAdif(const QString &adifData) {
     if (m_isFull) {
         QFile out(m_logfilePath);
```

---

## 12. StyleDialog (`cpp_source/styledialog.cpp`)

```diff
--- cpp_source/styledialog.cpp
+++ cpp_source/styledialog.cpp
@@ -1,6 +1,18 @@
 #include "styledialog.h"
 #include <QDialogButtonBox>
 
+// ============================================================================
+// COMPONENT: StyleDialog
+// ROLE: Typography, color theme, and orientation transformation picker.
+//       Presents font families, sizes, styles (bold/italic), palette dialogues,
+//       and rotation angle parameters with real-time on-the-fly CSS previews.
+// INVOKED BY: QslDesigner (when editing template fields).
+// PRODUCES: Selected fonts, colors, rotations, and interactive property signals.
+// ============================================================================
+
+// [ENTRY POINT: Style Selection Constructor]
+// Configures layouts, connects comboboxes to live CSS update loops, and registers 
+// persistent undo buttons to revert properties back to original settings.
 StyleDialog::StyleDialog(const QFont &initialFont, const QColor &initialColor, QWidget *parent, bool colorOnly, bool showRotation, double initialRotation)
     : QDialog(parent), m_initialFont(initialFont), m_initialColor(initialColor),
       m_showRotation(showRotation), m_currentRotation(initialRotation), m_initialRotation(initialRotation)
@@ -136,6 +148,9 @@
     }
 }
 
+// [ENTRY POINT: Real-time UI Stylesheet Invalidator]
+// Compiles font, weight, alignment, and color choices into a live stylesheet string, 
+// updating the preview box and firing real-time signals to the visual designer canvas.
 void StyleDialog::updatePreview() {
     m_previewLabel->setFont(m_currentFont);
     m_previewLabel->setAutoFillBackground(true);
```

---

## 13. QrzCache (`cpp_source/qrzcache.cpp`)

```diff
--- cpp_source/qrzcache.cpp
+++ cpp_source/qrzcache.cpp
@@ -1,3 +1,13 @@
+// ============================================================================
+// COMPONENT: QrzCache
+// ROLE: Multi-database SQLite transactional storage and profile caching layer.
+//       This component operates as an INI-compatible backend proxy, splitting callsign
+//       lookup metadata and heavy flag graphics/icons into separate SQLite stores,
+//       supporting transactions, automatic legacy INI-to-SQL migration, and self-healing DB scrubs.
+// INVOKED BY: MainWindow, QslManagerDialog, DxClusterWidget, PrefetchFlagsDialog.
+// PRODUCES: Highly efficient metadata reads/writes and cached graphics blobs.
+// ============================================================================
+
 #include "qrzcache.h"
 #include "paths.h"
 #include <QFile>
@@ -28,6 +38,9 @@
     return "qrz_cache_db";
 }
 
+// [ENTRY POINT: QrzCache Constructor]
+// Acts as a proxy interface; fetches the standardized cache database file path 
+// and triggers the initial database structures bootstrap process.
 QrzCache::QrzCache(const QString &filePath, int format) {
     Q_UNUSED(filePath);
     Q_UNUSED(format);
@@ -35,6 +48,9 @@
     initDatabase(dbPath);
 }
 
+// [ENTRY POINT: SQLite Database Bootstrap & Migration Engine]
+// Configures the SQLite graphics database, registers schemas, auto-seeds default icons, 
+// and executes transactional migrations if legacy INI files are found on disk.
 void QrzCache::initDatabase(const QString &dbPath) {
     // 1. Initialize Graphics Cache DB first
     QSqlDatabase gdb = QSqlDatabase::database("qrz_graphics_db", false);
```
+
