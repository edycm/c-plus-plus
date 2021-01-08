#!/bin/bash

openssl version >/dev/null 2>&1
if [ $? -eq 127 ]; then
	echo "Openssl is not installed!"
	exit
fi

[ ! -d ca ] && mkdir ca

SERVER_IP=127.0.0.1
CLIENT_IP=127.0.0.1

#ca
openssl genrsa -out ca/ca.key 2048
openssl req -x509 -new -days 365 -key ca/ca.key -out ca/ca.crt -subj "/C=CN/ST=SiChuan/L=ChengDu/O=Device Certificate/OU=IOT/CN=127.0.0.1"
openssl x509 -in ca/ca.crt -out ca/ca.pem

#server
openssl genrsa -out ca/server.key 2048
openssl req -new -key ca/server.key -out ca/server.csr -subj "/C=CN/ST=SiChuan/L=ChengDu/O=Project server Certificate/OU=IOT/CN=$SERVER_IP"
openssl x509 -req -in ca/server.csr -out ca/server.crt -CAkey ca/ca.key -CA ca/ca.crt -days 365 -sha1 -extensions v3_req -CAcreateserial -CAserial ca/server.srl
cat ca/server.crt ca/server.key >ca/server.pem

#client
openssl genrsa -out ca/client.key 2048
openssl req -new -key ca/client.key -out ca/client.csr -subj "/C=CN/ST=SiChuan/L=ChengDu/O=Device client Certificate/OU=IOT/CN=$CLIENT_IP"
openssl x509 -req -in ca/client.csr -out ca/client.crt -CAkey ca/ca.key -CA ca/ca.crt -days 365 -sha1 -extensions v3_req -CAcreateserial -CAserial ca/client.srl
cat ca/client.crt ca/client.key >ca/client.pem

openssl dhparam -out ca/dh.pem 2048
