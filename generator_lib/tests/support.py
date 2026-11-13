"""Shared by the generator's tests, each run as: <test>.py <generator command...>"""

import os
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
    return generate_source(source, tmpdir)


def generate_source(source: str, tmpdir: str | None = None) -> subprocess.CompletedProcess:
    source = textwrap.dedent(source).strip() + "\n"
    env = dict(os.environ, TMPDIR=tmpdir) if tmpdir else None
    with tempfile.TemporaryDirectory() as tmp:
        arc = Path(tmp) / "subject.hxx.arc"
        arc.write_text(source)
        out = Path(tmp) / "subject.hxx"
        result = subprocess.run(GENERATOR + ["-q", "-i", str(arc), "-o", str(out)],
                                capture_output=True, text=True, env=env)
        result.generated = out.read_text() if out.exists() else ""
    return result



# A cluster declaring trunk AB over traits A to D, each exposed by its node, with `trunks` as further header annotations
def provider(trunks: str) -> str:
    return f"""
        cluster test::Provider [Trunk AB = A + B, {trunks}]
        {{
            abcd = node::ABCD

            [A] .. --> abcd
            [B] .. --> abcd
            [C] .. --> abcd
            [D] .. --> abcd
        }}
    """



class Accepts(unittest.TestCase):
    def generate(self, source: str) -> str:
        r = generate_source(source)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        return r.generated


class Refuses(unittest.TestCase):
    def assertRefused(self, source: str, *messages: str):
        r = generate_source(source)
        self.assertNotEqual(r.returncode, 0, r.generated)
        for message in messages:
            self.assertIn(message, r.stderr + r.stdout)
