# Embeddable Font Generator

The goal of this project is to create bitmap definitions of (monospaced)
characters using a regular TrueType font file. The resulting data file
can be used in embedded software to draw text on LCD oder e-paper
displays.

The converter uses the FreeType2 library to render the bitmaps of the
glyphs. Static plugins of so called "renderer" and "writer" provides
additional steps to get glyph bitmaps rendered into equal sized
bitmap matrices. The writeres stores the rendered matrices into files.

## The Plugins

The current implementations allows to add linked (static) source modules
into the code to add additional renderer and writer with ease. There
are no plans to implement loadable plugins.

### Renderer

...

### Writer

...


## Tipps

The FreeType library doesn't allow to specify the final bounding box of
the resulting in pixel. Instead the size in *points* is used. This size
can be scaled by using the *DPI* settings.

To create a font with a specific pixel size, use the parameter `--check` and
try to get a result by variing *size* which is near by the size you need.
After that, use `--dpi 72` and `--hdpi` to get the aspect ratio you need.


## License

This project is licend under the GPL V3.
