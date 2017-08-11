=========================
Troubleshoot
=========================

Screen doesn't rotate by ``Alt+Left`` and ``Alt+Right`` keys
======================================================================

Make sure that ``d3d8.dll`` or ``d3d9.dll`` has been copied to the appropriate location.
If it doesn't work correctly even after copy, make sure that the appropriate file has been copied.
For example, ``d3d9.dll`` doesn't work on games run on Direct3D 8. Follow instruction described in :doc:`install` carefully.


``Show THRotator window`` or ``Open THRotator`` doesn't appears from title bar
=================================================================================================

Make sure that ``d3d8.dll`` or ``d3d9.dll`` has been copied to the appropriate location.
If it doesn't work correctly even after copy, make sure that the appropriate file has been copied.
For example, ``d3d9.dll`` doesn't work on games run on Direct3D 8. Follow instruction described in :doc:`install` carefully.


Pixels are not smooth in windowed mode when the resolution is 960×720 or 1280×960
=================================================================================================

Make sure that ``d3d8.dll`` or ``d3d9.dll`` has been copied to the appropriate location.
If it doesn't work correctly even after copy, make sure that the appropriate file has been copied.
For example, ``d3d9.dll`` doesn't work on games run on Direct3D 8. Follow instruction described in :doc:`install` carefully.


HUD elements are not rearranged for vertically-long screen
=================================================================================================

Make sure that config file correctly corresponds to the product.
Also, make sure that the destination is correct.
Since Double Spoiler (Th125), destination directory is confusing.
Follow instruction described in :doc:`install` carefully.


Scores, bombs and other HUD elements are not shown
=================================================================================================

Make sure that config file correctly corresponds to the product.
Also, make sure that the destination is correct.
Since Double Spoiler (Th125), destination directory is confusing.
Follow instruction described in :doc:`install` carefully.


Misalignment in Touhou 9.5, 12.5, 12.8, and 14.3
=================================================================================================

Make sure that config file correctly corresponds to the product.
Also, make sure that the destination is correct.
Since Double Spoiler (Th125), destination directory is confusing.
Follow instruction described in :doc:`install` carefully.


Player's inputs are not accepted or the game terminates after customization window is opened
=================================================================================================

Try the following steps:

1. Open configuration file with a text editor
2. Replace the line of ``"use_modal_editor": false`` with ``"use_modal_editor": true``.
3. Re-launch the game

Although these steps force the game to suspend while customization window is open,
they provide a workaround for this issue.
