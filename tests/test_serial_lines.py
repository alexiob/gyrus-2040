"""The reader must preserve partial JSON yet bound noisy/unterminated input."""
import unittest
from tools.serial_lines import LineReader

class Clock:
    def __init__(self): self.now=0.0
    def __call__(self): return self.now

class Stream:
    def __init__(self, clock, chunks=None):
        self.clock=clock; self.chunks=iter(chunks) if chunks is not None else None
        self.timeout=0.05
    def read(self, size):
        chunk=next(self.chunks,b'') if self.chunks is not None else b'x'*size
        self.clock.now+=0.1 if not chunk else 0.001
        return chunk

class LineReaderTests(unittest.TestCase):
    def test_preserves_fragment_across_timeout(self):
        clock=Clock();reader=LineReader(Stream(clock,[b'abc',b'',b'\n']),clock=clock)
        self.assertIsNone(reader.read_line(0.05))
        self.assertEqual(reader.read_line(1.0),b'abc')
    def test_unterminated_stream_cannot_grow_without_limit(self):
        clock=Clock();reader=LineReader(Stream(clock),clock=clock,max_line=128)
        with self.assertRaisesRegex(RuntimeError,'exceeds'):reader.read_line(1.0)
    def test_continuous_bytes_still_obey_overall_deadline(self):
        clock=Clock();reader=LineReader(Stream(clock),clock=clock)
        self.assertIsNone(reader.read_line(0.003))
        self.assertLessEqual(clock.now,0.004)
    def test_multiple_lines_in_one_chunk(self):
        clock=Clock();reader=LineReader(Stream(clock,[b'one\ntwo\n']),clock=clock)
        self.assertEqual(reader.read_line(1.0),b'one')
        self.assertEqual(reader.read_line(1.0),b'two')
