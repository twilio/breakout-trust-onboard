#!/usr/bin/python3

from http.server import HTTPServer,BaseHTTPRequestHandler
import socket
import ssl
import argparse
import sys

class MiasExampleHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        resp = '{"hello":"mias"}\n'
        self.send_response(200)
        self.send_header('Content-type','application/json')
        self.send_header('Content-Length',len(resp))
        self.end_headers()
        self.wfile.write(resp.encode())

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="port to listen")
    parser.add_argument("certificate", help="server certificate in PEM format")
    parser.add_argument("pkey", help="server private key in PEM format")
    parser.add_argument("clientCA", help="client CA certificate chain")
    args = parser.parse_args()

    if sys.version_info >= (3,6):
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    else:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS)

    context.load_cert_chain(args.certificate, args.pkey)
    context.load_verify_locations(args.clientCA)
    context.verify_mode = ssl.CERT_REQUIRED

    httpd = HTTPServer(('', int(args.port)), MiasExampleHandler)
    httpd.socket = context.wrap_socket (httpd.socket, server_side=True)
    httpd.serve_forever()

if __name__ == "__main__":
    main()
