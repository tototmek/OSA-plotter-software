from osa_lib import OsaPlotter
import math

osa = OsaPlotter()
osa.set_move_speed(33)

for i in range(132):
    osa.set_position(
        math.sin(i / 20) * 30 + 35,
        math.cos(i / 20 + 3.14) * 30 + 35, 36)
