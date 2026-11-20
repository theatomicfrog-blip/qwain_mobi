#ifndef AWARDSWIDGET_H
#define AWARDSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QStringList>
#include <QVector>

class QLabel;
class QTableWidget;
class QNetworkAccessManager;
class QListWidget;

struct AwardGoal {
    QString id;
    QString name;
    QString description;
    QString imageUrl;
    QStringList reqLabels;
};

class AwardsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AwardsWidget(const QString &adifPath, QWidget *parent = nullptr);
    ~AwardsWidget();

private slots:
    void onImageClicked(const QString &awardId);
    void onImageDoubleClicked(const QString &awardId);
    void downloadImage(const QString &url, QLabel *label);

private:
    QString m_adifPath;
    QNetworkAccessManager *m_net;

    QMap<QString, AwardGoal> m_awards;
    QMap<QString, QLabel*> m_imgLabels;

    QLabel *m_titleLabel;
    QLabel *m_descLabel;
    QTableWidget *m_reqTable;

    QString m_currentAwardId;

    QMap<QString, bool> m_workedReqs; // req label -> worked?

    void loadAwards();
    void parseAdif();
    void updateRightPanel(const QString &awardId);
    void setupUI();
    
    // logic specific to UN awards
    void evaluateUN43(const QString &call, const QString &country, const QString &dxcc);
    
    // track standard UN 43 entities
    QMap<QString, bool> m_eu43Worked;
    bool m_specialWorked = false;
    bool m_4u1aMemberWorked = false;
};

#endif // AWARDSWIDGET_H
