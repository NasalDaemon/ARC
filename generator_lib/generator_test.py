"""What the generator accepts and emits for [[nodiscard]] trait methods, and what it refuses.

Usage: generator_test.py <generator command...>
"""

import os
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

GENERATOR = sys.argv[1:]
del sys.argv[1:]


def generate(trait_body: str, tmpdir: str | None = None) -> subprocess.CompletedProcess:
    source = "trait test::trait::Subject\n{\n" + textwrap.indent(textwrap.dedent(trait_body).strip(), "    ") + "\n}\n"
    env = dict(os.environ, TMPDIR=tmpdir) if tmpdir else None
    with tempfile.TemporaryDirectory() as tmp:
        arc = Path(tmp) / "subject.hxx.arc"
        arc.write_text(source)
        out = Path(tmp) / "subject.hxx"
        result = subprocess.run(GENERATOR + ["-q", "-i", str(arc), "-o", str(out)],
                                capture_output=True, text=True, env=env)
        result.generated = out.read_text() if out.exists() else ""
    return result


class Accepted(unittest.TestCase):
    def test_every_call_surface_of_a_nodiscard_method_carries_it(self):
        r = generate("""
            [[nodiscard]] plain() const -> int
            [[nodiscard]] defaulted() const -> int { return 1; }
            template<class T>
            [[nodiscard]] ! explicitly(T value) const -> T
            [[nodiscard]] ? optionally() const -> int
            discardable() const -> int
        """)
        self.assertEqual(r.returncode, 0, r.stderr)
        for name in ("plain", "defaulted", "explicitly", "optionally"):
            self.assertIn(f"static void nodiscard(ARC_This_Trait::{name});", r.generated)
            self.assertRegex(r.generated, rf"ARC_DUCK_METHOD_NODISCARD\(\s*{name}\)")
        for name in ("plain", "defaulted", "optionally"):
            self.assertRegex(r.generated, rf"ARC_SIGNATURE_METHOD_HEAD_NODISCARD\(\s*{name}\)")
        self.assertIn("[[nodiscard]] ARC_INLINE constexpr decltype(auto)", r.generated)
        self.assertRegex(r.generated, r"ARC_SIGNATURE_METHOD_HEAD\(\s*discardable\)")
        self.assertRegex(r.generated, r"ARC_DUCK_METHOD\(\s*discardable\)")
        self.assertNotIn("nodiscard(ARC_This_Trait::discardable)", r.generated)

    def test_assertion_messages_name_the_method_without_the_attribute(self):
        r = generate("[[nodiscard]] plain() const -> int")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('"Subject::plain() const -> int"', r.generated)

    def test_every_overload_may_be_nodiscard(self):
        r = generate("""
            [[nodiscard]] scaled(int factor) const -> int
            [[nodiscard]] scaled(int factor, int offset) const -> int
        """)
        self.assertEqual(r.returncode, 0, r.stderr)


class Refused(unittest.TestCase):
    def assertRefused(self, trait_body: str, message: str):
        r = generate(trait_body)
        self.assertNotEqual(r.returncode, 0, r.generated)
        self.assertIn(message, r.stderr + r.stdout)

    def test_a_method_with_no_declared_return_type(self):
        self.assertRefused("[[nodiscard]] plain() const", "must declare a non-void return type")

    def test_a_void_method(self):
        self.assertRefused("[[nodiscard]] plain() -> void", "must declare a non-void return type")

    def test_a_deduced_return_type(self):
        self.assertRefused("[[nodiscard]] plain() -> auto", "must declare a non-void return type")

    def test_overloads_that_disagree(self):
        self.assertRefused("""
            [[nodiscard]] scaled(int factor) const -> int
            scaled(int factor, int offset) const -> int
        """, "must be [[nodiscard]] on every overload or on none")


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
