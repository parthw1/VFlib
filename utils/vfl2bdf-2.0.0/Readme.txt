     vfl2bdf - make a BDF font from VFlib font.
                     May 15 1997
                  Hirotsugu Kakugawa 
               h.kakugawa@computer.org


Usage: vfl2bdf [Options] FONT START END
   - Make a BDF font from VFlib font FONT.
     Range of code points is from START to END.

Options:
  -p PIXEL : pixel size of BDF font.
  -o FILE  : output file name.
  -v FILE  : vflibcap file.
  -f FONT  : font name.
  -h       : print how to use this program.

Examples:
  ./vfl2bdf -v /usr/local/lib/vflibcap timR24.pcf 0x20 0x7e
  ./vfl2bdf -v /usr/local/lib/vflibcap jiskan16.pcf 0x2121 0x7e7e
  ./vfl2bdf -v /usr/local/lib/vflibcap cmr10.400pk 0x00 0x7f

/*EOF*/
