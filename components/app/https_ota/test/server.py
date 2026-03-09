import http.server
import ssl
import os

PORT = 8070

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[{self.log_date_time_string()}] Request from {self.client_address[0]}: {format % args}")

print("=================================================")
print(f"Starting HTTPS OTA Server...")

httpd = http.server.HTTPServer(('0.0.0.0', PORT), MyHandler)

try:
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile="ca_cert.pem", keyfile="server_key.pem")
    
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
except FileNotFoundError as e:
    print(f"ERROR: Certificate files not found! Make sure ca_cert.pem and server_key.pem are in this directory.")
    print(f"Details: {e}")
    exit(1)

print(f" Server successfully started!")
print(f" Firmware directory: {os.getcwd()}")
print(f" OTA Command URL: https://<YOUR_IP>:{PORT}/<your_firmware_name>.bin")
print("=================================================\n")

try:
    httpd.serve_forever()
except KeyboardInterrupt:
    print("\n Server stopped by the user.")