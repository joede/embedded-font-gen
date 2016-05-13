/* -*- Mode: C -*-
 * --------------------------------------------------------------------------
 * Copyright  (c) Völker Video- und Datentechnik GmbH
 * --------------------------------------------------------------------------
 * PROJECT: FONT Generator
 * MODULE.: fontgen.c
 * AUTHOR.: Dipl.-Ing. Joerg Desch
 * CREATED: 04.05.2016 09:34:50 CEST
 * --------------------------------------------------------------------------
 * DESCRIPTION:
 *
 * --------------------------------------------------------------------------
 * COMPILER-FLAGS:
 *
 * --------------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <ft2build.h>
#include FT_GLYPH_H

#define __FONTGEN_C__

#include "config.h"
#include "fontgen.h"

// the plugins
#include "renderer-1bit.h"
#include "renderer-2bit.h"
#include "writer-ascii.h"
#include "writer-c-raw.h"



/*+=========================================================================+*/
/*|                      CONSTANT AND MACRO DEFINITIONS                     |*/
/*`========================================================================='*/
//{{{
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

typedef struct tagRENDERER_LIST
{
    const char *name;			// name of the plugin to reference it in the CLI
    const t_renderer_creator factory;	// factory method
} t_renderer_list;

typedef struct tagWRITER_LIST
{
    const char *name;			// name of the plugin to reference it in the CLI
    const t_writer_creator factory;	// factory method
} t_writer_list;


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

static int dpi = DEFAULT_DPI;
static int hdpi = 0;			// 0 means "same as dpi"
static int size = 0;
static int forced_origin = 0;
static int font_from_char = 32;
static int font_to_char = 126;
static char filename[MAXPATH+1] = {""};
static char output[MAXPATH+1] = {""};

/* The used plugins.
 */
static char lookup_renderer[MAXNAME+1] = {"1bit"};
static const t_renderer_plugin *curr_renderer = (t_renderer_plugin*)0;
static char lookup_writer[MAXNAME+1] = {"ascii"};
static const t_writer_plugin *curr_writer = (t_writer_plugin*)0;

/* FreeType related variables
 */
static FT_Library library;
static FT_Face face;

/* Configuration Flags
 */
static int flag_verbose = 0;		// verbose output
static int flag_check_only = 0;		// only check metrics,..
static int flag_calc_baseline = 0;	// use calculated baseline
static int flag_show_help = 0;		//

/* The options to get parsed
 */
static struct option long_options[] =
{
    /* These options set a flag. */
    {"verbose",   no_argument, &flag_verbose, 1},
    {"check",     no_argument, &flag_check_only, 1},
    {"calc",      no_argument, &flag_calc_baseline, 1},
    {"help",      no_argument, &flag_show_help, 1},
    /* These options don’t set a flag. We distinguish them by their indices. */
    {"hdpi",      required_argument, 0, 'H'},
    {"dpi",       required_argument, 0, 'd'},
    {"origin",    required_argument, 0, 'O'},
    {"output",    required_argument, 0, 'o'},
    {"from",      required_argument, 0, 'f'},
    {"to",        required_argument, 0, 't'},
    {"renderer",  required_argument, 0, 'R'},
    {"writer",    required_argument, 0, 'W'},
    {0, 0, 0, 0}
};


/* Dictionary for "Renderer" plugins
 */
static const t_renderer_list renderer[] =
{
    {"1bit", renderer_1bit_creator},
    {"2bit", renderer_2bit_creator},
    {NULL,NULL}
};

/* Dictionary for "Writer" plugins
 */
static const t_writer_list writer[] =
{
    {"c-raw", writer_c_raw_creator},
    {"ascii", writer_ascii_creator},
    {NULL,NULL}
};


//}}}

/*+=========================================================================+*/
/*|                      PROTOTYPES OF LOCAL FUNCTIONS                      |*/
/*`========================================================================='*/
//{{{

static void usage ( void );
static bool find_renderer ( void );
static bool find_writer ( void );
static bool generate_font ( void );
static bool prepare_font_creation ( t_font_metrics *font, const char *filename, int size, int preset_origin );
static bool check_font_metrics ( t_font_metrics *font );
static t_glyph_matrix *create_glyph_matrices ( const t_font_metrics *font );
static bool create_glyph ( FT_Glyph *glyph, int character );
static inline int max ( int a, int b );

//}}}

/*+=========================================================================+*/
/*|                     IMPLEMENTATION OF THE FUNCTIONS                     |*/
/*`========================================================================='*/
//{{{

int main ( int argc, char **argv )
{
    bool done = false;
    int option_index = 0;		// getopt_long stores the option index here
    int val;
    int c=0;

    fprintf(stderr,"%s -- font generation helper\n\n",PACKAGE_STRING);
    do
    {
	c = getopt_long(argc,argv,"?vcCH:d:o:O:f:t:R:W:",long_options,&option_index);
	switch ( c )
	{
	    case 0:
		// a flag is set. keep on going...
		break;
	    case 1:
		// plain command line options reached
		done = true;
		break;
	    case -1:
		// end of options
		done = true;
		break;
	    case ':':
		/* missing option argument */
		fprintf(stderr,"%s: option `-%c' requires an argument\n",argv[0],optopt);
		return 1;
		break;
	    case 'v':
		flag_verbose = 1;
		break;
	    case 'c':
		flag_check_only = 1;
		break;
	    case 'C':
		flag_calc_baseline = 1;
		break;
	    case '?':
		flag_show_help = 1;
		break;
	    case 'd':
		if ( optarg )
		{
		    val = atoi(optarg);
		    if ( val==0 && *optarg!= '0' )
		    {
			fprintf(stderr,"error: invalid parameter (%s) for option -d\n",optarg);
			return 1;
		    }
		    dpi = val;
		    if ( flag_verbose )
			fprintf(stderr,"info: dpi set to %d\n",dpi);
		}
		break;
	    case 'H':
		if ( optarg )
		{
		    val = atoi(optarg);
		    if ( val==0 && *optarg!= '0' )
		    {
			fprintf(stderr,"error: invalid parameter (%s) for option -H\n",optarg);
			return 1;
		    }
		    hdpi = val;
		    if ( flag_verbose )
			fprintf(stderr,"info: hdpi set to %d\n",hdpi);
		}
		break;
	    case 'O':
		if ( optarg )
		{
		    val = atoi(optarg);
		    if ( val==0 && *optarg!= '0' )
		    {
			fprintf(stderr,"error: invalid parameter (%s) for option -O\n",optarg);
			return 1;
		    }
		    forced_origin = val;
		    if ( flag_verbose )
			fprintf(stderr,"info: force detected origin to %d\n",forced_origin);
		}
		break;
	    case 'o':
		if ( optarg )
		{
		    strncpy(output,optarg,MAXPATH);
		    output[MAXPATH]='\0';
		    if ( flag_verbose )
			fprintf(stderr,"info: use output file '%s'\n",output);
		}
		break;
	    case 'R':
		if ( optarg )
		{
		    strncpy(lookup_renderer,optarg,MAXNAME);
		    lookup_renderer[MAXNAME]='\0';
		    if ( flag_verbose )
			fprintf(stderr,"info: renderer to lookup '%s'\n",lookup_renderer);
		}
		break;
	    case 'W':
		if ( optarg )
		{
		    strncpy(lookup_writer,optarg,MAXNAME);
		    lookup_writer[MAXNAME]='\0';
		    if ( flag_verbose )
			fprintf(stderr,"info: writer to lookup '%s'\n",lookup_writer);
		}
		break;
	    case 'f':
		if ( optarg )
		{
		    val = atoi(optarg);
		    if ( val==0 && *optarg!= '0' )
		    {
			fprintf(stderr,"error: invalid parameter (%s) for option -f\n",optarg);
			return 1;
		    }
		    if ( val<0 || val>255 )
		    {
			fprintf(stderr,"error: parameter %d for option -f out of range\n",val);
			return 1;
		    }
		    font_from_char = val;
		    if ( flag_verbose )
			fprintf(stderr,"info: from-char set to %d\n",font_from_char);
		}
		break;
	    case 't':
		if ( optarg )
		{
		    val = atoi(optarg);
		    if ( val==0 && *optarg!= '0' )
		    {
			fprintf(stderr,"error: invalid parameter (%s) for option -t\n",optarg);
			return 1;
		    }
		    if ( val<0 || val>255 )
		    {
			fprintf(stderr,"error: parameter %d for option -t out of range\n",val);
			return 1;
		    }
		    font_to_char = val;
		    if ( flag_verbose )
			fprintf(stderr,"info: to-char set to %d\n",font_to_char);
		}
		break;
	}
    } while (!done);

    if ( optind < argc )
    {
	if ( (argc-optind) >= 2 )
	{
	    // first parm is the point size
	    val = atoi(argv[optind]);
	    if ( val==0 && *argv[optind]!= '0' )
	    {
		fprintf(stderr,"error: invalid parameter (%s) for option -d\n",argv[optind]);
		return 1;
	    }
	    optind++;
	    size = val;
	    if ( flag_verbose )
		fprintf(stderr,"info: use size %d\n",size);
	    // second parm is the filename of the font file
	    strncpy(filename,argv[optind++],MAXPATH);
	    filename[MAXPATH]='\0';
	    if ( flag_verbose )
		fprintf(stderr,"info: use font file '%s'\n",filename);
	}
	else
	    flag_show_help = true;
    }
	else
	    flag_show_help = true;

    if ( size <= 0 )
    {
	fprintf(stderr,"error: font size not specified\n");
	return 1;
    }
    if ( filename[0] == '\0' )
    {
	fprintf(stderr,"error: font file not specified\n");
	return 1;
    }

    if ( flag_show_help )
    {
	usage();
	return 0;
    }
    if ( flag_check_only )		// if we only check the metrics,
    {
	fprintf(stderr,"**check only mode**\n");
	flag_verbose = 1;		// allways enable verbose output
    }
    if ( font_to_char < font_from_char )
    {
        int tmp = font_from_char;
        font_from_char = font_to_char;
        font_to_char = tmp;
    }

    if ( !find_renderer() )
    {
	fprintf(stderr,"error: don't know renderer '%s'\n",lookup_renderer);
	return 1;
    }
    if ( !find_writer() )
    {
	fprintf(stderr,"error: don't know writer '%s'\n",lookup_writer);
	return 1;
    }

    return generate_font()?0:2;
}


//}}}

/*             .-----------------------------------------------.             */
/* ___________/  Exported functions                             \___________ */
/*            `-------------------------------------------------'            */
//{{{

bool is_verbose (void)
{
    return flag_verbose;
}

//}}}

/*+=========================================================================+*/
/*|                    IMPLEMENTATION OF LOCAL FUNCTIONS                    |*/
/*`========================================================================='*/
//{{{

static void usage ( void )
{
    fprintf(stderr,"SYNOPSIS\n");
    fprintf(stderr,"  %s [options] pt-size font\n",PACKAGE);
    fprintf(stderr,"\nOPTIONS\n");
    fprintf(stderr,"  -d|--dpi <num>       set a special dpi value (default 72)\n");
    fprintf(stderr,"  -H|--hdpi <num>      set a special hor. dpi value (default is same as dpi)\n");
    fprintf(stderr,"  -O|--origin <d>      force detected origin to <d>.\n");
    fprintf(stderr,"  -C|--calc            use calculated origin. default is detect.\n");
    fprintf(stderr,"  -o|--output <name>   force a basename (without extension).\n");
    fprintf(stderr,"  -f|--from <idx>      start rendering with character code <idx>.\n");
    fprintf(stderr,"  -t|--to <idx>        stop rendering at character code <idx>.\n");
    fprintf(stderr,"  -R|--renderer <name> use renderer <name> (default '1pix')\n");
    fprintf(stderr,"  -W|--writer <name>   use writerer <name> (default 'ascii')\n");
    fprintf(stderr,"  -v|--verbose         enable more verbose messages.\n");
    fprintf(stderr,"  -c|--check           check metrics only. No fonts are generated.\n");
    fprintf(stderr,"\nRENDERDER\n");
    fprintf(stderr,"  1bit                 1 bit for each pixel in the fix matrix (monospaced)\n");
    fprintf(stderr,"  2bit                 2 bits for each pixel in the fix matrix (monospaced)\n");
    fprintf(stderr,"\nWRITER\n");
    fprintf(stderr,"  ascii                simple ASCII arts of the font\n");
    fprintf(stderr,"  c-raw                raw C-source of the final matrix data\n");
}

static bool find_renderer ( void )
{
    int i = 0;
    while ( renderer[i].factory )
    {
	if ( strcmp(renderer[i].name,lookup_renderer)==0 )
	{
	    curr_renderer = renderer[i].factory();
	    return curr_renderer!=0;
	}
	++i;
    }
    return false;
}

static bool find_writer ( void )
{
    int i = 0;
    while ( writer[i].factory )
    {
	if ( strcmp(writer[i].name,lookup_writer)==0 )
	{
	    curr_writer = writer[i].factory();
	    return curr_writer!=0;
	}
	++i;
    }
    return false;
}

/* Combine all the functions to create the font.
 */
static bool generate_font ( void )
{
    t_font_definition defs;
    t_font_metrics font;
    t_glyph_matrix *gmatrices;
    bool result;
    int err;

    err = FT_Init_FreeType(&library);
    if ( err )
    {
        fprintf(stderr,"error: init FreeType failed (%d)\n",err);
        return false;
    }
    if ( !prepare_font_creation(&font,filename,size,forced_origin) )
	return false;
    err = FT_New_Face(library,filename,0,&face);
    if ( err )
    {
        fprintf(stderr,"error: create of FreeType face failed (%d)\n",err);
        FT_Done_FreeType(library);
        return false;
    }
    if ( !check_font_metrics(&font) )
    {
        // FT_Done_Face(face);
        FT_Done_FreeType(library);
	return false;
    }

    if ( flag_check_only )		// if we only check the metrics,
    {
	// FT_Done_Face(face);
	FT_Done_FreeType(library);
	return true;
    }
    if ( font.baseline > 0 )
    {
	fprintf(stderr,"error: illegal baseline! Must be less than 0! (%d)\n",font.baseline);
	// FT_Done_Face(face);
	FT_Done_FreeType(library);
	return false;
    }

    if ( !curr_renderer || !curr_writer )
    {
        fprintf(stderr,"fatal: bad setup of renderer/writer \n");
	// FT_Done_Face(face);
	FT_Done_FreeType(library);
        return false;
    }

    if ( flag_calc_baseline )
	font.baseline = font.calculated_baseline;
    else
	font.baseline = font.detected_baseline;

    gmatrices = create_glyph_matrices(&font);
    result = curr_renderer->init(&defs,&font,font_from_char,font_to_char);
    if ( result )
    {
	result = curr_renderer->generate(&defs,gmatrices);
	if ( result )
	{
	    result = curr_writer->init(&defs,output);
	    if ( result )
		result = curr_writer->create(&defs);
	    if ( result )
		curr_writer->done(&defs);
	}
	curr_renderer->done(&defs);
    }
    // TODO: free gmatrices? no, we leave

    //FT_Done_Face(face);
    FT_Done_FreeType(library);
    return result;
}


/* Prepare the font generation. This includes the initialisation of \c font.
 * Sone variables are set based on the commandline options.
 */
static bool prepare_font_creation ( t_font_metrics *font, const char *filename, int size, int preset_origin )
{
    const char *ptr;
    char *p;
    int i;

    if ( !font || !filename || size<2 )
    {
	fprintf(stderr,"error: prepare_font_creation: illegal parameters\n");
	return false;
    }
    ptr = strrchr(filename,'/');	// find last slash of path
    if ( ptr )
        ptr++;				// and strip it
    else
        ptr = filename;			// else no path is given

    // copy plain name of the file as font name and strip some chars.
    strncpy(font->name,ptr,sizeof(font->name)-1);
    p = strrchr(font->name,'.');	// Find last period (file ext)
    if ( p )
	*p = '\0';
    for ( i=0; font->name[i]; i++ )
    {
        if ( isspace(font->name[i]) || ispunct(font->name[i]) )
            font->name[i] = '_';
    }
    if ( flag_verbose )
	fprintf(stderr,"info: use font name '%s'\n",font->name);

    font->pt_size = size;
    font->dpi = dpi;
    font->hdpi = hdpi;
    font->detected_baseline = preset_origin;

    // clear the remaining values.
    font->max_ascent = 0;
    font->max_descent = 0;
    font->baseline = 0;
    font->calculated_baseline = 0;
    font->matrix.height = 0;
    font->matrix.width = 0;
    font->absolute_height = 0;
    font->em.height = 0;
    font->em.width = 0;
    font->ex.height = 0;
    font->ex.width = 0;
    font->we.height = 0;
    font->we.width = 0;
    return true;
}

/* Try to load a font and calculate the needed metrics. The results are passed
 * back in \c fonts.
 */
static bool check_font_metrics ( t_font_metrics *font )
{
    FT_Glyph glyph;
    FT_BitmapGlyphRec *g;	// little helper
    FT_Bitmap *bitmap;
    int regular_height;
    int ascent;
    int descent;
    int i;

    if ( !font )
    {
	fprintf(stderr,"error: prepare_font_creation: illegal parameters\n");
	return false;
    }

    // the point size must be converted to '26dot6' fixed-point formast
    FT_Set_Char_Size(face,0,(font->pt_size)<<6,font->hdpi,font->dpi);

    if ( create_glyph(&glyph,'M') )
    {
	if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
	{
	    FT_Done_Glyph(glyph);
	    fprintf(stderr,"error: check_font_metrics: glyph not in BITMAP format!\n");
	    return false;
	}
        //g = (FT_BitmapGlyphRec*)glyph;
        bitmap = &face->glyph->bitmap;
	font->absolute_height     = (int)(face->size->metrics.height)>>6;
	font->matrix.height       = font->absolute_height;
	// (jd) using the verAdvance value sometimes leads to characters bigger
	//than the matrix height!
	// font->matrix.height   = (int)(face->glyph->metrics.vertAdvance)>>6;
	regular_height            = (int)(face->glyph->metrics.vertAdvance)>>6;
	font->matrix.width        = (int)(face->glyph->metrics.horiAdvance)>>6;
	font->calculated_baseline = ((int)(face->glyph->metrics.vertAdvance)>>6) - (font->absolute_height);
	font->em.height           = bitmap->rows;
	font->em.width            = bitmap->width;
	FT_Done_Glyph(glyph);
    }
    else
	return false;

    if ( create_glyph(&glyph,'x') )
    {
	if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
	{
	    FT_Done_Glyph(glyph);
	    fprintf(stderr,"error: check_font_metrics: glyph not in BITMAP format!\n");
	    return false;
	}
        bitmap = &face->glyph->bitmap;
	font->ex.height = bitmap->rows;
	font->ex.width  = bitmap->width;
	FT_Done_Glyph(glyph);
    }
    else
	return false;
    if ( create_glyph(&glyph,'W') )
    {
	if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
	{
	    FT_Done_Glyph(glyph);
	    fprintf(stderr,"error: check_font_metrics: glyph not in BITMAP format!\n");
	    return false;
	}
        bitmap = &face->glyph->bitmap;
	font->we.height = bitmap->rows;
	font->we.width  = bitmap->width;
	FT_Done_Glyph(glyph);
    }
    else
	return false;

    /* Scan the whole font to determine the correct values for the maximal
     * size of the glyphs. This is needed to get correct values for ascend and
     * descend.
     */
    if ( flag_verbose )
	fprintf(stderr,"info: scan font...\n");
    font->max_ascent = 0;
    font->max_descent = 0;
    for ( i=1; i<255; i++ )
    {
	if ( create_glyph(&glyph,i) )
	{
	    if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
	    {
		FT_Done_Glyph(glyph);
		fprintf(stderr,"error: check_font_metrics: glyph not in BITMAP format!\n");
		return false;
	    }
	    bitmap = &face->glyph->bitmap;
	    g = (FT_BitmapGlyphRec*)glyph;
#ifdef DEBUG
	    // should be the same.... just to be sure
	    if ( (int)(face->glyph->bitmap_top) != (int)(g->top) )
		fprintf(stderr,"check_font_metrics: bitmap_top=% differ from top=%d!\n",(int)(face->glyph->bitmap_top),(int)(g->top));
#endif
	    // Detect the highest and lowest point. In the case the glyph has
	    // no descender, the value must be 0!
	    descent = max(0,face->glyph->bitmap.rows - g->top);
	    ascent = max(0, max(g->top,face->glyph->bitmap.rows) - descent);
	    if ( descent > font->max_descent )
	      font->max_descent = descent;
	    if ( ascent > font->max_ascent )
	      font->max_ascent = ascent;
	    FT_Done_Glyph(glyph);
	}
	else
	    return false;
    }

    /* If no baseline is given, we use the maximum descend value as
     * baseline.
     */
    if ( font->detected_baseline == 0 )
    {
	if ( flag_verbose )
	    fprintf(stderr,"info: use determined baseline...\n");
	font->detected_baseline = -(font->max_descent);
    }

    if ( (font->max_ascent+font->max_descent) > font->absolute_height )
    {
	fprintf(stderr,"warn: absolute height exceeded! %d>%d\n",(font->max_ascent+font->max_descent),font->absolute_height);
    }
    else if ( (font->max_ascent+font->max_descent) > regular_height )
    {
	if ( flag_verbose )
	    fprintf(stderr,"info: regular height exceeded! %d>%d\n",(font->max_ascent+font->max_descent),regular_height);
    }
    if ( (font->max_ascent+font->max_descent) > font->matrix.height )
    {
	if ( flag_verbose )
	    fprintf(stderr,"info: enlarge matrix height from %d to %d\n",font->matrix.height,(int)(font->max_ascent+font->max_descent));
	font->matrix.height = (font->max_ascent+font->max_descent);
    }


    if ( flag_verbose )
    {
	fprintf(stderr,"name:        '%s'\n",font->name);
	fprintf(stderr,"range:       #%d..#%d\n",font_from_char,font_to_char);
	fprintf(stderr,"size:        %d pt\n",font->pt_size);
	if ( font->hdpi )
	    fprintf(stderr,"dpi:         %d x %d\n",font->hdpi,font->dpi);
	else
	    fprintf(stderr,"dpi:         %d\n",font->dpi);
#ifdef DEBUG
	fprintf(stderr,"matrix:      %d x %d  (%d x %d)\n",font->matrix.width,font->matrix.height,font->matrix.width,regular_height);
#else
	fprintf(stderr,"matrix:      %d x %d\n",font->matrix.width,font->matrix.height);
#endif
	fprintf(stderr,"nl-height:   %d\n",font->absolute_height);
	fprintf(stderr,"max-ascent:  %d\n",font->max_ascent);
	fprintf(stderr,"max-descent: %d\n",font->max_descent);
	fprintf(stderr,"baseline:    calculated: %d  detected: %d (%s)\n",font->calculated_baseline,font->detected_baseline,forced_origin?"forced":"detected");
	fprintf(stderr,"em:          %d x %d\n",font->em.width,font->em.height);
	fprintf(stderr,"ex:          %d x %d\n",font->ex.width,font->ex.height);
	fprintf(stderr,"we:          %d x %d\n",font->we.width,font->we.height);
    }
    return true;
}

/* Create the glyph matrices for all needed characters. The bitmap and the metric
 * details are written into \c font.
 */
static t_glyph_matrix *create_glyph_matrices ( const t_font_metrics *font )
{
    FT_Glyph glyph;
    FT_BitmapGlyphRec *g;	// little helper
    FT_Bitmap *bitmap;
    t_glyph_matrix *gmatrices;
    bool failed = false;
    uint8_t *p;
    int num;
    int idx;
    int tmp;
    int i;

    if ( !font )
    {
	fprintf(stderr,"error: create_glyph_matrices: illegal parameters\n");
	return NULL;
    }
    num = font_to_char - font_from_char +1;
    if ( num <= 0 )
    {
	fprintf(stderr,"error: create_glyph_matrices: bad number of chars\n");
	return NULL;
    }
    gmatrices = malloc(num*sizeof(t_glyph_matrix));
    if ( !gmatrices )
    {
	fprintf(stderr,"error: create_glyph_matrices: memory allocation failed\n");
	return NULL;
    }
    for ( idx=0; idx<num; idx++ )
    {
	gmatrices[idx].sz_buffer = 0;
	gmatrices[idx].buffer = 0;
    }

    // the point size must be converted to '26dot6' fixed-point formast
    FT_Set_Char_Size(face,0,(font->pt_size)<<6,font->hdpi,font->dpi);
    for ( idx=0,i=font_from_char; i<font_to_char; i++,idx++ )
    {
	if ( create_glyph(&glyph,i) )
	{
	    if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
	    {
		FT_Done_Glyph(glyph);
		fprintf(stderr,"error: check_font_metrics: glyph not in BITMAP format!\n");
		return NULL;
	    }
	    bitmap = &face->glyph->bitmap;
	    g = (FT_BitmapGlyphRec*)glyph;
	    gmatrices[idx].width = bitmap->width;		// width of the glyph bitmap
	    gmatrices[idx].height = bitmap->rows;		// height of the glyph bitmap
	    gmatrices[idx].pitch = bitmap->pitch;		// no. of bytes per bitmap row
	    gmatrices[idx].advance = face->glyph->advance.x;	// real width for proportional fonts

	    gmatrices[idx].offset_x = g->left;			// x offset into char matrix
	    gmatrices[idx].offset_y = font->matrix.height +
				      font->baseline -
				      g->top;			// y offset into char matrix

	    if ( gmatrices[idx].offset_y < 0 )
	    {
		// move down
		fprintf(stderr,"warn: create_glyph_matrices: glyph #%d out of matrix. Move down (%d->%d).\n",idx,gmatrices[idx].offset_y,0);
		gmatrices[idx].offset_y = 0;
	    }
	    else if ( (gmatrices[idx].offset_y+gmatrices[idx].height) > font->matrix.height )
	    {
		// move up
		tmp = font->matrix.height - gmatrices[idx].height;
		fprintf(stderr,"warn: create_glyph_matrices: glyph #%d out of matrix. Move up (%d->%d).\n",idx,gmatrices[idx].offset_y,tmp);
//~ fprintf(stderr,"    : create_glyph_matrices: glyph=%d/%d offs=%d/%d matrix=%d/%d base=%d\n",
	//~ gmatrices[idx].width,gmatrices[idx].height,
	//~ g->left,g->top,
	//~ font->matrix.width,font->matrix.height,
	//~ font->baseline);
		gmatrices[idx].offset_y = tmp; // or tmp-1?
	    }
	    if ( (gmatrices[idx].offset_y<0) || ((gmatrices[idx].offset_y+gmatrices[idx].height) > font->matrix.height) )
		fprintf(stderr,"warn: create_glyph_matrices: glyph #%d out of matrix (Y). Will be clipped.\n",idx);
	    if ( (gmatrices[idx].offset_x<0) || ((gmatrices[idx].offset_x+gmatrices[idx].width) > font->matrix.width) )
		fprintf(stderr,"warn: create_glyph_matrices: glyph #%d out of matrix (X). Will be clipped.\n",idx);

	    /* Calculate the size used by the glyph bitmap and allocate the
	     * buffer. Copy the bitmap buffer provided by the FreeType-library.
	     * An empty bitmap leads to a size of 0 and a buffer pointer of NULL!
	     */
	    gmatrices[idx].sz_buffer = (bitmap->rows)*(bitmap->pitch);	// size of the buffer
	    if ( gmatrices[idx].sz_buffer )
	    {
		p = malloc(gmatrices[idx].sz_buffer*sizeof(uint8_t));
		if ( !p )
		{
		    // TODO: clean up prev. allocated memory here? no, we leave
		    fprintf(stderr,"error: create_glyph_matrices: memory allocation failed\n");
		    return NULL;
		}
	    }
	    else
		p = NULL;
	    gmatrices[idx].buffer = p;
	    memcpy(gmatrices[idx].buffer,bitmap->buffer,gmatrices[idx].sz_buffer);
#ifdef DEBUG_OFF
	    fprintf(stderr,"create_glyph_matrices: width %d needs %d bytes. size=%d\n",bitmap->width,bitmap->pitch,gmatrices[idx].sz_buffer);
#endif
	    FT_Done_Glyph(glyph);
	}
	else
	{
	    failed = true;
	    break;
	}
    }

    return failed?NULL:gmatrices;
}


/* Load a glyph into the passed \c glyph variable. The caller must ensure to call
 * \c FT_Done_Glyph(glyph) after processing to free the memory.
 */
static bool create_glyph ( FT_Glyph *glyph, int character )
{
    int err;

    err = FT_Load_Char(face,character,FT_LOAD_TARGET_MONO);
    if ( err )
    {
	fprintf(stderr,"error: create_glyph: loading char #%d (%d)\n",character,err);
	return false;
    }
    err = FT_Render_Glyph(face->glyph,FT_RENDER_MODE_MONO);
    if ( err )
    {
	fprintf(stderr,"error: create_glyph: rendering char #%d (%d)\n",character,err);
	return false;
    }
    err = FT_Get_Glyph(face->glyph,glyph);
    if ( err )
    {
	fprintf(stderr,"error: create_glyph: getting glyph #%d (%d)\n",character,err);
	return false;
    }
    return true;
}

static inline int max ( int a, int b )
{
  return (a > b) ? a : b;
}

//}}}

/* ==[End of file]========================================================== */
