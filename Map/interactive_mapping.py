import tkinter as tk
from PIL import Image, ImageTk, ImageDraw
import math
import os

# --- CONFIG ---
MAP_PATH = "map_centrale.png"
SAVE_PATH = "map_troncons.png"
POINT_RADIUS = 5
POINT_COLOR = "blue"
LINE_COLOR = "cyan"
ARC_COLOR = "green"
LINE_TOLERANCE = 10  # snap sur ligne

class MapEditor:
    def __init__(self, master):
        self.master = master
        self.master.title("Éditeur de tronçons")
        
        # Chargement image
        if not os.path.exists(MAP_PATH):
            raise FileNotFoundError(MAP_PATH)
        self.img_pil = Image.open(MAP_PATH).convert("RGB")
        self.img_draw = self.img_pil.copy()
        self.draw = ImageDraw.Draw(self.img_draw)
        self.tk_img = ImageTk.PhotoImage(self.img_draw)
        
        # Canvas
        self.canvas = tk.Canvas(master, width=self.img_pil.width, height=self.img_pil.height)
        self.canvas.pack()
        self.image_on_canvas = self.canvas.create_image(0,0,anchor="nw", image=self.tk_img)
        
        # Données
        self.points = []       # [(x,y)]
        self.lines = []        # [(pt1_index, pt2_index)]
        self.arcs = []         # [(pt1_index, pt2_index, rayon)]
        self.circles = []         # [(centre_x, centre_y, rayon)]
        self.selected_points = []  # liste pour ordre stable

        # Bind events
        self.canvas.bind("<Button-1>", self.left_click)
        self.master.bind("<Return>", self.create_segment_or_arc)
        self.master.bind("s", self.save_image)
        self.master.bind("<Delete>", self.delete_selected_points)
        
        self.update_canvas()
    
    # --- UTILS ---
    def distance_point_to_segment(self, px, py, x1, y1, x2, y2):
        line_mag = math.hypot(x2 - x1, y2 - y1)
        if line_mag < 1e-6:
            return math.hypot(px - x1, py - y1), (x1, y1)
        u = ((px - x1)*(x2 - x1) + (py - y1)*(y2 - y1)) / (line_mag**2)
        u = max(0, min(1, u))
        ix = x1 + u*(x2 - x1)
        iy = y1 + u*(y2 - y1)
        return math.hypot(px - ix, py - iy), (ix, iy)
    
    # --- EVENTS ---
    def left_click(self, event):
        x, y = event.x, event.y
        # Vérifier si proche d'un point existant
        for i, (px, py) in enumerate(self.points):
            if math.hypot(px-x, py-y) <= POINT_RADIUS*2:
                if i in self.selected_points:
                    self.selected_points.remove(i)
                    print(f"[INFO] Point désélectionné : {i}")
                else:
                    self.selected_points.append(i)
                    print(f"[INFO] Point sélectionné : {i}")
                self.update_canvas()
                return
        # Snap sur ligne
        for idx1, idx2 in self.lines:
            p1 = self.points[idx1]
            p2 = self.points[idx2]
            dist, proj = self.distance_point_to_segment(x, y, p1[0], p1[1], p2[0], p2[1])
            if dist <= LINE_TOLERANCE:
                x, y = proj
                break
        self.points.append((x, y))
        self.selected_points.append(len(self.points)-1)
        print(f"[INFO] Point ajouté et sélectionné : ({x},{y})")
        self.update_canvas()
        self.save_image()
    
    def create_segment_or_arc(self, event=None):
        if len(self.selected_points) == 2:
            self.create_line(self.selected_points[0], self.selected_points[1])
        elif len(self.selected_points) == 3:
            print("selected = ", self.selected_points)
            a, b, c = self.order_points(self.selected_points[0],self.selected_points[1],self.selected_points[2])
            print("order = ", a, b, c)
            obtus = self.is_angle_obtuse(a, b, c)
            self.create_arc(a, b, c, circle=not obtus)
        else:
            print("[WARN] Sélectionner 2 points (ligne) ou 3 points (arc)")
        self.selected_points.clear()
        self.update_canvas()
        self.save_image()
    
    def delete_selected_points(self, event=None):
        for idx in sorted(self.selected_points, reverse=True):
            print(f"[INFO] Point supprimé : {idx}")
            self.points.pop(idx)
            # Supprimer les lignes/arcs utilisant ce point
            self.lines = [l for l in self.lines if idx not in l]
            self.arcs = [a for a in self.arcs if idx not in a]
        self.selected_points.clear()
        self.update_canvas()
        self.save_image()
    
    # --- CREATION SEGMENT / ARC ---
    def create_line(self, idx1, idx2):
        p1 = self.points[idx1]
        p2 = self.points[idx2]
        self.draw.line([p1, p2], fill=LINE_COLOR, width=2)
        self.lines.append((idx1, idx2))
        print(f"[INFO] Ligne créée entre points {idx1}-{idx2}")
    
    def create_arc(self, a, b, c, circle=False):
        """
        Crée un arc ou un cercle complet :
        - a, b, c : indices dans self.points
        - circle : True → cercle complet, False → arc minimal passant par a->b->c
        """
        p1 = self.points[a]
        p2 = self.points[b]
        p3 = self.points[c]

        circle_data = self.compute_center_and_radius(p1, p2, p3)
        if circle_data is None:
            print("[WARN] Impossible de créer un arc (points alignés ?)")
            return

        cx, cy, r = circle_data

        if circle:
            # Cercle complet
            self.circles.append((cx, cy, r))
            # Dessin immédiat
            bbox = [cx-r, cy-r, cx+r, cy+r]
            self.draw.ellipse(bbox, outline=ARC_COLOR, width=2)
            print(f"[INFO] Cercle complet créé : centre ({cx:.1f},{cy:.1f}), r={r:.1f}")
        else:
            # Calcul rayon signé pour indiquer le sens
            # Positif si le point p2 est à gauche de la ligne p1->p3
            dx, dy = p3[0]-p1[0], p3[1]-p1[1]
            det = (p2[0]-p1[0])*dy - (p2[1]-p1[1])*dx
            signed_r = r if det >=0 else -r

            # Stockage minimal
            self.arcs.append((a, c, signed_r))

            # Dessin immédiat
            start_angle = math.degrees(math.atan2(p1[1]-cy, p1[0]-cx))
            end_angle   = math.degrees(math.atan2(p3[1]-cy, p3[0]-cx))
            bbox = [cx-r, cy-r, cx+r, cy+r]
            self.draw.arc(bbox, start=start_angle, end=end_angle, fill=ARC_COLOR, width=2)

            print(f"[INFO] Arc créé : points {a}-{b}, r={signed_r:.1f}")



    def compute_center_and_radius(self, p1, p2, p3):
        x1, y1 = p1
        x2, y2 = p2
        x3, y3 = p3
        temp = x2**2 + y2**2
        bc = (x1**2 + y1**2 - temp) / 2
        cd = (temp - x3**2 - y3**2) / 2
        det = (x1 - x2)*(y2 - y3) - (x2 - x3)*(y1 - y2)
        if abs(det) < 1e-6:
            return None
        cx = (bc*(y2 - y3) - cd*(y1 - y2)) / det
        cy = ((x1 - x2)*cd - (x2 - x3)*bc) / det
        r = math.hypot(cx - x1, cy - y1)
        return (cx, cy, r)
    
    def is_angle_obtuse(self, a, b, c):
        xa, ya = self.points[a]
        xb, yb = self.points[b]
        xc, yc = self.points[c]
        BA = (xa - xb, ya - yb)
        BC = (xc - xb, yc - yb)
        dot = BA[0]*BC[0] + BA[1]*BC[1]
        return dot < 0
    
    def sens_rotation(self, a, b, c):
        """
        Détermine le sens de rotation passant par les points a -> b -> c.
        Si l'angle est positif (rotation anti-horaire), renvoie (a, b, c)
        Sinon (horaire), renvoie (c, b, a)
        """
        xa, ya = self.points[a]
        xb, yb = self.points[b]
        xc, yc = self.points[c]

        # Vecteurs AB et BC
        ABx, ABy = xb - xa, yb - ya
        BCx, BCy = xc - xb, yc - yb

        # Produit vectoriel 2D (détermine le sens de rotation)
        cross = ABx * BCy - ABy * BCx

        if cross >= 0:
            return a, b, c  # rotation anti-horaire (positive)
        else:
            return c, b, a  # rotation horaire (négative)

    

    def order_points(self, a, b, c):
        def dist(u, v):
            xu, yu = self.points[u]
            xv, yv = self.points[v]
            return math.sqrt((xu-xv)**2 + (yu-yv)**2)
        print("dist(a, b)=", dist(a, b), ", dist(b, c)=", dist(b, c), ", dist(a, c)=", dist(a, c))
        if dist(a, b) > dist(b, c):
            if dist(a, b) > dist(a, c):
                ord = a, c, b
            else:
                ord = a, b, c
        else:
            if dist(b, c) > dist(a, c):
                ord = b, a, c
            else:
                ord = a, b, c
        return self.sens_rotation(*ord)


    
    def update_canvas(self):
        temp_img = self.img_draw.copy()
        draw_temp = ImageDraw.Draw(temp_img)

        # Droites
        for idx1, idx2 in self.lines:
            draw_temp.line([self.points[idx1], self.points[idx2]], fill="cyan", width=2)

        # Arcs
        for a, b, signed_r in self.arcs:
            p1 = self.points[a]
            p2 = self.points[b]
            # Recalculer centre du cercle à partir des deux points et du rayon signé
            dx, dy = p2[0]-p1[0], p2[1]-p1[1]
            d = math.hypot(dx, dy)
            if d/2 > abs(signed_r):
                continue  # rayon trop petit pour ces points

            # Centre du cercle
            mx, my = (p1[0]+p2[0])/2, (p1[1]+p2[1])/2
            h = math.sqrt(abs(signed_r**2 - (d/2)**2))
            # vecteur perpendiculaire
            ux, uy = -dy/d* h, dx/d* h
            if signed_r < 0:
                ux, uy = -ux, -uy
            cx, cy = mx+ux, my+uy

            start_angle = math.degrees(math.atan2(p1[1]-cy, p1[0]-cx))
            end_angle   = math.degrees(math.atan2(p2[1]-cy, p2[0]-cx))
            bbox = [cx-abs(signed_r), cy-abs(signed_r), cx+abs(signed_r), cy+abs(signed_r)]
            draw_temp.arc(bbox, start=start_angle, end=end_angle, fill=ARC_COLOR, width=2)

        # Cercles complets
        for cx, cy, r in self.circles:
            draw_temp.ellipse([cx-r, cy-r, cx+r, cy+r], outline=ARC_COLOR, width=2)

        # Points
        for idx, (x, y) in enumerate(self.points):
            draw_temp.ellipse([x-POINT_RADIUS, y-POINT_RADIUS, x+POINT_RADIUS, y+POINT_RADIUS],
                            fill=POINT_COLOR)

        # Points sélectionnés
        for idx in self.selected_points:
            x, y = self.points[idx]
            draw_temp.ellipse([x-POINT_RADIUS, y-POINT_RADIUS, x+POINT_RADIUS, y+POINT_RADIUS],
                            outline="yellow", width=2)

        self.tk_img = ImageTk.PhotoImage(temp_img)
        self.canvas.itemconfig(self.image_on_canvas, image=self.tk_img)

    
    def save_image(self, event=None):
        self.img_draw.save(SAVE_PATH)
        print(f"[INFO] Image sauvegardée : {SAVE_PATH}")

if __name__=="__main__":
    root = tk.Tk()
    editor = MapEditor(root)
    root.mainloop()
