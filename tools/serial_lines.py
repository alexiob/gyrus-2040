"""Bounded serial line receiver; partial input survives overall deadline expiry."""
import time

class LineReader:
    def __init__(self, device, clock=time.monotonic, max_line=4096):
        if max_line < 1:
            raise ValueError('max_line must be positive')
        self.device = device
        self.clock = clock
        self.max_line = max_line
        self.buffer = bytearray()

    def read_line(self, deadline):
        while True:
            newline = self.buffer.find(b'\n')
            if newline >= 0:
                if newline > self.max_line:
                    raise RuntimeError('Serial line exceeds configured limit')
                line = bytes(self.buffer[:newline])
                del self.buffer[:newline + 1]
                return line
            if len(self.buffer) > self.max_line:
                raise RuntimeError('Serial line exceeds configured limit')
            remaining = deadline - self.clock()
            if remaining <= 0:
                return None
            self.device.timeout = min(0.05, remaining)
            chunk = self.device.read(min(128, self.max_line + 1 - len(self.buffer)))
            self.buffer.extend(chunk)
