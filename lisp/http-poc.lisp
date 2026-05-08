;;; http-poc.lisp -- Milestone 1 foreground HTTP demo for uLisp PicoCalc Pico 2W.
;;;
;;; Load/evaluate this file after connecting Wi-Fi or starting an AP, then run:
;;;
;;;   (http-demo-start)
;;;
;;; Example AP-mode setup:
;;;
;;;   (wifi-softap "ulisp-poc")
;;;   (http-demo-start)
;;;
;;; Browse to http://192.168.4.1/ or the IP returned by (wifi-localip).

(defvar *http-demo-count* 0)

(defun req-get (req key)
  (cdr (assoc key req :test string=)))

(defun http-demo-page (req)
  (setq *http-demo-count* (1+ *http-demo-count*))
  (format nil "<!doctype html><html><head><meta charset='utf-8'><title>uLisp HTTP POC</title></head><body><h1>uLisp PicoCalc HTTP POC</h1><p>Path: ~a</p><p>Query: ~a</p><p>Requests served: ~a</p></body></html>"
          (html-escape (req-get req "path"))
          (html-escape (req-get req "query"))
          *http-demo-count*))

(defun http-demo-step ()
  (let ((req (http-accept)))
    (when req
      (if (string= (req-get req "path") "/")
          (http-respond 200 "text/html; charset=utf-8" (http-demo-page req))
          (http-respond 404 "text/plain; charset=utf-8" "not found")))))

(defun http-demo-run ()
  (loop (http-demo-step)))

(defun http-demo-start ()
  (http-server-start)
  (http-demo-run))
