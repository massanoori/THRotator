THRotator
=====

THRotator allows screen rotation of Touhou Project.
The user can customize the positions of HUD elements.

THRotator works on D3D8-based works (Th06-95) and D3D9-based works (Th10 and later).

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
Because the last release of D3DX8 SDK is 2004 Oct according to [this page](https://github.com/apitrace/apitrace/wiki/DirectX-SDK),
the files have been copied from it or an earlier version.


Motivation
=====

Development of THRotator was initially inspired by Vertical Play developed by [niisaka at Tari Lari Run](http://bygzam.seesaa.net/),
which is the first screen rotation tool for Touhou Project to my best knowledge.
In fact, THRotator and Vertical Play share the way how a user rotates a screen (alt+arrow keys).

But in terms of the following points, THRotator and Vertical Play are different.

Support on earlier works
-----

Vertical Play provides screen rotation functionality only to works working on Direct3D 9 (Touhou 10 and later).

In addition to supporting works on Direct3D 9, THRotator also supports works built upon Direct3D 8 (from Touhou 6 to 9.5).


Future product support
-----

Although I haven't seen any specific implementation of Vertical Play,
the rearrangements of HUD elements seem hard coded.
When a new title is released, users must wait for implementation by the developer.

THRotator allows users to configure how HUD elements are arranged.
Users can enjoy a new product immediately after its release with THRotator on vertically-long screen.


How user's configuration is saved
-----

Vertical Play saves user's configuration via registry keys on Windows.

On the other hand, THRotator saves user's configuration through .ini file.



Performance
-----

I haven't measured performance and seen implementation of Vertical Play.
But what Vertical Play does looks simply hacking transform matrix update from Touhou Project.

On the other hand, THRotator copies pixels from dummy back buffer,
which is undoubtedly more costly than just overwriting transform matrices.


Neatness of rendering
-----

What THRotator does is just copying rects.
Thus noticeable pixel discontinuity appears near boundary of rects.

Vertical Play results in much cleaner rendering since it overrides draw calls with transparency.
HUD elements and background is well blended with Vertical Play.

License
=====

.ini files
-----

Public domain.

Other files
-----

Under GPL v3
