#include "LogFilterModel.h"

#include "LogManager.h"

LogFilterModel::LogFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

QString LogFilterModel::levelFilter() const { return m_levelFilter; }
QString LogFilterModel::categoryFilter() const { return m_categoryFilter; }
QString LogFilterModel::searchText() const { return m_searchText; }
bool LogFilterModel::showTechnical() const { return m_showTechnical; }

void LogFilterModel::setLevelFilter(const QString &level)
{
    if (m_levelFilter == level)
        return;
    m_levelFilter = level;
    invalidateRowsFilter();
    emit levelFilterChanged();
}

void LogFilterModel::setCategoryFilter(const QString &category)
{
    if (m_categoryFilter == category)
        return;
    m_categoryFilter = category;
    invalidateRowsFilter();
    emit categoryFilterChanged();
}

void LogFilterModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    m_searchText = text;
    invalidateRowsFilter();
    emit searchTextChanged();
}

void LogFilterModel::setShowTechnical(bool show)
{
    if (m_showTechnical == show)
        return;
    m_showTechnical = show;
    invalidateRowsFilter();
    emit showTechnicalChanged();
}

bool LogFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString display = sourceModel()->data(index, LogManager::DisplayRole).toString();
    if (display == QStringLiteral("file_only"))
        return false;
    if (display == QStringLiteral("diagnostic") && !m_showTechnical)
        return false;

    const QString level = sourceModel()->data(index, LogManager::LevelRole).toString();
    if (!m_levelFilter.isEmpty() && level != m_levelFilter)
        return false;

    const QString category = sourceModel()->data(index, LogManager::CategoryRole).toString();
    if (!m_categoryFilter.isEmpty() && category != m_categoryFilter)
        return false;

    if (m_searchText.trimmed().isEmpty())
        return true;

    const QString search = m_searchText.trimmed();
    const QList<int> roles {
        LogManager::MessageRole,
        LogManager::EventRole,
        LogManager::ComponentRole,
        LogManager::VehicleIdRole,
        LogManager::RequestIdRole
    };
    for (const int role : roles) {
        if (sourceModel()->data(index, role).toString().contains(search, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
