#!/usr/local/bin/scm 

;;
;; Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
;; All rights reserved.
;;
;; License: GPLv3 and FreeType Project License (FTL)
;;

(require 'format)

(define (mk-builtin-jfm ifile ofile)
  (with-output-to-file  ofile
    (lambda () (with-input-from-file ifile mk-builtin-jfm-2))))

(define (mk-builtin-jfm-2)
  (let ((s (read)))
    (if (not (eof-object? s))
	(begin
	  (if (eq? (car s) 'type)
	      (let ((base (cadr s))
		    (ct (if (>= (caddr s) 10)
			    (- (caddr s) 2) 
			    (caddr s)))
		    (wd (caddr (car (cdddr s))))
		    (ht (caddr (cadr (cdddr s))))
		    (dp (caddr (caddr (cdddr s)))))
		(display (format "  {~2D, ~F, ~F, ~F},\n"
				 ct wd ht dp))))
	  (mk-builtin-jfm-2)))))

(if #f
    (begin
      (system "tftopl-j `kpsewhich min10.tfm` foo.pl 2>&1 >/dev/null" )
      (mk-builtin-jfm "foo.pl" "__chardimen")
      (system "cat __chardimen")
      (system "rm -f foo.pl __chardimen"))
    (begin
      (system "tftopl-j `kpsewhich tmin10.tfm` foo.pl 2>&1 >/dev/null" )
      (mk-builtin-jfm "foo.pl" "__chardimen")
      (system "cat __chardimen")
      (system "rm -f foo.pl __chardimen")))

(quit)
