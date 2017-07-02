=========================
Troubleshoot
=========================

- Screen doesn't rotate by ``Alt+Left`` and ``Alt+Right`` keys.
- ``Show THRotator window`` or ``Open THRotator`` doesn't appears by right-clicking title bar.
- Pixels are not smooth in windowed mode when the resolution is 960×720 or 1280×960.

  - Make sure that ``d3d8.dll`` or ``d3d9.dll`` has been copied to the appropriate location.
    If it doesn't work correctly even after copy, make sure that the appropriate file has been copied.
    For example, ``d3d9.dll`` doesn't work on games run on Direct3D 8. Follow instruction described in :doc:`install` carefully.

- HUD elements are not rearranged for vertically-long screen even when they should be.
- Scores, bombs and other HUD elements are not shown.
- Misalignment in Shoot the Bullets (Th095), Double Spoiler (Th125), Fairy Wars (Th128), and Impossible Spell Card (Th143).

  - Make sure that config file correctly corresponds to the product.
    Also, make sure that the destination is correct.
    Since Double Spoiler (Th125), destination directory is confusing.
    Follow instruction described in :doc:`install` carefully.
