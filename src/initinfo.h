// SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sonicinitinfo_export.h"

#include <QString>

/*!
 * Runtime information about the init system and seat manager.
 *
 * Detection reads PID 1's identity and that init's own configuration; it never
 * consults runtime marker files. Nothing is cached, so callers that query
 * repeatedly should keep the result.
 */
namespace Sonic::InitInfo {
//! The program running as PID 1.
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

//! What manages services, which can differ from PID 1 (for example OpenRC under sysvinit).
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

enum class SeatManager {
    None,
    SystemdLogind,
    Elogind,
    ConsoleKit2,
    Seatd,
};

//! Where to look; the defaults describe the live system. Tests point these at a fake tree.
struct Environment {
    QString procRoot = QStringLiteral("/proc");
    QString fsRoot; //!< prefix for /etc, /sbin and /run
    bool useDBus = true;
};

SONICINITINFO_EXPORT Pid1 pid1(const Environment& environment = {});
SONICINITINFO_EXPORT ServiceManager serviceManager(const Environment& environment = {});
SONICINITINFO_EXPORT SeatManager seatManager(const Environment& environment = {});

//! True when journald's native socket exists and is a socket.
SONICINITINFO_EXPORT bool hasJournald(const Environment& environment = {});

SONICINITINFO_EXPORT QString name(Pid1 value);
SONICINITINFO_EXPORT QString name(ServiceManager value);
SONICINITINFO_EXPORT QString name(SeatManager value);

//! The VT a display manager starts on: 2 where getty usually owns VT 1 (OpenRC, runit, s6), else 1.
SONICINITINFO_EXPORT int defaultInitialVt(ServiceManager manager);
}
