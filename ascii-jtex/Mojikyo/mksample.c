/*
 *  mksample.c
 *    Make list of all characters of Mojikyo font in LaTeX form.
 *  
 *
 *  How to use this program:
 *
 *  1. Compile this program.
 *         % gcc -o mksample mksample.c
 *  2. Run this program to generate (huge) LaTeX input file.
 *         % ./mksample  > sample.tex
 *  3. Run pLaTeX to generate DVI file.
 *         % platex sample.tex
 *  4. View/Print it. For example, 
 *         % xgdvi sample.dvi 
 *
 */
 
#include <stdio.h>

#define MINCHAR        1
#define MAXCHAR   115000
#define NCHLINE       20
#define SIZECMD  "normalsize"


int    ku_from, ku_to, nchline, title;
char  *sizecmd;
int    mtype;


#define  TYPE_TTF    1
#define  TYPE_TYPE1  2


void
parse_args(int argc, char **argv)
{
  if (argc == 1)
    return;

  mtype = TYPE_TTF;
  title = 1;
  argc--;
  while (argc > 0){
    if (strcmp(*argv, "-f") == 0){
      argc--; argv++;
      ku_from = atoi(*argv) / 100;      
    } else if (strcmp(*argv, "-t") == 0){
      argc--; argv++;
      ku_to = atoi(*argv) / 100;      
    } else if (strcmp(*argv, "-n") == 0){
      argc--; argv++;
      if ((nchline = atoi(*argv)) < 1){
	fprintf(stderr, "-n option: value must be positive.\n");
      }
    } else if (strcmp(*argv, "-s") == 0){
      argc--; argv++;
      sizecmd = *argv;
    } else if (strcmp(*argv, "-ttf") == 0){
      mtype = TYPE_TTF;
    } else if (strcmp(*argv, "-type1") == 0){
      mtype = TYPE_TYPE1;
    } else if ((strcmp(*argv, "-h") == 0)
	       || (strcmp(*argv, "-help") == 0)
	       || (strcmp(*argv, "--help") == 0)){
      fprintf(stderr, 
	      "Usage: mksample  [-ttf] [-type1] "
	      "[-f FROM_CODE] [-t TO_CODE] [-n LINE_NCHARS] [-s SIZE_CMD]\n");
      fprintf(stderr, "Default Values:\n");
      fprintf(stderr, "  FROM_CODE = %d\n", MINCHAR);
      fprintf(stderr, "  TO_CODE = %d\n", MAXCHAR);
      fprintf(stderr, "  LINE_NCHARS = %d\n", NCHLINE);
      fprintf(stderr, "  SIZE_CMD = %s\n", SIZECMD);
      exit(0);
    }
    argc--; argv++;
  }
}


int
main(int argc, char **argv)
{
  int  ku, ten, i;


  ku_from = MINCHAR/100;
  ku_to   = (MAXCHAR+99)/100;
  title   = 0;
  nchline = NCHLINE;
  sizecmd = SIZECMD;

  parse_args(argc, argv);

  printf("\\documentclass[a4paper]{jarticle}\n");
  printf("\\setlength{\\topmargin}{-25mm}\n");
  printf("\\setlength{\\evensidemargin}{-10mm}\n");
  printf("\\setlength{\\oddsidemargin}{-10mm}\n");
  printf("\\setlength{\\textwidth}{180mm}\n");
  printf("\\setlength{\\textheight}{263mm}\n");
  printf("\\usepackage{mojikyo}\n");
  printf("\\usepackage{array}\n");
  printf("\\renewcommand{\\arraystretch}{0.9}\n");
#if 1
  printf("\\def\\CH#1{#1}\n");
#else
  printf("\\def\\CH#1{\\mbox{#1}}\n");
#endif
  printf("\\begin{document}\n");
  if (title == 0){
    printf("\\begin{center}\n");
    printf("\\textsf{\\textbf{\\LARGE 「文字鏡」 \\quad 文字一覧}}\n");
    printf("\\end{center}\n");
    printf("\\vskip 3em\n");
  }
  printf("\\%s\n", sizecmd);


  for (ku = ku_from; ku <= ku_to; ku++){

    printf("\\vskip 1.2em\n");
    printf("\\vbox{\n");
    printf("\\noindent\\textsf{\\textbf{%06d}}\\par\n", ku*100 + 1);
    printf("\\begin{center}");
    printf("\\begin{tabular}{|r|");
    for (i = 0; i < nchline; i++)
      printf("c");
    printf("|}\n");
    printf("\\hline\n");
    printf(" \n");
    for (i = 0; i < nchline; i++)
      printf("&\\texttt{%02d}", i+1);
    printf("\\\\\n");
    printf("\\hline\n");
    for (ten = 1; ten <= 100; ten++){
      if ((ten % nchline) == 1){
	if (ten > 1)
	  printf("\\\\\n");
	printf("\\texttt{%06d}\n", ku*100 + ten);
      }
      if (mtype == TYPE_TTF){
	printf(" &\\CH{\\TMO{%06d}}\n", ku*100 + ten);
      } else {
	printf(" &\\CH{\\PMO{%06d}}\n", ku*100 + ten);
      }
    }
    while ((ten % nchline) != 1){
      printf(" &\n");
      ten++;
    }
    printf("\\\\\n");
    printf("\\hline\n");
    printf("\\end{tabular}\n");
    printf("\\end{center}");
    printf("}\n");
    printf("\n");
  }

  printf("\\end{document}\n");
  
  return 0;
}
