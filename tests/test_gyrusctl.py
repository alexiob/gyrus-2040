"""Protocol client framing and image integrity regressions, standard library only."""
import importlib.util
import json
from pathlib import Path
import struct
import unittest
import zlib
spec = importlib.util.spec_from_file_location('gyrusctl', Path(__file__).parents[1] / 'tools/gyrusctl.py')
gyrusctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gyrusctl)


class ClientTests(unittest.TestCase):
    def test_frame_crc_palette_and_png(self):
        payload = bytes([0, 0, 255, 255, 0, 1, 1, 0])
        header = dict(v=1, id=1, ok=True, format='indexed8', width=2, height=2,
                      palette_bytes=4, pixel_bytes=4, crc32=zlib.crc32(payload))
        stream = bytearray(json.dumps(header).encode() + b'\n' + payload + b'\n')
        writes = []
        def read(count, timeout):
            del timeout
            # Deliberately fragment headers and frame payloads.
            count = min(count, 2)
            result = bytes(stream[:count]); del stream[:count]
            return result
        response, png = gyrusctl.Client(read, writes.append).request('FRAME')
        self.assertEqual(writes, [b'G1 1 FRAME\n'])
        self.assertEqual(response, header)
        self.assertTrue(png.startswith(b'\x89PNG\r\n\x1a\n'))
        self.assertEqual(struct.unpack('>II', png[16:24]), (2, 2))
        with self.assertRaisesRegex(gyrusctl.ProtocolError, 'CRC32'):
            gyrusctl.png_from_frame(header, payload[:-1] + b'\x01')

    def test_bad_frame_bounds_and_indices(self):
        for width, payload in [(241, b'\0\0\0'), (1, b'\0\0\1')]:
            header = dict(width=width, height=1, palette_bytes=2, pixel_bytes=1, crc32=zlib.crc32(payload))
            with self.assertRaises(gyrusctl.ProtocolError):
                gyrusctl.png_from_frame(header, payload)

    def test_rejects_wrong_response_id(self):
        stream = bytearray(b'{"v":1,"id":99,"ok":true}\n')
        def read(count, timeout):
            del timeout
            result = bytes(stream[:count]); del stream[:count]
            return result
        with self.assertRaisesRegex(gyrusctl.ProtocolError, 'ID mismatch'):
            gyrusctl.Client(read, lambda data: None).request('STATE')

    def test_rejects_line_injection_before_writing(self):
        writes = []
        with self.assertRaises(gyrusctl.ProtocolError):
            gyrusctl.Client(lambda n, t: b'', writes.append).request('STATE\nSET score 1')
        self.assertEqual(writes, [])

if __name__ == '__main__':
    unittest.main()
