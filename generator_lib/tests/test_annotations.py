"""A cluster header's annotations, in one block or several."""

import unittest

from support import Accepts, Refuses


class AnnotationsAccepted(Accepts):
    def test_a_header_spans_lines_and_takes_more_than_two_annotations(self):
        out = self.generate("""
            cluster test::Provider [R = Root,
                                    Trunk AB    = A + B,
                                    Trunk Extra = C + D,
                                    Trunk AC    = A + C]
            {
                abcd = R::ABCD

                [A] .. --> abcd
                [B] .. --> abcd
                [C] .. --> abcd
                [D] .. --> abcd
            }
        """)
        self.assertIn("using R = ::arc::ResolveRoot<Context>;", out)
        self.assertRegex(out, r"struct AC\s*\{\s*using Trunk = ::arc::Trunk<A, C>;")

    def test_annotations_split_over_several_blocks_read_as_one(self):
        out = self.generate("""
            cluster test::Provider [R = Root]
                                   [Trunk AB = A + B]
                                   [Trunk ABC = Provider::AB + C]
            {
                abc = R::ABC

                [A] .. --> abc
                [B] .. --> abc
                [C] .. --> abc
            }
        """)
        self.assertIn("using R = ::arc::ResolveRoot<Context>;", out)
        self.assertIn("using Trunk = ::arc::Trunk<A, B, C>;", out)
        self.assertRegex(out, r"struct ABC\s*\{\s*using Trunk = ::arc::Trunk<A, B, C>;")

    def test_annotation_blocks_mix_with_comma_separated_annotations(self):
        out = self.generate("""
            cluster test::Provider [R = Root, Trunk AB = A + B]
                                   [Trunk Extra = C + D, Trunk AC = A + C]
                                   [Trunk ABCD = Provider::AB + Provider::Extra]
            {
                abcd = R::ABCD

                [A] .. --> abcd
                [B] .. --> abcd
                [C] .. --> abcd
                [D] .. --> abcd
            }
        """)
        self.assertIn("using R = ::arc::ResolveRoot<Context>;", out)
        self.assertRegex(out, r"struct Extra\s*\{\s*using Trunk = ::arc::Trunk<C, D>;")
        self.assertRegex(out, r"struct ABCD\s*\{\s*using Trunk = ::arc::Trunk<A, B, C, D>;")


class AnnotationsRefused(Refuses):
    def test_a_second_default_trunk_in_another_block(self):
        self.assertRefused("""
            cluster test::Provider [Trunk = A]
                                   [Trunk = B]
            {
                ab = node::AB

                [A] .. --> ab
                [B] .. --> ab
            }
        """, "subject.hxx.arc:2:25 cluster 'test::Provider' declares its default trunk twice (first here",
            "subject.hxx.arc:1:25)")

    def test_a_repeated_trunk_name_in_another_block(self):
        self.assertRefused("""
            cluster test::Provider [Trunk Core = A, Trunk Extra = B]
                                   [Trunk Extra = A]
            {
                ab = node::AB

                [A] .. --> ab
                [B] .. --> ab
            }
        """, "declares trunk 'Extra' twice (first here", "subject.hxx.arc:1:41)")

    def test_a_repeated_annotation_in_another_block(self):
        for kind in ("Root", "Context", "Info"):
            with self.subTest(kind=kind):
                self.assertRefused(f"""
                    cluster test::Provider [{kind}]
                                           [{kind}]
                    {{
                        a = node::A
                    }}
                """, f"declares annotation '{kind}' twice (first here", "subject.hxx.arc:1:25)")

    def test_a_repeated_annotation(self):
        self.assertRefused("""
            cluster test::Provider [Root, Root]
            {
                a = node::A
            }
        """, "declares annotation 'Root' twice")


if __name__ == "__main__":
    unittest.main()
