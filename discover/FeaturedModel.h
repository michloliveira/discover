/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FEATUREDMODEL_H
#define FEATUREDMODEL_H

#include <QAbstractItemModel>
#include <QPointer>

namespace KIO { class StoredTransferJob; }
class AbstractResource;
class AbstractResourcesBackend;

class FeaturedModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
public:
    explicit FeaturedModel(QObject *parent = nullptr);
    ~FeaturedModel() override {}

    void setResources(int category, const QVector<AbstractResource*> &resources);
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    bool isFetching() const { return m_isFetching != 0; }

Q_SIGNALS:
    void isFetchingChanged();

private:
    void refreshCurrentApplicationBackend();
    void setUris(const QVector<QVector<QUrl>> &uris);
    void refresh();
    void removeResource(AbstractResource* resource);

    void acquireFetching(bool f);

    QHash<int, QVector<AbstractResource*>> m_resources;
    int m_isFetching = 0;
    AbstractResourcesBackend* m_backend = nullptr;
};

#endif // FEATUREDMODEL_H
