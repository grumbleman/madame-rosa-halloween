"""Erzeugt Rahmen und Deckel (Sockel-Erweiterung) für die Wahrsagerin als STL.

Aufruf: python -I make_sockel.py   (benötigt numpy, shapely, trimesh, manifold3d, mapbox_earcut)
Rahmen wird mit der Klebekante (oben) nach oben gedruckt, Deckel mit der Platte unten.
"""
import numpy as np
import trimesh
from shapely.geometry import box

# Maße in mm
DECKEL_B, DECKEL_T = 155.0, 150.0   # Außenmaß Deckel
DECKEL_DICKE = 3.0
DECKEL_RADIUS = 5.0
RAND = 10.0                          # Rahmen ringsum kleiner als Deckel
WAND = 3.0
RAHMEN_H = 30.0
SPIEL = 0.3                          # Spiel zwischen Stecklippe und Rahmen innen
STECK_WAND = 2.0
STECK_H = 6.0
RASTER_H = 0.4                       # Rastnase außen an der Stecklippe
RASTER_Z = 2.2                       # Höhe der Nase über Deckelplatte
NUT_TIEFE = 0.5                      # Nut im Rahmen innen
NUT_Z = 2.0                          # Unterkante der Nut ab Rahmenunterkante
NUT_H = 1.8

# Vorderseite = lange Seite (y minimal): PIR-Fenster mittig, Schalllöcher links und rechts
PIR_FENSTER = 23.0
FENSTER_Z = RAHMEN_H / 2
LOCH_D = 3.0
LOCH_RASTER = 4.5                    # Abstand der Löcher (Sechseckraster)
LOCH_FELD_R = 12.0                   # Radius des runden Lochfelds
LOCH_X = 40.0                        # Abstand der Lochfeld-Mitten von der Mitte


def rrect(b, t, r):
    r = max(min(r, b / 2 - 0.01, t / 2 - 0.01), 0.01)
    return box(-b / 2 + r, -t / 2 + r, b / 2 - r, t / 2 - r).buffer(r, resolution=16)


def extrude(poly, h, z0=0.0):
    m = trimesh.creation.extrude_polygon(poly, h)
    m.apply_translation([0, 0, z0])
    return m


def diff(a, b):
    return a.difference(b, engine="manifold")


def union(a, b):
    return a.union(b, engine="manifold")


rahmen_b, rahmen_t = DECKEL_B - 2 * RAND, DECKEL_T - 2 * RAND
innen_b, innen_t = rahmen_b - 2 * WAND, rahmen_t - 2 * WAND

# --- Rahmen ---
aussen = rrect(rahmen_b, rahmen_t, 4.0)
innen = rrect(innen_b, innen_t, 1.0)
rahmen = extrude(aussen.difference(innen), RAHMEN_H)
nut = rrect(innen_b + 2 * NUT_TIEFE, innen_t + 2 * NUT_TIEFE, 1.5).difference(innen)
# Nut: unten gerade, oben mit Abschrägung (zwei Stufen) gegen Überhang
nut2 = rrect(innen_b + 2 * NUT_TIEFE * 0.5, innen_t + 2 * NUT_TIEFE * 0.5, 1.25).difference(innen)
rahmen = diff(rahmen, extrude(nut, NUT_H - 0.4, NUT_Z))
rahmen = diff(rahmen, extrude(nut2, 0.4, NUT_Z + NUT_H - 0.4))

# --- Öffnungen in der Vorderwand ---
y_vorn = -rahmen_t / 2 - 1.0
tiefe = WAND + 2.0
fenster = trimesh.creation.box([PIR_FENSTER, tiefe, PIR_FENSTER])
fenster.apply_translation([0, y_vorn + tiefe / 2, FENSTER_Z])
rahmen = diff(rahmen, fenster)
loecher = []
for zeile, dz in enumerate(np.arange(-LOCH_FELD_R, LOCH_FELD_R + 0.01, LOCH_RASTER * 0.866)):
    versatz = LOCH_RASTER / 2 if zeile % 2 else 0.0
    for dx in np.arange(-LOCH_FELD_R, LOCH_FELD_R + 0.01, LOCH_RASTER):
        x = dx + versatz
        if x * x + dz * dz > (LOCH_FELD_R - LOCH_D / 2) ** 2:
            continue
        for seite in (-1, 1):
            c = trimesh.creation.cylinder(radius=LOCH_D / 2, height=tiefe, sections=24)
            c.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
            c.apply_translation([seite * LOCH_X + x, y_vorn + tiefe / 2, FENSTER_Z + dz])
            loecher.append(c)
rahmen = diff(rahmen, trimesh.util.concatenate(loecher))
print("Schallloecher:", len(loecher))

# --- Deckel ---
platte = extrude(rrect(DECKEL_B, DECKEL_T, DECKEL_RADIUS), DECKEL_DICKE)
steck_b, steck_t = innen_b - 2 * SPIEL, innen_t - 2 * SPIEL
steck_aussen = rrect(steck_b, steck_t, 1.0)
steck_innen = rrect(steck_b - 2 * STECK_WAND, steck_t - 2 * STECK_WAND, 0.5)
lippe = extrude(steck_aussen.difference(steck_innen), STECK_H, DECKEL_DICKE)
# Rastnase: zwei Stufen (0,2 / 0,4 mm), unten mit Abschrägung
nase1 = rrect(steck_b + 2 * RASTER_H / 2, steck_t + 2 * RASTER_H / 2, 1.1).difference(steck_innen)
nase2 = rrect(steck_b + 2 * RASTER_H, steck_t + 2 * RASTER_H, 1.2).difference(steck_innen)
deckel = union(platte, lippe)
deckel = union(deckel, extrude(nase1, 0.4, DECKEL_DICKE + RASTER_Z))
deckel = union(deckel, extrude(nase2, 0.6, DECKEL_DICKE + RASTER_Z + 0.4))
deckel = union(deckel, extrude(nase1, 0.4, DECKEL_DICKE + RASTER_Z + 1.0))

for name, m in (("rahmen_30mm", rahmen), ("deckel", deckel)):
    m.export(f"{name}.stl")
    print(name, "watertight:", m.is_watertight, "Maße:", np.round(m.extents, 1))
