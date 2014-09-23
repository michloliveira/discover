/***************************************************************************
 *   Copyright © 2010-2013 Jonathan Thomas <echidnaman@kubuntu.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "MainWindow.h"
#include <MuonBackendsFactory.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <KAboutData>
#include <klocalizedstring.h>
#include <kdbusservice.h>
#include "../MuonVersion.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme("system-software-update"));
    KAboutData about("muonupdater", i18n("Muon Update Manager"), version, i18n("An update manager"),
                     KAboutLicense::GPL, i18n("©2010-2013 Jonathan Thomas"), QString(), 0);
    about.addAuthor(i18n("Jonathan Thomas"), QString(), "echidnaman@kubuntu.org");
    about.setProductName("muon/updater");

    {
        QCommandLineParser parser;
        parser.addOption(QCommandLineOption("backends", i18n("List all the backends we'll want to have loaded, separated by coma ','."), "names"));
        MuonBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.addHelpOption();
        parser.addVersionOption();
        parser.process(app);
        about.processCommandLine(&parser);
        MuonBackendsFactory::processCommandLine(&parser);
        MuonBackendsFactory::setRequestedBackends(parser.value("backends").split(",", QString::SkipEmptyParts));

    }

    KDBusService service(KDBusService::Unique);
//     app.disableSessionManagement();


    MainWindow *mainWindow = new MainWindow;
    mainWindow->show();

    return app.exec();
}
