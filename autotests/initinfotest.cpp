// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "initinfo.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace Sonic::InitInfo;

Q_DECLARE_METATYPE(Sonic::InitInfo::Pid1)
Q_DECLARE_METATYPE(Sonic::InitInfo::ServiceManager)
Q_DECLARE_METATYPE(Sonic::InitInfo::SeatManager)

namespace {
const QString sonicOpenRCInittab = QStringLiteral(
    "id:3:initdefault:\n"
    "si::sysinit:/sbin/openrc sysinit\n"
    "rc::bootwait:/sbin/openrc boot\n"
    "l0:0:wait:/sbin/openrc shutdown\n");

// A fake /proc and filesystem root. Each test gets a fresh tree.
class FakeSystem {
public:
    FakeSystem()
    {
        QVERIFY(m_directory.isValid());
        QVERIFY(QDir().mkpath(procRoot()));
        QVERIFY(QDir().mkpath(fsRoot()));
    }

    QString procRoot() const
    {
        return m_directory.filePath(QStringLiteral("proc"));
    }

    QString fsRoot() const
    {
        return m_directory.filePath(QStringLiteral("root"));
    }

    Environment environment() const
    {
        return Environment{procRoot(), fsRoot(), false};
    }

    void writeFile(const QString& path, const QString& content) const
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(content.toLocal8Bit());
    }

    void setProcess(const QString& pid, const QString& comm) const
    {
        writeFile(procRoot() + QLatin1Char('/') + pid + QStringLiteral("/comm"), comm + QLatin1Char('\n'));
    }

    void setInittab(const QString& content) const
    {
        writeFile(fsRoot() + QStringLiteral("/etc/inittab"), content);
    }

private:
    QTemporaryDir m_directory;
};
}

class InitInfoTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void pid1_data()
    {
        QTest::addColumn<QString>("comm");
        QTest::addColumn<Pid1>("expected");
        QTest::newRow("systemd") << QStringLiteral("systemd") << Pid1::Systemd;
        QTest::newRow("sysvinit") << QStringLiteral("init") << Pid1::SysVinit;
        QTest::newRow("openrc-init") << QStringLiteral("openrc-init") << Pid1::OpenRCInit;
        QTest::newRow("runit") << QStringLiteral("runit") << Pid1::RunitInit;
        QTest::newRow("s6") << QStringLiteral("s6-svscan") << Pid1::S6LinuxInit;
        QTest::newRow("dinit") << QStringLiteral("dinit") << Pid1::Dinit;
        QTest::newRow("busybox") << QStringLiteral("busybox") << Pid1::Busybox;
        QTest::newRow("unknown") << QStringLiteral("something-else") << Pid1::Unknown;
    }

    void pid1()
    {
        QFETCH(QString, comm);
        QFETCH(Pid1, expected);
        FakeSystem system;
        system.setProcess(QStringLiteral("1"), comm);
        QCOMPARE(Sonic::InitInfo::pid1(system.environment()), expected);
    }

    void pid1FallsBackToInitSymlink()
    {
        FakeSystem system;
        QVERIFY(QDir().mkpath(system.fsRoot() + QStringLiteral("/sbin")));
        QVERIFY(QFile::link(QStringLiteral("../lib/systemd/systemd"), system.fsRoot() + QStringLiteral("/sbin/init")));
        QCOMPARE(Sonic::InitInfo::pid1(system.environment()), Pid1::Systemd);
    }

    void pid1UnknownWithoutCommOrSymlink()
    {
        FakeSystem system;
        system.writeFile(system.fsRoot() + QStringLiteral("/sbin/init"), QStringLiteral("binary"));
        QCOMPARE(Sonic::InitInfo::pid1(system.environment()), Pid1::Unknown);
    }

    void markerFilesAreIgnored()
    {
        FakeSystem system;
        QVERIFY(QDir().mkpath(system.fsRoot() + QStringLiteral("/run/systemd/system")));
        QVERIFY(QDir().mkpath(system.fsRoot() + QStringLiteral("/run/openrc")));
        QCOMPARE(Sonic::InitInfo::pid1(system.environment()), Pid1::Unknown);
        QCOMPARE(Sonic::InitInfo::serviceManager(system.environment()), ServiceManager::Unknown);
    }

    void serviceManager_data()
    {
        QTest::addColumn<QString>("comm");
        QTest::addColumn<QString>("inittab");
        QTest::addColumn<bool>("writeInittab");
        QTest::addColumn<ServiceManager>("expected");
        QTest::newRow("systemd") << QStringLiteral("systemd") << QString() << false << ServiceManager::Systemd;
        QTest::newRow("openrc-init") << QStringLiteral("openrc-init") << QString() << false << ServiceManager::OpenRC;
        QTest::newRow("runit") << QStringLiteral("runit") << QString() << false << ServiceManager::Runit;
        QTest::newRow("s6") << QStringLiteral("s6-svscan") << QString() << false << ServiceManager::S6;
        QTest::newRow("dinit") << QStringLiteral("dinit") << QString() << false << ServiceManager::Dinit;
        QTest::newRow("sysvinit starting openrc") << QStringLiteral("init") << sonicOpenRCInittab << true << ServiceManager::OpenRC;
        QTest::newRow("busybox starting openrc") << QStringLiteral("busybox") << QStringLiteral("::sysinit:/sbin/openrc sysinit\n") << true
                                                 << ServiceManager::OpenRC;
        QTest::newRow("commented openrc") << QStringLiteral("init") << QStringLiteral("#si::sysinit:/sbin/openrc sysinit\n") << true
                                          << ServiceManager::SysVinit;
        QTest::newRow("runsvdir") << QStringLiteral("init") << QStringLiteral("SV:123456:respawn:/usr/bin/runsvdir -P /etc/service\n") << true
                                  << ServiceManager::Runit;
        QTest::newRow("runsvdir-start is not runsvdir") << QStringLiteral("init") << QStringLiteral("SV:123456:respawn:/sbin/runsvdir-start\n") << true
                                                        << ServiceManager::SysVinit;
        QTest::newRow("plain sysvinit") << QStringLiteral("init") << QStringLiteral("si::sysinit:/etc/init.d/rcS\n") << true
                                        << ServiceManager::SysVinit;
        QTest::newRow("no inittab") << QStringLiteral("init") << QString() << false << ServiceManager::SysVinit;
        QTest::newRow("unknown pid 1") << QStringLiteral("something-else") << QString() << false << ServiceManager::Unknown;
    }

    void serviceManager()
    {
        QFETCH(QString, comm);
        QFETCH(QString, inittab);
        QFETCH(bool, writeInittab);
        QFETCH(ServiceManager, expected);
        FakeSystem system;
        system.setProcess(QStringLiteral("1"), comm);
        if (writeInittab) {
            system.setInittab(inittab);
        }
        QCOMPARE(Sonic::InitInfo::serviceManager(system.environment()), expected);
    }

    void seatManager_data()
    {
        QTest::addColumn<QStringList>("processes");
        QTest::addColumn<SeatManager>("expected");
        QTest::newRow("systemd-logind") << QStringList{QStringLiteral("systemd-logind")} << SeatManager::SystemdLogind;
        QTest::newRow("elogind-daemon") << QStringList{QStringLiteral("elogind-daemon")} << SeatManager::Elogind;
        QTest::newRow("elogind") << QStringList{QStringLiteral("elogind")} << SeatManager::Elogind;
        QTest::newRow("consolekit truncated") << QStringList{QStringLiteral("console-kit-dae")} << SeatManager::ConsoleKit2;
        QTest::newRow("seatd") << QStringList{QStringLiteral("seatd")} << SeatManager::Seatd;
        QTest::newRow("elogind preferred over seatd") << QStringList{QStringLiteral("seatd"), QStringLiteral("elogind-daemon")} << SeatManager::Elogind;
        QTest::newRow("none") << QStringList{QStringLiteral("bash"), QStringLiteral("init")} << SeatManager::None;
    }

    void seatManager()
    {
        QFETCH(QStringList, processes);
        QFETCH(SeatManager, expected);
        FakeSystem system;
        int pid = 100;
        for (const QString& comm : processes) {
            system.setProcess(QString::number(pid++), comm);
        }
        // Non-numeric entries and numeric entries without comm are skipped.
        system.writeFile(system.procRoot() + QStringLiteral("/self/comm"), QStringLiteral("elogind\n"));
        QVERIFY(QDir().mkpath(system.procRoot() + QStringLiteral("/999")));
        QCOMPARE(Sonic::InitInfo::seatManager(system.environment()), expected);
    }

    void hasJournald()
    {
        FakeSystem system;
        const QString directory = system.fsRoot() + QStringLiteral("/run/systemd/journal");
        QVERIFY(QDir().mkpath(directory));
        const QString socketPath = directory + QStringLiteral("/socket");
        QVERIFY(!Sonic::InitInfo::hasJournald(system.environment()));

        system.writeFile(socketPath, QStringLiteral("not a socket"));
        QVERIFY(!Sonic::InitInfo::hasJournald(system.environment()));
        QVERIFY(QFile::remove(socketPath));

        const QByteArray encoded = QFile::encodeName(socketPath);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        QVERIFY2(size_t(encoded.size()) < sizeof(address.sun_path), "temporary path too long for a Unix socket");
        memcpy(address.sun_path, encoded.constData(), encoded.size());
        const int descriptor = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        QVERIFY(descriptor >= 0);
        QCOMPARE(::bind(descriptor, reinterpret_cast<const sockaddr*>(&address), sizeof(address)), 0);
        QVERIFY(Sonic::InitInfo::hasJournald(system.environment()));
        ::close(descriptor);
    }

    void defaultInitialVt_data()
    {
        QTest::addColumn<ServiceManager>("manager");
        QTest::addColumn<int>("expected");
        QTest::newRow("systemd") << ServiceManager::Systemd << 1;
        QTest::newRow("openrc") << ServiceManager::OpenRC << 2;
        QTest::newRow("runit") << ServiceManager::Runit << 2;
        QTest::newRow("s6") << ServiceManager::S6 << 2;
        QTest::newRow("dinit") << ServiceManager::Dinit << 1;
        QTest::newRow("sysvinit") << ServiceManager::SysVinit << 1;
        QTest::newRow("unknown") << ServiceManager::Unknown << 1;
    }

    void defaultInitialVt()
    {
        QFETCH(ServiceManager, manager);
        QFETCH(int, expected);
        QCOMPARE(Sonic::InitInfo::defaultInitialVt(manager), expected);
    }
};

QTEST_GUILESS_MAIN(InitInfoTest)

#include "initinfotest.moc"
