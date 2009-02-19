#!/bin/sh
#
#  This script creates a certificate depending on the root certificate created 
#  by createMasterCA.sh
#
# BASED ON:
#  http://www.madboa.com/geek/openssl/#cert-self


echo ' This script will create a new certificate depending on the '
echo ' the certificate of the CA created with createMasterCA.sh.'
echo ''
echo ' Following files will be created (or overwritten):'
echo ' - requests/myreq.pem'
echo ' - slaves/key.pem'
echo ' - slaves/cert.cer'
echo ' - slaves/cert.pem'
echo ' - slaves/cert%/key%.pem'
echo ' - slaves/cert%/cert%.cer'
echo ' - slaves/cert%/cert%.pem'
echo ' - slaves/cert%/ca.cer'
echo ' - slaves/cert%/ca.pem'
echo ' - slaves/cert%/request%.pem'
echo       Where % is a fresh number!
echo ''
echo 'WARNING: The child certificate in slaves/ will be replaced !'
echo '         The certificates in slaves/cert%/ will not be touched!'
echo ''
echo    '         Press Strg+C to abort!'
echo ''
read -p "         Press a key to proceed"



echo ''
mkdir  slaves
mkdir  requests

echo ' Create key for the new certificate and create a request for the CA.'
echo ' (slaves/key.pem requests/myreq.pem)'
openssl req \
  -new -newkey rsa:1024 -nodes \
  -keyout slaves/key.pem -out requests/myreq.pem


#openssl rsa -in key.pem -out newkey.pem


echo ' Create the new certificate (360 days slaves/cert.cer)'
openssl ca -policy policy_anything -config openssl.conf -cert certs/ca.cer -in requests/myreq.pem -keyfile keys/ca.key -days 360 -out slaves/cert.cer 


#echo 'IT will now be converted into X509'
#openssl x509 -in slaves/cert.cer -out slaves/certx509.cer

echo ' Convert the certificate into PEM format (slaves/cert.pem).'
openssl x509 -in slaves/cert.cer -out slaves/cert.pem -outform PEM


echo ' Collect files and copy it into a fresh directory...'

i=1
while true
	do

	if [ -d slaves/cert$i  ]
	then
		echo slaves/cert$i already exists...
	else
	        echo Using directory: slaves/cert$i
		mkdir slaves/cert$i

		cp slaves/key.pem slaves/cert$i/key$i.pem
		cp slaves/cert.cer slaves/cert$i/cert$i.cer
                cp slaves/cert.pem slaves/cert$i/cert$i.pem
		cp certs/ca.cer slaves/cert$i/ca.cer
		cp certs/ca.pem slaves/cert$i/ca.pem
		cp requests/myreq.pem slaves/cert$i/request$i.pem
		break
	fi
	#dd if=/dev/rmt12 of=file$i ibs=3200 conv=unblock,lcase cbs=80 || break
	i=`expr $i + 1`
done






