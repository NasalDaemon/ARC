"""Where the generator keeps its parser cache, and what it refuses to trust."""

import os
import stat
import tempfile
import unittest
from pathlib import Path

from support import generate


@unittest.skipUnless(hasattr(os, "getuid"), "the parser cache needs a uid to own it")
class ParserCache(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.cache_dir = Path(self.tmp.name) / f"arc-generator-{os.getuid()}"

    def tearDown(self):
        self.tmp.cleanup()

    def test_is_kept_in_a_directory_only_this_user_can_write(self):
        r = generate("plain() const -> int", self.tmp.name)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(stat.S_IMODE(os.lstat(self.cache_dir).st_mode) & 0o077, 0)
        self.assertEqual(len(list(self.cache_dir.iterdir())), 1)

    def test_is_not_written_into_a_directory_others_can_write(self):
        self.cache_dir.mkdir()
        self.cache_dir.chmod(0o777)
        r = generate("plain() const -> int", self.tmp.name)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(list(self.cache_dir.iterdir()), [])

    def test_is_not_read_through_a_link_planted_in_its_place(self):
        generate("plain() const -> int", self.tmp.name)
        (cached,) = self.cache_dir.iterdir()
        planted = Path(self.tmp.name) / "planted"
        planted.write_bytes(b"not a parser")
        cached.unlink()
        cached.symlink_to(planted)
        r = generate("plain() const -> int", self.tmp.name)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertTrue(cached.is_symlink())
        self.assertEqual(planted.read_bytes(), b"not a parser")


if __name__ == "__main__":
    unittest.main()
