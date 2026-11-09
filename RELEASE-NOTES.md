# Sonic Frameworks Init Info Release Notes

Sonic Frameworks Init Info lets SonicDE programs learn how your computer starts up and manages logins, so the desktop works well with or without systemd.

## 6.31.0 (2026-11-10)

- New: SonicDE programs can now detect which init system (the program that starts your computer's services) and seat manager (the service that handles logins and devices) your computer uses, so they behave correctly without systemd.
- Behind the scenes: switched to SonicDE's own logging system, and its developer documentation now appears on the SonicDE website.
