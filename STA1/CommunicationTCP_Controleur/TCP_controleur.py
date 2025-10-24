import socket
import struct
import time

MESSAGE_ITINERAIRE = 4
MESSAGE_POSITION   = 1
MESSAGE_FIN        = 5

POINT_STRUCT = struct.Struct("<4f2i")

POSITION_VOITURE_STRUCT = struct.Struct("<7f")

def parse_position_voiture(data):
    x, y, z, theta, vx, vy, vz = POSITION_VOITURE_STRUCT.unpack(data)
    return {
        "x": x, "y": y, "z": z,
        "theta": theta, "vx": vx, "vy": vy, "vz": vz
    }

HEADER_STRUCT = struct.Struct("<iiQ")

def serialize_itineraire(msg):
    """
    msg = {
        "voiture_id": int,
        "itineraire": {
            "nb_points": int,
            "points": [(x, y, z, theta, pont, depacement), ...]
        }
    }
    """
    data = struct.pack("<i", msg["voiture_id"])  # voiture_id
    iti = msg["itineraire"]
    data += struct.pack("<i", iti["nb_points"])  # nb_points
    for p in iti["points"]:
        data += POINT_STRUCT.pack(*p)
    return data


def send_message(sock, msg_type, payload_bytes):
    """
    Envoie un message sur la socket sans voiture_id.
    
    :param sock: socket TCP connectée
    :param msg_type: type du message (int)
    :param payload_bytes: données binaires à envoyer
    """
    header = HEADER_STRUCT.pack(msg_type, len(payload_bytes))
    sock.sendall(header + payload_bytes)

def recv_all(sock, size):
    data = b""
    while len(data) < size:
        packet = sock.recv(size - len(data))
        if not packet:
            return None
        data += packet
    return data

def recv_message(sock):
    header_bytes = recv_all(sock, HEADER_STRUCT.size)
    if not header_bytes:
        return None, None, None
    voiture_id, payload_size = HEADER_STRUCT.unpack(header_bytes)
    payload = recv_all(sock, payload_size)
    return voiture_id, payload

def main():
    host = "127.0.0.1"
    port = 5001

    print(f"[IHM] Connexion au serveur {host}:{port}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    print("[IHM] Connecté au serveur ✅")

    try:
        # ---- Envoi d’un itinéraire pour voiture 1 ----
        msg1 = {
            "voiture_id": 1,
            "itineraire": {
                "nb_points": 3,
                "points": [
                    (0.0, 0.0, 0.0, 0.0, 0, 0),
                    (1000.0, 0.0, 0.0, 0.0, 1, 0),
                    (1000.0, 1000.0, 0.0, 90.0, 0, 1),
                ]
            }
        }
        payload = serialize_itineraire(msg1)
        send_message(sock, voiture_id=1, payload_bytes=payload)

        # ---- Envoi d’un itinéraire pour voiture 1 ----
        msg2 = {
            "voiture_id": 2,
            "itineraire": {
                "nb_points": 4,
                "points": [
                    (0.0, 0.0, 0.0, 0.0, 0, 0),
                    (500.0, 200.0, 0.0, 15.0, 0, 0),
                    (1200.0, 400.0, 0.0, 30.0, 1, 0),
                    (1500.0, 1000.0, 0.0, 90.0, 0, 1),
                ]
            }
        }
        payload2 = serialize_itineraire(msg2)
        send_message(sock, voiture_id=2, payload_bytes=payload2)

        print("[IHM] Itinéraire envoyé pour voiture 1 🚗")

        # ---- Attente de position envoyée par le serveur ----
        while True:
            result = recv_message(sock)
            if result == (None,None):
                print("[IHM] Connexion fermée par le serveur.")
                break

            voiture_id, payload = result

            pos = parse_position_voiture(payload)
            print(f"[IHM] Position reçue pour voiture {voiture_id} : {pos}")
            
            time.sleep(0.5)

    finally:
        sock.close()
        print("[IHM] Connexion fermée proprement ✅")


if __name__ == "__main__":
    main()
