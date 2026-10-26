<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Sonic Init Info

Sonic Init Info tells SonicDE components, at runtime, which init system and seat manager the machine is running. Code can then do the right thing on systemd, OpenRC, runit, s6, dinit, sysvinit and the BSDs instead of assuming systemd.

> **Status:** in development. The API below is the planned design and may still change. See [Sonic-DE/sonic#17](https://github.com/Sonic-DE/sonic/issues/17).

## What it reports

- **PID 1:** the init process itself, for example systemd, sysvinit, openrc-init, runit, s6-linux-init or dinit.
- **Service manager:** what actually manages services. It can differ from PID 1; for example, OpenRC commonly runs under sysvinit.
- **Seat manager:** systemd-logind, elogind, ConsoleKit2, seatd, or none.

Detection uses stable sources rather than transient runtime files:

- PID 1 is identified from `/proc/1/comm`.
- The service manager is identified from PID 1 and the system configuration, such as `/etc/inittab`.
- The seat manager is found by looking for its running daemon.

None of this needs D-Bus or root.

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

It needs Qt 6.11 or newer and Extra CMake Modules. Qt D-Bus is optional and only adds an extra detection signal.
