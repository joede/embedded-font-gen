/* -*- Mode: C -*-
 * --------------------------------------------------------------------------
 * Copyright  (c) Joerg Desch <github.de>
 * --------------------------------------------------------------------------
 * PROJECT: FONT Generator
 * MODULE.: renderer-2pix.c
 * AUTHOR.: Joerg Desch
 * CREATED: 09.05.2016 10:41:02 CEST
 * --------------------------------------------------------------------------
 * DESCRIPTION:
 *
 * --------------------------------------------------------------------------
 * COMPILER-FLAGS:
 *
 * --------------------------------------------------------------------------
 */

#define __RENDERER_2BIT_C__
#include "fontgen.h"
#include "renderer-2bit.h"


/*+=========================================================================+*/
/*|                      CONSTANT AND MACRO DEFINITIONS                     |*/
/*`========================================================================='*/
//{{{

#define MODULE_NAME "2bit"


//}}}

/*             .-----------------------------------------------.             */
/* ___________/  local macro declaration                        \___________ */
/*            `-------------------------------------------------'            */
//{{{
//}}}

/*+=========================================================================+*/
/*|                          LOCAL TYPEDECLARATIONS                         |*/
/*`========================================================================='*/
//{{{
//}}}

/*+=========================================================================+*/
/*|                            PUBLIC VARIABLES                             |*/
/*`========================================================================='*/
//{{{
//}}}

/*+=========================================================================+*/
/*|                             LOCAL VARIABLES                             |*/
/*`========================================================================='*/
//{{{

static t_renderer_plugin this_plugin;

//}}}

/*+=========================================================================+*/
/*|                      PROTOTYPES OF LOCAL FUNCTIONS                      |*/
/*`========================================================================='*/
//{{{

static bool init_font_definition ( t_font_definition *fnt, const t_font_metrics *metrics, int from, int to );
static bool generate ( t_font_definition *fnt, const t_glyph_matrix *gmatrices );
static bool done ( t_font_definition *fnt );

//}}}

/*+=========================================================================+*/
/*|                     IMPLEMENTATION OF THE FUNCTIONS                     |*/
/*`========================================================================='*/
//{{{

const t_renderer_plugin* renderer_2bit_creator ( void )
{
    if ( is_verbose() )
    {
	fprintf(stderr,"[%s] renderer_2bit_creator: create renderer.\n",MODULE_NAME);
    }
    this_plugin.done = done;
    this_plugin.generate = generate;
    this_plugin.init = init_font_definition;
    return &this_plugin;
}

//}}}

/*             .-----------------------------------------------.             */
/* ___________/  Group...                                       \___________ */
/*            `-------------------------------------------------'            */
//{{{
//}}}

/*+=========================================================================+*/
/*|                    IMPLEMENTATION OF LOCAL FUNCTIONS                    |*/
/*`========================================================================='*/
//{{{

/* Initialize the passed font definition \c fnt for the implemented renderer.
 * The metrics of the generated glyphs is passed in \c metrics. Here we have to
 * prepare the rendering of the glyph bitmaps into the final matrix bitmaps.
 */
static bool init_font_definition ( t_font_definition *fnt, const t_font_metrics *metrics, int from, int to )
{
    int sz;

    if ( !fnt || !metrics || from<0 || to<0 )
    {
	fprintf(stderr,"[%s] error: init_font_definition: illegal parameters\n",MODULE_NAME);
	return false;
    }
    if ( is_verbose() )
	fprintf(stderr,"[%s] init_font_definition: called\n",MODULE_NAME);

    /* Remember the font metrics and save the range as start character and
     * number of characters.
     */
    fnt->metrics = metrics;
    fnt->first = from;
    fnt->num = to-from+1;
    if ( fnt->num <= 0 )
    {
	fprintf(stderr,"[%s] error: init_font_definition: invalid number of entries (%d)\n",MODULE_NAME,fnt->num);
	return false;
    }

    /* fill the renderer related values
     */
    strncpy(fnt->renderer,MODULE_NAME,MAXNAME); fnt->renderer[MAXNAME]='\0';
    fnt->matrix_width = 2*(metrics->matrix.width);
    fnt->matrix_height = metrics->matrix.height;

    /* calculate the number of bytes used to store a row inside the buffer.
     */
    fnt->matrix_pitch = fnt->matrix_width/8;
    if ( fnt->matrix_width%8 )
	fnt->matrix_pitch++;
    fnt->matrix_size = fnt->matrix_pitch * fnt->matrix_height;

    // allocate buffer for all data bytes of the final matrices
    sz = (fnt->matrix_size)*(fnt->num)*sizeof(uint8_t);
    fnt->buffer = malloc(sz);
    if ( !fnt->buffer )
    {
	fprintf(stderr,"[%s] error: init_font_definition: buffer allocation failed (%d)\n",MODULE_NAME,sz);
	return false;
    }
    memset(fnt->buffer,0,sz);
#ifdef DEBUG
    fprintf(stderr,"[%s] init_font_definition: width %d needs %d bytes. buffer-size=%d num=%d alloc=%d\n",MODULE_NAME,
	    metrics->matrix.width,fnt->matrix_pitch,fnt->matrix_size,fnt->num,sz);
#endif
    return true;
}

/*
 *
 * NOTE: an empty glyph matrix leads to a size (\c sz_buffer in \c gmatrices)
 * of 0 and a buffer pointer (\c buffer in \c gmatrices) of NULL!
 *
 * @param fnt
 * @param gmatrices the array with all glyph data and the (reduced) glyph matrix
 */
static bool generate ( t_font_definition *fnt, const t_glyph_matrix *gmatrices )
{
    int offs;				// offset into fnt->buffer
    int idx;				// index into gmatrices[]
    uint8_t bit;			// bitmask
    int byte;				// byte within row
    uint8_t mbit;			// bitmask for matrix
    int mbyte;				// byte within matrix row
    int gx, gy;				// pixel coordinates inside the glyph
    int mx, my;				// pixel coordinates inside the output matrix

    if ( !fnt || !gmatrices )
    {
	fprintf(stderr,"[%s] error: generate: illegal parameters\n",MODULE_NAME);
	return false;
    }
    if ( is_verbose() )
	fprintf(stderr,"[%s] generate: called\n",MODULE_NAME);

    for ( idx=0; idx<(fnt->num-1); idx++ )
    {
	if ( gmatrices[idx].buffer )
	{
	    // render gmatrices[idx] into fnt->buffer[offs]
	    mx = gmatrices[idx].offset_x;
	    my = gmatrices[idx].offset_y;
#ifdef DEBUG
	    fprintf(stderr,"[%s] generate: #%d offs=%d/%d glyph=%d/%d base=%d\n",MODULE_NAME,
	                   idx,mx,my,
			   gmatrices[idx].width,gmatrices[idx].height,
			   fnt->metrics->baseline);
#endif
	    for ( gy=0; gy<gmatrices[idx].height; gy++ )
	    {
		for ( gx=0; gx<gmatrices[idx].width; gx++ )
		{
		    byte = gx / 8;
		    bit = 0x80 >> (gx&7);
#ifdef DEBUG_OFF
		    if ( (gmatrices[idx].buffer[gy*gmatrices[idx].pitch+byte]&bit) )
			printf("*");
		    else
			printf(".");
#endif
		    if ( (gmatrices[idx].buffer[gy*gmatrices[idx].pitch+byte]&bit) )
		    {
			// set the pixel[mx+gx|my+gy] inside the matrix and leave
			// pixel[mx+gx+1|my+gy] untouched.
			mbyte = ((mx+gx)*2) / 8;
			mbit = 0x80 >> (((mx+gx)*2)&7);
			offs = ((fnt->matrix_pitch)*(my+gy)+mbyte);
			if ( offs >= fnt->matrix_size )
			{
#ifdef DEBUG
			    fprintf(stderr,"[%s] fatal: generate: #%d access out of buffer for pixel[%d|%d] (absolute %d|%d)! (%d > %d)\n",
			            MODULE_NAME,idx,gx,gy,(mx+gx)*2,(my+gy),offs,fnt->matrix_size);
#else
			    fprintf(stderr,"[%s] fatal: generate: #%d access out of buffer for pixel[%d|%d]!\n",
			            MODULE_NAME,idx,gx,gy);
#endif
			    return false;
			}
			offs += (fnt->matrix_size) * idx;
			fnt->buffer[offs] |= mbit;
		    }
		}
#ifdef DEBUG_OFF
		printf("\n");
#endif
	    }
#ifdef DEBUG_OFF
	    for ( my=0; my<(fnt->metrics->matrix.height); my++ )
	    {
		for ( mx=0; mx<(fnt->metrics->matrix.width); mx++ )
		{
		    offs = (fnt->matrix_size) * idx;
		    mbyte = mx / 8;
		    mbit = 0x80 >> (mx&7);
		    offs += (mpitch*(my)+mbyte);
		    if ( fnt->buffer[offs]&mbit )
			printf("*");
		    else
			printf(".");
		}
		printf("\n");
	    }
#endif
	}
#ifdef DEBUG
	else
	    fprintf(stderr,"[%s] generate: #%d has no matrix\n",MODULE_NAME,idx);
#endif
    }

    return true;
}


static bool done ( t_font_definition *fnt )
{
    if ( !fnt )
    {
	fprintf(stderr,"[%s] error: done: illegal parameters\n",MODULE_NAME);
	return false;
    }
    if ( is_verbose() )
	fprintf(stderr,"[%s] done: called\n",MODULE_NAME);

    if ( fnt->buffer )
    {
	free(fnt->buffer);
	fnt->buffer = NULL;
    }

    return true;
}



//}}}

/* ==[End of file]========================================================== */
