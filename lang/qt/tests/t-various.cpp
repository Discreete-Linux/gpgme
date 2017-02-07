/* t-various.cpp

    This file is part of qgpgme, the Qt API binding for gpgme
    Copyright (c) 2017 Intevation GmbH

    QGpgME is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    QGpgME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <QDebug>
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "keylistjob.h"
#include "protocol.h"
#include "keylistresult.h"
#include "context.h"
#include "engineinfo.h"

#include "t-support.h"

using namespace QGpgME;
using namespace GpgME;

class TestVarious: public QGpgMETest
{
    Q_OBJECT

Q_SIGNALS:
    void asyncDone();

private Q_SLOTS:

    void testQuickUid()
    {
        if (GpgME::engineInfo(GpgME::GpgEngine).engineVersion() < "2.1.13") {
            return;
        }
        KeyListJob *job = openpgp()->keyListJob(false, true, true);
        std::vector<GpgME::Key> keys;
        GpgME::KeyListResult result = job->exec(QStringList() << QStringLiteral("alfa@example.net"),
                                                false, keys);
        delete job;
        QVERIFY (!result.error());
        QVERIFY (keys.size() == 1);
        Key key = keys.front();

        QVERIFY (key.numUserIDs() == 3);
        const char uid[] = "Foo Bar (with comment) <foo@bar.baz>";

        auto ctx = Context::createForProtocol(key.protocol());
        QVERIFY (ctx);
        TestPassphraseProvider provider;
        ctx->setPassphraseProvider(&provider);
        ctx->setPinentryMode(Context::PinentryLoopback);

        QVERIFY(!ctx->addUid(key, uid));
        delete ctx;
        key.update();

        QVERIFY (key.numUserIDs() == 4);
        bool id_found = false;;
        for (const auto &u: key.userIDs()) {
            if (!strcmp (u.id(), uid)) {
                QVERIFY (!u.isRevoked());
                id_found = true;
                break;
            }
        }
        QVERIFY (id_found);

        ctx = Context::createForProtocol(key.protocol());
        QVERIFY (!ctx->revUid(key, uid));
        delete ctx;
        key.update();

        bool id_revoked = false;;
        for (const auto &u: key.userIDs()) {
            if (!strcmp (u.id(), uid)) {
                id_revoked = true;
                break;
            }
        }
        QVERIFY(id_revoked);
    }

    void initTestCase()
    {
        QGpgMETest::initTestCase();
        const QString gpgHome = qgetenv("GNUPGHOME");
        QVERIFY(copyKeyrings(gpgHome, mDir.path()));
        qputenv("GNUPGHOME", mDir.path().toUtf8());
    }

private:
    QTemporaryDir mDir;
};

QTEST_MAIN(TestVarious)

#include "t-various.moc"
