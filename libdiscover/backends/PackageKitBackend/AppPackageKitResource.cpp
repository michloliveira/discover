/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppPackageKitResource.h"
#include "utils.h"
#include <AppStreamQt/icon.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <KLocalizedString>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QProcess>
#include <QStandardPaths>
#include <appstream/AppStreamUtils.h>

AppPackageKitResource::AppPackageKitResource(const AppStream::Component &data, const QString &packageName, PackageKitBackend *parent)
    : PackageKitResource(packageName, QString(), parent)
    , m_appdata(data)
{
    Q_ASSERT(data.isValid());
}

QString AppPackageKitResource::name() const
{
    if (m_name.isEmpty()) {
        if (!m_appdata.extends().isEmpty()) {
            const auto components = backend()->componentsById(m_appdata.extends().constFirst());

            if (components.isEmpty())
                qWarning() << "couldn't find" << m_appdata.extends() << "which is supposedly extended by" << m_appdata.id();
            else
                m_name = components.constFirst().name() + QLatin1String(" - ") + m_appdata.name();
        }

        if (m_name.isEmpty())
            m_name = m_appdata.name();
    }
    return m_name;
}

QString AppPackageKitResource::longDescription()
{
    const auto desc = m_appdata.description();
    if (!desc.isEmpty())
        return desc;

    return PackageKitResource::longDescription();
}

static QIcon componentIcon(const AppStream::Component &comp)
{
    QIcon ret;
    foreach (const AppStream::Icon &icon, comp.icons()) {
        QStringList stock;
        switch (icon.kind()) {
        case AppStream::Icon::KindLocal:
            ret.addFile(icon.url().toLocalFile(), icon.size());
            break;
        case AppStream::Icon::KindCached:
            ret.addFile(icon.url().toLocalFile(), icon.size());
            break;
        case AppStream::Icon::KindStock: {
            const auto ret = QIcon::fromTheme(icon.name());
            if (!ret.isNull())
                return ret;
            break;
        }
        default:
            break;
        }
    }
    if (ret.isNull()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    }
    return ret;
}

QVariant AppPackageKitResource::icon() const
{
    return componentIcon(m_appdata);
}

QJsonArray AppPackageKitResource::licenses()
{
    return m_appdata.projectLicense().isEmpty() ? PackageKitResource::licenses() : AppStreamUtils::licenses(m_appdata);
}

QStringList AppPackageKitResource::mimetypes() const
{
    return m_appdata.provided(AppStream::Provided::KindMimetype).items();
}

static constexpr auto s_addonKinds = {AppStream::Component::KindAddon, AppStream::Component::KindCodec};

QStringList AppPackageKitResource::categories()
{
    auto cats = m_appdata.categories();
    if (!kContainsValue(s_addonKinds, m_appdata.kind()))
        cats.append(QStringLiteral("Application"));
    return cats;
}

QString AppPackageKitResource::comment()
{
    const auto summary = m_appdata.summary();
    if (!summary.isEmpty())
        return summary;

    return PackageKitResource::comment();
}

QString AppPackageKitResource::appstreamId() const
{
    return m_appdata.id();
}

QSet<QString> AppPackageKitResource::alternativeAppstreamIds() const
{
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto ret = m_appdata.provided(AppStream_Provided_KindId).items();
    return QSet<QString>(ret.begin(), ret.end());
}

QUrl AppPackageKitResource::homepage()
{
    return m_appdata.url(AppStream::Component::UrlKindHomepage);
}

QUrl AppPackageKitResource::helpURL()
{
    return m_appdata.url(AppStream::Component::UrlKindHelp);
}

QUrl AppPackageKitResource::bugURL()
{
    return m_appdata.url(AppStream::Component::UrlKindBugtracker);
}

QUrl AppPackageKitResource::donationURL()
{
    return m_appdata.url(AppStream::Component::UrlKindDonation);
}

AbstractResource::Type AppPackageKitResource::type() const
{
    static QString desktop = QString::fromUtf8(qgetenv("XDG_CURRENT_DESKTOP"));
    const auto desktops = m_appdata.compulsoryForDesktops();
    return kContainsValue(s_addonKinds, m_appdata.kind()) ? Addon : (desktops.isEmpty() || !desktops.contains(desktop)) ? Application : Technical;
}

void AppPackageKitResource::fetchScreenshots()
{
    const auto sc = AppStreamUtils::fetchScreenshots(m_appdata);
    Q_EMIT screenshotsFetched(sc.first, sc.second);
}

QStringList AppPackageKitResource::allPackageNames() const
{
    auto ret = m_appdata.packageNames();
    if (ret.isEmpty()) {
        ret = QStringList{PackageKit::Daemon::packageName(availablePackageId())};
    }
    return ret;
}

QList<PackageState> AppPackageKitResource::addonsInformation()
{
    const auto res = kFilter<QVector<AppPackageKitResource *>>(backend()->extendedBy(m_appdata.id()), [this](AppPackageKitResource *r) {
        return r->allPackageNames() != allPackageNames();
    });
    return kTransform<QList<PackageState>>(res, [](AppPackageKitResource *r) {
        return PackageState(r->appstreamId(), r->name(), r->comment(), r->isInstalled());
    });
}

QStringList AppPackageKitResource::extends() const
{
    return m_appdata.extends();
}

QString AppPackageKitResource::changelog() const
{
    return AppStreamUtils::changelogToHtml(m_appdata);
}

bool AppPackageKitResource::canExecute() const
{
    static QSet<QString> cannotExecute = {QStringLiteral("org.kde.development")};
    return !cannotExecute.contains(m_appdata.id());
}

void AppPackageKitResource::invokeApplication() const
{
    auto trans = PackageKit::Daemon::getFiles({installedPackageId()});
    connect(trans, &PackageKit::Transaction::errorCode, backend(), &PackageKitBackend::transactionError);
    connect(trans, &PackageKit::Transaction::files, this, [this](const QString & /*packageID*/, const QStringList &_filenames) {
        // This workarounds bug in zypper's backend (suse) https://github.com/hughsie/PackageKit/issues/351
        QStringList filenames = _filenames;
        if (filenames.count() == 1 && !QFile::exists(filenames.constFirst())) {
            filenames = filenames.constFirst().split(QLatin1Char(';'));
        }
        const auto allServices = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, m_appdata.id());
        if (!allServices.isEmpty()) {
            const auto packageServices = kFilter<QStringList>(allServices, [filenames](const QString &file) {
                return filenames.contains(file);
            });
            runService(packageServices);
            return;
        } else {
            const QStringList exes = m_appdata.provided(AppStream::Provided::KindBinary).items();
            const auto packageExecutables = kFilter<QStringList>(exes, [filenames](const QString &exe) {
                return filenames.contains(QLatin1Char('/') + exe);
            });
            if (!packageExecutables.isEmpty()) {
                QProcess::startDetached(exes.constFirst(), QStringList());
                return;
            } else {
                const auto locations = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
                const auto desktopFiles = kFilter<QStringList>(filenames, [locations](const QString &exe) {
                    for (const auto &location : locations) {
                        if (exe.startsWith(location))
                            return exe.contains(QLatin1String(".desktop"));
                    }
                    return false;
                });
                if (!desktopFiles.isEmpty()) {
                    runService(desktopFiles);
                    return;
                }
            }
            Q_EMIT backend()->passiveMessage(i18n("Cannot launch %1", name()));
        }
    });
}

QDate AppPackageKitResource::releaseDate() const
{
    if (!m_appdata.releases().isEmpty()) {
        auto release = m_appdata.releases().constFirst();
        return release.timestamp().date();
    }

    return {};
}

QString AppPackageKitResource::author() const
{
    return m_appdata.developerName();
}

void AppPackageKitResource::fetchChangelog()
{
    emit changelogFetched(changelog());
}
