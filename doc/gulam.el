;;
;; Info-ize gulam.texinfo using our texinfmt.el
;;
(defun make-gulam-info-file ()
  "Info-ize gulam.texinfo using our texinfmt.el. bammi@mandrill.CWRU.edu"
  (interactive)
  (find-file "gulam.texinfo")
  (load-file "texinfmt.el")
  (texinfo-format-buffer)
  (save-buffer))
