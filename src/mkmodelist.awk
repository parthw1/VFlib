# make a device mode list from 'modes.mf' file (given in stdin)
# by h.kakugawa

# Line: e.g.,  mode_def ljfour =                       % 600dpi HP LaserJet 4

#
# Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
# All rights reserved.
#
# License: GPLv3 and FreeType Project License (FTL)
#


/^mode_def/ { 
  mode=$2; 
  ppi=-1; 
  ppiv=-1; 
  asp=1.0;
  i=index($0, "%");
  x=substr($0, i);
  desc=substr($0, i + match(x, "[a-zA-Z0-9]") - 1);
}


# Line: e.g.,     mode_param (pixels_per_inch, 600);

/^[ \t]*mode_param[ \t]*\([ \t]*pixels_per_inch, [ \t]*[0-9.]+\);/ {
  match($3, "[0-9.]*");
  ppi=substr($3, 1, RLENGTH);
  ppiv=-1; 
}


### Currently, aspect ratio must be 1

# Line: e.g.,     mode_param (aspect_ratio, 4/3);

/^[ \t]*mode_param[ \t]*\([ \t]*aspect_ratio[ \t]*,[ \t]*[0-9./]+[ \t]*);/ {
  i=match($0, ",[ \t]*");
  s0=i+RLENGTH;
  x=substr($0, s0);
  len=match(x, ")");
  asp=substr($0, s0, len-1);
  ppiv=asp*ppi;
  ppi=-1; ### ignore this entry.
}


# Line: e.g.,   mode_param (aspect_ratio, 180 / pixels_per_inch);

/^[ \t]*mode_param[ \t]*\([ \t]*aspect_ratio, [ \t]*.*\/[ \t]*pixels_per_inch)/ {
  i=match($0, ",[ \t]*[0-9.]");
  x=substr($0, i+RLENGTH-1);
  s0=i+RLENGTH-1;
  match(x, "[ \t]*/");
  ppiv=substr($0, s0, RSTART-1);
  asp=ppi/ppiv;
  ppi=-1; ### ignore this entry.
}


# Line: e.g., enddef;
/^enddef/ {
  if ((mode != "") && (ppi > 0)){
    if ((ppiv < 0) && (mode == DEVNAME)){
      printf("%s\n", ppi); 
    }
    mode=""; 
    ppi=-1; 
    ppiv=-1; 
    desc="";
  }
}
