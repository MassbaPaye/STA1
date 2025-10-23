import socket
import struct
import time

# ===============================
# === Types de messages (comme en C)
# ===============================
MESSAGE_ITINERAIRE = 4
MESSAGE_POSITION   = 1
MESSAGE_FIN        = 5

# ===============================
# === Structures de données
# ===============================

# Structure Point_iti (4 floats + 2 ints)
# float x, y, z, theta; int pont, depacement
POINT_ITI_STRUCT = struct.Struct("<4f2i")

# Structure Itineraire (int + n * Point_iti)
def serialize_itineraire(itineraire):
    """
    itineraire = {
        "nb_points": int,
        "points": [(x, y, z, theta, pont, depacement), ...]
    }
    """
    data = struct.pack("<i", itineraire["nb_points"])
    for p in itineraire["points"]:
        data += POINT_ITI_STRUCT.pack(*p)
    return data

# Structure PositionVoiture (7 floats)
POSITION_VOITURE_STRUCT = struct.Struct("<7f")

def parse_position_voiture(data):
    x, y, z, theta, vx, vy, vz = POSITION_VOITURE_STRUCT.unpack(data)
    return {
        "x": x, "y": y, "z": z,
        "theta": theta, "vx": vx, "vy": vy, "vz": vz
    }

# ===============================
# === Communication (header)
# ===============================
HEADER_STRUCT = struct.Struct("<iiQ")  # int voiture_id, int type, size_t payload_size

def send_message(sock, voiture_id, msg_type, payload_bytes):
    header = HEADER_STRUCT.pack(voiture_id, msg_type, len(payload_bytes))
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
    voiture_id, msg_type, payload_size = HEADER_STRUCT.unpack(header_bytes)
    payload = recv_all(sock, payload_size)
    return voiture_id, msg_type, payload

# ===============================
# === Client principal
# ===============================
def main():
    host = "127.0.0.1"
    port = 5001

    print(f"[IHM] Connexion au serveur {host}:{port}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    print("[IHM] Connecté au serveur ✅")

    try:
        # ---- Envoi d’un itinéraire pour voiture 1 ----
        itineraire = {
            "nb_points": 3,
            "points": [
                (0.0, 0.0, 0.0, 0.0, 0, 0),
                (1000.0, 0.0, 0.0, 0.0, 1, 0),
                (1000.0, 1000.0, 0.0, 90.0, 0, 1),
            ]
        }
        payload = serialize_itineraire(itineraire)
        send_message(sock, voiture_id=1, msg_type=MESSAGE_ITINERAIRE, payload_bytes=payload)
        print("[IHM] Itinéraire envoyé pour voiture 1 🚗")

        # ---- Attente de position envoyée par le serveur ----
        while True:
            result = recv_message(sock)
            if result == (None, None, None):
                print("[IHM] Connexion fermée par le serveur.")
                break

            voiture_id, msg_type, payload = result

            if msg_type == MESSAGE_POSITION:
                pos = parse_position_voiture(payload)
                print(f"[IHM] Position reçue pour voiture {voiture_id} : {pos}")

            elif msg_type == MESSAGE_FIN:
                message = payload.decode(errors="ignore")
                print(f"[IHM] Message de fin reçu : {message}")
                break

            else:
                print(f"[IHM] Message inconnu reçu (type={msg_type})")

            time.sleep(0.5)

        # ---- Fin propre ----
        print("[IHM] Envoi d’un message de fin...")
        send_message(sock, voiture_id=0, msg_type=MESSAGE_FIN, payload_bytes=b"fin")

    finally:
        sock.close()
        print("[IHM] Connexion fermée proprement ✅")


if __name__ == "__main__":
    main()
