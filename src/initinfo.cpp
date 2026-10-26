// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "initinfo.h"
#include "initinfo_debug.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

#ifdef SONIC_INITINFO_HAVE_DBUS
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#endif

#include <sys/stat.h>

namespace Sonic::InitInfo {
namespace {
    // Returns a null QString when the file can't be read, so callers can tell
    // "unreadable" from "empty".
    QString readTrimmed(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromLocal8Bit(file.readAll()).trimmed();
    }

    Pid1 pid1FromName(const QString& name)
    {
        if (name == QLatin1String("systemd")) {
            return Pid1::Systemd;
        }
        if (name == QLatin1String("openrc-init")) {
            return Pid1::OpenRCInit;
        }
        if (name == QLatin1String("runit") || name == QLatin1String("runit-init")) {
            return Pid1::RunitInit;
        }
        if (name == QLatin1String("s6-svscan") || name == QLatin1String("s6-linux-init")) {
            return Pid1::S6LinuxInit;
        }
        if (name == QLatin1String("dinit")) {
            return Pid1::Dinit;
        }
        if (name == QLatin1String("init")) {
            return Pid1::SysVinit;
        }
        if (name == QLatin1String("busybox")) {
            return Pid1::Busybox;
        }
        return Pid1::Unknown;
    }

    // The service manager an inittab starts: the first token of each entry's
    // process field decides. Entries are id:runlevels:action:process; the process
    // may itself contain ':' so only the first three separators split.
    ServiceManager serviceManagerFromInittab(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCDebug(SONIC_INITINFO) << "No readable inittab at" << path << "- assuming sysvinit manages services";
            return ServiceManager::SysVinit;
        }

        static const QRegularExpression whitespace(QStringLiteral("\\s+"));
        while (!file.atEnd()) {
            const QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            int position = -1;
            for (int separator = 0; separator < 3; ++separator) {
                position = line.indexOf(QLatin1Char(':'), position + 1);
                if (position < 0) {
                    break;
                }
            }
            if (position < 0) {
                continue;
            }
            const QStringList tokens = line.mid(position + 1).split(whitespace, Qt::SkipEmptyParts);
            if (tokens.isEmpty()) {
                continue;
            }
            const QString program = QFileInfo(tokens.first()).fileName();
            if (program == QLatin1String("openrc")) {
                qCDebug(SONIC_INITINFO) << "inittab starts OpenRC:" << line;
                return ServiceManager::OpenRC;
            }
            if (program == QLatin1String("runsvdir") || program == QLatin1String("runit")) {
                qCDebug(SONIC_INITINFO) << "inittab starts runit:" << line;
                return ServiceManager::Runit;
            }
        }
        qCDebug(SONIC_INITINFO) << "inittab starts no other service manager; sysvinit manages services";
        return ServiceManager::SysVinit;
    }

    SeatManager seatManagerFromName(const QString& name)
    {
        if (name == QLatin1String("systemd-logind")) {
            return SeatManager::SystemdLogind;
        }
        if (name == QLatin1String("elogind-daemon") || name == QLatin1String("elogind")) {
            return SeatManager::Elogind;
        }
        // comm is truncated to 15 characters: console-kit-daemon becomes console-kit-dae.
        if (name == QLatin1String("console-kit-dae") || name == QLatin1String("console-kit-daemon")) {
            return SeatManager::ConsoleKit2;
        }
        if (name == QLatin1String("seatd")) {
            return SeatManager::Seatd;
        }
        return SeatManager::None;
    }

    // Lower is preferred when several seat managers run at once.
    int seatPreference(SeatManager manager)
    {
        switch (manager) {
        case SeatManager::SystemdLogind:
            return 0;
        case SeatManager::Elogind:
            return 1;
        case SeatManager::ConsoleKit2:
            return 2;
        case SeatManager::Seatd:
            return 3;
        case SeatManager::None:
            break;
        }
        return 4;
    }

    bool isAllDigits(const QString& text)
    {
        if (text.isEmpty()) {
            return false;
        }
        for (const QChar character : text) {
            if (!character.isDigit()) {
                return false;
            }
        }
        return true;
    }

#ifdef SONIC_INITINFO_HAVE_DBUS
    SeatManager seatManagerFromDBus(const Environment& environment)
    {
        const QDBusConnection bus = QDBusConnection::systemBus();
        if (!bus.isConnected() || !bus.interface()) {
            qCDebug(SONIC_INITINFO) << "System bus unavailable for seat manager detection:" << bus.lastError().message();
            return SeatManager::None;
        }
        const QDBusReply<bool> login1 = bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.login1"));
        if (login1.isValid() && login1.value()) {
            return serviceManager(environment) == ServiceManager::Systemd ? SeatManager::SystemdLogind : SeatManager::Elogind;
        }
        const QDBusReply<bool> consoleKit = bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.ConsoleKit"));
        if (consoleKit.isValid() && consoleKit.value()) {
            return SeatManager::ConsoleKit2;
        }
        return SeatManager::None;
    }
#endif
}

Pid1 pid1(const Environment& environment)
{
#if defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN)
    Q_UNUSED(environment)
    return Pid1::BsdInit;
#else
    const QString commPath = environment.procRoot + QStringLiteral("/1/comm");
    const QString comm = readTrimmed(commPath);
    if (!comm.isNull()) {
        const Pid1 result = pid1FromName(comm);
        if (result == Pid1::Unknown) {
            qCWarning(SONIC_INITINFO) << "Unrecognised PID 1:" << comm;
        } else {
            qCDebug(SONIC_INITINFO) << "PID 1 is" << name(result) << "from" << commPath << "=" << comm;
        }
        return result;
    }

    // /proc/1/comm is hidden (for example hidepid). /proc/1/exe needs root, so
    // fall back to where /sbin/init points instead.
    const QFileInfo init(environment.fsRoot + QStringLiteral("/sbin/init"));
    if (init.isSymbolicLink()) {
        const QString target = QFileInfo(init.readSymLink()).fileName();
        const Pid1 result = pid1FromName(target);
        qCDebug(SONIC_INITINFO) << "PID 1 unreadable; /sbin/init points to" << init.readSymLink() << "->" << name(result);
        return result;
    }
    qCWarning(SONIC_INITINFO) << "Cannot identify PID 1:" << commPath << "is unreadable and /sbin/init is not a symlink";
    return Pid1::Unknown;
#endif
}

ServiceManager serviceManager(const Environment& environment)
{
#if defined(Q_OS_OPENBSD)
    Q_UNUSED(environment)
    return ServiceManager::OpenBsdRc;
#elif defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN)
    // Unverified on real BSD systems: OpenRC can replace rc.d on FreeBSD-based systems.
    if (QFileInfo(environment.fsRoot + QStringLiteral("/etc/init.d")).isDir()
        && !QStandardPaths::findExecutable(QStringLiteral("openrc-run")).isEmpty()) {
        return ServiceManager::OpenRC;
    }
    return ServiceManager::BsdRc;
#else
    ServiceManager result = ServiceManager::Unknown;
    switch (pid1(environment)) {
    case Pid1::Systemd:
        result = ServiceManager::Systemd;
        break;
    case Pid1::OpenRCInit:
        result = ServiceManager::OpenRC;
        break;
    case Pid1::RunitInit:
        result = ServiceManager::Runit;
        break;
    case Pid1::S6LinuxInit:
        result = ServiceManager::S6;
        break;
    case Pid1::Dinit:
        result = ServiceManager::Dinit;
        break;
    case Pid1::SysVinit:
    case Pid1::Busybox:
        result = serviceManagerFromInittab(environment.fsRoot + QStringLiteral("/etc/inittab"));
        break;
    case Pid1::BsdInit:
    case Pid1::Unknown:
        break;
    }
    qCDebug(SONIC_INITINFO) << "Service manager:" << name(result);
    return result;
#endif
}

SeatManager seatManager(const Environment& environment)
{
    SeatManager best = SeatManager::None;
#if !(defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN))
    const QDir proc(environment.procRoot);
    const QStringList entries = proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (!isAllDigits(entry)) {
            continue;
        }
        // Processes can exit while we scan; an unreadable entry is skipped.
        const QString comm = readTrimmed(proc.filePath(entry + QStringLiteral("/comm")));
        if (comm.isNull()) {
            continue;
        }
        const SeatManager candidate = seatManagerFromName(comm);
        if (candidate != SeatManager::None && seatPreference(candidate) < seatPreference(best)) {
            qCDebug(SONIC_INITINFO) << "Seat manager process" << entry << comm;
            best = candidate;
        }
    }
#else
    // Unverified: BSD process listing via sysctl(KERN_PROC) is not implemented yet.
#endif

#ifdef SONIC_INITINFO_HAVE_DBUS
    if (best == SeatManager::None && environment.useDBus) {
        best = seatManagerFromDBus(environment);
        if (best != SeatManager::None) {
            qCDebug(SONIC_INITINFO) << "Seat manager from D-Bus name ownership:" << name(best);
        }
    }
#endif
    qCDebug(SONIC_INITINFO) << "Seat manager:" << name(best);
    return best;
}

bool hasJournald(const Environment& environment)
{
    const QByteArray path = QFile::encodeName(environment.fsRoot + QStringLiteral("/run/systemd/journal/socket"));
    struct stat info{};
    return ::lstat(path.constData(), &info) == 0 && S_ISSOCK(info.st_mode);
}

QString name(Pid1 value)
{
    switch (value) {
    case Pid1::Unknown:
        return QStringLiteral("unknown");
    case Pid1::Systemd:
        return QStringLiteral("systemd");
    case Pid1::SysVinit:
        return QStringLiteral("sysvinit");
    case Pid1::OpenRCInit:
        return QStringLiteral("openrc-init");
    case Pid1::RunitInit:
        return QStringLiteral("runit");
    case Pid1::S6LinuxInit:
        return QStringLiteral("s6-linux-init");
    case Pid1::Dinit:
        return QStringLiteral("dinit");
    case Pid1::Busybox:
        return QStringLiteral("busybox");
    case Pid1::BsdInit:
        return QStringLiteral("BSD init");
    }
    return QStringLiteral("unknown");
}

QString name(ServiceManager value)
{
    switch (value) {
    case ServiceManager::Unknown:
        return QStringLiteral("unknown");
    case ServiceManager::Systemd:
        return QStringLiteral("systemd");
    case ServiceManager::OpenRC:
        return QStringLiteral("OpenRC");
    case ServiceManager::Runit:
        return QStringLiteral("runit");
    case ServiceManager::S6:
        return QStringLiteral("s6");
    case ServiceManager::Dinit:
        return QStringLiteral("dinit");
    case ServiceManager::SysVinit:
        return QStringLiteral("sysvinit");
    case ServiceManager::BsdRc:
        return QStringLiteral("BSD rc");
    case ServiceManager::OpenBsdRc:
        return QStringLiteral("OpenBSD rc");
    }
    return QStringLiteral("unknown");
}

QString name(SeatManager value)
{
    switch (value) {
    case SeatManager::None:
        return QStringLiteral("none");
    case SeatManager::SystemdLogind:
        return QStringLiteral("systemd-logind");
    case SeatManager::Elogind:
        return QStringLiteral("elogind");
    case SeatManager::ConsoleKit2:
        return QStringLiteral("ConsoleKit2");
    case SeatManager::Seatd:
        return QStringLiteral("seatd");
    }
    return QStringLiteral("none");
}

int defaultInitialVt(ServiceManager manager)
{
    switch (manager) {
    case ServiceManager::OpenRC:
    case ServiceManager::Runit:
    case ServiceManager::S6:
        return 2;
    default:
        return 1;
    }
}
}
