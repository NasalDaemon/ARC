"""The links that take trunks: what the generator emits for the trunk checks, and what it refuses."""

import textwrap
import unittest

from support import Accepts, Refuses, provider


class TrunkLinksAccepted(Accepts):
    def test_each_trunk_link_is_listed_for_the_trunk_checks(self):
        out = self.generate("""
            cluster test::Consumer
            {
                provider = cluster::Provider
                full     = node::Full
                partial  = node::Partial
                named    = node::Named
                inline   = node::Inline
                other    = node::Other

                [[cluster::Provider]]        full ==> provider
                [[cluster::Provider*]]       partial ==> provider
                [[cluster::Provider::Extra]] named ==> provider
                [[A + B]]                    inline ==> provider
                [C]                          full --> other
            }
        """)
        self.assertRegex(out, r'struct TrunkLink0_ : ::arc::TrunkLink<cluster::Provider> '
                              r'\{ static constexpr char const Pos\[\] = "subject\.hxx\.arc:10:\d+"; \};')
        self.assertIn("::arc::TraitLink<C>", out)
        self.assertIn("::arc::PartialTrunkLink<cluster::Provider>", out)
        self.assertIn("::arc::TrunkLink<cluster::Provider::Extra>", out)
        self.assertIn("::arc::InlineTrunkLink<::arc::Trunk<A, B>>", out)
        self.assertIn("::arc::pred::TrunkLinks<typename full_::TrunkLink0_, typename full_::TrunkLink1_>", out)
        # A trunk link takes the trunk alike in full or in part: only the check differs
        self.assertIn("ARC_LINK((::arc::TrunkOf<cluster::Provider>), provider_)", out)
        self.assertNotIn("cluster::Provider*", out)

    def test_nodes_without_a_trunk_link_list_no_links(self):
        out = self.generate("""
            cluster test::Plain
            {
                a = node::A
                b = node::B

                [A] a --> b
            }
        """)
        self.assertNotIn("TrunkLink", out)

    def test_what_each_node_takes_from_the_parent_is_listed_with_the_node_for_the_trunk_checks(self):
        out = self.generate("""
            cluster test::Middle
            {
                full    = node::Full
                partial = node::Partial
                inline  = node::Inline
                single  = node::Single

                [[cluster::Provider]]  full ==> ..
                [[cluster::Provider*]] partial ==> ..
                [[A + B]]              inline ==> ..
                [C]                    single --> ..
                [D]                    single (E) --> ..
            }
        """)
        # With the trait the node asks for, which the renaming link [D] single (E) --> .. makes E
        self.assertIn("using Depends = ::arc::Depends<"
                      "::arc::ParentUse<::arc::Trunk<A, B>, ::arc::Trunk<A, B>, inline_, false>*, "
                      "::arc::ParentUse<::arc::TrunkOf<cluster::Provider>, ::arc::TrunkOf<cluster::Provider>, full_, false>*, "
                      "::arc::ParentUse<::arc::TrunkOf<cluster::Provider>, ::arc::TrunkOf<cluster::Provider>, partial_, true>*, "
                      "::arc::ParentUse<C, C, single_, false>*, ::arc::ParentUse<D, E, single_, false>*>;", out)
        # The node contexts the dependencies name are declared before them
        self.assertLess(out.index("struct partial_;"), out.index("using Depends"))

    def test_a_trait_passed_back_out_to_the_parent_is_used_by_no_node(self):
        out = self.generate("""
            cluster test::Forwarding
            {
                lis = node::Listener

                [C] .. --> {lis, ..}
            }
        """)
        self.assertIn("using Depends = ::arc::Depends<::arc::ParentUse<C, C, void, false>*>;", out)

    def test_a_cluster_taking_nothing_from_its_parent_lists_no_use(self):
        out = self.generate("""
            cluster test::Plain
            {
                a = node::A
            }
        """)
        self.assertIn("using Depends = ::arc::Depends<>;", out)

    def test_the_parent_s_trunk_links_are_checked_where_the_cluster_is(self):
        out = self.generate("""
            cluster test::Outer
            {
                provider = cluster::Provider
                other    = node::Other

                [[cluster::Provider]] .. ==> provider
                [C] .. --> other
            }
        """)
        self.assertIn("struct ParentTrunkLink0_ : ::arc::TrunkLink<cluster::Provider>", out)
        self.assertIn("static_assert(::arc::detail::assertParentTrunkLinksDisjoint<ParentTrunkLink0_, ParentTrunkLink1_>());", out)

    def test_traits_of_trunks_declared_elsewhere_are_left_to_the_compiler(self):
        self.generate("""
            cluster test::Consumer
            {
                provider = other::cluster::Provider
                client   = node::Client
                other    = node::Other

                [[other::cluster::Provider]] client ==> provider
                [A] client --> other
            }
        """)


class TrunkLinksRefused(Refuses):
    def consumer(self, links: str) -> str:
        return provider("Trunk Extra = C + D") + f"""
            cluster test::Consumer
            {{
                provider = cluster::Provider
                client   = node::Client
                other    = node::Other
                second   = node::Other

                {textwrap.indent(textwrap.dedent(links).strip(), " " * 16).strip()}
            }}
        """

    def test_a_trait_through_a_trunk_and_a_link_for_it(self):
        self.assertRefused(self.consumer("""
            [[cluster::Provider]] client ==> provider
            [A] client --> other
        """), "'client' in cluster 'test::Consumer' takes trait 'A' through [[cluster::Provider]] here",
            "and through [A]; a trunk cannot be disambiguated inside the node")

    def test_a_trait_through_a_trunk_named_as_the_cluster_sees_it_and_a_link_for_it(self):
        self.assertRefused(self.consumer("""
            [[Provider::Extra]] client ==> provider
            [D] client --> other
        """), "takes trait 'D' through [[Provider::Extra]]", "a trunk cannot be disambiguated")

    def test_a_trait_through_a_named_trunk_and_a_fan_out_of_it(self):
        self.assertRefused(self.consumer("""
            [[cluster::Provider::Extra]] client ==> provider
            [C] client --> {other, second}
        """), "takes trait 'C' through [[cluster::Provider::Extra]]", "a trunk cannot be disambiguated")

    def test_a_trait_through_two_inline_trunks(self):
        self.assertRefused(self.consumer("""
            [[A + B]] client ==> provider
            [[B + C]] client ==> other
        """), "takes trait 'B' through [[A + B]]", "and through [[B + C]]")

    def test_a_trait_through_a_trunk_and_a_sink(self):
        self.assertRefused(self.consumer("""
            [B] other

            [[cluster::Provider*]] client ==> provider
        """), "takes trait 'B' through", "[[cluster::Provider*]]")

    def test_a_trait_the_parent_takes_through_a_trunk_and_a_link(self):
        self.assertRefused(self.consumer("""
            [[A + B]] .. ==> provider
            [B] .. --> other
        """), "'@parent' in cluster 'test::Consumer' takes trait 'B'")

    def test_a_trunk_fan_out(self):
        self.assertRefused(self.consumer("""
            [[cluster::Provider]] client ==> {provider, other}
        """), "takes trunk [[cluster::Provider]] more than once", "a trunk cannot fan out")

    def test_a_trunk_taken_twice_in_full_and_in_part(self):
        self.assertRefused(self.consumer("""
            [[cluster::Provider]]  client ==> provider
            [[cluster::Provider*]] client ==> other
        """), "more than once")

    def test_a_partial_mark_on_an_inline_trunk_member(self):
        self.assertRefused(self.consumer("[[A* + B]] client ==> provider"),
                           "only a cluster's trunk may be marked with one '*'")

    def test_a_doubled_partial_mark(self):
        self.assertRefused(self.consumer("[[cluster::Provider**]] client ==> provider"),
                           "only a cluster's trunk may be marked with one '*'")

    def test_a_partial_mark_where_the_parent_takes_the_trunk(self):
        self.assertRefused(self.consumer("[[cluster::Provider*]] .. ==> provider"),
                           "has no effect where @parent takes the trunk")


if __name__ == "__main__":
    unittest.main()
