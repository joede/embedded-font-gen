/* -*- Mode: C -*-
 * --------------------------------------------------------------------------
 * Copyright  (c) Joerg Desch <github.de>
 * --------------------------------------------------------------------------
 * PROJECT: FONT Generator
 * MODULE.: fontgen.h
 * AUTHOR.: Joerg Desch
 * CREATED: 09.05.2016 10:32:44 CEST
 * --------------------------------------------------------------------------
 * DESCRIPTION:
 *
 *
 *
 * --------------------------------------------------------------------------
 * COMPILER-FLAGS:
 *
 *
 * --------------------------------------------------------------------------
 */

#ifndef __FONTGEN_H__
#define __FONTGEN_H__ 1

#include <stdint.h>
#include <stdbool.h>
#include <ft2build.h>
#include FT_GLYPH_H


/*+=========================================================================+*/
/*|                      CONSTANT AND MACRO DEFINITIONS                     |*/
/*`========================================================================='*/
//{{{

/* FreeType uses 72dpi as the base to calculate pixels from points. Since we
 * want to have pixels for our generated fonts, we use this DPI value.
 * pixel = (point*dpi)/72
 */
#define DEFAULT_DPI 72

/* Max. length of a path to an file
 */
#define MAXPATH 256

/* max. length of names (fonts, renderer, writer, ...)
 */
#define MAXNAME 80


//}}}

/*             .-----------------------------------------------.             */
/* ___________/  local macro declaration                        \___________ */
/*            `-------------------------------------------------'            */
//{{{
//}}}

/*+=========================================================================+*/
/*|                            TYPEDECLARATIONS                             |*/
/*`========================================================================='*/
//{{{


typedef struct tagPIXEL_SIZE
{
    int width;
    int height;
} t_pixel_size;

/* Every thing we need to know about the font in general.
 *
 *     matrix.width
 *    |<--------->|<--------->|<--------->|
 *    |___________|___________|___________|_______________________________
 *    |           |    **     |           |  ^                 ^
 *    |  *      * |    **     |           |  |                 |
 *    |  *      * |           |           |  |                 |
 *    |  *      * |           |           |  |                 |
 *    |  *      * |   ***     |  *** ***  |  |matrix.height    |max_ascend
 *    |  *      * |     *     | *   *     |  |                 |
 *    |  ******** |     *     |*     *    |  |                 |
 *    |  *      * |     *     |*     *    |  |                 |
 *    |  *      * |     *     | *   *     |  |                 |
 *    |  *      * |     *     | ****      |  |                 |
 *    |  *      * |     *     |*          |  |                 |
 *    |  *      * |     *     |*          |  |                 |
 *    |_ *_____ *_|___*****___| ****** ___|__|_________________v__________ origin (0)
 *    |           |           |*      *   |  |   ^             ^
 *    |           |           |*      *   |  |   |baseline     |max_descend
 *    |           |           |*     *    |  |   | (<=0!)      |
 *    |___________|___________|_*****_____|__v___|_____________v__________
 *    |           |           |           |
 *
 */
typedef struct tagFONT_METRICS
{
    char name[MAXNAME+1];	// guess what, the name of the font based on the filename
    int pt_size;		// the passed size in points.
    int dpi;			// selected DPI. If hdpi is 0, this is used for both, H and V!
    int hdpi;			// optional horizontal DPI
    t_pixel_size matrix;	// final width+height of the matrix
    int absolute_height;	// absolute height of the font (with line spacing)
    int max_ascent;		// absolute value of max height above the baseline for all glyph bitmaps
    int max_descent;		// absolute value of max height below the baseline for all glyph bitmaps
    int baseline;		// active baseline
    int calculated_baseline;	// calculated y value for the baseline / origin line
    int detected_baseline;	// detected y value for the baseline / origin line
    t_pixel_size em;		// glyph size of "M"
    t_pixel_size ex;		// glyph size of "x"
    t_pixel_size we;		// glyph size of "W"
} t_font_metrics;

/* The definition of one single glyph. The resulting "glyph bitmap" can be smaller
 * than the bitmap of the final (general) matrix of the font. The offsets
 * \c width and \c height shows the position of the glyph bitmap within the
 * matrix bitmap.
 *
 *     offset_x
 *    |<->|
 *    |___|___________|______
 *    |               | ^
 *    |               | | offset_y
 *    |               | |
 *    |               |_v______________
 *    |   ..***.***   |              ^
 *    |   .*...*...   |              |
 *    |   *.....*..   |              |
 *    |   *.....*..   |              |
 *    |   .*...*...   |              | height
 *    |   .****....   |              |
 *    |   *........   |              |
 *    |   *........   |              |
 *   _|   .******..   |__(origin)    |
 *    |   *......*.   |              |
 *    |   *......*.   |              |
 *    |   *.....*..   |              |
 *    |   .*****...   |______________v_
 *    |_______________|
 *        |       |
 *          width
 */
typedef struct tagGLYPH_MATRIX
{
    int width;			// width of the glyph bitmap
    int height;			// height of the glyph bitmap
    int pitch;			// no of bytes per bitmap line
    int advance;		// real width for proportional fonts (for positioning)
    int offset_x;		// x offset into char matrix
    int offset_y;		// y offset into char matrix
    int sz_buffer;		// size of the buffer
    uint8_t *buffer;		// pointer to an allocated buffer
} t_glyph_matrix;


/* Font definition created by the renderer and used by the writer. The renderer
 * related (created) values are preficed with "matrix_".
 */
typedef struct tagFONT_DEFINITION
{
    char renderer[MAXNAME+1];	// guess what, the name of the renderer
    int first;			// first character code used
    int num;			// number of character codes following
    int matrix_width;		// renderer related width of the output matrix
    int matrix_height;		// renderer related height of the output matrix
    int matrix_pitch;		// no of bytes per bitmap line of the matrix
    int matrix_size;		// size of the buffer for one single "final matrix"
    uint8_t *buffer;		// buffer for num*matrix_size bytes
    const t_font_metrics *metrics;
} t_font_definition;


/* Interface to "Renderer" plugin -- a renderer plugin is used to complete the
 * process of the font creation. The previous step created an array of glyph
 * bitmaps, which all have it's own sizes. The goal of an renderer is to use the
 * detected baseline and the meassured bounding box (which is sized to include
 * all glyphs) and render a final bitmap matrix for all characters.
 *
 * The rendering process can include special requirements like "use 2 bits for
 * each pixel".
 *
 *   * init:     initialize the rendering process. Fills the passed font
 *               definition structure. Therefor it calculates the size of the
 *               resulting matrix based on the meassured bounding box. It
 *               allocates the buffer to hold all the generated bitmaps.
 *   * generate: create the matrices for all characters.
 *   * done:     cleanup after the rendering proccess has finished.
 */

typedef bool (*t_renderer_init_font_definition) ( t_font_definition *fnt, const t_font_metrics *metrics, int from, int to );
typedef bool (*t_renderer_generate) ( t_font_definition *fnt, const t_glyph_matrix *gmatrices );
typedef bool (*t_renderer_done) ( t_font_definition *fnt );

typedef struct tagRENDERER_PLUGIN
{
    t_renderer_init_font_definition init;	// init structure and allocate buffer
    t_renderer_generate generate;		// render all glyphs into bit matrixes
    t_renderer_done done;			// cleanup
} t_renderer_plugin;

typedef const t_renderer_plugin* (*t_renderer_creator) ( void );


/* Interface to "Writer" plugin -- a writer plugin is used to write / store the
 * generated maxtrix into a data file. This matrix data is created by the previous
 * call to the generate function of an selected renderer.
 *
 *   * init:   initialize the writing process. Generate filesnames and open
 *             main data file here. Additional files with metadata can be
 *             created here too. init is called after the rederer/generate
 *             has finished successfully.
 *   * create: create the content of the data file.
 *   * done:   cleanup after all data is written.
 */

typedef bool (*t_writer_init) ( t_font_definition *fnt, const char *filename );
typedef bool (*t_writer_create) ( t_font_definition *fnt );
typedef bool (*t_writer_done) ( t_font_definition *fnt );

typedef struct tagWRITER_PLUGIN
{
    t_writer_init init;				// init file creation
    t_writer_create create;			// write all definitions
    t_writer_done done;				// cleanup
} t_writer_plugin;

typedef const t_writer_plugin* (*t_writer_creator) ( void );


//}}}

/*+=========================================================================+*/
/*|                            PUBLIC VARIABLES                             |*/
/*`========================================================================='*/
//{{{
//}}}

/*+=========================================================================+*/
/*|                     PROTOTYPES OF GLOBAL FUNCTIONS                      |*/
/*`========================================================================='*/
//{{{

/* exported from fontgen.c
 */
bool is_verbose (void);

//}}}

/*             .-----------------------------------------------.             */
/* ___________/  Group...                                       \___________ */
/*            `-------------------------------------------------'            */
//{{{
//}}}

#endif
/* ==[End of file]========================================================== */
