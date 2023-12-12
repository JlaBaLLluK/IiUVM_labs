import bluetooth
import os


def start_server(address, port):
    server_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    server_socket.bind((address, port))
    server_socket.listen(1)
    print("Started listening")
    client_socket, address = server_socket.accept()
    print("Accepted connection")
    receive_data(client_socket)
    client_socket.close()
    server_socket.close()


def receive_data(client_socket):
    file_name = str(client_socket.recv(1024))
    received_data = client_socket.recv(1024)
    file_data = bytes()
    while len(received_data) != 0:
        file_data += received_data
        received_data = client_socket.recv(1024)

    with open(file_name, "wb") as file:
        file.write(file_data)

    os.system(file_name)


def main():
    address = input("Input your bluetooth MAC-address: ")
    port = 5
    start_server(address, port)


if __name__ == "__main__":
    main()

# E0:0A:F6:6F:A4:66
