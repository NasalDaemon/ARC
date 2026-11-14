"""How the generator asserts a trait's requirements: `requires <constraint>` as the constraint itself, so a failure
gets the compiler's concept diagnostics, and `requires requires {...}` inside a requires-expression naming the
implementation."""

import re
import unittest

from support import generate_source


def generate_typed(requirement: str):
    return generate_source(f"""
        trait test::trait::Subject [Types]
        {{
            type Value
            {requirement}
            get() const -> Types::Value
        }}
    """)


# The requirement each assert of a `requires` line checks, in order
def asserted(generated: str) -> list[str]:
    return re.findall(r'ARC_ASSERT_IMPLEMENTS\(Subject, Impl, Types, "[^"]*Trait requirement not satisfied: requires [^"]*", '
                      r'(.*)\);', generated)


class RequirementAsserts(unittest.TestCase):
    def test_a_constraint_is_asserted_as_itself(self):
        r = generate_typed("requires std::signed_integral<typename Types::Value>")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(asserted(r.generated), ["std::signed_integral<typename Types::Value>"])

    def test_satisfies_requirements_tests_a_constraint_inside_a_requires_expression(self):
        r = generate_typed("requires std::signed_integral<typename Types::Value>")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("&& requires (Impl impl_, Impl const const_) { requires std::signed_integral<typename Types::Value>; }",
                      r.generated)

    def test_a_constraint_naming_the_implementation_is_refused(self):
        for requirement, named in (("requires std::same_as<decltype(const_.get()), typename Types::Value>", "const_"),
                                   ("requires (requires { impl_.get(); })", "impl_")):
            with self.subTest(requirement=requirement):
                r = generate_typed(requirement)
                self.assertNotEqual(r.returncode, 0)
                self.assertIn(f"names {named}, which a constraint cannot: name the implementor with [Impl], "
                              f"or check it in 'requires requires {{ requires ", r.stderr)

    def test_a_constraint_may_name_the_annotated_implementor(self):
        r = generate_source("""
            trait test::trait::Subject [Impl]
            {
                requires std::default_initializable<Impl>
                get() const -> int
            }
        """)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("std::default_initializable<Impl>);", r.generated)

    def test_a_parenthesised_constraint_keeps_its_parentheses(self):
        r = generate_typed("requires (sizeof(typename Types::Value) >= 2)")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(asserted(r.generated), ["(sizeof(typename Types::Value) >= 2)"])

    def test_a_requires_expression_is_asserted_naming_the_implementation(self):
        r = generate_typed("requires requires { { const_.get() } -> std::same_as<typename Types::Value>; }")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(asserted(r.generated), [
            "requires (Impl impl_, Impl const const_) { requires requires { { const_.get() } -> "
            "std::same_as<typename Types::Value>; }; }",
        ])


if __name__ == "__main__":
    unittest.main()
