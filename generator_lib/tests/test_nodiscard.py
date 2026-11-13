"""What the generator accepts and emits for [[nodiscard]] trait methods, and what it refuses."""

import unittest

from support import generate


class NodiscardAccepted(unittest.TestCase):
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


class NodiscardRefused(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
