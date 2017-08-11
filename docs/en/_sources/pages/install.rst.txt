================
Install
================

This page shows the steps to install THRotator.

Download THRotator
=============================

Download the latest release from `<https://github.com/massanoori/THRotator/releases>`_.
The file named like ``THRotator_X_Y_Z.zip`` is what has to be downloaded.


Determine which DLL is needed
=============================

If you would like to install THRotator to one of the following, the file to copy is ``d3d8/d3d8.dll``.

- the Embodiment of Scarlet Devil (Th06)
- Perfect Cherry Blossom (Th07)
- Imperishable Night (Th08)
- Phantasmagoria of Flower View (Th09)
- Shoot the Bullet (Th095)

If you would like to install THRotator to one of the following or later products, the file to copy is ``d3d9/d3d9.dll``.

- Mountain of Faith (Th10)
- Subterranean Animism (Th11)
- Undefined Fantastic Object (Th12)
- Double Spoiler (Th125)
- Fairy Wars (Th128)
- Ten Desires (Th13)
- Double Dealing Character (Th14, tested on demo version)
- Impossible Spell Card (Th143)
- Legacy of Lunatic Kingdom (Th15, tested on demo version)
- (Hidden Star in Four Seasons)
- Uwabami Breakers (alcostg)

Copy DLL file
=========================

Copy DLL file to the location where the game is installed (where the .exe file exists).
Windows may ask you to input an administrator's name and password to copy the DLL file.


Copy config file
=========================

Copy config file corresponding to the game ``<filename of executable>.throtator`` to the following directory.

For Th06, Th07, Th08, Th09, Th095, Th10, Th11, Th12, and alcostg
  Where the game is installed.

For Th125, Th128, Th13, Th14, Th143, Th15, and later
  Where the save data (score, replay, screen capture, and others) is saved.

.. note:: Location of save data
   
   - On Windows Vista, 7, 8, and 10
   
     - ``C:\Users\<user name>\AppData\Roaming\ShanghaiAlice\th<product number>\``

   - Executing command ``set APPDATA=`` before launching
   
     - Where the game is installed.

Localize
========================

GUI and messages are in Japanese by default.

If you copy ``d3d8/en-US`` (for  ``d3d8/d3d8.dll``) or ``d3d9/en-US`` (for ``d3d9/d3d9.dll``) folder to the same directory where DLL file exists, GUI and messages become in English.



Aspect ratio distortion by default config files
===============================================================

In order to show all digits in HUD elements,
their aspect ratios are not preserved in the following products.

- Th08 (To show all digits)
- Th095 (To show all digits)
- Th125 (To show all digits)
- Th128 (To show `Perfect Freeze`)
- Since Th13 (To show the denominators of fragments)

