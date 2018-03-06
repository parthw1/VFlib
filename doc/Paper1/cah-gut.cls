% Classe de document cah-gut.cls pour les Cahiers GUTenberg
% et les publications exceptionnelles
%
% Classe prevue pour LaTeX2e uniquement
\NeedsTeXFormat{LaTeX2e}
% Informations diverses
\ProvidesClass{cah-gut}
        [1998/01/12 v2.06.6 Classe des Cahiers GUTenberg]
% Pas d'article presentes a l'italienne
\DeclareOption{landscape}{\@latexerr{Option not supported}%
   {}}
% Autres options autorisees
\DeclareOption*{%
    \PassOptionsToClass  {\CurrentOption}{article}}
% Traitement des options
\ProcessOptions
% Chargement de la classe de base article
\LoadClass[twoside]{article}
%
%%%%%%%%%%%%%% FICHIER EN COURS DE TRAVAUX... %%%%%%%%%%%%%%%%%%
%
\typeout{Feuille provisoire de style des Cahiers GUTenberg, version 2.06.6 (12/1/98)}
%
% utilisation du mode dense pour les listes J.A. 8/8/97
% redefinition de Abstract Resume Keyword et Motsclef pour etre 
%               etre plus serre et independant de french J.A. 8/8/97
% espace ajoute apres --- dans headers par Denis Girou D.G. 19/6/96
% corrige par E.P. pour pb de shipout, ouf !!! le 25/4/96
%% Cette version a les valeurs corrigees pour les parametres lies a l'imposition
% a la demande de l'imprimerie de l'universite de Rennes 1. J.A. 25/7/95
% et de  l'imprimerie Louis-jean a Gap (cf Maurice Laugier!) J.A. 10/09/95
% qui demande une largeur papier < 17.5cm (d'ou reduction de textwidth)
% et a la suite du CA du 12/09/95 ou il a ete dit qu'en ayant une hauteur de
% la page papier de 22.4 cm on pouvait faire 2 exemplaires par copie 
% sur Ducotech (d'ou modif de )             J.A. 13/09/95
%% Par ailleurs :
% Le titre courant Cahiers GUTenberg est mis en italique (10/9/95)
% le foliotage a lieu desormais dans le header (13/09/95)
%% Cette version du 1/2 contient des modifs recommandees par Thierry Bouche
% pour remettre en points certains parametres donnes en cm
% Les numéros de footnotes ont été adaptés à LaTeX2e T.B. 21/12/95 (signale
% aussi par bernard gaulle 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% hyphenation commands (For MLTeX)
\ifx\fhyph\undefined\relax\else % si \fhyph est connu, on suppose \ehyph connu
   \ifx\enhyph\undefined\let\enhyph=\ehyph\fi
   \ifx\frhyph\undefined\let\frhyph=\fhyph\fi
\fi

\newcommand{\GUT}{\mbox{{\sc GUT}enberg}}

\newcommand{\NoC}[1]{\gdef\N@C{#1}}
  \NoC{\string??}
\newcommand{\DateC}[1]{\gdef\D@teC{#1}}
  \global\let\D@teC\empty


\def\sectsize{\@setfontsize\sectsize\@xivpt{21pt}}      % titre section
\def\subsectsize{\@setfontsize\subsectsize\@xipt{14pt}} % titre sous-section 
\def\abstsize{\@setfontsize\abstsize\@viiipt{11pt}}     % abstract
\def\captsize{\@setfontsize\captsize@xpt{12pt}}         % legendes
\def\titlesize{\@setfontsize\titlesize\@xxpt{25pt}}     % Grand titre
\def\authsize{\@setfontsize\authsize\@xipt{14pt}}       % auteur
\def\affilsize{\@setfontsize\affilsize\@xpt{12pt}}      % Affiliation

% A MODIFIER...
\def\@citex[#1]#2{\if@filesw\immediate\write\@auxout{\string\citation{#2}}\fi
  \def\@citea{}\@cite{\@for\@citeb:=#2\do
    {\@citea\def\@citea{, }\@ifundefined
       {b@\@citeb}{{\bf ?}\@warning
       {Citation `\@citeb' on page \thepage \space undefined}}%
\hbox{\csname b@\@citeb\endcsname}}}{#1}}

\newcommand\affiliation[1]{\gdef\@affiliation{#1}}
  \gdef\@affiliation{Affiliation inconnue ?}

% espacement titre
\def\titlesp{\list{}{\leftmargin0pc}\item[]}
\let\endtitlesp=\endlist

\renewcommand{\@maketitle}{\newpage
 \null \vspace*{-28pt}
 \begin{titlesp}
  {\titlesize 
   \hyphenchar\font=-1\raggedright 
   \@title \par}
  \vskip 8pt \hrule \vskip 14pt                 
  {\authsize \@author \par}
  \vskip 6pt 
  {\affilsize\it \@affiliation \par}
\end{titlesp}
 \par \vskip 16pt}

\renewcommand{\maketitle}{\par
\begingroup
\def\@fndot{\null}
\def\thefootnote{\fnsymbol{footnote}}
\def\@makefnmark{\hbox 
 to 0pt{\@textsuperscript{\@thefnmark}}} % T.B. 21/12/95
\if@twocolumn 
\twocolumn[\@maketitle] 
\else \newpage
\global\@topnum\z@ \@maketitle \fi\thispagestyle{titre}\@thanks
\endgroup
\setcounter{footnote}{0}
\let\maketitle\relax
\let\@maketitle\relax
\gdef\@thanks{}\gdef\@author{}\gdef\@title{}\let\thanks\relax}

\newcommand{\Abstract}{\noindent\small{\bf Abstract}.      % J.A. 8/8/97
\em\ifx\enhyph\undefined\relax\else\enhyph\fi}
\def\endabstract{
\normalsize\rm\ifx\frhyph\undefined\relax\else\frhyph\fi\Nospacing}

%\re...                                                 % J.A. 8/8/97
\newcommand{\Resume}{\noindent\small{\bf R\'esum\'e.}
\rm\ifx\frhyph\undefined\relax\else\frhyph\fi}
\def\endResume{
\normalsize\ifx\frhyph\undefined\relax\else\frhyph\fi\medskip\Nospacing}

%\re...                                                  % J.A. 8/8/97
\newcommand{\Motsclef}{\noindent\small\textbf{Mot-clés :}
\textrm\bgroup}
\def\endMotsclef{\egroup\ \normalsize\Nospacing}


%\re...                                                  % J.A. 8/8/97
\newcommand{\Keywords}{\noindent\small\textbf{Keywords:} 
\textrm\bgroup}
\def\endKeywords{\egroup\ \normalsize\Nospacing}

\@ifundefined{chapter}{\newcounter{chapter}}{\@addtoreset{chapter}{part}}%
\newcommand{\chaptermark}[1]{}% gobble the second argument
\newcommand{\chapter}{\@startsection{chapter}{0}{\z@}{-12pt plus -2pt minus%
-2pt}{10pt plus 1pt minus 1pt}{\sectsize\raggedright{\bf\@chapapp\space}}}%

\renewcommand{\section}{\@startsection {section}{1}{\z@}{-12pt plus -2pt minus 
-2pt}{10pt plus 1pt minus 1pt}{\reset@font\sectsize\raggedright}}

\renewcommand{\subsection}{\@startsection{subsection}{2}{\z@}%
{-10pt plus -2pt minus -1pt}%
{8pt plus 1pt minus 1pt}{\subsectsize\bf\raggedright}}
\renewcommand{\subsubsection}{\@startsection{subsubsection}{3}{\z@}{-8pt plus 
-2pt minus -1pt}{6pt plus 1pt minus 1 pt}{\subsectsize\it\raggedright}}
\renewcommand{\paragraph}{\@startsection
{paragraph}{4}{\z@}{6pt plus 2pt minus 2pt}{-1em}{\normalsize\bf}}
\renewcommand{\subparagraph}{\@startsection
{subparagraph}{4}{\parindent}{6pt plus 2pt minus 
2pt}{-1em}{\normalsize\bf}}

\newenvironment{abst}{\list{}{\leftmargin0pc}\abstsize\item[]}% 
{\endlist\vskip-7pt}  

\newcommand{\listelabel}{$\bullet$}
\settowidth{\labelwidth}{\listelabel}
\newenvironment{liste}{\list{}{\settowidth{\labelwidth}{\listelabel}
\leftmargin\labelwidth\advance\leftmargin\labelsep\rightmargin0pt\labelsep0pt
\itemsep0pt \let\makelabel\listelabel}\small}{\endlist}

\newenvironment{enumeration}{\list{{\small \arabic{enumi}.\ }}%
{\settowidth{\labelwidth}{{\small 1.\ }}
\leftmargin\labelwidth\advance\leftmargin\labelsep\rightmargin0pt\labelsep0pt
\itemsep0pt \usecounter{enumi}}\small}{\endlist}

 
%%% Definir des listes << denses >>   % J.A. 8/8/97 d'apres michel goossens
\let\Otemize =\itemize
\let\Onumerate =\enumerate
\let\Oescription =\description
\def\Nospacing{\itemsep=0pt\topsep=0pt\partopsep=0pt\parskip=0pt\parsep=0pt}
% Définition des nouvelles listes en fonction des listes standard
\renewenvironment{itemize}{\Otemize\Nospacing}{\endlist} % was \Itemize chez mg
\renewenvironment{enumerate}{\Onumerate\Nospacing}{\endlist} %\En..
\renewenvironment{description}{\Oescription\Nospacing}{\endlist} %\Des...
 



\def\@sect#1#2#3#4#5#6[#7]#8{\ifnum #2>\c@secnumdepth
     \def\@svsec{}\else 
     \refstepcounter{#1}\edef\@svsec{\csname the#1\endcsname{.}\hskip 1em }\fi
     \@tempskipa #5\relax
      \ifdim \@tempskipa>\z@ 
        \begingroup #6\relax
          \@hangfrom{\hskip #3\relax\@svsec}{\interlinepenalty \@M #8\par}
        \endgroup
       \csname #1mark\endcsname{#7}\addcontentsline
         {toc}{#1}{\ifnum #2>\c@secnumdepth \else
                      \protect\numberline{\csname the#1\endcsname}\fi
                    #7}\else
        \def\@svsechd{#6\hskip #3\@svsec #8\csname #1mark\endcsname
                      {#7}\addcontentsline
                           {toc}{#1}{\ifnum #2>\c@secnumdepth \else
                             \protect\numberline{\csname the#1\endcsname}\fi
                       #7}}\fi
     \@xsect{#5}}

%Corrige' ChR --bg
\renewcommand{\refname}{R\'ef\'erences bibliographiques}
\ifx\bibname\undefined\let\bibname\refname\fi% --bg
\def\thebibliography#1{\section*{\bibname}%
\list{[\arabic{enumi}]}{\settowidth\labelwidth{[#1]}\leftmargin\labelwidth
\advance\leftmargin\labelsep\usecounter{enumi}}%\small %J.A. 31/07/95
}\let\endthebibliography=\endlist 
\newif\if@restonecol

\def\theindex{\@restonecoltrue\if@twocolumn\@restonecolfalse\fi \columnseprule
\z@ \columnsep 35pt\twocolumn[\section*{\indexname}]
\parindent\z@
\parskip\z@ plus .3pt\relax\let\item\@idxitem} \def\@idxitem{\par\hangindent
40pt} \def\subitem{\par\hangindent 40pt \hspace*{20pt}}
\renewcommand{\subsubitem}{\par\hangindent 40pt \hspace*{30pt}}
\renewcommand{\endtheindex}{\if@restonecol\onecolumn\else\clearpage\fi}
\renewcommand{\indexspace}{\par \vskip 10pt plus 5pt minus 3pt\relax}

\long\def\@makefntext#1{\parindent 1em\noindent 
            \hbox to 1.8em{\@textsuperscript{\@thefnmark}}#1} % T.B. 21/12/95


\setlength\textfloatsep{13\p@ \@plus 2\p@ \@minus 4\p@}

%\def\caption{\refstepcounter\@captype \@dblarg{\@caption\@captype}}
%\long\def\@caption#1[#2]#3{\par\addcontentsline{\csname
%  ext@#1\endcsname}{#1}{\protect\numberline{\csname
%  the#1\endcsname}{\ignorespaces #2}}\begingroup
%    \@parboxrestore
%    \captsize
%    \@makecaption{\csname fnum@#1\endcsname}{\ignorespaces #3}\par
%  \endgroup}

\setlength\abovecaptionskip{1\p@}
%\long\def\@makecaption#1#2{
%   \vskip 1pt % almost no skip at all
%   \setbox\@tempboxa\hbox{#1~#2}
%   \ifdim \wd\@tempboxa >\hsize   % IF longer than one line:
%       \unhbox\@tempboxa\par      %   THEN set as ordinary paragraph.
%     \else                        %   ELSE  center.
%       \hbox to\hsize{\hfil\box\@tempboxa\hfil}
%   \fi}

\setcounter{topnumber}{1}
\renewcommand{\bottomfraction}{0.01}

\newcommand{\titlehead}[1]{\gdef\@titlehead{#1}}
\newcommand{\authorhead}[1]{\gdef\@authorhead{#1}}

\long\def\author#1{\def\@author{#1}
                   \ifx\@authorhead\undefined\let\p@authorhead\@author
                                        \else\let\p@authorhead\@authorhead\fi
                  }
\global\let\p@authorhead\relax% a good default for .toc first read BG 3/2/94
\global\let\p@title\relax
\long\def\title#1{\def\@title{#1}\let\p@title\@title
                  \addtocontents{toc}{\protect\contentsline{sommaire}
                  {\protect\p@authorhead\protect\space:\space
                   \protect\p@title}{\thepage}}
                 }

%%%% version ancienne avant modif du 13/09/95 J.A.
%\def\g@neralhead{\vskip 10pt \hbox to\textwidth{\hrulefill}}%  BG 9/2/94
%\global\let\generalhead\relax% empty pagestyle default BG 9/2/94
% 
%\def\ps@gut{\let\@mkboth\@gobbletwo%
%            \let\generalhead\g@neralhead% BG 9/2/94
%     \def\@evenhead{\normalsize\it{%
%\ifx\@authorhead\undefined\@author\else\@authorhead\fi}\hfil}%
%     \def\@oddfoot{\rm\hfil\thepage}\def\@oddhead{\normalsize\it\hfil{%   
%\ifx\@titlehead\undefined\@title\else\@titlehead\fi}}%
%     \def\@evenfoot{\rm\thepage\hfil}}
                 

%% Version modifiee J.A. 13/09/95 pour mettre foliotage dans headers
\def\g@neralhead{\vskip 10pt \hbox to\textwidth{\hrulefill}}%  BG 9/2/94
\global\let\generalhead\relax% empty pagestyle default BG 9/2/94

\def\ps@gut{\let\@mkboth\@gobbletwo%
            \let\generalhead\g@neralhead% BG 9/2/94
     \def\@evenhead{\rm\thepage\normalsize\it\hfil{%
\ifx\@authorhead\undefined\@author\else\@authorhead\fi}}%
     \def\@oddfoot{}
\def\@oddhead{\normalsize\it{%
\ifx\@titlehead\undefined\@title\else\@titlehead\fi}\hfil\rm\thepage}%
     \def\@evenfoot{}}


% pas beau car dependant de la fonte, a revoir. BG 24/12/93
%\gdef\Ptinumero{n\char'027}% plus tard on utilisera \numero de french.sty
%\gdef\Ptinumero{n\char'006}% provisoire pour codage Cork
% EP 25/07/95
\DeclareTextSymbol{\Ptideg}{OT1}{23}
\DeclareTextSymbol{\Ptideg}{T1}{6}
\gdef\Ptinumero{n\kern-.12em\Ptideg}
                           % qd il adoptera toutes les tailles. 
\def\ps@titre{\let\@mkboth\@gobbletwo%
            \let\generalhead\g@neralhead% BG 9/2/94
   \def\@evenhead{\rm\thepage\hfil{\footnotesize\Etat % BG 20/1/94 et J.A. 13/09/95
   \textit{Cahiers GUTenberg} \Ptinumero{}\N@C{} --- % DG 19/6/94
            \D@teC}}%  %J.A. 10/09/95  
   \def\@oddhead{{\footnotesize\Etat % BG 20/1/94
   \textit{Cahiers GUTenberg} \Ptinumero{}\N@C{} --- % DG 19/6/94
            \D@teC}\hfil\rm\thepage}% %J.A. 10 et 13/09/95
%     \def\@oddfoot{\rm\hfil\thepage}%         %J.A. 13/09/95
%     \def\@evenfoot{\rm\thepage\hfil}         %J.A. 13/09/95
}

% Pour utiliser le format cahiers.sty dans des 
% occasion particulieres... Ch. R. 24/05/94
\gdef\EnTete#1{
  \def\ps@titre{\let\@mkboth\@gobbletwo%
              \let\generalhead\g@neralhead%
%     \def\@evenhead{\hfill{\footnotesize #1}}%            % J.A. 13/09/95
%     \def\@oddhead{{\footnotesize #1}\hfill}%             % J.A. 13/09/95
      \def\@evenhead{\rm\thepage\hfil{\footnotesize #1}}%  % J.A. 13/09/95
     \def\@oddhead{{\footnotesize #1}\hfil\rm\thepage}%    % J.A. 13/09/95
%       \def\@oddfoot{\rm\hfil\thepage}%       % J.A. 13/09/95
     \def\@oddfoot{}                           % J.A. 13/09/95
%       \def\@evenfoot{\rm\thepage\hfil}}}     % J.A. 13/09/95
     \def\@evenfoot{}}}                        % J.A. 13/09/95

\ds@twoside
%Bricolage Ch. R. 15/12/94, modifie E> P. 25/04/96
\def\@outputpage{%
\begingroup           % the \endgroup is put in by \aftergroup
  \let \protect \noexpand
  \@resetactivechars
  \let\-\@dischyph
  \let\'\@acci\let\`\@accii\let\=\@acciii
  \let\\\@normalcr
  \let\par\@@par          %% 15 Sep 87
  \shipout \vbox{%
    \set@typeset@protect
%    \aftergroup \endgroup% moved in its normal place  --bg 97/12 
%    \aftergroup \set@typeset@protect% idem  --bg 97/12 
                                % correct? or just restore by ending
                                % the group?
  \if@specialpage
    \global\@specialpagefalse\@nameuse{ps@\@specialstyle}%
  \fi
  \if@twoside
    \ifodd\count\z@ \let\@thehead\@oddhead \let\@thefoot\@oddfoot
         \let\@themargin\oddsidemargin
    \else \let\@thehead\@evenhead
       \let\@thefoot\@evenfoot \let\@themargin\evensidemargin
    \fi
  \fi
  \reset@font
  \normalsize
  \baselineskip\z@skip \lineskip\z@skip \lineskiplimit\z@
    \@begindvi
    \vskip \topmargin
    \moveright\@themargin \vbox {%
      \setbox\@tempboxa \vbox to\headheight{%
        \vfil
        \color@hbox
          \normalcolor
          \hb@xt@\textwidth {%
            \let \label \@gobble
            \let \index \@gobble
            \let \glossary \@gobble %% 21 Jun 91
            \@thehead
            }%
        {\vspace*{10pt}\hbox to\textwidth{\hrulefill}}
        \color@endbox
        }%                        %% 22 Feb 87
      \dp\@tempboxa \z@
      \box\@tempboxa
      \vskip \headsep
      \box\@outputbox
      \baselineskip \footskip
      \color@hbox
        \normalcolor
        \hb@xt@\textwidth{%
          \let \label \@gobble
          \let \index \@gobble      %% 22 Feb 87
          \let \glossary \@gobble   %% 21 Jun 91
          \@thefoot
          }%
      \color@endbox
      }%
    }%
\endgroup% --bg 97/12
\set@typeset@protect% --bg 97/12 
\global \@colht \textheight
\stepcounter{page}%
\let\firstmark\botmark
}

% heading needs a bar

%\setlength\oddsidemargin{0mm}    
%\setlength\evensidemargin{35mm}  %J.A. 25/7/95
\setlength\oddsidemargin{-2mm}    %J.A. 13/09/75
\setlength\evensidemargin{42mm}   %J.A. 13/09/95 7mm=4x .5 +(4.5-2)mm
\setlength\marginparwidth {1.75cm}  
\setlength\marginparsep{10pt}
\setlength\marginparpush{7\p@}
\setlength\topmargin{2.7cm} %J.A. 25/7/95
\setlength\headheight{1cm}
\setlength\headsep{14pt}
\setlength\footskip{1cm}
%\setlength\textheight{546pt} %J.A. 11/09/95
\setlength\textheight{507pt} %T.B. 21/12/95 (17.8cm) 
%\setlength\textwidth{12.45cm}
\setlength\textwidth{12cm}    % J.A. 11/09/95
\setlength\columnsep{10pt}
\setlength\columnseprule{0pt}
\setlength\footnotesep{8.4pt}
\setlength{\skip\footins}{10.8\p@ \@plus 4\p@ \@minus 2\p@}
\setlength\floatsep    {14\p@ \@plus 2\p@ \@minus 2\p@}
\setlength\textfloatsep{20\p@ \@plus 2\p@ \@minus 4\p@}
\setlength\intextsep   {14\p@ \@plus 2\p@ \@minus 2\p@}
\setlength\dblfloatsep    {14\p@ \@plus 2\p@ \@minus 4\p@}
\setlength\dbltextfloatsep{20\p@ \@plus 2\p@ \@minus 4\p@}
\setlength\@fptop{0\p@ \@plus 1fil}
\setlength\@fpsep{10\p@ \@plus 2fil}
\setlength\@fpbot{0\p@ \@plus 1fil}
\setlength\@dblfptop{0\p@ \@plus 1fil}
\setlength\@dblfpsep{10\p@ \@plus 2fil}
\setlength\@dblfpbot{0\p@ \@plus 1fil}
\setlength\partopsep{3\p@ \@plus 2\p@ \@minus 2\p@}
\setlength\parskip{1.1ex plus 1pt}
\setlength\parindent{1.5em}
\setlength\topsep{10pt plus 4pt minus 6pt}
\setlength\itemsep{5pt plus 2.5pt minus 1pt}

\@lowpenalty   51 
\@medpenalty  151 
\@highpenalty 301
\@beginparpenalty -\@lowpenalty
\@endparpenalty   -\@lowpenalty
\@itempenalty     -\@lowpenalty

\setlength\hfuzz{2pt}
\hbadness        10000
\hyphenpenalty   50
\exhyphenpenalty 50

\setcounter{topnumber}{3}
\renewcommand{\topfraction}{.99}
\renewcommand{\textfraction}{.01}

\clubpenalty  10000
\widowpenalty 10000

\renewcommand{\labelitemi}{---}
\renewcommand{\labelitemii}{--}
\renewcommand{\labelitemiii}{-}

\sloppy
\frenchspacing

\let\@Mti\maketitle
\def\maketitle{\pagestyle{gut}
               \thispagestyle{titre}
               \@Mti}

% pas beau car dependant de la fonte, a revoir. BG 24/12/93
% (la plupart se retrouve dans french.sty)
\def\at{{\char '100}}
\def\boi{{\tt\char '134}}
\def\circonflexe{{\char '136}}
\def\tild{{\char '176}}
\def\degre{\kern-.3em{\char '027}}
%% remove the 3 following lines if you don't the guill font.
%\ifx\oguill\undefined %  normally defined in french.sty BG 20/194
%\font\trom=guill
%\def\oguill{{\trom A}\nobreak\mbox{\hglue.25em}\nobreak}
%\def\fguill{\nobreak\mbox{\hglue.25em}\nobreak{\trom B}\,}
%\fi

\let\@afterindentfalse\@afterindenttrue
\@afterindenttrue

% BG 24/12/93 :
% ce n'est pas la bonne place dans le style, a mettre dans refcahiers.tex 
%\NoC{14}
%\DateC{Mars 1993}
\let\Etat\relax% valeur par defaut.
\InputIfFileExists{refcahiers.tex}{}{\typeout{cahiers.sty : fichier refcahiers.tex non trouve'.}}

% tant que la date n'est pas figee il s'agit d'un brouillon
\ifx\D@teC\empty% defaut BG 20/01/94
    \def\D@teC{\today}% redefine it with the current language BG 25/01/94
    \ifx\Etat\relax\gdef\Etat{Brouillon d'article pour les }\fi%
\fi%

\ifx\frhyph\undefined\relax\else\frhyph\fi


% Ce qui suit est dans french.sty et independant des fontes.
%\gdef\AllTeX{{\rm(L\kern-.36em\raise.3ex\hbox{\sc a}\kern-.15em)%
%              T\kern-.1667em\lower.7ex\hbox{E}\kern-.125emX}}
\def\SommaireC{\def\sommairename{\center
                                 \Huge Cahiers \GUT{} \N@C\\\D@teC\\
                                 \medskip
                                 \Large S O M M A I R E\\
                                 \vspace{1cm}}
               \ifx\sommaire\undefined\def\sommaire[4]{\sommairename}\fi
               \let\SaveNl=\\
               \def\LigneSC##1##2{\bgroup\def\\{\SaveNl\hspace*{0.75cm}}
                                \noindent ##1 \dotfill\ ##2
                                \egroup\\}
               \bgroup\let\l@sommaire\LigneSC\let\thanks\@gobble
               \sommaire[4]
               \egroup
              }% fin de \SommaireC

\def\endSommaireC{\vspace*{1cm}\bgroup
                  \center
                  R\'edaction des {\em Cahiers \GUT}\\
                  c/o Irisa/Inria, campus universitaire de Beaulieu\\
                  F-35042 Rennes cedex, France\\
                               \egroup}
\gdef\l@sommaire#1#2{}

\def\@ufo{\def\figurename{{\sc Figure}}%
          \def\tablename{{\sc Table}}}%% 
\@ufo% J.A. Avril 96 (avait disparu...?)


\parindent0pt                   %J.A. 8/8/97


%%%%%%%%%%%%%%%%%% Fin inclusion %%%%%%%%%%%%%%%%%%
\endinput
%% 
%% Fin du fichier << cah-offgut.cls >>,.


