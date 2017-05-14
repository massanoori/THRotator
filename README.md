THRotator
=====

THRotator allows the rotation of Touhou Project rendering.
The user can customize the positions of HUD elements.

THRotator works on D3D8-based works (Th06-95) and D3D9-based works (Th10-Th13).

Dependencies
=====

This repository is self-contained for building, except for Boost C++ Library.

To build:

* change include and library paths to point Boost C++ Library properly.
* build filesystem module in Boost C++ Library in advance. Adding `--with-filesystem` argument to `b2` saves your time!

THRotator uses D3DX9 library, which are not provided by up-to-date Windows SDK.
This repository contains headers and libs of D3DX9 from DirectX SDK June 2010.

THRotator also uses D3D8 and D3DX8.
Include files and libraries are copied from an older DirectX SDK.
Unfortunately, the precise version of SDK has been missed.
Because the last release of D3DX8 SDK is 2004 Oct according to <https://github.com/apitrace/apitrace/wiki/DirectX-SDK>,
the files have been copied from it or an earlier version.
