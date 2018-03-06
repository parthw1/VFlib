/*
 * ekan.h - a header file for drv_ekan.c
 * by Hirotsugu Kakugawa
 *
 */
/*
 * Copyright (C) 1999-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#define   FONTCLASS_NAME      "ekanji"

#define   DIRECTION_HORIZONTAL    0
#define   DIRECTION_VERTICAL      1
#define   DEFAULT_TO_REF_PT_H    0.87
#define   DEFAULT_TO_REF_PT_V   -0.5

#define   MOCK_FONT_ENC_RAW               0
#define   MOCK_FONT_ENC_SUBBLOCKS_94X94   1
#define   MOCK_FONT_ENC_SUBBLOCKS_94X60   2
#define   MOCK_FONT_ENC_WITH_OFFSET       10


#define VF_CAPE_EK_FONT_DOT_SIZE         "font-dot-size"
#define VF_CAPE_EK_MOCK_FONT_ENC         "mock-font-encoding"
#define    CAPE_MOCK_FONT_ENC_RAW                 "raw"
#define    CAPE_MOCK_FONT_ENC_SUBBLOCKS_94X94     "subblocks-94x94"
#define    CAPE_MOCK_FONT_ENC_SUBBLOCKS_94X60     "subblocks-94x60"
#define    CAPE_MOCK_FONT_ENC_WITH_OFFSET         "with-offset"


#define   DEFAULT_FONT_DOT_SIZE    24
#define   DEFAULT_MOCK_FONT_ENC    MOCK_FONT_ENC_RAW
#define   POINTS_PER_INCH        72.27
#define   DEFAULT_DPI            72.27
#define   DEFAULT_POINT_SIZE     10.0
#define   DEFAULT_PIXEL_SIZE       24
#define   DEFAULT_DIRECTION        DIRECTION_HORIZONTAL


/*EOF*/
