import yaml
import time
import serial
from serial.tools import list_ports


class Command():
    PING = 0
    CONNECT = 1
    HOME = 2
    GET_POS = 16
    SET_POS = 17
    SEND_BUFFER = 18
    SET_MOVE_SPEED = 19
    NULL = 127


class Connection:

    def __init__(self, baudrate: int = 9600, port: str = None):
        if port is None:
            for port in list_ports.comports():
                print(port.device)
                raise NotImplementedError()

        self._ser = serial.Serial(port, baudrate)
        # Delay is needed in order to allow the arduino serial to reboot
        time.sleep(2)
        self.connect()

    def connect(self):
        print(f"Connected to device: {self.send_command(Command.PING)}")
        self.send_command(Command.CONNECT)

    def send_command(self, cmd_code: Command):
        self.send_command_nonblocking(cmd_code)
        response = self.wait_for_response()

        # Handle arduino side errors:
        if response[:2] == "E0":
            raise Exception("Invalid command used")
        if response[:2] == "E1":
            raise Exception("Already connected")
        if response[:2] == "E2":
            raise Exception("Requested position out of range")
        if response[:2] == "E3":
            raise Exception("Command buffer too large")
        return response

    def send_command_nonblocking(self, cmd_code: Command):
        self._ser.read_all()
        self._ser.write(bytearray(f'{chr(cmd_code)}', 'ascii'))

    def wait_for_response(self):
        return self._ser.readline().decode("ascii")[:-1]

    def send_integer(self, name: str, value: int):
        self._ser.write(bytearray(f'{name}{value}', 'ascii'))

    def __del__(self):
        self._ser.close()


class CommandBuffer:

    def __init__(self):
        self.commands = []

    def clear(self):
        self.commands = []

    def addCommand(self, x: float, y: float, z: float):
        self.commands.append([x, y, z])

    def pop(self):
        result = self.commands[0]
        self.commands = self.commands[1:]
        return result

    def size(self):
        return len(self.commands)


class OsaPlotter:

    def __init__(self, port: str = "COM3"):
        self._con = Connection(9600, port)
        self.params = None
        self._load_params("Python/osa_params.yaml")
        self.cmd_buffer = CommandBuffer()
        self.home()
        self.position = self.get_position()
        self.set_move_speed(self.params["max_speed"] * 0.666)

    def _load_params(self, path):
        with open(path, 'r') as stream:
            try:
                self.params = yaml.safe_load(stream)
            except yaml.YAMLError as e:
                print(e)

    def get_position(self):
        response = self._con.send_command(Command.GET_POS).split("/")
        x = int(response[0]) / self.params["axes"]["x"]["steps_per_mm"]
        y = int(response[1]) / self.params["axes"]["y"]["steps_per_mm"]
        z = int(response[2]) / self.params["axes"]["z"]["steps_per_mm"]
        return x, y, z

    def set_position(self, x: float, y: float, z: float):
        if x > self.params["axes"]["x"]["length"] or y > self.params["axes"][
                "y"]["length"] or z > self.params["axes"]["z"][
                    "length"] or x < 0 or y < 0 or z < 0:
            raise Exception("Requested position out of range!")
        self.cmd_buffer.addCommand(x, y, z)
        if self.cmd_buffer.size() == self.params["cmd_buffer_max_size"]:
            self.execute_cmd_buffer()

    def set_move_speed(self, value: float):
        if value > self.params["max_speed"]:
            raise Exception["Requested speed exceeds limit"]
        period = 1000000 / (self.params["axes"]["x"]["steps_per_mm"] * value)
        self._con.send_command_nonblocking(Command.SET_MOVE_SPEED)
        self._con.send_integer("p", period)
        return self._con.wait_for_response()

    def execute_cmd_buffer(self):
        n = self.cmd_buffer.size()
        self._con.send_command_nonblocking(Command.SEND_BUFFER)
        self._con.send_integer("n", n)
        for i in range(n):
            x, y, z = self.cmd_buffer.pop()
            dx = (x -
                  self.position[0]) * self.params["axes"]["x"]["steps_per_mm"]
            dy = (y -
                  self.position[1]) * self.params["axes"]["y"]["steps_per_mm"]
            dz = (z -
                  self.position[2]) * self.params["axes"]["z"]["steps_per_mm"]
            self._con.send_integer("dX", int(dx))
            self._con.send_integer("dY", int(dy))
            self._con.send_integer("dZ", int(dz))
            self.position = [x, y, z]
        return (self._con.wait_for_response())

    # def send_position_cmd(self, x: float, y: float, z: float):
    #     self._con.send_command_nonblocking(Command.SET_POS)
    #     dx = (x - self.position[0]) * self.params["axes"]["x"]["steps_per_mm"]
    #     dy = (y - self.position[1]) * self.params["axes"]["y"]["steps_per_mm"]
    #     dz = (z - self.position[2]) * self.params["axes"]["z"]["steps_per_mm"]
    #     self._con.send_integer("dX", int(dx))
    #     self._con.send_integer("dY", int(dy))
    #     self._con.send_integer("dZ", int(dz))
    #     self.position = [x, y, z]
    #     return (self._con.wait_for_response())

    def home(self):
        return (self._con.send_command(Command.HOME))

    def __del__(self):
        self.execute_cmd_buffer()
        self.home()
