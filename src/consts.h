/*
 * consts.h - a definition file for VFlib constants
 * by Hirotsugu Kakugawa
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


/*
 * Site specific runtime directory, 
 */
#ifndef VF_ENV_DIR_RUNTIME_SITE_LIB
#  define VF_ENV_DIR_RUNTIME_SITE_LIB    "VFLIB_RUNTIME_SITE_DIRECTORY"
#endif

/*
 * Runtime directory
 */
#ifndef VF_ENV_DIR_RUNTIME_LIB
#  define VF_ENV_DIR_RUNTIME_LIB         "VFLIB_RUNTIME_DIRECTORY"
#endif


/*
 * vflibcap
 */
#ifndef VF_DEFAULT_VFLIBCAP_FILE
#  define VF_DEFAULT_VFLIBCAP_FILE         "vflibcap"
#endif
#define VF_ENV_VFLIBCAP_PATH             "VFLIB_VFLIBCAP_PATH"
#define VF_ENV_VFLIBCAP_DIR              "VFLIB_VFLIBCAP_DIRECTORY"
#define VF_ENV_VFLIBCAP_PARAM_PREFIX     "VFLIBCAP_PARAM_"


/* 
 * An env variable to cheange the CCV file directory
 */
#define VF_ENV_CCV_DIR                   "VFLIB_CCV_DIRECTORY"


/*
 * A font file hint database file, used for fast font file searching 
 */
#ifndef VF_FONT_FILE_HINT_DB 
#  define VF_FONT_FILE_HINT_DB           "VFlib.fdb"
#endif


/* 
 * Max # of implicit font classes
 */
#ifndef MAX_DEFAULT_IMPLICT_FONT_CLASSES
#  define  MAX_DEFAULT_IMPLICT_FONT_CLASSES  32
#endif


/*
 * Max # of font open nestings.
 * (This limits the number of depth of recursive VF_OpenFont() calls.)
 */
#ifndef VF_MAX_OPEN_NESTING
#  define VF_MAX_OPEN_NESTING  64
#endif


/*
 * The maximum number of file descripters simultaneously opened
 * for reading fonts
 */ 
#ifndef VF_MAX_FILE_DESCRIPTERS
#  define VF_MAX_FILE_DESCRIPTERS  8
#endif

/*
 * An environment variable for to change value for VF_MAX_FILE_DESCRIPTERS
 * on runtime.
 */ 
#ifndef VF_ENV_MAX_FILE_DESCRIPTERS
#  define VF_ENV_MAX_FILE_DESCRIPTERS   "VFLIB_MAX_FD"
#endif


/*
 * Envs for for debugging
 */
#define VF_ENV_DEBUG_FONT_OPEN    "VFLIB_DEBUG_FONT_OPEN"
#define VF_ENV_DEBUG_FONT_SEARCH  "VFLIB_DEBUG_FONT_SEARCH"
#define VF_ENV_DEBUG_KPATHSEA     "VFLIB_DEBUG_KPATHSEA"
#define VF_ENV_DEBUG_VFLIBCAP     "VFLIB_DEBUG_VFLIBCAP"
#define VF_ENV_DEBUG_PARAMETERS   "VFLIB_DEBUG_PARAMETERS"
#define VF_ENV_DEBUG_CCV          "VFLIB_DEBUG_CCV"
#define VF_ENV_DEBUG_CCV_MAPPING  "VFLIB_DEBUG_CCV_MAPPING"
#define VF_ENV_DEBUG_FILEMAN      "VFLIB_DEBUG_FILEMAN"
#define VF_ENV_DEBUG_LOG          "VFLIB_DEBUG_LOG"


/* 
 * Default resolusion
 */
#ifndef VF_DEFAULT_DPI
#  define VF_DEFAULT_DPI    300
#endif


/*
 * The maximum path length
 */ 
#ifndef MAXPATHLEN
#  define MAXPATHLEN  1024
#endif 


/*
 * Directory delimeter. "/" for Unix and "\\" for MS-DOS 
 * Can  be specified in vflibcap.
 */
#ifndef VF_DIRECTORY_DELIMITER 
#	ifdef WIN32
#		define VF_DIRECTORY_DELIMITER	"\\"
#	else
#		define  VF_DIRECTORY_DELIMITER   "/"
#	endif
#endif


/*EOF*/
