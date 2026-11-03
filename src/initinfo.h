// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicinitinfo_export.h"

#include <QString>

/*!
 * \namespace Sonic::InitInfo
 * \inmodule SonicInitInfo
 * \inheaderfile Sonic/InitInfo/InitInfo
 *
 * \brief Runtime information about the init system and seat manager.
 *
 * Detection reads PID 1's identity and that init's own configuration; it never
 * consults runtime marker files. Nothing is cached, so callers that query
 * repeatedly should keep the result.
 *
 * In containers and sandboxes PID 1 is the sandbox helper, so Pid1::Unknown,
 * ServiceManager::Unknown and SeatManager::None are normal results that callers
 * must handle.
 */
namespace Sonic::InitInfo {
/*!
 * \enum Sonic::InitInfo::Pid1
 *
 * The program running as PID 1.
 *
 * \value Unknown Not recognized, for example a container's or sandbox's helper.
 * \value Systemd systemd.
 * \value SysVinit sysvinit.
 * \value OpenRCInit openrc-init.
 * \value RunitInit runit.
 * \value S6LinuxInit s6-linux-init.
 * \value Dinit dinit.
 * \value Busybox BusyBox init.
 * \value BsdInit A BSD init.
 */
enum class Pid1 {
    Unknown,
    Systemd,
    SysVinit,
    OpenRCInit,
    RunitInit,
    S6LinuxInit,
    Dinit,
    Busybox,
    BsdInit,
};

/*!
 * \enum Sonic::InitInfo::ServiceManager
 *
 * What manages services, which can differ from PID 1 (for example OpenRC under sysvinit).
 *
 * \value Unknown Not recognized.
 * \value Systemd systemd.
 * \value OpenRC OpenRC.
 * \value Runit runit.
 * \value S6 s6.
 * \value Dinit dinit.
 * \value SysVinit sysvinit's own rc scripts.
 * \value BsdRc The rc.d system of the BSDs other than OpenBSD.
 * \value OpenBsdRc OpenBSD's rc.
 */
enum class ServiceManager {
    Unknown,
    Systemd,
    OpenRC,
    Runit,
    S6,
    Dinit,
    SysVinit,
    BsdRc,
    OpenBsdRc,
};

/*!
 * \enum Sonic::InitInfo::SeatManager
 *
 * What manages seats and sessions.
 *
 * \value None No seat manager runs.
 * \value SystemdLogind systemd-logind.
 * \value Elogind elogind.
 * \value ConsoleKit2 ConsoleKit2.
 * \value Seatd seatd.
 */
enum class SeatManager {
    None,
    SystemdLogind,
    Elogind,
    ConsoleKit2,
    Seatd,
};

/*!
 * \class Sonic::InitInfo::Environment
 * \inmodule SonicInitInfo
 * \inheaderfile Sonic/InitInfo/InitInfo
 *
 * \brief Where to look; the defaults describe the live system.
 *
 * Tests point these at a fake tree.
 */
struct Environment {
    /*!
     * \variable Sonic::InitInfo::Environment::procRoot
     * The proc file system.
     */
    QString procRoot = QStringLiteral("/proc");
    /*!
     * \variable Sonic::InitInfo::Environment::fsRoot
     * Prefix for /etc, /sbin and /run.
     */
    QString fsRoot;
    /*!
     * \variable Sonic::InitInfo::Environment::useDBus
     * Whether seatManager() may also ask the system bus, when the library is built with Qt D-Bus.
     */
    bool useDBus = true;
};

/*!
 * \fn Sonic::InitInfo::Pid1 Sonic::InitInfo::pid1(const Sonic::InitInfo::Environment &environment)
 *
 * The program running as PID 1, from /proc/1/comm, or from where /sbin/init
 * points when /proc hides it. \a environment says where to look.
 */
SONICINITINFO_EXPORT Pid1 pid1(const Environment& environment = {});

/*!
 * \fn Sonic::InitInfo::ServiceManager Sonic::InitInfo::serviceManager(const Sonic::InitInfo::Environment &environment)
 *
 * What manages services, from PID 1 and that init's own configuration, such as
 * /etc/inittab. \a environment says where to look.
 */
SONICINITINFO_EXPORT ServiceManager serviceManager(const Environment& environment = {});

/*!
 * \fn Sonic::InitInfo::SeatManager Sonic::InitInfo::seatManager(const Sonic::InitInfo::Environment &environment)
 *
 * The seat manager whose daemon is running; when several run, systemd-logind
 * comes first, then elogind, ConsoleKit2 and seatd. When built with Qt D-Bus,
 * name ownership on the system bus is an extra signal. \a environment says
 * where to look.
 */
SONICINITINFO_EXPORT SeatManager seatManager(const Environment& environment = {});

/*!
 * \fn bool Sonic::InitInfo::hasJournald(const Sonic::InitInfo::Environment &environment)
 *
 * True when journald's native socket exists and is a socket. \a environment
 * says where to look.
 */
SONICINITINFO_EXPORT bool hasJournald(const Environment& environment = {});

/*!
 * \fn QString Sonic::InitInfo::name(Sonic::InitInfo::Pid1 value)
 *
 * The name of \a value for logging and display, for example "openrc-init".
 */
SONICINITINFO_EXPORT QString name(Pid1 value);

/*!
 * \fn QString Sonic::InitInfo::name(Sonic::InitInfo::ServiceManager value)
 * \overload
 *
 * The name of \a value for logging and display, for example "OpenRC".
 */
SONICINITINFO_EXPORT QString name(ServiceManager value);

/*!
 * \fn QString Sonic::InitInfo::name(Sonic::InitInfo::SeatManager value)
 * \overload
 *
 * The name of \a value for logging and display, for example "elogind".
 */
SONICINITINFO_EXPORT QString name(SeatManager value);

/*!
 * \fn int Sonic::InitInfo::defaultInitialVt(Sonic::InitInfo::ServiceManager manager)
 *
 * The VT a display manager starts on under \a manager: 2 where getty usually
 * owns VT 1 (OpenRC, runit, s6), else 1.
 */
SONICINITINFO_EXPORT int defaultInitialVt(ServiceManager manager);
}
