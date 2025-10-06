#!/usr/bin/python3

import argparse
from bisect import bisect_right
from collections import defaultdict
from functools import cached_property
import jinja2
from lark import Lark, Tree, Token, UnexpectedInput
from lark.visitors import Visitor_Recursive
from lark.reconstruct import Reconstructor
from pathlib import Path


dir_path = Path(__file__).resolve().parent

arg_parser = argparse.ArgumentParser(
                    prog='ARC generator',
                    description='Generates cpp source files from ARC DSL called arc')

arg_parser.add_argument('-i', '--input')
arg_parser.add_argument('-o', '--output')
arg_parser.add_argument('-m', '--module', action='store_true')

args = arg_parser.parse_args()

input_file: str = args.input
input_path = Path(input_file).resolve()
output_file: str = args.output
is_module: bool = args.module
is_embedded: bool = input_path.suffixes[-1] != '.arc'

grammar_file = dir_path.joinpath(dir_path, 'arc_module.lark' if is_module else 'arc_header.lark')
arc_parser = Lark.open(grammar_file, maybe_placeholders=False, parser='lalr', cache=True)

reconstructor = Reconstructor(arc_parser)
section_lines: list[tuple[int, int, int]] = []


def get_line(line: int, col: int) -> tuple[int, int]:
    if not is_embedded:
        return line, col
    source_lines = [sl[0] for sl in section_lines]
    index = bisect_right(source_lines, line) - 1

    if line == section_lines[index][0] or line == 1:
        col += section_lines[index][2]
        col += len("arc-embed-begin") - 1
    line = line - section_lines[index][0] + section_lines[index][1]

    return line, col


with open(input_file, 'r') as file:
    text = file.read()
    if is_embedded:
        sections = []
        outer_line_count = 0
        inner_line_count = 0
        begin = 'arc-embed-begin'
        end = 'arc-embed-end'
        while True:
            begin_pos = text.find(begin)
            outer_line_count += text[:begin_pos].count("\n")
            if (npos := text.rfind("\n", 0, begin_pos)) != -1:
                outer_col_count = begin_pos - npos
            section_lines.append((inner_line_count, outer_line_count, outer_col_count))
            if begin_pos == -1:
                assert sections, f"'{begin}' not found in {input_path}"
                break
            begin_pos += len(begin)
            end_pos = text.find(end, begin_pos)
            assert end_pos != -1, f"matching '{end}' not found in {input_path}"

            section = text[begin_pos:end_pos]
            section_line_count = section.count("\n") + 1
            inner_line_count += section_line_count
            outer_line_count += section_line_count
            sections.append(section)
            text = text[end_pos + len(end):]
        text = "".join(sections)

    def on_parse_error(e: UnexpectedInput) -> bool:
        line, col = get_line(e.line, e.column)
        print(f"{input_path}:{line}:{col} parse error:\n{e.get_context(text)}")
        return False

    parsed = arc_parser.parse(text, on_error=on_parse_error)


def imported(lark_rule: str):
    return f'arc__{lark_rule}'


def get_pos(t: Tree | Token | None) -> str:
    if t is None:
        return str(input_path)
    if isinstance(t, Token):
        line, col = get_line(t.line, t.column)
        return f"{input_path}:{line}:{col}"
    if isinstance(t, Tree):
        return get_pos(t.children[0])
    else:
        raise TypeError(f"Expected Tree or Token, got {type(t)}")


class AddColonToRequiresStatements(Visitor_Recursive):
    def arc__cpp_requires_statement(self, tree):
        if not isinstance(tree.children[-1], Token) or tree.children[-1].type != imported('SEMICOLON'):
            tree.children.append(Token(imported('SEMICOLON'), ';'))

add_colon_to_requires_statements = AddColonToRequiresStatements()

NO_TRAIT = "~"

def is_no_trait(trait: str | None) -> bool:
    if trait is None:
        return False
    return trait == NO_TRAIT or "arc::NoTrait<" in trait


class CppType:
    def __init__(self, string: str, *, is_auto: bool = False, tree: Tree | None = None):
        if string.endswith("..."):
            self.str = string.removesuffix("...")
            self.pack = "..."
        else:
            self.str = string
            self.pack = ""
        self._is_auto = is_auto
        self.tree = tree

    @classmethod
    def from_tree(cls, tree):
        string = reconstructor.reconstruct(tree)
        return cls(string, tree=tree)

    @cached_property
    def is_auto(self):
        if self.tree is None:
            return self._is_auto
        return bool(next(self.tree.scan_values(lambda t: t.value == "auto"), None))

    def __repr__(self):
        return self.str

    def __lt__(self, other):
        return (self.str, self.pack) < (other.str, other.pack)


class Repeater:
    def __init__(self, name: str, trait: str, repeater_id: int):
        if name == "..":
            name = "parent"
        assert name[0].islower(), name
        self.name = f'_{name}Repeater{repeater_id}'
        self.is_parent = False
        self.is_nexus = False
        self.is_unary = True
        self.has_state = False
        self.trait = trait
        self.connections: list['Connection'] = []
        self.context = f'{name[0].upper()}{name[1:]}Repeater{repeater_id}_'

    @property
    def node_alias(self):
        return f"{self.context}Node_"

    @property
    def is_user_node(self):
        return False

    @property
    def impl(self):
        return f'arc::Repeater<{self.trait}, {len(self.connections)}>'

    def add_connection(self, connection: 'Connection'):
        assert connection.trait == self.trait, (connection.trait, self.trait)
        connection.trait = f"arc::RepeaterTrait<{len(self.connections)}>"
        self.connections.append(connection)


class Connection:
    def __init__(self, to_node: "Node | Repeater", trait: str, to_trait: str | None = None, to_repeater: Repeater | None = None):
        self.to_node: Node | Repeater = to_node
        self.trait: str = trait
        if self.trait == NO_TRAIT:
            self.trait = f"arc::NoTrait<{to_node.node_alias}>"
            self.to_trait = self.trait
        else:
            self.to_trait: str = to_trait or trait
        self.to_repeater = to_repeater

    @property
    def context(self):
        return self.to_node.context

    def copy(self):
        return Connection(self.to_node, self.trait, self.to_trait, self.to_repeater)

    def is_renaming(self):
        return self.trait != self.to_trait


class Node:
    def __init__(self, name: str, tree: Tree | None, impl: str, cluster: 'Cluster | Domain', is_first: bool):
        self.repeaters: list[Repeater] = []
        self.connections: list[Connection] = []
        self.clients: list[tuple[str, 'Node', str]] = [] # [(position, client_node, trait)]
        self.name: str = name
        self.impl: str = impl
        self.cluster = cluster
        self.is_nexus: bool = is_first and isinstance(cluster, Domain)
        self.is_parent = False
        self.is_global = False
        self.is_sink_node = False
        self.no_traits = False
        if name == '..':
            self.is_parent = True
            self.context = "Context"
        elif name == '*':
            self.is_global = True
            self.context = "Context"
        else:
            self.is_unary = name.upper() != name
            self.has_state = name[0].isupper()
            self.context: str = name + '_'
            if self.is_nexus and not self.is_unary:
                raise SyntaxError(f"{get_pos(tree)} Nexus node '{name}' in '{cluster.full_name}' must be a unary node")

    @property
    def is_user_node(self):
        return True

    @property
    def node_alias(self):
        return f"{self.context}Node_"

    def add_connection(self, pos, is_override: bool, to_node: 'Node', trait: str, *, to_trait: str | None = None):
        assert self.cluster == to_node.cluster
        if to_node == self:
            raise SyntaxError(f"{pos} cannot connect '{self.name}' to itself")
        if self.is_global:
            raise SyntaxError(f"{pos} cannot connect from global node '*' to any other node")
        if self.is_sink_node and not to_node.is_sink_node:
            raise SyntaxError(f"{pos} cannot connect from sink node '{self.name}' to a non-sink node")
        if not is_no_trait(trait) and trait in self.cluster.sink_traits:
            raise SyntaxError(f"{pos} Trait '{trait}' already allocated to sink node '{self.cluster.sink_traits[trait][0][0].name}' "
                              f"in {self.cluster.cluster_class} '{self.cluster.full_name}' here {self.cluster.sink_traits[trait][0][1]}")
        if not is_no_trait(to_trait) and to_trait in self.cluster.sink_traits:
            raise SyntaxError(f"{pos} Trait '{to_trait}' already allocated to sink node '{self.cluster.sink_traits[to_trait][0][0].name}' "
                              f"in {self.cluster.cluster_class} '{self.cluster.full_name}' here {self.cluster.sink_traits[to_trait][0][1]}")
        if to_trait is not None and is_no_trait(trait) != is_no_trait(to_trait):
            raise SyntaxError(f"{pos} Cannot redirect trait '{trait}' to trait '{to_trait}' in {self.cluster.cluster_class} '{self.cluster.full_name}'")
        if error := self.cluster.get_connection_error(self, to_node, is_override):
            raise SyntaxError(f"{pos} Cannot connect '{self.name}' to '{to_node.name}' in {self.cluster.cluster_class} '{self.cluster.full_name}':\n{error}")

        effective_to_trait = trait if to_trait is None else to_trait
        if effective_to_trait == NO_TRAIT:
            if to_node.is_global or to_node.is_parent:
                raise SyntaxError(f"{pos} Cannot use no-trait shorthand '~' to connect to global '*' or parent '..' node in {self.cluster.cluster_class} '{self.cluster.full_name}'. "
                                  "Use a named trait like 'arc::NoTrait<TargetNode>' instead.")
        if to_node.is_global:
            to_trait = f"arc::Global<{effective_to_trait}>"
        to_node.add_client(pos, self, effective_to_trait)
        connection = Connection(to_node, trait=trait, to_trait=to_trait)
        if existing := next((c for c in self.connections if c.trait == connection.trait), None):
            if existing.to_repeater is None:
                repeater = Repeater(self.name, connection.trait, len(self.repeaters))
                repeater.add_connection(existing.copy())
                repeater.add_connection(connection)
                existing.to_node = repeater
                existing.to_trait = existing.trait
                existing.to_repeater = repeater
                self.repeaters.append(repeater)
            else:
                existing.to_repeater.add_connection(connection)
        else:
            self.connections.append(connection)

    def add_client(self, pos, client, trait):
        if self.no_traits:
            if not is_no_trait(trait):
                raise SyntaxError(f"{pos} Cannot connect '{client.name}' to '{self.name}' in {self.cluster.cluster_class} '{self.cluster.full_name}':\n"
                                f"'{self.name}' has no traits, but a named trait '{trait}' was specified")
        elif is_no_trait(trait):
            if len(self.clients) > 0:
                raise SyntaxError(f"{pos} Cannot connect '{client.name}' to '{self.name}' in {self.cluster.cluster_class} '{self.cluster.full_name}':\n"
                                  f"'{self.name}' already has a client with a named trait connection here {self.clients[0][0]}, but a no-trait connection was specified")
            if self.is_nexus:
                raise SyntaxError(f"{pos} Nexus node '{self.name}' in domain '{self.cluster.full_name}' cannot have no-trait connections")

        self.clients.append((pos, client, trait))
        if is_no_trait(trait):
            self.no_traits = True


class Cluster:
    def __init__(self, name: str, namespace: "Namespace"):
        self.name = name
        self.namespace = namespace
        self.templates: list[tuple[CppType, str]] = []
        self.context_name: str = "Context"
        self.root_name: str | None = None
        self.info_name: str | None = None
        self.parent_node = Node("..", None, self.name, cluster=self, is_first=False)
        self.global_node = Node("*", None, self.name, cluster=self, is_first=False)
        self.user_nodes: list[Node] = []
        self.repeaters: list[Repeater] = []
        self.nodes: list[Node | Repeater] = []
        self.aliases: list[tuple[str, str]] = []
        self.dependencies: list[str] = []
        # trait: [(Node, position)] where len(list) > 1 only when trait is NO_TRAIT
        self.sink_traits: dict[str, list[tuple[Node, str]]] = defaultdict(list)

    @property
    def cluster_type(self) -> str:
        return "arc::Cluster"

    @property
    def cluster_class(self) -> str:
        return "cluster"

    @property
    def full_name(self) -> str:
        if self.namespace.name:
            return f"{self.namespace.name}::{self.name}"
        else:
            return self.name

    def predicates(self, node) -> list[str]:
        return []

    def predicates_str(self, node) -> str:
        if preds := self.predicates(node):
            return ", " + ", ".join(preds)
        else:
            return ""

    def node_name(self):
        if self.templates:
            return "Node"
        else:
            return f'{self.name}_'

    def add_template(self, token):
        for c in token.children:
            if c.data == imported('tparam_type'):
                self.templates.append((CppType(c.children[0].value.replace("typename", "class")), c.children[1].value))
            elif c.data == imported('tparam_non_type'):
                type_ = CppType.from_tree(c.children[0])
                self.templates.append((type_, c.children[1].value))
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown cluster template: {c.data}')

    def get_connection_error(self, lnode: Node, rnode: Node, is_override: bool) -> None:
        return None

    def arrow_sign(self, arrow: Tree) -> str:
        return next((c.value for c in arrow.children if isinstance(c, Token)))

    def validate_arrow(self, arrow: Tree):
        sign = self.arrow_sign(arrow)
        chevrons = max(sign.count('<'), sign.count('>'))
        if chevrons > 1:
            raise SyntaxError(f"{get_pos(arrow)} Only one chevron is allowed per arrow in clusters.")
        return False

    def walk(self, children):
        aliases: dict[str, str] = {}
        nodes: dict[str, Node] = {}
        nodes[".."] = self.parent_node
        nodes["*"] = self.global_node
        explicit_connection_seen = False
        left_trait: str
        right_trait: str
        bi_trait: bool
        for child in children:
            if child.data == imported('cluster_annotations'):
                for ann in child.children:
                    if ann.children[-1].value == "Context":
                        self.context_name = ann.children[0].value
                    elif ann.children[-1].value == "Root":
                        self.root_name = ann.children[0].value
                    elif ann.children[-1].value == "Info":
                        self.info_name = ann.children[0].value
                    else:
                        raise SyntaxError(f"{get_pos(ann)} Unknown cluster annotation: {ann.children[0].value}")
            elif child.data == imported('node'):
                name = child.children[0].value
                if name in nodes:
                    raise SyntaxError(f"{get_pos(child)} Node '{name}' already defined in cluster '{self.full_name}'")
                impl = reconstructor.reconstruct(child.children[1])
                if len(child.children) > 2:
                    for wrapper in child.children[2:]:
                        cls = wrapper.children[0].value
                        if len(wrapper.children) > 1:
                            args = [impl]
                            args.extend(reconstructor.reconstruct(arg) for arg in wrapper.children[1].children[1:-1:2])
                            impl = f"{cls}<{', '.join(args)}>"
                        else:
                            impl = f"{cls}<{impl}>"
                is_first = len(nodes) == 2
                nodes[name] = Node(name, child, impl, cluster=self, is_first=is_first)
            elif child.data == imported('connection_block'):
                for child in child.children:
                    if child.data == imported('connection_aliases'):
                        for child in child.children:
                            alias, trait_type = child.children
                            type_string = reconstructor.reconstruct(trait_type)
                            if alias in aliases:
                                if aliases.get(alias) != type_string:
                                    raise SyntaxError(f"{get_pos(alias)} Alias '{alias}' changed from {aliases.get(alias)} to {type_string}")
                            else:
                                aliases[alias] = type_string
                    elif child.data == imported('connection_trait'):
                        left_trait = reconstructor.reconstruct(child.children[0])

                        if len(child.children) == 1:
                            bi_trait = False
                            right_trait = left_trait
                        else:
                            bi_trait = True
                            right_trait = reconstructor.reconstruct(child.children[-1])
                        if left_trait != NO_TRAIT and left_trait in self.sink_traits:
                            raise SyntaxError(f"{get_pos(child.children[0])} Trait '{left_trait}' already allocated to sink node "
                                              f"'{self.sink_traits[left_trait][0][0].name}' in cluster '{self.full_name}' here {self.sink_traits[left_trait][0][1]}")
                        if right_trait != NO_TRAIT and right_trait in self.sink_traits:
                            raise SyntaxError(f"{get_pos(child.children[-1])} Trait '{right_trait}' already allocated to sink node "
                                              f"'{self.sink_traits[right_trait][0][0].name}' in cluster '{self.full_name}' here {self.sink_traits[right_trait][0][1]}")
                    elif child.data == imported('sink_node'):
                        for c in child.children[0].children:
                            name = c.value
                            node = nodes[name]
                            if self.cluster_class == "domain":
                                raise SyntaxError(f"{get_pos(child)} Sink node '{name}' not permitted in domain '{self.full_name}'")
                            if explicit_connection_seen:
                                raise SyntaxError(f"{get_pos(child)} Sink node '{name}' in cluster '{self.full_name}' must be declared before any explicit connections")
                            if bi_trait:
                                raise SyntaxError(f"{get_pos(child)} Sink node '{name}' in cluster '{self.full_name}' cannot have bi-directional trait")
                            if left_trait != NO_TRAIT and left_trait in self.sink_traits:
                                raise SyntaxError(f"{get_pos(child)} Sink node '{name}' in cluster '{self.full_name}' is using the trait '{left_trait}' "
                                                f"already used by another sink node '{self.sink_traits[left_trait][0][0].name}' here {self.sink_traits[left_trait][0][1]}")
                            node.is_sink_node = True
                            self.sink_traits[left_trait].append((node, get_pos(child)))
                    elif child.data == imported('connection'):
                        explicit_connection_seen = True
                        for i in range(0, len(child.children) - 1, 2):
                            lnames, arrow, rnames = (child.children[i], child.children[i+1], child.children[i+2])
                            is_override = self.validate_arrow(arrow)
                            pos = get_pos(arrow)
                            lnodes = [nodes[name.value] for name in lnames.children]
                            rnodes = [nodes[name.value] for name in rnames.children]

                            lrnodes = ((lnode, rnode) for rnode in rnodes for lnode in lnodes)
                            if arrow.data == imported('left_arrow'):
                                for lnode, rnode in lrnodes:
                                    rnode.add_connection(pos, is_override, lnode, left_trait)
                            elif arrow.data == imported('right_arrow'):
                                for lnode, rnode in lrnodes:
                                    lnode.add_connection(pos, is_override, rnode, right_trait)
                            elif arrow.data == imported('bi_arrow'):
                                for lnode, rnode in lrnodes:
                                    rnode.add_connection(pos, is_override, lnode, left_trait)
                                    lnode.add_connection(pos, is_override, rnode, right_trait)
                            elif arrow.data == imported('left_arrow_from'):
                                from_trait = reconstructor.reconstruct(arrow.children[-1].children[0])
                                to_trait = left_trait
                                for lnode, rnode in lrnodes:
                                    rnode.add_connection(pos, is_override, lnode, from_trait, to_trait=to_trait)
                            elif arrow.data == imported('right_arrow_from'):
                                from_trait = reconstructor.reconstruct(arrow.children[0].children[0])
                                to_trait = right_trait
                                for lnode, rnode in lrnodes:
                                    lnode.add_connection(pos, is_override, rnode, from_trait, to_trait=to_trait)
                            elif arrow.data == imported('left_arrow_to'):
                                from_trait = left_trait
                                to_trait = reconstructor.reconstruct(arrow.children[0].children[0])
                                for lnode, rnode in lrnodes:
                                    rnode.add_connection(pos, is_override, lnode, from_trait, to_trait=to_trait)
                            elif arrow.data == imported('right_arrow_to'):
                                from_trait = right_trait
                                to_trait = reconstructor.reconstruct(arrow.children[-1].children[0])
                                for lnode, rnode in lrnodes:
                                    lnode.add_connection(pos, is_override, rnode, from_trait, to_trait=to_trait)
                            elif arrow.data == imported('left_arrow_both'):
                                to_trait = reconstructor.reconstruct(arrow.children[0].children[0])
                                from_trait = reconstructor.reconstruct(arrow.children[-1].children[0])
                                for lnode, rnode in lrnodes:
                                    rnode.add_connection(pos, is_override, lnode, from_trait, to_trait=to_trait)
                            elif arrow.data == imported('right_arrow_both'):
                                from_trait = reconstructor.reconstruct(arrow.children[0].children[0])
                                to_trait = reconstructor.reconstruct(arrow.children[-1].children[0])
                                for lnode, rnode in lrnodes:
                                    lnode.add_connection(pos, is_override, rnode, from_trait, to_trait=to_trait)
                            else:
                                raise SyntaxError(f'{pos} Unknown arrow: {arrow.data}')
                    else:
                        raise SyntaxError(f'{get_pos(child)} Unknown connection section: {child.data}')
            else:
                raise SyntaxError(f'{get_pos(child)} Unknown cluster section: {child.data}')

        # Connect all nodes (including parent) to sink nodes at very end
        sink_traits = self.sink_traits
        self.sink_traits = {}
        for trait, sink_nodes in sink_traits.items():
            for sink_node, pos in sink_nodes:
                for node in nodes.values():
                    if node.name not in ["*", sink_node.name]:
                        node.add_connection(pos, False, sink_node, trait)

        self.user_nodes = [node for node in nodes.values() if node.name not in ['..', '*']]
        self.aliases = sorted(aliases.items())
        self.parent_node.connections.sort(key=lambda v: v.trait)
        self.repeaters.extend(self.parent_node.repeaters)
        for node in self.user_nodes:
            node.connections.sort(key=lambda v: v.trait)
            self.repeaters.extend(node.repeaters)
        self.nodes.extend(self.user_nodes)
        self.repeaters.sort(key=lambda r: r.name)
        self.nodes.extend(self.repeaters)
        self.dependencies = [aliases.get(trait, trait) for _, _, trait in self.parent_node.clients]
        self.dependencies.sort()

    def finalise(self):
        self.dependencies = [f"{dep}*" for dep in self.dependencies]


class Domain(Cluster):
    def __init__(self, name: str, namespace: "Namespace"):
        super().__init__(name, namespace)
        self.extra_chevrons_seen = 0
        self.overrides_allowed = 0
        self.overrides_seen = 0
        self.min_extra_chevrons = 2
        self.overrides_per_extra_chevron = 2

    @property
    def cluster_type(self) -> str:
        return "arc::Domain<arc::DomainParams{.MaxDepth=3}>"

    @property
    def cluster_class(self) -> str:
        return "domain"

    def predicates(self, node: Node | Repeater) -> list[str]:
        if isinstance(node, Repeater):
            return []
        preds = ["arc::pred::HasDepends"]
        if not node.is_unary:
            preds.append("arc::pred::NonUnary")
            return preds
        preds.append("arc::pred::Unary")
        if not node.has_state:
            preds.append("arc::pred::Stateless")
        return preds

    def validate_arrow(self, arrow: Tree) -> bool:
        sign = self.arrow_sign(arrow)
        l_extra = sign.count('<') - 1
        r_extra = sign.count('>') - 1
        is_override = False
        if l_extra > 0:
            is_override = True
            if self.extra_chevrons_seen and l_extra != self.extra_chevrons_seen:
                raise SyntaxError(f"{get_pos(arrow)} Inconsistent number of extra chevrons in '{self.full_name}'")
            self.extra_chevrons_seen = l_extra
        if r_extra > 0:
            is_override = True
            if self.extra_chevrons_seen and r_extra != self.extra_chevrons_seen:
                raise SyntaxError(f"{get_pos(arrow)} Inconsistent number of extra chevrons in '{self.full_name}'")
            self.extra_chevrons_seen = r_extra

        if self.extra_chevrons_seen:
            if self.extra_chevrons_seen < self.min_extra_chevrons:
                raise SyntaxError(f"{get_pos(arrow)} Overrides must have at least {1+self.min_extra_chevrons} chevrons")
            if sign.count('-') - 1 < self.extra_chevrons_seen:
                raise SyntaxError(f"{get_pos(arrow)} Overrides must have at least as many dashes ('-') as chevrons ('<' or '>')")
            self.overrides_allowed = self.overrides_per_extra_chevron * self.extra_chevrons_seen

        return is_override

    def get_connection_error(self, from_node: Node, to_node: Node, is_override: bool) -> str | None:
        # nexus can connect to anything in any direction
        if from_node.is_nexus or to_node.is_nexus:
            if is_override:
                return "No override is needed for this connection, but an override was specified"
            return None

        # non-nexus nodes may not connect to parent in any direction
        if from_node.is_parent or to_node.is_parent:
            return "Only nexus node may be connected to parent in domains"

        # unary peers may not connect to anything
        if from_node.is_unary or to_node.is_unary:
            if is_override and from_node.is_unary and to_node.is_unary:
                if self.overrides_seen < self.overrides_allowed:
                    self.overrides_seen += 1
                    return None
                else:
                    return (f"Too many unary-to-unary connections in '{self.name}' ({self.overrides_allowed} allowed) - "
                            "use more chevrons to allow more overrides, or remove some connections")
            return ("Unary nodes may only be connected to the domain nexus. "
                "To explicitly allow a unary-to-unary connection you can override with extra chevrons in the arrow ('<' and/or '>')")

        if is_override:
            return "No override is needed for this connection, but an override was specified"
        return None

    def finalise(self):
        min_overrides = self.min_extra_chevrons * self.overrides_per_extra_chevron
        if self.overrides_allowed > min_overrides and (self.overrides_allowed - self.overrides_seen) >= self.overrides_per_extra_chevron:
            raise SyntaxError(
                f"{input_file} "
                f"More extra chevrons used in '{self.full_name}' than is needed for the required number of overrides ({self.overrides_seen} required). "
                f"Each extra chevron allows you {self.overrides_per_extra_chevron} more overrides in the domain, so only "
                f"{1+max(self.min_extra_chevrons, -(self.overrides_seen // -self.overrides_per_extra_chevron))} chevrons are needed in total per override.")


class Method:
    __reserved_names__ = ['impl', 'isTrait']

    def __init__(self):
        self.name: str = "<unknown>"
        self.templates: list[tuple[CppType, str]] = []
        self.return_type: CppType = CppType("decltype(auto)", is_auto=True)
        self.params: list[tuple[CppType, str]] = []
        self.is_const: bool = False
        self.optional: bool = False

    def add_template(self, children):
        for c in children:
            if c.data == imported('tparam_type'):
                self.templates.append((CppType("class"), c.children[0].value))
            elif c.data == imported('tparam_non_type'):
                type_ = CppType.from_tree(c.children[0])
                self.templates.append((type_, c.children[1].value))
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown method template: {c.data}')

    @cached_property
    def is_template(self):
        if self.templates or next((True for t, n in self.params if t.is_auto), False):
            return True
        return False

    @cached_property
    def is_unconstrained_return(self):
        return self.return_type.str == "decltype(auto)" or self.return_type.str == "void" or self.return_type.str.replace("&", "") == "auto"

    @cached_property
    def is_auto_return(self):
        return self.return_type.is_auto and self.return_type.str != "decltype(auto)"

    def walk(self, children):
        for c in children:
            if c.data == imported('template_params'):
                self.add_template(c.children)
            elif c.data == imported('cpp_type'):
                self.return_type = CppType.from_tree(c)
            elif c.data == imported('method_name'):
                self.optional = c.children[0].type == imported('OPTIONAL_METHOD')
                self.name = c.children[-1].value
                if self.name in self.__reserved_names__:
                    raise SyntaxError(f"{get_pos(c)} '{self.name}' cannot be the name of a method, it is reserved for arc::TraitView")
            elif c.data == imported('cpp_func_params'):
                for c in c.children:
                    type_ = CppType.from_tree(c.children[0])
                    name = c.children[1].value
                    self.params.append((type_, name))
            elif c.data == imported('method_qualifier'):
                self.is_const = True
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown method entity: {c.data}')


class Trait:
    def __init__(self, name: str):
        self.name: str = name[0].upper() + name[1:]
        self.variable: str = name[0].lower() + name[1:]
        self.types_name: str | None = None
        self.root_name: str | None = None
        self.info_name: str | None = None
        self.impl_name: str = "Impl"
        self.impl_named = False
        self.methods: list[Method] = []
        self.method_names: list[str] = []
        self.requires: list[str] = []
        self.has_const_requires = False
        self.has_mutable_requires = False

    def walk(self, children):
        for c in children:
            if c.data == imported('trait_annotations'):
                for ann in c.children:
                    if ann.children[-1].value == "Types":
                        self.types_name = ann.children[0].value
                    elif ann.children[-1].value == "Root":
                        self.root_name =  ann.children[0].value
                    elif ann.children[-1].value == "Info":
                        self.info_name =  ann.children[0].value
                    elif ann.children[-1].value == "Impl":
                        self.impl_name =  ann.children[0].value
                        self.impl_named = True
                    else:
                        raise SyntaxError(f"{get_pos(ann)} Unknown trait annotation: {ann.children[0].value}")
            elif c.data == imported('trait_body'):
                c = c.children[0]
                if c.data == imported('trait_type'):
                    if self.types_name is None:
                        self.types_name = "Types_T_" # use ugly name if not specified to avoid shadowing
                    self.requires.append(f"typename {self.types_name}::{c.children[0].value};")
                elif c.data == imported('trait_root'):
                    if self.root_name is None:
                        self.root_name = "Root_T_" # use ugly name if not specified to avoid shadowing
                    self.requires.append(f"typename {self.root_name}::{c.children[0].value};")
                elif c.data == imported('trait_info'):
                    if self.info_name is None:
                        self.info_name = "Info_T_" # use ugly name if not specified to avoid shadowing
                    self.requires.append(f"typename {self.info_name}::{c.children[0].value};")
                elif c.data == imported('trait_method_signature'):
                    method = Method()
                    method.walk(c.children)
                    self.methods.append(method)
                elif c.data == imported('trait_method_elipsis'):
                    method = Method()
                    method.name = c.children[0].value
                    method.params.append((CppType("auto&&...", is_auto=True), "args"))
                    self.methods.append(method)
                elif c.data == imported('trait_requires'):
                    if c.children[0].data == imported('trait_requires_block'):
                        add_colon_to_requires_statements.visit(c.children[0])
                        requires = "requires " + reconstructor.reconstruct(c.children[0])
                    else:
                        requires = reconstructor.reconstruct(c.children[0].children[0])
                    if not requires.endswith(';'):
                        requires += ';'
                    self.requires.append(requires)
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown trait entity: {c.data}')
        self.methods.sort(key=lambda v: (v.name, v.params))
        self.method_names = sorted(set(method.name for method in self.methods))
        self.has_const_requires = next((method.is_const for method in self.methods if method.is_const and not method.is_template), False)
        self.has_mutable_requires = next((not method.is_const for method in self.methods if not method.is_const and not method.is_template), False)


class Namespace:
    def __init__(self, name: str, repr_: 'Repr'):
        self.name: str = name
        self.repr: 'Repr' = repr_
        self.trait_names: set[str] = set()
        self.traits: list['Trait'] = []
        self.trait_aliases: list[list[str]] = []
        self.cluster_names: set[str] = set()
        self.clusters: list['Cluster'] = []

    def walk(self, children):
        for c in children:
            if c.data == 'cluster':
                self.repr.visit_cluster(self.name, c)
            elif c.data == imported('trait_def'):
                self.repr.visit_trait_def(self.name, c)
            elif c.data == imported('trait_alias'):
                self.repr.visit_trait_alias(self.name, c)
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown namespace entity: {c.data}')

    def add_cluster(self, name: str, tree: Tree, is_domain: bool) -> 'Cluster | Domain':
        if name in self.cluster_names:
            raise SyntaxError(f"{get_pos(tree)} cluster by name '{name}' already defined in namespace '{self.name}'")
        self.cluster_names.add(name)
        cluster = Domain(name, self) if is_domain else Cluster(name, self)
        self.clusters.append(cluster)
        return cluster

    def add_trait(self, name: str, tree: Tree) -> 'Trait':
        if name in self.trait_names:
            raise SyntaxError(f"{get_pos(tree)} trait by name '{name}' already defined in namespace '{self.name}'")
        if not name[0].isupper():
            raise SyntaxError(f'{get_pos(tree)} First character of trait name {name} is not Uppercase')
        self.trait_names.add(name)
        trait = Trait(name)
        self.traits.append(trait)
        return trait

    def add_trait_alias(self, names: list[str], tree: Tree):
        name = names[0]
        if name in self.trait_names:
            raise SyntaxError(f"{get_pos(tree)} trait by name '{name}' already defined in namespace '{self.name}'")
        self.trait_names.add(name)
        if not name[0].isupper():
            raise SyntaxError(f'{get_pos(tree)} First character of trait name {name} is not Uppercase')
        self.trait_aliases.append(names)

    def finalise(self):
        self.clusters.sort(key=lambda v: v.name)
        for c in self.clusters:
            c.finalise()
        self.traits.sort(key=lambda v: v.name)


class Repr:
    def __init__(self, parsed):
        self.input_file = input_file
        self.includes: list[str] = []
        self.export_module: str
        self.import_modules: list[str] = []
        self.namespaces: list[Namespace] = []
        self.namespaces_dict: dict[str, Namespace] = {}
        self.has_cluster = False
        self.has_trait = False
        self.walk(parsed)
        self.finalise()

    def walk(self, parsed):
        includes: set[str] = set()
        import_modules: set[tuple[str, str]] = set()
        for t in parsed.children:
            if t.data == 'include':
                includes.add(t.children[0].value)
            elif t.data == 'includes':
                for include in t.children:
                    includes.add(include.children[0].value)
            elif t.data == 'export_module':
                self.export_module = t.children[0].value
            elif t.data == 'import_module':
                import_modules.add((t.children[0].value, t.children[1].value))
            elif t.data == 'namespace':
                name = t.children[0].value
                self.get_namespace(name).walk(t.children[1:])
            elif t.data == 'cluster':
                self.visit_cluster("", t)
            elif t.data == imported('trait_def'):
                self.visit_trait_def("", t)
            elif t.data == imported('trait_alias'):
                self.visit_trait_alias("", t)
            else:
                raise SyntaxError(f'{get_pos(t)} Unknown token: {t.data}')
        self.includes = sorted(includes)
        self.import_modules = [f"{impt} {name}" for impt, name in sorted(import_modules, key=lambda v: v[1])]

    def visit_cluster(self, source_ns: str, tree: Tree):
        has_template = not isinstance(tree.children[0], Token)
        type_index = 1 if has_template else 0
        name_index = type_index + 1
        name, namespace = self.split_namespace(tree, source_ns, tree.children[name_index].value)
        cluster = namespace.add_cluster(name, tree.children[name_index], is_domain=tree.children[type_index].value == "domain")
        if has_template:
            cluster.add_template(tree.children[0])
        cluster.walk(tree.children[(name_index+1):])

    def visit_trait_def(self, source_ns: str, tree: Tree):
        name, namespace = self.split_namespace(tree, source_ns, tree.children[0].value)
        trait = namespace.add_trait(name, tree)
        trait.walk(tree.children[1:])

    def visit_trait_alias(self, source_ns: str, tree: Tree):
        name, namespace = self.split_namespace(tree, source_ns, tree.children[0].value)
        namespace.add_trait_alias([c.value for c in tree.children], tree)

    def split_namespace(self, tree: Tree, source_ns: str, fq_name: str) -> tuple[str, Namespace]:
        pos = fq_name.rfind("::")
        if pos == -1:
            if not source_ns:
                raise SyntaxError(
                    f"{get_pos(tree)} trait/alias may not be defined in root namespace, please namespace-qualify {fq_name} "
                    f"as your::ns::{fq_name} or wrap {fq_name} in namespace your::ns {'{ ... }'}")
            return (fq_name, self.get_namespace(source_ns))
        else:
            namespace = fq_name[0:pos]
            if namespace.startswith("::"):
                raise SyntaxError(f"{get_pos(tree)} trait/alias namespace-qualifier '{namespace}' may not reference the root namespace")
            if source_ns:
                namespace = source_ns + "::" + namespace
            name = fq_name[pos+2:]
            return (name, self.get_namespace(namespace))

    def get_namespace(self, name: str) -> Namespace:
        if name not in self.namespaces_dict:
            self.namespaces_dict[name] = Namespace(name, self)
        return self.namespaces_dict[name]

    def finalise(self):
        self.namespaces = sorted(self.namespaces_dict.values(), key=lambda v: v.name)
        for n in self.namespaces:
            n.finalise()
        self.has_cluster = any(len(namespace.clusters) != 0 for namespace in self.namespaces)
        self.has_trait = any(len(namespace.traits) != 0 for namespace in self.namespaces)


template_loader = jinja2.FileSystemLoader(searchpath=dir_path)
template_env = jinja2.Environment(loader=template_loader, trim_blocks=True, lstrip_blocks=True)
jinja_template = template_env.get_template(f"template.{'ixx' if is_module else 'hxx'}.jinja")
output_text = jinja_template.render(repr=Repr(parsed), export="export " if is_module else "ARC_MODULE_EXPORT\n")

with open(output_file, 'a+') as file:
    file.seek(0)
    currentText = file.read()
    if currentText != output_text:
        print(f"ARC {"created" if currentText == "" else "changed"}: {output_file}")
        file.truncate(0)
        file.write(output_text)
