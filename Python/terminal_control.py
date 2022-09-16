from osa_lib import OsaPlotter

osa = OsaPlotter()

while True:
    value = input("Podaj pozycję: ")
    x, y, z = None, None, None
    try:
        value = value.split(", ")
        x = int(value[0])
        y = int(value[1])
        z = int(value[2])
    except Exception:
        break
    osa.set_position(x, y, z)
    osa.execute_cmd_buffer()
    print(osa.get_position())