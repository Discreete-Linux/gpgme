/* t-keylist.cpp

    This file is part of qgpgme, the Qt API binding for gpgme
    Copyright (c) 2016 Intevation GmbH

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

#include <QDebug>
#include <QTest>
#include <QSignalSpy>
#include <QMap>
#include "keylistjob.h"
#include "qgpgmebackend.h"
#include "keylistresult.h"

#include "t-support.h"

using namespace QGpgME;
using namespace GpgME;

class KeyListTest : public QGpgMETest
{
    Q_OBJECT

Q_SIGNALS:
    void asyncDone();

private Q_SLOTS:
    void testSingleKeyListSync()
    {
        KeyListJob *job = openpgp()->keyListJob(false, false, false);
        std::vector<GpgME::Key> keys;
        GpgME::KeyListResult result = job->exec(QStringList() << QStringLiteral("alfa@example.net"),
                                                false, keys);
        delete job;
        Q_ASSERT (!result.error());
        Q_ASSERT (keys.size() == 1);
        const QString kId = QLatin1String(keys.front().keyID());
        Q_ASSERT (kId == QStringLiteral("2D727CC768697734"));

        Q_ASSERT (keys[0].subkeys().size() == 2);
        Q_ASSERT (keys[0].subkeys()[0].publicKeyAlgorithm() == Subkey::AlgoDSA);
        Q_ASSERT (keys[0].subkeys()[1].publicKeyAlgorithm() == Subkey::AlgoELG_E);
    }

    void testPubkeyAlgoAsString()
    {
        static const QMap<Subkey::PubkeyAlgo, QString> expected {
            { Subkey::AlgoRSA,    QStringLiteral("RSA") },
            { Subkey::AlgoRSA_E,  QStringLiteral("RSA-E") },
            { Subkey::AlgoRSA_S,  QStringLiteral("RSA-S") },
            { Subkey::AlgoELG_E,  QStringLiteral("ELG-E") },
            { Subkey::AlgoDSA,    QStringLiteral("DSA") },
            { Subkey::AlgoECC,    QStringLiteral("ECC") },
            { Subkey::AlgoELG,    QStringLiteral("ELG") },
            { Subkey::AlgoECDSA,  QStringLiteral("ECDSA") },
            { Subkey::AlgoECDH,   QStringLiteral("ECDH") },
            { Subkey::AlgoEDDSA,  QStringLiteral("EdDSA") },
            { Subkey::AlgoUnknown, QString() }
        };
        Q_FOREACH (Subkey::PubkeyAlgo algo, expected.keys()) {
            Q_ASSERT(QString::fromUtf8(Subkey::publicKeyAlgorithmAsString(algo)) ==
                     expected.value(algo));
        }
    }

    void testKeyListAsync()
    {
        KeyListJob *job = openpgp()->keyListJob();
        connect(job, &KeyListJob::result, job, [this, job](KeyListResult, std::vector<Key> keys, QString, Error)
        {
            Q_ASSERT(keys.size() == 1);
            Q_EMIT asyncDone();
        });
        job->start(QStringList() << "alfa@example.net");
        QSignalSpy spy (this, SIGNAL(asyncDone()));
        Q_ASSERT(spy.wait());
    }
};

QTEST_MAIN(KeyListTest)

#include "t-keylist.moc"
