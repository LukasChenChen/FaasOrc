## a http server that listen on localhost:3333

import http.client
import json
import socket

def run_server():
    # Define socket host and port
    SERVER_HOST = '0.0.0.0'
    SERVER_PORT = 3333

    # Create socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((SERVER_HOST, SERVER_PORT))
    server_socket.listen(1)
    print('Listening on port %s ...' % SERVER_PORT)

    while True:    
        # Wait for client connections
        client_connection, client_address = server_socket.accept()

        # Get the client request
        request = client_connection.recv(1024).decode()
        print(request)

        # Send HTTP response
        response = 'HTTP/1.0 200 OK\n\nResults Received'
        client_connection.sendall(response.encode())
        client_connection.close()
        break

    # Close socket
    server_socket.close()


def main():
    run_server()

if __name__ == "__main__":
    main()
