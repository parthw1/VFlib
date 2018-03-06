#ifndef WIN32

#ifdef FREETYPE2
#  include <ft2build.h>
typedef FT_Face		TT_Face;
typedef FT_UShort	TT_UShort;
typedef FT_Error	TT_Error;
#else
#  include "freetype.h"
#endif

#else /* WIN32 */

#define FREETYPE2 1  /* temporary define here... move to with.h */
#  ifdef FREETYPE2
#  ifdef _DEBUG
#pragma comment(lib, "../../freetype-2.2.1/objs/freetype221_D.lib")
#  else
#pragma comment(lib, "../../freetype-2.2.1/objs/freetype221.lib")
#  endif
#  include <ft2build.h>
#  include FT_FREETYPE_H
#  include FT_GLYPH_H
#  include FT_TRIGONOMETRY_H 
typedef FT_Face		TT_Face;
typedef FT_UShort	TT_UShort;
typedef FT_Error	TT_Error;
#else
#  include "freetype.h"
#pragma comment(lib, "libttf.lib")
#endif
#endif
