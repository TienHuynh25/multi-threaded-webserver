import socket
import sys
import threading
import os
import datetime
import mimetypes
import time
import pytz

#
class WebServer:
    def __init__(self, port, path, multithreaded=False):
        self.host = ""
        self.port = port
        self.path = path
        self.multithreaded = multithreaded
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        print(f"Server listening on {self.host}:{self.port}")

    def start(self):
        while True:
            try:
                client_socket, address = self.server_socket.accept()
                if self.multithreaded:
                    client_thread = threading.Thread(target=self.handle_client, args=(client_socket,))
                    client_thread.start()
                else:
                    self.handle_client(client_socket)
            except KeyboardInterrupt as ki:
                print("Keyboard Interrupt")
                sys.exit()
            except Exception as e:
                print(e)
                print("Error accepting connection")

    def handle_client(self, client_socket):
        data = client_socket.recv(1024).decode('utf-8')
        request_line = data.split()

        if len(request_line) >= 2:
            request_method, path = request_line[:2]

            if request_method == 'GET' or request_method == 'HEAD':
                filepath = os.path.join(self.path, path[1:])
                if os.path.exists(filepath):
                    if os.path.isdir(filepath):
                        # If the requested path is a directory, serve the directory listing
                        self.send_response(client_socket, 200, content=self.generate_directory_listing(filepath),
                                           method=request_method)
                    elif os.path.isfile(filepath):
                        # If it's a file, serve the file
                        self.send_response(client_socket, 200, filepath, request_method)
                else:
                    self.send_response(client_socket, 404)
            else:
                self.send_response(client_socket, 400)
        else:
            self.send_response(client_socket, 400)

        client_socket.close()

    def generate_directory_listing(self, dir_path):
        listing = f"<html><head><title>Index of {dir_path}</title></head><body><h1>Index of {dir_path}</h1><ul>"
        for item in os.listdir(dir_path):
            item_path = os.path.join(dir_path, item)
            item_type = "Directory" if os.path.isdir(item_path) else "File"
            listing += f"<li><a href='{item}'>{item}</a> ({item_type})</li>"
        listing += "</ul></body></html>"
        return listing
    def send_response(self, client_socket, status_code, filepath=None, method='GET', content=None):
        response_headers = self.get_headers(status_code, filepath, method)
        client_socket.send(response_headers.encode('utf-8'))

        if method == 'GET' and status_code == 200 and content is not None:
            client_socket.send(content.encode('utf-8'))
        elif method == 'GET' and status_code == 200 and filepath is not None:
            with open(filepath, 'rb') as file:
                client_socket.send(file.read())

    def get_headers(self, status_code, filepath=None, method='GET'):
        last_updated_pattern = "%a, %d %b %Y %H:%M:%S %Z"

        if status_code == 200:
            if filepath is not None:
                content_type, _ = mimetypes.guess_type(filepath)
                modified_timestamp = os.path.getmtime(filepath)
                modified_time = datetime.datetime.fromtimestamp(modified_timestamp,
                                                                tz=pytz.timezone("America/Winnipeg"))
                last_modified = modified_time.strftime(last_updated_pattern)
            else:
                # Set content type for directory listings
                content_type = 'text/html'
                last_modified = time.strftime(last_updated_pattern, time.gmtime())
        else:
            content_type = 'text/plain'
            last_modified = time.strftime(last_updated_pattern, time.gmtime())

        headers = f"HTTP/1.1 {status_code}\r\n"
        headers += f"Content-Type: {content_type}\r\n"
        headers += f"Last-Modified: {last_modified}\r\n"
        headers += "Server: CustomWebServer\r\n\r\n"

        return headers


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Simple Multi-Threaded Web Server")
    parser.add_argument("port", type=int, help="Port number to use")
    parser.add_argument("path", type=str, help="Path to serve")
    parser.add_argument("-m", "--multithreaded", action="store_true", help="Run as a multi-threaded application")
    args = parser.parse_args()

    web_server = WebServer(args.port, args.path, args.multithreaded)
    web_server.start()