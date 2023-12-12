import bluetooth
import os


def get_devices():
    devices = bluetooth.discover_devices(lookup_names=True)
    for i, device in enumerate(devices):
        print(f"{i + 1}. {device[1]}")

    return devices


def send_data(devices, receiver_num):
    uuid = 'd2d8d2ba-b30b-48a7-a844-734b1edb5482'
    receiver_address = devices[receiver_num - 1][0]
    service = bluetooth.find_service(uuid=uuid, address=receiver_address)
    receiver_port = 0
    if len(service) != 0:
        receiver_port = service[0]["port"]

    sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    sock.connect((receiver_address, receiver_port))
    path_to_file = input("Input name of file to send: ")
    file_name = path_to_file.split('\\')[-1]
    sock.send(file_name)
    with open(path_to_file, "rb") as file:
        read_data = file.read(1024)
        while len(read_data) != 0:
            sock.send(read_data)
            read_data = file.read(1024)

    sock.close()


def main():
    devices = get_devices()
    receiver_num = int(input("Select a device to send data to: "))
    send_data(devices, receiver_num)


if __name__ == "__main__":
    main()
