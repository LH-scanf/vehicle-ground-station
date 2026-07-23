#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class LogFilterModel final : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString levelFilter READ levelFilter WRITE setLevelFilter NOTIFY levelFilterChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool showTechnical READ showTechnical WRITE setShowTechnical NOTIFY showTechnicalChanged)

public:
    explicit LogFilterModel(QObject *parent = nullptr);

    [[nodiscard]] QString levelFilter() const;
    [[nodiscard]] QString categoryFilter() const;
    [[nodiscard]] QString searchText() const;
    [[nodiscard]] bool showTechnical() const;

public slots:
    void setLevelFilter(const QString &level);
    void setCategoryFilter(const QString &category);
    void setSearchText(const QString &text);
    void setShowTechnical(bool show);

signals:
    void levelFilterChanged();
    void categoryFilterChanged();
    void searchTextChanged();
    void showTechnicalChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_levelFilter;
    QString m_categoryFilter;
    QString m_searchText;
    bool m_showTechnical = false;
};
