#!/bin/bash

#ca_key
cat > ca_cert.conf <<EOF
[req]
distinguished_name=req_distinguished_name
prompt=no
[req_distinguished_name]
O=TLS Project Dodgy Certificate Authority
EOF
openssl genrsa -out ca.key 1024
openssl req -out ca.req -key ca.key -new -config ca_cert.conf
openssl x509 -req -in ca.req -out ca.crt -sha1 -days 5000 -signkey ca.key
openssl x509 -in ca.crt -outform DER -out ca.der

#server_key
cat > server_cert.conf <<EOF
[req]
distinguished_name=req_distinguished_name
prompt=no
[req_distinguished_name]
O=TLS Project
CN=192.168.10.37
EOF
openssl genrsa -out server.key 1024
openssl req -out server.req -key server.key -new -config server_cert.conf
openssl x509 -req -in server.req -out server.crt -sha1 -CAcreateserial -days 5000 -CA ca.crt -CAkey ca.key
openssl x509 -in server.crt -outform DER -out server.der

#ca_key
cat > client_cert.conf <<EOF
[req]
distinguished_name=req_distinguished_name
prompt=no
[req_distinguished_name]
O=TLS Project Device Certificate
CN=192.168.10.35
EOF
openssl genrsa -out client.key 1024
openssl req -out client.req -key client.key -new -config client_cert.conf
openssl x509 -req -in client.req -out client.crt -sha1 -CAcreateserial -days 5000 -CA ca.crt -CAkey ca.key
openssl x509 -in client.crt -outform DER -out client.der

mkdir ca server client certDER
mv ca.crt ca.key ca/
mv server.crt server.key server/
mv client.crt client.key client/
mv ca.der server.der client.der certDER/
rm *.conf
rm *.req
rm *.srl
