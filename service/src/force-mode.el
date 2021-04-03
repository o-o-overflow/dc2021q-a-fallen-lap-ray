;;; force-mode.el --- major mode for the FORCE langauge

(defconst force-mode-syntax-table
  (let ((table (make-syntax-table c-mode-syntax-table)))
    ;; / is punctuation, but // is a comment starter
    (modify-syntax-entry ?/ ". 12" table)
    ;; # is a comment starter
    (modify-syntax-entry ?# "<" table)
    ;; \n is a comment ender
    (modify-syntax-entry ?\n ">" table)
    table))


(define-derived-mode force-mode c-mode "Force Mode"
  :syntax-table force-mode-syntax-table
  (font-lock-fontify-buffer))

(font-lock-add-keywords 'force-mode
                        '(("\\<\\(defun\\|OUTS_LITERAL\\|export\\|extern\\|asm\\|sync\\|new\\|OUTD\\|OUTS\\|OPEN\\|READ\\|WRITE\\|CLOSE\\|HALT\\|LOAD\\|LISTDIR\\|SENDFILE\\|UNLINK\\|LSEEK\\|RANDOM\\|NEW_ARRAY\\)\\>" . font-lock-keyword-face)))
