CSCI420 17FALL ASSIGNMENT2

AUTHOR:CHANGYU JIANG
USC#:3277031698

BASIC FEATURES:
- Window size: 640 * 480;
- Perspective view;
- Complete level 1-5;
- Properly rendered Catmull-Rom splines;
- Enclose the entire scene into a cube, and then texture-map the faces of this cube with some sky-like texture;
- Render a rail cross-section;
- Move the camera at a reasonable speed in a continuous path and orientation along the coaster.
rendering the sky and determining coaster normals in particular

EXTRAS:
- Drawn splines using recursive subdivision (vary step size to draw short lines) instead of using brute force (vary u with fixed step size).
- Solved the visible seams of sky box;
- Instead of naive one, determined up vectors using Sloan's method (decides each coordinate system using a function of the previous one, to ensure continuity);
- Rendered double rail (like in real railroad tracks);
- Drawn texture-mapped wooden crossbars;
- Generated track from several different sequences of splines (multiple track files), so you can shuffle these around and create a random track;
- Made track circular and close it with C1 continuity (small credit, i.e., 1 point).