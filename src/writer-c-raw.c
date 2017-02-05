/* -*- Mode: C -*-
 * --------------------------------------------------------------------------
 * Copyright  (c) Joerg Desch <github.de>
 * --------------------------------------------------------------------------
 * PROJECT: FONT Generator
 * MODULE.: writer-c-raw.c
 * AUTHOR.: Joerg Desch
 * CREATED: 09.05.2016 11:18:02 CEST
 * --------------------------------------------------------------------------
 * DESCRIPTION:
 * This "writer" crerates a C header file with all information needed. The
 * matrix data is written into a C file. The format is designed to be
 * included into another C file directly into a array declaration.
 *
 * I've choose this way to be able to use special classifiers like PROGMEM
 * for embedded AVR systems.
 *
 * Sample:
 *
 *  ,--------------------------
 *  |#include <stdio.h>
 *  |#include "Inconsolata_Regular_17_9x18.h"
 *  |static const uint8_t my_font_data[FONT_BUFFER_SIZE] = {
 *  |  #include "Inconsolata_Regular_17_9x18.c"
 *  |};
 *  `--------------------------
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

#define __WRITER_C_RAW_C__
#include "fontgen.h"


/*+=========================================================================+*/
/*|                      CONSTANT AND MACRO DEFINITIONS                     |*/
/*`========================================================================='*/
//{{{

#define MODULE_NAME "c-raw"

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

static char define_name[MAXPATH+1] = {""};
static char output_file[MAXPATH+1] = {""};
static FILE *output = NULL;


//}}}

/*+=========================================================================+*/
/*|                      PROTOTYPES OF LOCAL FUNCTIONS                      |*/
/*`========================================================================='*/
//{{{

/* the API
 */
static bool init ( t_font_definition *fnt, const char *filename );
static bool create ( t_font_definition *fnt );
static bool done ( t_font_definition *fnt );

/* local helpers
 */
static void create_output_filename ( t_font_definition *fnt , const char *filename, const char* extension );
static bool write_header_file ( t_font_definition *fnt );
static bool write_file_head ( t_font_definition *fnt );


//}}}

/*+=========================================================================+*/
/*|                     IMPLEMENTATION OF THE FUNCTIONS                     |*/
/*`========================================================================='*/
//{{{

const t_writer_plugin* writer_c_raw_creator ( void )
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

    create_output_filename(fnt,filename,"h");
    output = fopen(output_file,"w");
    if ( !output )
    {
	fprintf(stderr,"[%s] error: init: unable to create output file '%s'\n",MODULE_NAME,output_file);
	return false;
    }
    write_header_file(fnt);

    create_output_filename(fnt,filename,"c");
    output = fopen(output_file,"w");
    if ( !output )
    {
	fprintf(stderr,"[%s] error: init: unable to create output file '%s'\n",MODULE_NAME,output_file);
	return false;
    }
    write_file_head(fnt);

    return true;
}


static void create_output_filename ( t_font_definition *fnt , const char *filename, const char* extension )
{
    int i;
    if ( filename[0] == '\0')
    {
	if ( is_verbose() )
	    fprintf(stderr,"[%s] init: creating output filename...\n",MODULE_NAME);
	snprintf(define_name,MAXPATH,"%s_%d_%dx%d",
		 fnt->metrics->name,
		 fnt->metrics->pt_size,
		 fnt->metrics->matrix.width,fnt->metrics->matrix.height);
	snprintf(output_file,MAXPATH,"%s_%d_%dx%d.%s",
		 fnt->metrics->name,
		 fnt->metrics->pt_size,
		 fnt->metrics->matrix.width,fnt->metrics->matrix.height,
		 extension);
    }
    else
    {
	strncpy(define_name,filename,MAXPATH);
	snprintf(output_file,MAXPATH,"%s.%s",filename,extension);
    }
    define_name[MAXPATH] = '\0';
    output_file[MAXPATH] = '\0';
    i=0;
    while ( define_name[i] )
    {
	if ( define_name[i]==' ' )
	    define_name[i]='_';
	else if ( define_name[i]=='-' )
	    define_name[i]='_';
	else if ( define_name[i]>='a' && define_name[i]<='z' )
	     define_name[i] = (int)define_name[i] + (int)'A' - (int)'a';
	else if ( (define_name[i]<'0' || define_name[i]>'9') && (define_name[i]<'A' || define_name[i]>'Z') )
	    define_name[i]='_';
	i++;
    }
    if ( is_verbose() )
    {
	fprintf(stderr,"[%s] init: use output filename '%s'\n",MODULE_NAME,output_file);
#ifdef DEBUG
	fprintf(stderr,"[%s] init: use define '%s'\n",MODULE_NAME,define_name);
#endif
    }
}


static bool write_header_file ( t_font_definition *fnt )
{
    int rc;
    int sz;

    sz = (fnt->matrix_size)*(fnt->num)*sizeof(uint8_t);
    rc = fprintf(output,"#ifndef __%s_H__\n",define_name);
    if ( rc < 0 )
	return false;
    fprintf(output,"#define __%s_H__ 1\n",define_name);
    fprintf(output,"/* ------------------------------------------------------\n");
    fprintf(output," * FONT:\n");
    fprintf(output," *   name:         '%s'\n",fnt->metrics->name);
    fprintf(output," *   range:        #%d..#%d\n",fnt->first,fnt->first+fnt->num-1);
    fprintf(output," *   size:         %d pt\n",fnt->metrics->pt_size);
    if ( fnt->metrics->hdpi )
	fprintf(output," *   dpi:          %d x %d\n",fnt->metrics->hdpi,fnt->metrics->dpi);
    else
	fprintf(output," *   dpi:          %d\n",fnt->metrics->dpi);
    fprintf(output," *   matrix:       %d x %d\n",fnt->matrix_width,fnt->matrix_height);
    fprintf(output," *   renderer:     '%s'\n",fnt->renderer);
    fprintf(output," * GLYPH:\n");
    fprintf(output," *   glyph-matrix: %d x %d\n",fnt->metrics->matrix.width,fnt->metrics->matrix.height);
    fprintf(output," *   nl-height:    %d\n",fnt->metrics->absolute_height);
    fprintf(output," *   max-ascent:   %d\n",fnt->metrics->max_ascent);
    fprintf(output," *   max-descent:  %d\n",fnt->metrics->max_descent);
    fprintf(output," *   baseline:     %d\n",fnt->metrics->baseline);
    fprintf(output," *   em:           %d x %d\n",fnt->metrics->em.width,fnt->metrics->em.height);
    fprintf(output," *   ex:           %d x %d\n",fnt->metrics->ex.width,fnt->metrics->ex.height);
    fprintf(output," *   we:           %d x %d\n",fnt->metrics->we.width,fnt->metrics->we.height);
    fprintf(output," * ------------------------------------------------------\n */\n\n");

    fprintf(output,"#define FONT_NAME           \"%s\"\n",fnt->metrics->name);
    fprintf(output,"#define FONT_START_WITH     %d\n",fnt->first);
    fprintf(output,"#define FONT_NUM_CHARS      %d\n",fnt->first+fnt->num-1);
    fprintf(output,"#define FONT_GLYPH_WIDTH    %d\n",fnt->metrics->matrix.width);
    fprintf(output,"#define FONT_GLYPH_HEIGHT   %d\n",fnt->metrics->matrix.height);
    fprintf(output,"#define FONT_MATRIX_WIDTH   %d\n",fnt->matrix_width);
    fprintf(output,"#define FONT_MATRIX_HEIGHT  %d\n",fnt->matrix_height);
    fprintf(output,"#define FONT_BUFFER_SIZE    %d\n",sz);

    fprintf(output,"\n\n#endif // __%s_H__\n",define_name);
    return true;
}

static bool write_file_head ( t_font_definition *fnt )
{
    int rc;
    rc = fprintf(output,"#define __%s_C__ 1\n",define_name);
    if ( rc < 0 )
	return false;
    fprintf(output,"/* ------------------------------------------------------\n");
    fprintf(output," * FONT:\n");
    fprintf(output," *   name:         '%s'\n",fnt->metrics->name);
    fprintf(output," *   range:        #%d..#%d\n",fnt->first,fnt->first+fnt->num-1);
    fprintf(output," *   size:         %d pt\n",fnt->metrics->pt_size);
    if ( fnt->metrics->hdpi )
	fprintf(output," *   dpi:          %d x %d\n",fnt->metrics->hdpi,fnt->metrics->dpi);
    else
	fprintf(output," *   dpi:          %d\n",fnt->metrics->dpi);
    fprintf(output," *   matrix:       %d x %d\n",fnt->matrix_width,fnt->matrix_height);
    fprintf(output," *   renderer:     '%s'\n",fnt->renderer);
    fprintf(output," * GLYPH:\n");
    fprintf(output," *   glyph-matrix: %d x %d\n",fnt->metrics->matrix.width,fnt->metrics->matrix.height);
    fprintf(output," *   nl-height:    %d\n",fnt->metrics->absolute_height);
    fprintf(output," *   max-ascent:   %d\n",fnt->metrics->max_ascent);
    fprintf(output," *   max-descent:  %d\n",fnt->metrics->max_descent);
    fprintf(output," *   baseline:     %d\n",fnt->metrics->baseline);
    fprintf(output," *   em:           %d x %d\n",fnt->metrics->em.width,fnt->metrics->em.height);
    fprintf(output," *   ex:           %d x %d\n",fnt->metrics->ex.width,fnt->metrics->ex.height);
    fprintf(output," *   we:           %d x %d\n",fnt->metrics->we.width,fnt->metrics->we.height);
    fprintf(output," * ------------------------------------------------------\n */\n\n");

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
    int i;

    if ( !fnt )
    {
	fprintf(stderr,"[%s] error: create: illegal parameters\n",MODULE_NAME);
	return false;
    }
    if ( is_verbose() )
	fprintf(stderr,"[%s] create: called\n",MODULE_NAME);

    mpitch = fnt->matrix_pitch;

    // not for raw array data
    // fprintf(output,"static uint8 __font_data[FONT_BUFFER_SIZE] = {\n");

    /* Walk through the generated character definitions
     */
    for ( idx=0; idx<(fnt->num-1); idx++ )
    {
	fprintf(output,"// ----- #%d ------------------ \n",fnt->first+idx);
	/* And dump the matrix of each character. The baseline is marked too.
	 */
	for ( my=0; my<(fnt->matrix_height); my++ )
	{
	    offs = (fnt->matrix_size)*idx + mpitch*my;
	    for ( i=0; i<mpitch; i++ )
	    {
		if ( (i==mpitch-1) && (my==fnt->matrix_height-1) && (idx==fnt->num-2) )
		    fprintf(output,"0x%2.2X",fnt->buffer[offs+i]);	// the last byte...
		else
		    fprintf(output,"0x%2.2X,",fnt->buffer[offs+i]);
	    }
	    fprintf(output,"     // ");
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
		fprintf(output,"  __");
	    }
	    fprintf(output,"\n");
	}
    }

    // fprintf(output,"};\n");
    fprintf(output,"\n\n\n// ==================== end of file ====================\n");
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
