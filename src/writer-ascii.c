/* -*- Mode: C -*-
 * --------------------------------------------------------------------------
 * Copyright  (c) Völker Video- und Datentechnik GmbH
 * --------------------------------------------------------------------------
 * PROJECT: FONT Generator
 * MODULE.: writer-ascii.c
 * AUTHOR.: Dipl.-Ing. Joerg Desch
 * CREATED: 09.05.2016 11:18:02 CEST
 * --------------------------------------------------------------------------
 * DESCRIPTION:
 *
 * --------------------------------------------------------------------------
 * COMPILER-FLAGS:
 *
 * --------------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#define __WRITER_ASCII_C__
#include "fontgen.h"


/*+=========================================================================+*/
/*|                      CONSTANT AND MACRO DEFINITIONS                     |*/
/*`========================================================================='*/
//{{{

#define MODULE_NAME "ascii"

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

static t_writer_plugin this_plugin;

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

static char output_file[MAXPATH+1] = {""};
static FILE *output = NULL;


//}}}

/*+=========================================================================+*/
/*|                      PROTOTYPES OF LOCAL FUNCTIONS                      |*/
/*`========================================================================='*/
//{{{

static bool init ( t_font_definition *fnt, const char *filename );
static bool create ( t_font_definition *fnt );
static bool done ( t_font_definition *fnt );


//}}}

/*+=========================================================================+*/
/*|                     IMPLEMENTATION OF THE FUNCTIONS                     |*/
/*`========================================================================='*/
//{{{

const t_writer_plugin* writer_ascii_creator ( void )
{
    if ( is_verbose() )
    {
	fprintf(stderr,"[%s] writer_ascii_creator: create writer.\n",MODULE_NAME);
    }
    this_plugin.done = done;
    this_plugin.create = create;
    this_plugin.init = init;
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

static bool init ( t_font_definition *fnt, const char *filename )
{
    if ( !fnt || !filename )
    {
	fprintf(stderr,"[%s] error: init: illegal parameters\n",MODULE_NAME);
	return false;
    }
#ifdef DEBUG
    if ( !fnt->metrics )
    {
	fprintf(stderr,"[%s] fatal: init: bad metrics pointer\n",MODULE_NAME);
	return false;
    }
#endif
    if ( is_verbose() )
	fprintf(stderr,"[%s] init: called\n",MODULE_NAME);

    if ( filename[0] == '\0')
    {
	if ( is_verbose() )
	    fprintf(stderr,"[%s] init: creating output filename...\n",MODULE_NAME);
	snprintf(output_file,MAXPATH,"%s_%d_%dx%d.txt",
		 fnt->metrics->name,
		 fnt->metrics->pt_size,
		 fnt->metrics->matrix.width,fnt->metrics->matrix.height);
    }
    else
    {
	snprintf(output_file,MAXPATH,"%s.txt",filename);
    }
    output_file[MAXPATH] = '\0';
    if ( is_verbose() )
	fprintf(stderr,"[%s] init: use output filename '%s'\n",MODULE_NAME,output_file);
    output = fopen(output_file,"w");
    if ( !output )
    {
	fprintf(stderr,"[%s] error: init: unable to create output file '%s'\n",MODULE_NAME,output_file);
	return false;
    }
    fprintf(output,"------------------------------------------------------\n");
    fprintf(output,"FONT:\n");
    fprintf(output,"  name:         '%s'\n",fnt->metrics->name);
    fprintf(output,"  range:        #%d..#%d\n",fnt->first,fnt->first+fnt->num-1);
    fprintf(output,"  size:         %d pt\n",fnt->metrics->pt_size);
    if ( fnt->metrics->hdpi )
	fprintf(output,"  dpi:          %d x %d\n",fnt->metrics->hdpi,fnt->metrics->dpi);
    else
	fprintf(output,"  dpi:          %d\n",fnt->metrics->dpi);
    fprintf(output,"  matrix:       %d x %d\n",fnt->matrix_width,fnt->matrix_height);
    fprintf(output,"  renderer:     '%s'\n",fnt->renderer);
    fprintf(output,"GLYPH:\n");
    fprintf(output,"  glyph-matrix: %d x %d\n",fnt->metrics->matrix.width,fnt->metrics->matrix.height);
    fprintf(output,"  nl-height:    %d\n",fnt->metrics->absolute_height);
    fprintf(output,"  max-ascent:   %d\n",fnt->metrics->max_ascent);
    fprintf(output,"  max-descent:  %d\n",fnt->metrics->max_descent);
    fprintf(output,"  baseline:     %d\n",fnt->metrics->baseline);
    fprintf(output,"  em:           %d x %d\n",fnt->metrics->em.width,fnt->metrics->em.height);
    fprintf(output,"  ex:           %d x %d\n",fnt->metrics->ex.width,fnt->metrics->ex.height);
    fprintf(output,"  we:           %d x %d\n",fnt->metrics->we.width,fnt->metrics->we.height);
    fprintf(output,"------------------------------------------------------\n\n");
    return true;
}

static bool create ( t_font_definition *fnt )
{
    int offs;			// offset info buffer of matrix data
    int mpitch;			// number of bytes per matrix row
    uint8_t mbit;		// bitmask for matrix
    int mbyte;			// byte within matrix row
    int mx, my;			// pixel coordinates inside the output matrix
    int idx;			// index into gmatrices[]

    if ( !fnt )
    {
	fprintf(stderr,"[%s] error: create: illegal parameters\n",MODULE_NAME);
	return false;
    }
    if ( is_verbose() )
	fprintf(stderr,"[%s] create: called\n",MODULE_NAME);

#ifdef DEBUG_oof
    /* calculate the number of bytes used to store a row inside the buffer.
     * We assume 1bit per pixel here! Goal is to check the previously calculated
     * value of fnt->matrix_pitch.
     */
    mpitch = (fnt->metrics->matrix.width)/8;
    if ( (fnt->metrics->matrix.width)%8 )
	mpitch++;
    if ( mpitch != fnt->matrix_pitch )
    {
	fprintf(stderr,"[%s] fatal: create: invalid matrix_pitch\n",MODULE_NAME);
	return false;
    }
#endif
    mpitch = fnt->matrix_pitch;

    /* Walk through the generated character definitions
     */
    for ( idx=0; idx<(fnt->num-1); idx++ )
    {
	fprintf(output,"----- #%d ------------------ \n",fnt->first+idx);

	/* And dump the matrix of each character. The baseline is marked too.
	 */
	for ( my=0; my<(fnt->matrix_height); my++ )
	{
	    for ( mx=0; mx<(fnt->matrix_width); mx++ )
	    {
		offs = (fnt->matrix_size) * idx;
		mbyte = mx / 8;
		mbit = 0x80 >> (mx&7);
		offs += (mpitch*(my)+mbyte);
		if ( fnt->buffer[offs]&mbit )
		{
		    // most important part... a bit is set!
		    fprintf(output,"*");
		}
		else
		    fprintf(output,".");
	    }
	    if ( my == (fnt->metrics->matrix.height + fnt->metrics->baseline -1) )
	    {
		fprintf(output,"    ____");
	    }
	    fprintf(output,"\n");
	}
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
    if ( output )
    {
	fclose(output);
	output = NULL;
    }
    return true;
}


//}}}

/* ==[End of file]========================================================== */
