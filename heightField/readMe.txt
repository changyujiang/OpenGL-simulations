CSCI420 17FALL ASSIGNMENT1

AUTHOR:CHANGYU JIANG
USC#:3277031698

BASIC FEATURES:
    ./assign1 spiral.jpg
- Render as POINTS
    Control: keys 'a'/'A' or menu;
- Render as LINES("wireframe"), 
    Control: keys 's'/'S' or menu; 
- Rneder as SOLID TRIANGLES, 
    Control: keys 'd'/'D' or menu;
- Window size: 640 * 480
- Perspective view.
- Utilize GL's depth buffer for hidden surface removal.
- Use input from the mouse to spin the heightfield around using glRotate.
- Use input from the mouse to move the heightfield around using glTranslate.
- Use input from the mouse to change the dimensions of the heightfield using glScale.
- Color the vertices using some smooth gradient.
- Be reasonably commented and written in an understandable manner--we will read your code.
- Be submitted along with JPEG frames for the required animation (see below).
- Be submitted along with a readme file documenting your program's features, describing any extra credit you have done, and anything else that you may want to bring to our attention.

EXTRAS:
- Experiment with material and lighting properties
    Input: ./assign1 spiral.jpg
    Control: keys 'q'/'Q' or menu
- Support color (bpp=3) in input images.
    Input: ./assign1 demo1.jpg
    Control: keys 'w'/'W' or menu
- Render wireframe on top of solid triangles (use glPolygonOffset to avoid z-buffer fighting).
    Input: ./assign1 demo1.jpg
    Control: keys 'e'/'E' or menu
- Color the vertices based on color values taken from another image of equal size. However, your code should still support also smooth color gradients as per the core requirements.
    Input: ./assign1 demo.jpg demo1.jpg
    Control: keys 'r'/'R' or menu
- Texturemap the surface with an arbitrary image.
    Input: ./assign1 demo.jpg demo2.jpg
    Control: keys 't'/'T' or menu