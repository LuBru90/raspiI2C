import time
import board
import busio

from adafruit_htu21d import HTU21D

i2c = busio.I2C(board.SCL, board.SDA)
sensor = HTU21D(i2c)

print(sensor.temperature)
print(sensor.relative_humidity)

