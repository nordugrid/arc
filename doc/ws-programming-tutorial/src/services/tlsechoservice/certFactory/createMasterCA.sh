#!/bin/sh
#
#     Script to create a new self-signed certifacte authority. 
#
# BASED ON
#    http://www.dylanbeattie.net/docs/openssl_iis_ssl_howto.html

echo ' This script will create a new self-signed certificate authority.'
echo ' Following files will be created (or overwritten):'
echo ' - database'
echo ' - serial'
echo ' - slaves/'
echo ' - requests/'
echo ' - keys/ca,key'
echo ' - certs/ca.cer'
echo ' - certs/ca.pem'
echo ''
echo 'WARNING: The old CA and all child certificate will be replaced completly !'
echo ''
echo    '         Press Strg+C to abort!'
echo ''
read -p "         Press a key to proceed"

echo ''
echo ''
echo ' Removing directories...'
echo '   keys/'
rm -f -r keys

echo '   certs/'
rm -f -r certs

echo '   slaves/'
rm -f -r slaves

echo '   requests/'
rm -f -r requests


echo ' Remove files...'
echo '   database.*'
rm -f database.*

echo '   serial.*'
rm -f serial.*


echo ' Create new directories...'
echo '   keys/'
mkdir keys
echo '   certs/'
mkdir certs


echo ' Create files...'
echo '   database.txt'
touch database.txt

echo '   serial.txt'
echo '01' > serial.txt



echo ' Create private and public key for CA (keys/ca.key).'
openssl genrsa -des3 -out keys/ca.key 1024

echo ' Create certificate for the CA (certs/ca.cer).'
openssl req -config openssl.conf -new -x509 -days 1001 -key keys/ca.key -out certs/ca.cer


#openssl x509 -in certs/ca.cer -out certs/cax509.cer
echo ' Convert certificate into PEM format (certs/ca.pem).'
openssl x509 -in certs/ca.cer -out certs/ca.pem -outform PEM




