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
	  (if (eq? (car s) 'charsintype)
	      (let ((base (cadr s))
		    (ct (if (>= (caddr s) 10)
			    (- (caddr s) 2) 
			    (caddr s)))
		    (chars (cdddr s)))
		(for-each (lambda (ch) 
			    (display (format "  {0x~X, ~2D},\n"
					     (symbol->integer ch) ct)))
			  chars)))
	  (mk-builtin-jfm-2)))))
(define (symbol->integer sym)
  (+ (* 256 (- (char->integer
		(string-ref (symbol->string sym) 0)) 128))
     (- (char->integer (string-ref (symbol->string sym) 1)) 128)))

(if #f
    (begin
      (system "tftopl-j `kpsewhich min10.tfm` foo.pl 2>&1 >/dev/null" )
      (mk-builtin-jfm "foo.pl" "__chartypes")
      (system "sort __chartypes")
      (system "rm -f foo.pl __chartypes"))
    (begin
      (system "tftopl-j `kpsewhich tmin10.tfm` foo.pl 2>&1 >/dev/null" )
      (mk-builtin-jfm "foo.pl" "__chartypes")
      (system "sort __chartypes")
      (system "rm -f foo.pl __chartypes")))


(quit)
