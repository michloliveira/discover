/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FeaturedModel.h"

#include "discover_debug.h"
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QtGlobal>
#include <KIO/StoredTransferJob>
#include <KLocalizedString>

#include <utils.h>
#include <resources/ResourcesModel.h>
#include <resources/StoredResultsStream.h>

Q_GLOBAL_STATIC(QString, featuredCache)

FeaturedModel::FeaturedModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    connect(ResourcesModel::global(), &ResourcesModel::currentApplicationBackendChanged, this, &FeaturedModel::refreshCurrentApplicationBackend);

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);

    const bool isMobile = QByteArrayList{"1", "true"}.contains(qgetenv("QT_QUICK_CONTROLS_MOBILE"));
    auto fileName = isMobile ? QLatin1String("/featured-mobile-5.9.json") : QLatin1String("/featured-5.9.json");
    *featuredCache = dir + fileName;
    const QUrl featuredUrl(QStringLiteral("https://autoconfig.kde.org/discover") + fileName);
    auto *fetchJob = KIO::storedGet(QUrl(QStringLiteral("file:/home/carl/discover-test.json")), KIO::NoReload, KIO::HideProgressInfo);
    acquireFetching(true);
    connect(fetchJob, &KIO::StoredTransferJob::result, this, [this, fetchJob]() {
        const auto dest = qScopeGuard([this] {
            acquireFetching(false);
            refresh();
        });
        if (fetchJob->error() != 0)
            return;

        QFile f(*featuredCache);
        if (!f.open(QIODevice::WriteOnly))
            qCWarning(DISCOVER_LOG) << "could not open" << *featuredCache << f.errorString();
        f.write(fetchJob->data());
        f.close();
    });

    refreshCurrentApplicationBackend();
}

void FeaturedModel::refreshCurrentApplicationBackend()
{
    auto backend = ResourcesModel::global()->currentApplicationBackend();
    if (m_backend == backend)
        return;

    if (m_backend) {
        disconnect(m_backend, &AbstractResourcesBackend::fetchingChanged, this, &FeaturedModel::refresh);
        disconnect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, &FeaturedModel::removeResource);
    }

    m_backend = backend;

    if (backend) {
        connect(backend, &AbstractResourcesBackend::fetchingChanged, this, &FeaturedModel::refresh);
        connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &FeaturedModel::removeResource);
    }

    if (backend && QFile::exists(*featuredCache))
        refresh();
}

void FeaturedModel::refresh()
{
    //usually only useful if launching just fwupd or kns backends
    if (!m_backend)
        return;
    

    acquireFetching(true);
    const auto dest = qScopeGuard([this] { acquireFetching(false); });
    QFile f(*featuredCache);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(DISCOVER_LOG) << "couldn't open file" << *featuredCache << f.errorString();
        return;
    }
    QJsonParseError error;
    const auto array = QJsonDocument::fromJson(f.readAll(), &error).array();
    if (error.error) {
        qCWarning(DISCOVER_LOG) << "couldn't parse" << *featuredCache << ". error:" << error.errorString();
        return;
    }
    
    QVector<QVector<QUrl>> urisCategory;
    for (const QJsonValue &category: array) {
        const auto uris = kTransform<QVector<QUrl>>(category.toArray(), [](const QJsonValue& uri) { return QUrl(uri.toString()); });
        urisCategory.append(uris);
    }

    setUris(urisCategory);
}

void FeaturedModel::setUris(const QVector<QVector<QUrl>>& uris)
{
    if (!m_backend)
        return;
    
    for (int i = 0; i < uris.count(); i++) {
        QSet<ResultsStream*> streams;
        for(const auto &uri : qAsConst(uris[i])) {
            AbstractResourcesBackend::Filters filter;
            filter.resourceUrl = uri;
            streams << m_backend->search(filter);
        }
        if (!streams.isEmpty()) {
            auto stream = new StoredResultsStream(streams);
            acquireFetching(true);
            connect(stream, &StoredResultsStream::finishedResources,
                    this, [i, this](const QVector<AbstractResource *> &resources) {
                setResources(i, resources);
            });
        }
    }
}

static void filterDupes(QVector<AbstractResource *> &resources)
{
    QSet<QString> found;
    for(auto it = resources.begin(); it!=resources.end(); ) {
        auto id = (*it)->appstreamId();
        if (found.contains(id)) {
            it = resources.erase(it);
        } else {
            found.insert(id);
            ++it;
        }
    }
}

void FeaturedModel::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching==0) || (f && m_isFetching==1)) {
        emit isFetchingChanged();
    }
    Q_ASSERT(m_isFetching>=0);
}

void FeaturedModel::setResources(int category, const QVector<AbstractResource *>& _resources)
{
    auto resources = _resources;
    filterDupes(resources);

    if (m_resources[category] != resources) {
        if (!m_resources.contains(category)) {
            beginInsertRows({}, category, category);
            m_resources[category] = QVector<AbstractResource *>();
            endInsertRows();
        }
        beginRemoveRows(index(category, 0), 0, m_resources[category].count());
        m_resources[category] = resources;
        endRemoveRows();
        beginInsertRows(index(category, 0), 0, m_resources[category].count());
        endInsertRows();
        // FIXME hack
        beginResetModel();
        endResetModel();
    }

    acquireFetching(false);
}

void FeaturedModel::removeResource(AbstractResource* resource)
{
    for (int category = 0; category < m_resources.count(); category++) {
        int i = m_resources[category].indexOf(resource);
        if (i < 0) {
            continue;
        }
        beginRemoveRows(index(category, 0), i, i);
        m_resources[category].removeAt(i);
        endRemoveRows();
    }
}

QVariant FeaturedModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};
    
    if (index.parent().isValid()) {
        if (role != Qt::UserRole) {
            return {};
        }
        auto res = m_resources[index.parent().row()].value(index.row());
        if (!res) {
            return {};
        }

        return QVariant::fromValue<QObject*>(res);
    }
    
    if (role == Qt::DisplayRole) {
        switch (index.row()) {
            case 0:
                return i18n("Create");
            case 1:
                return i18n("Work");
            case 2:
                return i18n("Play");
            case 3:
                return i18n("Develop");
            default:
                return {};
        }
    }
    return {};
}

QModelIndex FeaturedModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return createIndex(row, column, (intptr_t)parent.row() + 1);
    }
    return createIndex(row, column, nullptr);
}

QModelIndex FeaturedModel::parent(const QModelIndex &child) const
{
    if (child.internalId()) {
        return createIndex(child.internalId() - 1, 0, nullptr);
    }
    return QModelIndex();
}

int FeaturedModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return m_resources[parent.row()].count();
    }
    return m_resources.count();
}

int FeaturedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}

bool FeaturedModel::hasChildren(const QModelIndex& index) const
{
    if (index.parent().isValid()) {
        return false;
    }
    return m_resources.count() > 0;
}

QHash<int, QByteArray> FeaturedModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("categoryName")},
        {Qt::UserRole, QByteArrayLiteral("applicationObject")}
    };
}
