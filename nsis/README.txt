You need to do these steps after the ARC installation
-----------------------------------------------------

Install Grid certificate and private key:

   On Windows XP:

     Copy your usercert.pem file to: C:\Documents and Settings\<username>\.globus\usercert.pem
     Copy your userkey.pem  file to: C:\Documents and Settings\<username>\.globus\userkey.pem

   On Windows Vista, 7:

     Copy your usercert.pem file to: C:\Users\<username>\.globus\usercert.pem
     Copy your userkey.pem  file to: C:\Users\<username>\.globus\userkey.pem


Other information
-----------------

1. Default client.conf, jobs.xml directory location (these directories are hidden by default):

   On Windows XP:

     C:\Documents and Settings\<username>\Application Data\.arc

   On Windows Vista, 7:

     C:\Users\<username>\AppData\Roaming\.arc


2. The .globus directory creation can be problematic because the Explorer of the Windows
   does not like the "." at the beginning of the directory name.


   Solution for this: cmd (command line interface of Windows)

     mkdir "C:\Documents and Settings\<username>\.globus"

   or

   mkdir "C:\Users\<username>\.globus"
