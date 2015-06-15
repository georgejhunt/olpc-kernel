\ OLPC boot script

[ifdef] require-signatures?
   : no-signatures  ( -- )  false to require-signatures?  ;
[else]
   : is-valid  ( $ $ -- true )  4drop true  r> drop  ;
   : is-leased  ( -- )  " run" cn-buf place  ;
   : no-signatures  ( -- )
      ['] is-valid ['] fw-valid?  >body token!
      ['] is-valid ['] sha-valid? >body token!
      ['] false      ['] has-developer-key?  ['] load-from-list    (patch

      ['] is-leased  ['] ?leased             ['] load-from-device  (patch
   ;
[then]

: set-path-macros  ( -- )
   button-o game-key?  if  " \boot-alt"  else  " \boot"  then  pn-buf place

   " /chosen" find-package  if                       ( phandle )
      " bootpath" rot  get-package-property  0=  if  ( propval$ )
         get-encoded-string                          ( bootpath$ )
         [char] \ left-parse-string  2nip            ( dn$ )
         dn-buf place                                ( )
      then
   then
;

: unsigned-boot  ( -- )
   no-signatures
   alternate?  if  " \boot-alt"  else  " \boot"  then  pn-buf place
   " last:" load-from-list drop
;

: olpc-fth-boot-me
   set-path-macros
   " ${DN}${PN}\vmlinuz" expand$ 2dup $file-exists? if
      to boot-device
      " ${DN}${PN}\initrd.img" expand$ to ramdisk
      " rw console=ttyS0,115200 console=tty0 fbcon=font:SUN12x22 root=/dev/mmcblk0p2 rd_NO_FSTAB" to boot-file
      boot
   else
      2drop
      unsigned-boot
   then
;
visible
unfreeze
olpc-fth-boot-me
