<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Sonic Init Info

Sonic Init Info tells SonicDE components, at runtime, which init system and seat manager the machine is running. Code can then do the right thing on systemd, OpenRC, runit, s6, dinit, sysvinit and the BSDs instead of assuming systemd.

See [Sonic-DE/sonic#17](https://github.com/Sonic-DE/sonic/issues/17) for the background.

## What it reports

- **PID 1** (`pid1()`): the init process itself, for example systemd, sysvinit, openrc-init, runit, s6-linux-init, dinit or busybox.
- **Service manager** (`serviceManager()`): what actually manages services. It can differ from PID 1; for example, OpenRC commonly runs under sysvinit.
- **Seat manager** (`seatManager()`): systemd-logind, elogind, ConsoleKit2, seatd, or none.
- **journald** (`hasJournald()`): whether journald's native socket exists.
- **The initial VT** a display manager starts on (`defaultInitialVt()`): 2 where getty usually owns VT 1 (OpenRC, runit, s6), else 1.
- **Names** for logging and display (`name()`).

Detection uses stable sources rather than transient runtime files:

- PID 1 is identified from `/proc/1/comm`, or from the `/sbin/init` symlink when `/proc` hides it.
- The service manager is identified from PID 1 and that init's own configuration, such as `/etc/inittab`.
- The seat manager is found by looking for its running daemon; when built with Qt D-Bus, name ownership on the system bus is an extra signal.

None of this needs root. Nothing is cached; callers keep the result. In containers and sandboxes PID 1 is the sandbox helper, so `Unknown` and `None` are normal results that callers must handle.

Decisions are logged with Sonic Logging under the component `org.sonicde.frameworks.initinfo`, for example `QT_LOGGING_RULES="org.sonicde.frameworks.initinfo.debug=true"`.

## Usage

```cmake
find_package(SonicInitInfo REQUIRED)
target_link_libraries(myapp PRIVATE Sonic::InitInfo)
```

```cpp
#include <Sonic/InitInfo/InitInfo>

using namespace Sonic::InitInfo;

if (serviceManager() == ServiceManager::OpenRC) {
    // use rc-service / rc-update
}
if (seatManager() == SeatManager::Elogind) {
    // elogind-specific session handling
}
```

## Building

Sonic Init Info is built as part of SonicDE with the [sonicde-builder](https://github.com/Sonic-DE/sonicde-builder):

```shell
./build-repo sonic-frameworks-init-info
```

It needs Qt 6.11 or newer, Extra CMake Modules and [Sonic Logging](https://github.com/Sonic-DE/sonic-frameworks-logging). Qt D-Bus is optional and only adds an extra detection signal.
