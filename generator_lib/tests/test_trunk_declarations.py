"""The trunks a cluster declares: named trunks, their union, extension, names and exposure."""

import unittest

from support import Accepts, Refuses, provider


class TrunkDeclarationsAccepted(Accepts):
    def test_named_trunks_are_structs_beside_the_default_trunk_their_union(self):
        out = self.generate(provider("Trunk Extra = C + D"))
        self.assertIn("using Trunk = ::arc::Trunk<A, B, C, D>;", out)
        self.assertRegex(out, r"struct AB\s*\{\s*using Trunk = ::arc::Trunk<A, B>;")
        self.assertRegex(out, r"struct Extra\s*\{\s*using Trunk = ::arc::Trunk<C, D>;")

    def test_the_default_trunk_lists_each_trait_of_the_named_trunks_once_in_first_seen_order(self):
        out = self.generate(provider("Trunk DA = D + A, Trunk CB = C + B"))
        self.assertIn("using Trunk = ::arc::Trunk<A, B, D, C>;", out)

    def test_a_cluster_with_one_trunk_declares_it_as_its_default(self):
        out = self.generate("""
            cluster test::Provider [Trunk = A + B]
            {
                ab = node::AB

                [A] .. --> ab
                [B] .. --> ab
            }
        """)
        self.assertIn("using Trunk = ::arc::Trunk<A, B>;", out)
        self.assertNotIn("// Where the trunk is declared", out)
        self.assertRegex(out, r'static constexpr char const SourceLocation\[\] = "subject\.hxx\.arc:1:\d+";')

    def test_a_trunk_extends_one_declared_in_a_later_block(self):
        out = self.generate("""
            cluster test::Provider [Trunk ABC = Provider::AB + C]
                                   [Trunk AB = A + B]
            {
                abc = node::ABC

                [A] .. --> abc
                [B] .. --> abc
                [C] .. --> abc
            }
        """)
        self.assertRegex(out, r"struct ABC\s*\{\s*using Trunk = ::arc::Trunk<A, B, C>;")

    def test_a_trunk_extends_another_of_its_cluster_by_name(self):
        out = self.generate(provider("Trunk ABC = Provider::AB + C, Trunk ABCD = Provider::ABC + D"))
        self.assertRegex(out, r"struct ABC\s*\{\s*using Trunk = ::arc::Trunk<A, B, C>;")
        self.assertRegex(out, r"struct ABCD\s*\{\s*using Trunk = ::arc::Trunk<A, B, C, D>;")

    def test_trunks_extended_together_merge_the_traits_they_share(self):
        out = self.generate(provider("Trunk AC = A + C, Trunk CD = C + D, Trunk All = Provider::AB + Provider::AC + Provider::CD"))
        self.assertRegex(out, r"struct All\s*\{\s*using Trunk = ::arc::Trunk<A, B, C, D>;")

    def test_a_trunk_extends_several_trunks_and_lists_new_traits(self):
        out = self.generate(provider("Trunk AC = A + C, Trunk ABCD = Provider::AB + Provider::AC + D"))
        self.assertRegex(out, r"struct ABCD\s*\{\s*using Trunk = ::arc::Trunk<A, B, C, D>;")

    def test_a_trunk_may_extend_one_declared_after_it(self):
        out = self.generate(provider("Trunk ABCD = Provider::ABC + D, Trunk ABC = Provider::AB + C"))
        self.assertRegex(out, r"struct ABCD\s*\{\s*using Trunk = ::arc::Trunk<A, B, C, D>;")

    def test_a_trunk_exposed_through_an_inline_trunk_counts_as_exposed(self):
        self.generate("""
            cluster test::Provider [Trunk = A + B]
            {
                ab = node::AB

                [[A + B]] .. ==> ab
            }
        """)


class TrunkDeclarationsRefused(Refuses):
    def test_a_second_default_trunk(self):
        self.assertRefused("""
            cluster test::Provider [Trunk = A, Trunk = B]
            {
                ab = node::AB

                [A] .. --> ab
                [B] .. --> ab
            }
        """, "declares its default trunk twice", "'Trunk Name = ...'")

    def test_a_default_trunk_beside_named_trunks(self):
        self.assertRefused(provider("Trunk = C + D"), "declares a default trunk beside named trunks",
                           "its default trunk is the union of its named trunks")

    def test_extending_the_default_trunk(self):
        self.assertRefused(provider("Trunk More = Provider::Trunk + C"),
                           "trunk 'More' of cluster 'test::Provider' extends 'Provider::Trunk', the default trunk, which is "
                           "the union of the named trunks: name the trunk to extend")

    def test_a_repeated_trunk_name(self):
        self.assertRefused(provider("Trunk Extra = C, Trunk Extra = D"), "declares trunk 'Extra' twice")

    def test_a_reserved_trunk_name(self):
        for name in ("Node", "Trunk", "SourceLocation", "Provider"):
            with self.subTest(name=name):
                self.assertRefused(provider(f"Trunk {name} = C"), f"Trunk name '{name}'", "is reserved")

    def test_a_trait_listed_twice_in_a_trunk(self):
        self.assertRefused(provider("Trunk Extra = C + D + C"), "trunk 'Extra' of cluster 'test::Provider' lists trait 'C' twice "
                           "(first here")

    def test_a_trait_listed_beside_a_trunk_extended_after_it_that_has_it(self):
        self.assertRefused(provider("Trunk More = A + Provider::AB"),
                           "trunk 'More' of cluster 'test::Provider' lists trait 'A', which trunk 'AB' it extends already has")

    def test_a_trait_a_trunk_lists_twice_once_extended(self):
        self.assertRefused(provider("Trunk ABC = Provider::AB + C, Trunk More = Provider::ABC + A"),
                           "trunk 'More'", "lists trait 'A', which trunk 'ABC' it extends already has: a trait it lists "
                           "itself must be new")

    def test_extending_a_trunk_the_cluster_does_not_declare(self):
        self.assertRefused(provider("Trunk More = Provider::Missing + C"),
                           "extends 'Provider::Missing', but the cluster declares no such trunk", "Provider::AB")

    def test_extending_a_trunk_spelt_as_its_clients_spell_it(self):
        self.assertRefused(provider("Trunk ABC = cluster::Provider::AB + C"),
                           "trunk 'ABC' of cluster 'test::Provider' lists 'cluster::Provider::AB': to extend the trunk, "
                           "name it as 'Provider::AB'")

    def test_a_trunk_named_as_a_trait_the_cluster_spells_unqualified(self):
        self.assertRefused(provider("Trunk A = C + D"),
                           "Trunk name 'A' in cluster 'test::Provider' is also a name the cluster spells unqualified, in 'A'",
                           "it would name the trunk instead, so rename the trunk")

    def test_a_trunk_named_as_the_head_of_a_qualified_name_the_cluster_spells(self):
        self.assertRefused("""
            cluster test::Provider [Trunk node = A]
            {
                a = node::A

                [A] .. --> a
            }
        """, "Trunk name 'node'", "in 'node::A'")

    def test_a_trunk_named_as_a_type_a_connection_alias_spells(self):
        self.assertRefused("""
            cluster test::Provider [Trunk B = A + C]
            {
                a = node::ABC

                using b2 = B
                [b2] .. --> a
                [A]  .. --> a
                [C]  .. --> a
            }
        """, "Trunk name 'B' in cluster 'test::Provider' is also a name the cluster spells unqualified, in 'B' "
             "in a connection alias")

    def test_a_trunk_named_as_a_template_argument_the_cluster_spells(self):
        self.assertRefused("""
            cluster test::Provider [Trunk Key = A]
            {
                a = node::A : arc::StaticMap<Key>

                [A] .. --> a
            }
        """, "Trunk name 'Key'", "in 'arc::StaticMap<a_inner0_, Key>'")

    def test_a_node_named_as_a_member_type_of_every_cluster(self):
        self.assertRefused("""
            cluster test::Provider
            {
                Depends = node::A
            }
        """, "Node name 'Depends' in cluster 'test::Provider' is reserved")

    def test_a_trunk_named_as_a_template_parameter(self):
        self.assertRefused("""
            template<class T>
            cluster test::Provider [Trunk T = A]
            {
                a = node::A

                [A] .. --> a
            }
        """, "Trunk name 'T' in cluster 'test::Provider' is the name of a template parameter")

    def test_trunks_that_extend_each_other(self):
        self.assertRefused(provider("Trunk X = Provider::Y + C, Trunk Y = Provider::X + D"),
                           "declares trunks that extend each other: trunk 'X' -> trunk 'Y' -> trunk 'X'")

    def test_a_trunk_that_extends_itself(self):
        self.assertRefused(provider("Trunk X = Provider::X + C"), "extend each other: trunk 'X' -> trunk 'X'")

    def test_a_trunk_trait_the_cluster_does_not_expose(self):
        self.assertRefused("""
            cluster test::Provider [Trunk Core = A, Trunk Extra = B]
            {
                a = node::A

                [A] .. --> a
            }
        """, "trunk 'Extra' lists trait 'B' but there is no '[B] .. --> node' parent connection")


if __name__ == "__main__":
    unittest.main()
