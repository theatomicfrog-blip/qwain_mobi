# AdifMonitor QSL and Award Tracking

## QSL Layout & Award Rules XML Specification

To ensure robust and flexible expansion, ADIF Monitor provides an extensible schema for exchanging QSL card configurations, layout coordinates, and portable Award Trackers.

### Format Standards & Schema
Documents are described against `qsl_layout.xsd`, providing rigorous rules defining:
- **Print Coordinates**: Bounding box tracking for custom callsigns, QTH data, and graphical elements via mapping anchors.
- **Award Goal Integration**: Embedded within the XML is optional logical tagging for arbitrary Award Criteria (via `<AwardCriteria>`). It supports rendering portable trackers and sharing Custom Awards directly in pure XML formats, securely bound to JSON translations within the station config memory.
- **Strict Data Typings**: Patterns securely bind `Callsign` variables (`^[A-Za-z]{1,3}[0-9][A-Za-z0-9]{1,3}(/[A-Za-z0-9]+)?$`) and standard `POTA` references to prevent layout corruption during bulk rendering procedures.

See the bundled UNIX man page `QSLManagerXML.5` for formal technical layout references.
