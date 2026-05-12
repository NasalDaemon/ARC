#!/usr/bin/python3

import argparse
from bisect import bisect_right
from collections import defaultdict
from functools import cached_property
from typing import Optional
import jinja2
from lark import Lark, Tree, Token, UnexpectedInput
from lark.visitors import Visitor_Recursive
from lark.reconstruct import Reconstructor
from pathlib import Path
import re


dir_path = Path(__file__).resolve().parent

arg_parser = argparse.ArgumentParser(
    prog='ARC generator',
    description='Generates cpp source files from ARC DSL')

arg_parser.add_argument('-i', '--input')
arg_parser.add_argument('-o', '--output')
arg_parser.add_argument('-m', '--module', action='store_true')
arg_parser.add_argument('-q', '--quiet', action='store_true')

args = arg_parser.parse_args()

input_file: str = args.input
input_path = Path(input_file).resolve()
output_file: str = args.output
is_module: bool = args.module
is_embedded: bool = input_path.suffixes[-1] != '.arc'
quiet: bool = args.quiet

grammar_file = dir_path.joinpath(dir_path, 'arc_module.lark' if is_module else 'arc_header.lark')
arc_parser = Lark.open(grammar_file, maybe_placeholders=False, parser='lalr', cache=True)

reconstructor = Reconstructor(arc_parser)

def reconstruct(tree) -> str:
    """Reconstruct a parse tree to a C++ string, stripping DSL backtick escapes."""
    return reconstructor.reconstruct(tree).replace('`', '')

section_lines: list[tuple[int, int, int]] = []


def get_line(line: int, col: int) -> tuple[int, int]:
    if not is_embedded:
        return line, col
    source_lines = [sl[0] for sl in section_lines]
    index = bisect_right(source_lines, line) - 1

    if line == section_lines[index][0] or line == 1:
        col += section_lines[index][2]
        col += len("arc-begin") - 1
    line = line - section_lines[index][0] + section_lines[index][1]

    return line, col


with open(input_file, 'r') as file:
    text = file.read()
    if is_embedded:
        sections = []
        outer_line_count = 0
        inner_line_count = 0
        begin = 'arc-begin'
        end = 'arc-end'
        while True:
            begin_pos = text.find(begin)
            if begin_pos == -1:
                assert sections, f"'{begin}' not found in {input_path}"
                break
            outer_line_count += text[:begin_pos].count("\n")
            if (npos := text.rfind("\n", 0, begin_pos)) != -1:
                outer_col_count = begin_pos - npos
            section_lines.append((inner_line_count, outer_line_count, outer_col_count))
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
        print(f"{input_file}:{line}:{col} parse error:\n{e.get_context(text)}")
        return False

    parsed = arc_parser.parse(text, on_error=on_parse_error)


def imported(lark_rule: str):
    return f'arc__{lark_rule}'


def get_pos(t: Tree | Token | None, full_path: bool = True) -> str:
    if t is None:
        return str(input_path) if full_path else input_path.name
    if isinstance(t, Token):
        line, col = get_line(t.line, t.column)
        return f"{str(input_path) if full_path else input_path.name}:{line}:{col}"
    if isinstance(t, Tree):
        return get_pos(t.children[0], full_path=full_path)
    else:
        raise TypeError(f"Expected Tree or Token, got {type(t)}")


_NAMED_EXPR_RE = re.compile(r'^([A-Za-z_]\w*)\s*:\s*(.+)$', re.DOTALL)

def _parse_named_expr(text: str) -> tuple[str, str]:
    m = _NAMED_EXPR_RE.match(text)
    if m:
        return m.group(1), m.group(2).strip()
    return "_", text


def _expand_clause_body(tree) -> str:
    """Expand a proto_clause_body tree to a C++ expression string.
    Handles both the paren form '(expr)' and the bare proto ref 'Variant(args)'."""
    if tree.data == imported('proto_clause_body'):
        tree = tree.children[0]
    if tree.data == imported('clause_paren'):
        return reconstruct(tree.children[0]).strip()
    # proto_bare_ref: NAME ["(" [bracket_content] ")"]
    name = str(tree.children[0])
    if len(tree.children) > 1:
        args = reconstruct(tree.children[1]).strip()
        return f"proto.{name}({args})"
    return f"proto.{name}()"


_PROTO_CALL_RE = re.compile(r'\bproto\.(\w+)\s*\(')

def extract_proto_calls(expr: str) -> list[str]:
    """Return all proto.method(...) substrings found in expr (paren-balanced)."""
    calls = []
    pos = 0
    while m := _PROTO_CALL_RE.search(expr, pos):
        paren = m.end() - 1
        depth = 1
        i = paren + 1
        while i < len(expr) and depth > 0:
            if expr[i] == '(':
                depth += 1
            elif expr[i] == ')':
                depth -= 1
            i += 1
        calls.append(expr[m.start():i])
        pos = i
    return calls


class AddColonToRequiresStatements(Visitor_Recursive):
    def arc__cpp_requires_statement(self, tree):
        if not isinstance(tree.children[-1], Token) or tree.children[-1].type != imported('SEMICOLON'):
            tree.children.append(Token(imported('SEMICOLON'), ';'))

add_colon_to_requires_statements = AddColonToRequiresStatements()

def get_value(t: Tree | Token) -> str:
    """Get the string value from either a Token or a Tree containing a Token"""
    if isinstance(t, Token):
        return t.value
    elif isinstance(t, Tree):
        assert len(t.children) == 1, f"Expected Tree with single child, got {len(t.children)} children"
        return get_value(t.children[0])
    else:
        raise TypeError(f"Expected Tree or Token, got {type(t)}")

NO_TRAIT = ("~", "@notrait")
NO_GROUP = ("~", "@nogroup")
PARENT_NODE = ("..", "@parent")
GLOBAL_NODE = ("^", "@global")
GLOBAL_TRAIT = ("^", "@global")
ALL_NODES = ("*", "@all")
SPECIAL_NODES = PARENT_NODE + GLOBAL_NODE + ALL_NODES

def is_no_trait(trait: str | None) -> bool:
    if trait is None:
        return False
    return trait in NO_TRAIT or "arc::NoTrait<" in trait


def is_global_trait(trait: str | None) -> bool:
    if trait is None:
        return False
    return trait in GLOBAL_TRAIT or "arc::Global<" in trait


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
        string = reconstruct(tree)
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
        if name in PARENT_NODE:
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
        return f'::arc::Repeater<{self.trait}, {len(self.connections)}>'

    def add_connection(self, connection: 'Connection'):
        assert connection.trait == self.trait, (connection.trait, self.trait)
        connection.trait = f"::arc::RepeaterTrait<{len(self.connections)}>"
        self.connections.append(connection)


class Connection:
    def __init__(
        self,
        pos: str,
        to_node: "Node | Repeater",
        trait: str,
        to_trait: str | None = None,
        traitblock_id: Optional[int] = None,
        fanout_id: Optional[int] = None,
        to_repeater: Repeater | None = None,
    ):
        self.pos = pos
        self.to_node: Node | Repeater = to_node
        self.trait: str = trait
        if self.trait in NO_TRAIT:
            self.trait = f"::arc::NoTrait<{to_node.node_alias}>"
            self.to_trait = self.trait
        else:
            self.to_trait: str = to_trait or trait
        self.traitblock_id = traitblock_id
        self.fanout_id = fanout_id
        self.to_repeater = to_repeater

    @property
    def context(self):
        return self.to_node.context

    def copy(self):
        return Connection(self.pos, self.to_node, self.trait, self.to_trait, self.traitblock_id, self.fanout_id, self.to_repeater)

    def is_renaming(self):
        return self.trait != self.to_trait


class Node:
    def __init__(self, name: str, tree: Tree | None, impl: str, cluster: 'Cluster | Domain', is_first: bool, intermediate_aliases: list[tuple[str, str]] = []):
        self.repeaters: list[Repeater] = []
        self.connections: list[Connection] = []
        self.clients: list[tuple[str, 'Node', str]] = [] # [(position, client_node, trait)]
        self.name: str = name
        self.impl: str = impl
        self.cluster = cluster
        self.is_nexus: bool = is_first and isinstance(cluster, Domain)
        self.intermediate_aliases = intermediate_aliases
        self.is_parent = False
        self.is_global = False
        self.is_sink_node = False
        self.no_traits = False
        self.connected_to_global = False
        if name in PARENT_NODE:
            self.is_parent = True
            self.context = "Context"
        elif name in GLOBAL_NODE:
            self.is_global = True
            self.context = "Context"
        else:
            self.is_unary = name.upper() != name
            self.has_state = name[0].isupper()
            self.context: str = name + '_'
            if name[0] == '_' or name[-1] == '_':
                raise SyntaxError(f"{get_pos(tree)} Node name '{name}' in '{cluster.full_name}' must not start or end with an underscore '_'")
            if self.is_nexus and not self.is_unary:
                raise SyntaxError(f"{get_pos(tree)} Nexus node '{name}' in '{cluster.full_name}' must be a unary node")

    @property
    def is_user_node(self):
        return True

    @property
    def node_alias(self):
        return f"{self.context}Node_"

    def add_connection(self, pos, traitblock_id: Optional[int], fanout_id: Optional[int], is_override: bool, to_node: 'Node', trait: str, *, to_trait: str | None = None):
        assert self.cluster == to_node.cluster
        if to_node == self:
            raise SyntaxError(f"{pos} cannot connect '{self.name}' to itself")
        if self.is_global:
            raise SyntaxError(f"{pos} cannot connect from @global to any other node")
        if self.is_sink_node and not self.is_parent and not to_node.is_sink_node:
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

        if effective_to_trait in NO_TRAIT:
            if to_node.is_global or to_node.is_parent:
                raise SyntaxError(f"{pos} Cannot use no-trait shorthand '~' to connect to @global or @parent node in {self.cluster.cluster_class} '{self.cluster.full_name}'. "
                                  "Use a named trait like 'arc::NoTrait<TargetNode>' instead.")

        if effective_to_trait in GLOBAL_TRAIT:
            if not to_node.is_global:
                raise SyntaxError(f"{pos} Cannot use @global trait to connect to non-global node '{to_node.name}' in {self.cluster.cluster_class} '{self.cluster.full_name}'")
            self.connected_to_global = True
            # Nothing left to do if this is just a blanket connection to the global node
            return

        if is_global_trait(effective_to_trait):
            if not to_node.is_global:
                raise SyntaxError(f"{pos} Cannot use global trait '{effective_to_trait}' to connect to non-global node '{to_node.name}' in {self.cluster.cluster_class} '{self.cluster.full_name}'")
        elif to_node.is_global:
                to_trait = f"::arc::Global<{effective_to_trait}>"

        to_node.add_client(pos, self, effective_to_trait)
        connection = Connection(pos, to_node, trait=trait, to_trait=to_trait, traitblock_id=traitblock_id, fanout_id=fanout_id)

        # Deal with repeated connections to the same trait
        if existing := next((c for c in self.connections if c.trait == connection.trait), None):
            # Check that this new connection to the same trait is being added in the right place
            if self.cluster.is_domain and fanout_id is None:
                raise SyntaxError(f"{pos}: '{self.name}' in domain '{self.cluster.full_name}', repeated connections to trait '{connection.trait}' must be via explicit fanout")
            if existing.traitblock_id != traitblock_id or existing.fanout_id != fanout_id:
                error_msg = (
                    f"{pos}:\nTrait '{connection.trait}' connection from '{self.name}' to '{to_node.name}' in "
                    f"{self.cluster.cluster_class} '{self.cluster.full_name}' "
                    f"conflicts with previous connection: {existing.pos}.\n")
                if existing.fanout_id is not None:
                    error_msg += "Last connection was an explicit fanout, which cannot be added to in a later connection."
                elif fanout_id is not None:
                    error_msg += "Cannot add a fanout to an existing non-fanout connection."
                else:
                    error_msg += "Repeated connections must be in the same connection block."
                raise SyntaxError(error_msg)

            # Add to existing repeater or create a new one
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
        self.parent_node = Node("@parent", None, self.name, cluster=self, is_first=False)
        self.global_node = Node("@global", None, self.name, cluster=self, is_first=False)
        self.user_nodes: list[Node] = []
        self.repeaters: list[Repeater] = []
        self.nodes: list[Node | Repeater] = []
        self.aliases: dict[str, str] = {}
        self.dependencies: list[str] = []
        # trait: [(Node, position)] where len(list) > 1 only when trait is NO_TRAIT
        self.sink_traits: dict[str, list[tuple[Node, str]]] = defaultdict(list)
        self.trunk_traits: list[str] = []

    @property
    def is_domain(self) -> bool:
        return False

    @property
    def cluster_type(self) -> str:
        return "::arc::Cluster"

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

    def add_template(self, tree):
        for c in tree.children:
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

    def is_trunk_arrow(self, arrow: Tree | Token) -> bool:
        sign: str = self.arrow_sign(arrow) if isinstance(arrow, Tree) else arrow.value
        has_eq = '=' in sign
        has_dash = '-' in sign
        if has_eq and has_dash:
            raise SyntaxError(f"{get_pos(arrow)} Arrow '{sign}' in {self.cluster_class} '{self.full_name}' "
                              "cannot mix '-' and '=' bars.")
        if has_eq and isinstance(arrow, Tree) and arrow.data not in (imported('left_arrow'), imported('right_arrow'), imported('bi_arrow')):
            raise SyntaxError(f"{get_pos(arrow)} Trunk arrow '{sign}' in {self.cluster_class} '{self.full_name}' "
                              "cannot be combined with an inline arrow trait '(Trait)'.")
        return has_eq

    def validate_arrow(self, arrow: Tree):
        sign = self.arrow_sign(arrow)
        chevrons = max(sign.count('<'), sign.count('>'))
        if chevrons > 1:
            raise SyntaxError(f"{get_pos(arrow)} Only one chevron is allowed per arrow in clusters.")
        return False

    def normalise_name(self, name: str) -> str:
        if name in PARENT_NODE:
            return '@parent'
        if name in GLOBAL_NODE:
            return '@global'
        if name in ALL_NODES:
            return '@all'

        return name

    def walk(self, children):
        aliases: dict[str, str] = {}
        nodes: dict[str, Node] = {}
        nodes["@parent"] = self.parent_node
        nodes["@global"] = self.global_node
        explicit_connection_seen = False
        sink_connection_seen = False
        left_trait: str
        right_trait: str
        bi_trait: bool
        is_trunk_block: bool = False
        traitblock_id = 0
        fanout_id = 0

        def make_sink(name: str, token: Tree | Token, explicit: bool):
            nonlocal explicit_connection_seen, sink_connection_seen

            if left_trait in GLOBAL_TRAIT:
                if name not in GLOBAL_NODE:
                    raise SyntaxError(f"{get_pos(token)} Sink global node in {self.cluster_class} '{self.full_name}' must be named '@global'")
                if not explicit:
                    raise SyntaxError(f"{get_pos(token)} Sink global node in {self.cluster_class} '{self.full_name}' must use an explicit `@all -->` connection")
                if explicit_connection_seen or sink_connection_seen:
                    raise SyntaxError(f"{get_pos(token)} Sink global node in {self.cluster_class} '{self.full_name}' must be the very first connection in the cluster")
                for name, node in nodes.items():
                    if name not in GLOBAL_NODE:
                        node.connected_to_global = True
            else:
                sink_connection_seen = True
                if self.is_domain and name not in GLOBAL_NODE:
                    raise SyntaxError(f"{get_pos(token)} Sink node '{name}' not permitted in domain '{self.full_name}'")
                if explicit_connection_seen:
                    raise SyntaxError(f"{get_pos(token)} Sink node '{name}' in {self.cluster_class} '{self.full_name}' must be declared before any explicit connections")
                if bi_trait:
                    raise SyntaxError(f"{get_pos(token)} Sink node '{name}' in {self.cluster_class} '{self.full_name}' cannot have bi-directional trait")
                if left_trait not in NO_TRAIT and left_trait in self.sink_traits:
                    raise SyntaxError(f"{get_pos(token)} Sink node '{name}' in {self.cluster_class} '{self.full_name}' is using the trait '{left_trait}' "
                                    f"already used by another sink node '{self.sink_traits[left_trait][0][0].name}' here {self.sink_traits[left_trait][0][1]}")
                node = nodes[name]
                node.is_sink_node = True
                self.sink_traits[left_trait].append((node, get_pos(token)))
                if node.is_global and not is_global_trait(left_trait):
                    self.sink_traits[f"::arc::Global<{left_trait}>"].append((node, get_pos(token)))

        def check_trunk_arrow(arrow: Tree | Token):
            arrow_is_trunk = self.is_trunk_arrow(arrow)
            if is_trunk_block and not arrow_is_trunk:
                raise SyntaxError(f"{get_pos(arrow)} Trunk block '[[...]]' in {self.cluster_class} '{self.full_name}' requires '=='-style arrows")
            if not is_trunk_block and arrow_is_trunk:
                raise SyntaxError(f"{get_pos(arrow)} '=='-style arrows in {self.cluster_class} '{self.full_name}' are only allowed inside trunk blocks '[[...]]'")

        current_token: Tree | Token = children

        try:
            for child in children:
                current_token = child
                if child.data == imported('cluster_annotations'):
                    for ann in child.children:
                        current_token = ann
                        if ann.children[0].value == "Trunk":
                            traits = []
                            for c in ann.children[1:]:
                                if isinstance(c, Tree) and c.data == imported('trunk_trait'):
                                    traits.append(reconstruct(c))
                            self.trunk_traits = traits
                        elif ann.children[-1].value == "Context":
                            self.context_name = ann.children[0].value
                        elif ann.children[-1].value == "Root":
                            self.root_name = ann.children[0].value
                        elif ann.children[-1].value == "Info":
                            self.info_name = ann.children[0].value
                        elif ann.children[-1].value == "Trunk":
                            raise SyntaxError(f"{get_pos(ann)} Trunk annotation must use explicit 'Trunk = ...' syntax")
                        else:
                            raise SyntaxError(f"{get_pos(ann)} Unknown cluster annotation: {ann.children[0].value}")
                elif child.data == imported('node'):
                    name = child.children[0].value
                    if name in nodes:
                        raise SyntaxError(f"{get_pos(child)} Node '{name}' already defined in {self.cluster_class} '{self.full_name}'")
                    impl = reconstruct(child.children[1])
                    intermediate_aliases: list[tuple[str, str]] = []
                    if len(child.children) > 2:
                        for wrapper in child.children[2:]:
                            current_token = wrapper
                            cls = wrapper.children[0].value
                            impl_alias = f"{name}_inner{len(intermediate_aliases)}_"
                            intermediate_aliases.append((impl_alias, impl))
                            args = [impl_alias]
                            if len(wrapper.children) > 1:
                                args.extend(reconstruct(arg) for arg in wrapper.children[1].children[1:-1:2])
                            impl = f"{cls}<{', '.join(args)}>"

                    is_first = len(nodes) == 2
                    nodes[name] = Node(name, child, impl, cluster=self, is_first=is_first, intermediate_aliases=intermediate_aliases)
                elif child.data == imported('connection_block'):
                    traitblock_id += 1
                    for child in child.children:
                        current_token = child
                        if child.data == imported('connection_aliases'):
                            for child in child.children:
                                current_token = child
                                alias = get_value(child.children[0])
                                type_string = reconstruct(child.children[1])
                                if alias in aliases:
                                    if aliases.get(alias) != type_string:
                                        raise SyntaxError(f"{get_pos(alias)} Alias '{alias}' changed from {aliases.get(alias)} to {type_string}")
                                else:
                                    aliases[alias] = type_string
                        elif child.data in (imported('connection_trait_normal'), imported('connection_trait_trunk')):
                            is_trunk_block = child.data == imported('connection_trait_trunk')

                            if is_trunk_block:
                                # Split children at BARROW token (if present) for bi-trunk [[A <==> B]]
                                left_trait_trees: list[Tree] = []
                                right_trait_trees: list[Tree] = []
                                barrow_token: Token | None = None
                                for c in child.children:
                                    if isinstance(c, Token) and c.type == imported('BARROW'):
                                        barrow_token = c
                                        continue
                                    (left_trait_trees if barrow_token is None else right_trait_trees).append(c)

                                bi_trait = barrow_token is not None
                                if bi_trait and '-' in barrow_token.value:
                                    raise SyntaxError(f"{get_pos(barrow_token)} Bi-trunk separator in {self.cluster_class} '{self.full_name}' "
                                                      f"must use '='-style (e.g. '<=>'), got '{barrow_token.value}'")
                                if bi_trait and (len(left_trait_trees) > 1 or len(right_trait_trees) > 1):
                                    raise SyntaxError(f"{get_pos(barrow_token)} Bi-directional trunks only support cluster trunks "
                                                      f"(single type on each side, e.g. '[[cluster::A <=> cluster::B]]'). "
                                                      f"Inline trunks with '+' cannot be bi-directional in {self.cluster_class} '{self.full_name}'")

                                def build_trunk_trait(trees: list[Tree], pos_token: Tree | Token) -> str:
                                    trait_strs = []
                                    for t in trees:
                                        s = reconstruct(t)
                                        if s in NO_TRAIT:
                                            raise SyntaxError(f"{get_pos(t)} Trunk block '[[...]]' in {self.cluster_class} '{self.full_name}' "
                                                              f"cannot use special trait '{s}'")
                                        trait_strs.append(s)
                                    if len(trait_strs) == 1:
                                        s = trait_strs[0]
                                        if s in GLOBAL_TRAIT:
                                            return s
                                        return f"::arc::TrunkOf<{s}>"
                                    if any(t in GLOBAL_TRAIT for t in trait_strs):
                                        raise SyntaxError(f"{get_pos(pos_token)} Trunk block '[[...]]' in {self.cluster_class} '{self.full_name}' "
                                                          f"cannot use @global trait in inline trunk trait '{trait_strs}'")
                                    return f"::arc::Trunk<{', '.join(trait_strs)}>"

                                left_trait = build_trunk_trait(left_trait_trees, child)
                                if bi_trait:
                                    right_trait = build_trunk_trait(right_trait_trees, child)
                                else:
                                    right_trait = left_trait
                            else:
                                left_trait = reconstruct(child.children[0])
                                if len(child.children) == 1:
                                    bi_trait = False
                                    right_trait = left_trait
                                else:
                                    bi_trait = True
                                    right_trait = reconstruct(child.children[-1])

                            if left_trait in GLOBAL_TRAIT or right_trait in GLOBAL_TRAIT:
                                if not is_trunk_block:
                                    raise SyntaxError(f"{get_pos(child)} [@global] sink trait must use double brackets: [[@global]]")
                                if bi_trait:
                                    raise SyntaxError(f"{get_pos(child)} Bi-directional connections cannot use @global trait in {self.cluster_class} '{self.full_name}'")
                                if self.is_domain:
                                    raise SyntaxError(f"{get_pos(child)} Global sink trait '{left_trait}' not permitted in domain '{self.full_name}'. "
                                                      f"Use only qualified traits in domains, like `[arc::Global<app::Trait>] node --> @global`.")

                            if left_trait not in NO_TRAIT and left_trait in self.sink_traits:
                                raise SyntaxError(f"{get_pos(child.children[0])} Trait '{left_trait}' already allocated to sink node "
                                                  f"'{self.sink_traits[left_trait][0][0].name}' in {self.cluster_class} '{self.full_name}' here {self.sink_traits[left_trait][0][1]}")
                            if right_trait not in NO_TRAIT and right_trait in self.sink_traits:
                                raise SyntaxError(f"{get_pos(child.children[-1])} Trait '{right_trait}' already allocated to sink node "
                                                  f"'{self.sink_traits[right_trait][0][0].name}' in {self.cluster_class} '{self.full_name}' here {self.sink_traits[right_trait][0][1]}")
                        elif child.data in (imported('sink_node_implicit'), imported('sink_node_larrow')):
                            if is_trunk_block and left_trait not in GLOBAL_TRAIT:
                                raise SyntaxError(f"{get_pos(child)} Trunk block '[[...]]' in {self.cluster_class} '{self.full_name}' cannot contain sink connections")
                            if isinstance(child.children[0], Tree) and child.children[0].data == imported('node_names_fanout'):
                                raise SyntaxError(f"{get_pos(child.children[0])} Sink nodes cannot use explicit fan-out syntax '{{node1, node2}}'")
                            if child.data == imported('sink_node_larrow'):
                                check_trunk_arrow(child.children[1])
                            for c in child.children[0].children:
                                current_token = c
                                name = self.normalise_name(get_value(c))
                                make_sink(name, c, explicit=child.data.endswith('arrow'))
                        elif child.data == imported('sink_node_rarrow'):
                            if is_trunk_block and left_trait not in GLOBAL_TRAIT:
                                raise SyntaxError(f"{get_pos(child)} Trunk block '[[...]]' in {self.cluster_class} '{self.full_name}' cannot contain sink connections")
                            if isinstance(child.children[0], Tree) and child.children[0].data == imported('node_names_fanout'):
                                raise SyntaxError(f"{get_pos(child.children[0])} Sink nodes cannot use explicit fan-out syntax '{{node1, node2}}'")
                            check_trunk_arrow(child.children[1])
                            for c in child.children[-1].children:
                                current_token = c
                                name = self.normalise_name(get_value(c))
                                make_sink(name, c, explicit=True)
                        elif child.data == imported('connection'):
                            if left_trait in GLOBAL_TRAIT:
                                if explicit_connection_seen or sink_connection_seen:
                                    raise SyntaxError(f"{get_pos(child)} @global trait connections must be the very first connections in {self.cluster_class} '{self.full_name}'")
                            else:
                                # Treat only non-global connections as explicit connections
                                explicit_connection_seen = True

                            for i in range(0, len(child.children) - 1, 2):
                                current_token = child.children[i]
                                lnames, arrow, rnames = (child.children[i], child.children[i+1], child.children[i+2])
                                lnodes = [nodes[self.normalise_name(get_value(name))] for name in lnames.children]
                                rnodes = [nodes[self.normalise_name(get_value(name))] for name in rnames.children]
                                check_trunk_arrow(arrow)
                                is_override = self.validate_arrow(arrow)
                                pos = get_pos(arrow)

                                if is_trunk_block and arrow.data == imported('bi_arrow') and not bi_trait:
                                    raise SyntaxError(f"{pos} in {self.cluster_class} '{self.full_name}': bi-directional arrows in a trunk block require a bi-trunk header '[[A <==> B]]'.")

                                tid, lfid, rfid = traitblock_id, None, None
                                if lnames.data == imported('node_names_fanout'):
                                    fanout_id += 1
                                    lfid = fanout_id
                                if rnames.data == imported('node_names_fanout'):
                                    fanout_id += 1
                                    rfid = fanout_id

                                def validate_fanout(right_arrow: bool, double_headed: bool = False):
                                    tfid, sfid = (rfid, lfid) if right_arrow else (lfid, rfid)
                                    if sfid is not None and not double_headed:
                                        raise SyntaxError(f"{pos} many-to-one connections should not use fan-out syntax {'{node1, node2}'}")
                                    count = len(rnodes) if right_arrow else len(lnodes)
                                    if tfid is None:
                                        if count > 1:
                                            raise SyntaxError(f"{pos} Inline one-to-many connections must use explicit fan-out syntax {'{node1, node2}'}")
                                    else:
                                        if count < 2:
                                            raise SyntaxError(f"{pos} Explicit fanout connections require multiple target nodes {'{node1, node2}'}")

                                lrnodes = ((lnode, rnode) for rnode in rnodes for lnode in lnodes)
                                if arrow.data == imported('left_arrow'):
                                    validate_fanout(right_arrow=False)
                                    for lnode, rnode in lrnodes:
                                        if is_trunk_block and lnode.is_global:
                                            raise SyntaxError(f"{pos} in {self.cluster_class} '{self.full_name}': Trunk connection cannot target @global node.")
                                        rnode.add_connection(pos, tid, lfid, is_override, lnode, left_trait)
                                elif arrow.data == imported('right_arrow'):
                                    validate_fanout(right_arrow=True)
                                    for lnode, rnode in lrnodes:
                                        if is_trunk_block and rnode.is_global:
                                            raise SyntaxError(f"{pos} in {self.cluster_class} '{self.full_name}': Trunk connection cannot target @global node.")
                                        lnode.add_connection(pos, tid, rfid, is_override, rnode, right_trait)
                                elif arrow.data == imported('bi_arrow'):
                                    validate_fanout(right_arrow=False, double_headed=True)
                                    validate_fanout(right_arrow=True, double_headed=True)
                                    for lnode, rnode in lrnodes:
                                        rnode.add_connection(pos, tid, lfid, is_override, lnode, left_trait)
                                        lnode.add_connection(pos, tid, rfid, is_override, rnode, right_trait)
                                elif arrow.data == imported('left_arrow_from'):
                                    validate_fanout(right_arrow=False)
                                    from_trait = reconstruct(arrow.children[-1].children[0])
                                    to_trait = left_trait
                                    for lnode, rnode in lrnodes:
                                        rnode.add_connection(pos, tid, lfid, is_override, lnode, from_trait, to_trait=to_trait)
                                elif arrow.data == imported('right_arrow_from'):
                                    validate_fanout(right_arrow=True)
                                    from_trait = reconstruct(arrow.children[0].children[0])
                                    to_trait = right_trait
                                    for lnode, rnode in lrnodes:
                                        lnode.add_connection(pos, tid, rfid, is_override, rnode, from_trait, to_trait=to_trait)
                                elif arrow.data == imported('left_arrow_to'):
                                    validate_fanout(right_arrow=False)
                                    from_trait = left_trait
                                    to_trait = reconstruct(arrow.children[0].children[0])
                                    for lnode, rnode in lrnodes:
                                        rnode.add_connection(pos, tid, lfid, is_override, lnode, from_trait, to_trait=to_trait)
                                elif arrow.data == imported('right_arrow_to'):
                                    validate_fanout(right_arrow=True)
                                    from_trait = right_trait
                                    to_trait = reconstruct(arrow.children[-1].children[0])
                                    for lnode, rnode in lrnodes:
                                        lnode.add_connection(pos, tid, rfid, is_override, rnode, from_trait, to_trait=to_trait)
                                elif arrow.data == imported('left_arrow_both'):
                                    validate_fanout(right_arrow=False)
                                    to_trait = reconstruct(arrow.children[0].children[0])
                                    from_trait = reconstruct(arrow.children[-1].children[0])
                                    for lnode, rnode in lrnodes:
                                        rnode.add_connection(pos, tid, lfid, is_override, lnode, from_trait, to_trait=to_trait)
                                elif arrow.data == imported('right_arrow_both'):
                                    validate_fanout(right_arrow=True)
                                    from_trait = reconstruct(arrow.children[0].children[0])
                                    to_trait = reconstruct(arrow.children[-1].children[0])
                                    for lnode, rnode in lrnodes:
                                        lnode.add_connection(pos, tid, rfid, is_override, rnode, from_trait, to_trait=to_trait)
                                else:
                                    raise SyntaxError(f'{pos} Unknown arrow: {arrow.data}')
                        else:
                            raise SyntaxError(f'{get_pos(child)} Unknown connection section: {child.data}')
                else:
                    raise SyntaxError(f'{get_pos(child)} Unknown {self.cluster_class} section: {child.data}')
        except SyntaxError:
            raise
        except Exception as e:
            print(f"ERROR: {get_pos(current_token)} raised exception: {e}")
            raise

        # Connect all nodes (including parent) to sink nodes at very end
        sink_traits = self.sink_traits
        self.sink_traits = {}
        for trait, sink_nodes in sink_traits.items():
            for sink_node, pos in sink_nodes:
                for node in nodes.values():
                    if node.name not in GLOBAL_NODE + (sink_node.name,):
                        node.add_connection(pos, None, None, False, sink_node, trait)

        self.user_nodes = [node for node in nodes.values() if node.name not in PARENT_NODE + GLOBAL_NODE]
        self.aliases.update(sorted(aliases.items()))
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

        if self.trunk_traits is not None:
            parent_traits = {c.trait for c in self.parent_node.connections}
            for trait in self.trunk_traits:
                if trait not in parent_traits:
                    raise SyntaxError(f"{self.cluster_class} '{self.full_name}' trunk annotation lists trait '{trait}' "
                                      f"but there is no '[{trait}] .. --> node' parent connection")

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
    def is_domain(self) -> bool:
        return True

    @property
    def cluster_type(self) -> str:
        return "::arc::Domain<::arc::DomainParams{.MaxDepth=3}>"

    @property
    def cluster_class(self) -> str:
        return "domain"

    def predicates(self, node: Node | Repeater) -> list[str]:
        if isinstance(node, Repeater):
            return []
        preds = ["::arc::pred::HasDepends"]
        if not node.is_unary:
            preds.append("::arc::pred::NonUnary")
            return preds
        preds.append("::arc::pred::Unary")
        if not node.has_state:
            preds.append("::arc::pred::Stateless")
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
        # nexus can connect to anything in any direction, and anything can connect to global
        if from_node.is_nexus or to_node.is_nexus or to_node.is_global:
            if is_override:
                return "No override is needed for this connection, but an override was specified"
            return None

        # non-nexus nodes may not connect to parent in any direction
        if from_node.is_parent or to_node.is_parent:
            return "Only nexus node may be connected to @parent in domains"

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

    def __init__(self, trait: "Trait"):
        self.trait = trait
        self.name: str = "<unknown>"
        self.templates: list[tuple[CppType, str]] = []
        self.return_type: CppType = CppType("decltype(auto)", is_auto=True)
        self.params: list[tuple[CppType, str]] = []
        self.is_const: bool = False
        self.optional: bool = False
        self.pre_exprs: list[tuple[str, str]] = []
        self.post_exprs: list[tuple[str, str, str]] = []
        self.default_impl: str | None = None
        # Protocol clauses — use the preProto/postProto path with entry snapshotting
        self.proto_pre_exprs: list[tuple[str, str]] = []           # (pos, expr)  — `is (P)`
        self.proto_post_exprs: list[tuple[str, str, str]] = []     # (pos, name, expr) — `is (P) then (Q)` / `to (Q)`
        self.proto_if_exprs: list[tuple[str, str, str, str]] = []  # (pos, name, cond, then) — `if (P) then (Q)`
        self.implies: ImpliesData | None = None

    @property
    def has_proto_contracts(self) -> bool:
        return bool(self.proto_pre_exprs or self.proto_post_exprs or self.proto_if_exprs or self.implies)

    @property
    def has_contracts(self) -> bool:
        return bool(self.pre_exprs or self.post_exprs or self.has_proto_contracts)

    def proto_snap_calls(self, include_nullary: bool) -> list[str]:
        """Unique proto.method(...) calls to invoke in preProto for entry snapping."""
        seen: set[str] = set()
        result: list[str] = []
        for expr in (
            [e for _, _, e in self.proto_post_exprs]
            + [t for _, _, _, t in self.proto_if_exprs]
        ):
            for call in extract_proto_calls(expr):
                if call not in seen:
                    if not include_nullary and call.endswith("()"):
                        continue
                    seen.add(call)
                    result.append(call)
        return result

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
                    if name == "self":
                        raise SyntaxError(f"{get_pos(c)} Method parameters cannot be named 'self' as this is a reserved name")
                    self.params.append((type_, name))
            elif c.data == imported('method_qualifier'):
                self.is_const = True
            elif c.data == imported('pre_contract'):
                if len(self.params) == 0:
                    raise SyntaxError(f"{get_pos(c)} Pre-contracts cannot be specified on methods with no parameters")
                self.pre_exprs.append((get_pos(c, False), reconstruct(c.children[0])))
            elif c.data == imported('post_contract'):
                name, expr = _parse_named_expr(reconstruct(c.children[0]).strip())
                if self.return_type.str == "void" and name != "_":
                    raise SyntaxError(f"{get_pos(c)} Post-contracts cannot bind void to a name, use '_' instead")
                self.post_exprs.append((get_pos(c, False), name, expr))
            elif c.data == imported('is_clause'):
                if not self.trait.has_protocol:
                    raise SyntaxError(f"{get_pos(c)} 'is' contracts can only be used in traits with a protocol annotation")
                pre_expr = _expand_clause_body(c.children[0])
                self.proto_pre_exprs.append((get_pos(c, False), pre_expr))
                if len(c.children) > 1:
                    post_name, post_expr = _parse_named_expr(_expand_clause_body(c.children[1]))
                    self.proto_post_exprs.append((get_pos(c, False), post_name, post_expr))
            elif c.data == imported('if_clause'):
                if not self.trait.has_protocol:
                    raise SyntaxError(f"{get_pos(c)} 'if' contracts can only be used in traits with a protocol annotation")
                cond_expr = _expand_clause_body(c.children[0])
                then_name, then_expr = _parse_named_expr(_expand_clause_body(c.children[1]))
                self.proto_if_exprs.append((get_pos(c, False), then_name, cond_expr, then_expr))
            elif c.data == imported('to_clause'):
                if not self.trait.has_protocol:
                    raise SyntaxError(f"{get_pos(c)} 'to' contracts can only be used in traits with a protocol annotation")
                post_name, post_expr = _parse_named_expr(_expand_clause_body(c.children[0]))
                self.proto_post_exprs.append((get_pos(c, False), post_name, post_expr))
            elif c.data == imported('method_default_impl'):
                import textwrap

                self.default_impl = textwrap.dedent(reconstruct(c.children[0])).strip()
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
        self.templates: list[tuple[CppType, str]] = []
        self.protocol_name: str | None = None
        self.is_protocol: bool = False
        self.state_groups: list[StateGroup] = []

    @property
    def wrapping_namespace(self) -> str:
        return "inline namespace trait"

    @property
    def has_protocol(self) -> bool:
        return not self.is_protocol and self.protocol_name is not None

    def walk(self, children):
        for c in children:
            if c.data == imported('trait_annotations'):
                for ann in c.children:
                    if ann.data == imported('trait_protocol_annotation'):
                        self.protocol_name = ann.children[0].value
                    elif ann.children[-1].value == "Types":
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
                    method = Method(self)
                    method.walk(c.children)
                    self.methods.append(method)
                elif c.data == imported('trait_method_elipsis'):
                    method = Method(self)
                    method.name = c.children[0].value
                    method.params.append((CppType("auto&&...", is_auto=True), "args"))
                    self.methods.append(method)
                elif c.data == imported('trait_requires'):
                    if c.children[0].data == imported('trait_requires_block'):
                        add_colon_to_requires_statements.visit(c.children[0])
                        requires = "requires " + reconstruct(c.children[0])
                    else:
                        requires = reconstruct(c.children[0].children[0])
                    if not requires.endswith(';'):
                        requires += ';'
                    self.requires.append(requires)
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown trait entity: {c.data}')
        self.methods.sort(key=lambda v: (v.name, v.params))
        self.method_names = sorted(set(method.name for method in self.methods))
        self.has_const_requires = next((method.is_const for method in self.methods if method.is_const and not method.is_template), False)
        self.has_mutable_requires = next((not method.is_const for method in self.methods if not method.is_const and not method.is_template), False)

    def add_template(self, tree):
        for c in tree.children:
            if c.data == imported('tparam_type'):
                self.templates.append((CppType(c.children[0].value.replace("typename", "class")), c.children[1].value))
            elif c.data == imported('tparam_non_type'):
                type_ = CppType.from_tree(c.children[0])
                self.templates.append((type_, c.children[1].value))
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown cluster template: {c.data}')


class Group:
    def __init__(self, name: str):
        self.name: str = name
        self.connectionsTo: list[tuple[str, bool]] = []
        self.connectionsFrom: list[tuple[str, bool]] = []


class Policy:
    def __init__(self, name: str):
        self.name: str = name
        self.groups: list[Group] = []

    def walk(self, children):
        NO_GROUP_NAME = "::arc::NoGroup"
        aliases: dict[str, str] = {g: NO_GROUP_NAME for g in NO_GROUP}
        groups: dict[str, Group] = {}
        for c in children:
            current_token = c
            try:
                if c.data == imported('group_name'):
                    group_val = get_value(c.children[0])
                    groups[group_val] = Group(group_val)
                elif c.data == imported('group_alias'):
                    alias = get_value(c.children[0])
                    group_type = reconstruct(c.children[1])
                    if alias in aliases:
                        if aliases[alias] != group_type:
                            raise SyntaxError(f"{get_pos(c)} Alias '{alias}' changed from {aliases[alias]} to {group_type}")
                    else:
                        aliases[alias] = group_type
                elif c.data == imported('group_connection'):
                    for i in range(0, len(c.children) - 1, 2):
                        current_token = c.children[i]
                        lnames, arrow, rnames = (c.children[i], c.children[i+1], c.children[i+2])
                        pos = get_pos(arrow)
                        lnodes = [get_value(name) for name in lnames.children]
                        lnodes = [groups.get(names, aliases.get(names, names)) for names in lnodes]
                        rnodes = [get_value(name) for name in rnames.children]
                        rnodes = [groups.get(names, aliases.get(names, names)) for names in rnodes]

                        def addConnection(source: str | Group, target: str | Group, write: bool):
                            if not isinstance(target, Group) and target != NO_GROUP_NAME:
                                raise SyntaxError(f"{pos} Cannot target external group '{target}' in policy '{self.name}'. "
                                                  f"External groups can only gain access to the groups in the current policy '{self.name}'. "
                                                  f"To allow access to {target}, a connection must be added in the target's policy.")
                            if not isinstance(source, Group) and not isinstance(target, Group):
                                raise SyntaxError(f"{pos} Cannot connect two aliases '{source}' and '{target}' in group '{self.name}'")
                            if isinstance(source, Group):
                                target_name = target if isinstance(target, str) else target.name
                                source.connectionsTo.append((target_name, write))
                            if isinstance(target, Group):
                                source_name = source if isinstance(source, str) else source.name
                                target.connectionsFrom.append((source_name, write))

                        lrnodes = ((lnode, rnode) for rnode in rnodes for lnode in lnodes)
                        if arrow.data.startswith(imported('group_l_r_arrow')) or arrow.data == imported('group_l_w_arrow'):
                            for lnode, rnode in lrnodes:
                                addConnection(rnode, lnode, arrow.data == imported('group_l_w_arrow'))
                        elif arrow.data == imported('group_r_r_arrow') or arrow.data == imported('group_r_w_arrow'):
                            for lnode, rnode in lrnodes:
                                addConnection(lnode, rnode, arrow.data == imported('group_r_w_arrow'))
                        elif arrow.data.startswith(imported('group_bi_')) and arrow.data.endswith('_arrow'):
                            prefix = imported('group_bi_')
                            r_to_l, l_to_r = arrow.data[len(prefix)], arrow.data[len(prefix)+1]
                            assert r_to_l in ('r', 'w')
                            assert l_to_r in ('r', 'w')
                            for lnode, rnode in lrnodes:
                                addConnection(lnode, rnode, l_to_r == 'w')
                                addConnection(rnode, lnode, r_to_l == 'w')
                        else:
                            raise SyntaxError(f'{pos} Unknown arrow: {arrow.data}')
                else:
                    raise SyntaxError(f'{get_pos(c)} Unknown policy entity: {c.data}')
            except SyntaxError:
                raise
            except Exception as e:
                print(f"ERROR: {get_pos(current_token)} raised exception: {e}")
                raise

        if not groups:
            raise SyntaxError(f'Policy {self.name} has no groups defined')

        self.groups = list(groups.values())
        self.groups.sort(key=lambda v: v.name)
        for g in self.groups:
            connectionsTo = {}
            for name, write in g.connectionsTo:
                connectionsTo[name] = connectionsTo.get(name, False) or write
            g.connectionsTo = sorted(connectionsTo.items(), key=lambda v: v[0])

            connectionsFrom = {}
            for name, write in g.connectionsFrom:
                connectionsFrom[name] = connectionsFrom.get(name, False) or write
            g.connectionsFrom = sorted(connectionsFrom.items(), key=lambda v: v[0])


class StateTransition:
    def __init__(self, from_states: list[str], to_states: list[str], bidirectional: bool):
        self.from_states = from_states
        self.to_states = to_states
        self.bidirectional = bidirectional


class StateGroup:
    def __init__(self, name: str, params: list[tuple[CppType, str]], is_top_level: bool = False):
        self.name = name
        self.params = params
        self.variants: list[str] = []
        self.transitions: list[StateTransition] = []
        self.is_top_level = is_top_level

    def matching_param_names(self, method_params: list[tuple[CppType, str]]) -> list[str] | None:
        """Return method param names matching this group's param types in order, or None if not all match."""
        if not self.params:
            return []
        group_types = [str(t) for t, _ in self.params]
        result = []
        gi = 0
        for mt, mn in method_params:
            if gi < len(group_types) and str(mt) == group_types[gi]:
                result.append(mn)
                gi += 1
            if gi == len(group_types):
                break
        return result if gi == len(group_types) else None

    @property
    def has_edges(self) -> bool:
        return bool(self.transitions)

    def adjacency(self) -> dict[str, set[str]]:
        adj: dict[str, set[str]] = {}
        for t in self.transitions:
            for f in t.from_states:
                for to in t.to_states:
                    if f != to:
                        adj.setdefault(f, set()).add(to)
                    if t.bidirectional:
                        adj.setdefault(to, set()).add(f)
        return adj

    def reachable_from(self, start: str) -> set[str]:
        visited: set[str] = set()
        stack = [start]
        adj = self.adjacency()
        while stack:
            node = stack.pop()
            if node in visited:
                continue
            visited.add(node)
            for neighbor in adj.get(node, set()):
                if neighbor not in visited:
                    stack.append(neighbor)
        return visited

    def validate_reachability(self, pos: str):
        if not self.variants:
            return
        default = self.variants[0]
        reachable = self.reachable_from(default)
        unreachable = set(self.variants) - reachable
        if unreachable:
            raise SyntaxError(f"{pos} state{'s' if len(unreachable) > 1 else ''} {', '.join(sorted(unreachable))} not reachable from default state '{default}' in {self.name}")


class StatePred:
    def __init__(self, name: str, params: list[tuple[CppType, str]], pos: str):
        self.name = name
        self.params = params
        self.implies_expr: str | None = None
        self.kind: str = 'implies'
        self.pos = pos


class ImpliesData:
    def __init__(self, assert_msg: str, expr: str, kind: str = 'implies'):
        self.assert_msg = assert_msg
        self.expr = expr
        self.kind = kind


class VariantInvariant:
    """Cross-group invariant on a state variant.

    `Left implies On`  -> kind='implies'; when source group is in Left, target group must be in On.
    `Right iff Off`    -> kind='iff';     both directions: source==Right iff target==Off.

    `target_expr`  — the C++ expression text to call/evaluate for the target. For bare
                     targets resolved to a variant or predicate, this is `proto.<name>(...)`.
    `params_names` — names bound in scope (from enclosing per blocks), used as call args.
    """
    def __init__(self, source_group: str, source_variant: str, kind: str,
                 target_expr: str, pos: str, params_names: list[str] | None = None):
        assert kind in ('implies', 'iff')
        self.source_group = source_group
        self.source_variant = source_variant
        self.kind = kind
        self.target_expr = target_expr
        self.pos = pos
        self.params_names = list(params_names or [])
        self.source_group_ref: "StateGroup | None" = None
        # Set during validation for iff invariants whose target is a bare state variant.
        self.target_variant: str | None = None
        self.target_group_ref: "StateGroup | None" = None

    @property
    def target_group(self) -> str | None:
        return self.target_group_ref.name if self.target_group_ref else None

    @property
    def has_target_group(self) -> bool:
        """True for iff invariants where target is a known variant with same-typed params as source."""
        if (self.kind != 'iff'
                or self.target_group_ref is None
                or self.source_group_ref is None):
            return False
        src_types = [str(t) for t, _ in self.source_group_ref.params]
        tgt_types = [str(t) for t, _ in self.target_group_ref.params]
        return src_types == tgt_types


class Protocol(Trait):
    def __init__(self, name: str, namespace_name: str = ""):
        super().__init__(name)
        self.is_protocol = True
        self.namespace_name = namespace_name
        self._top_group: StateGroup | None = None
        # Cross-group variant invariants (Left implies On, Right iff Off)
        self.variant_invariants: list[VariantInvariant] = []
        # Raw (pre-resolution) records of bare-name targets to resolve after walk.
        # Each entry: (invariant_or_pred, target_name, pos, scope_param_names)
        self._pending_bare_targets: list[tuple[object, str, str, list[str]]] = []

    @property
    def predicate_methods(self) -> list[Method]:
        group_names = {'proto' + g.name for g in self.state_groups}
        variant_names = set()
        for g in self.state_groups:
            variant_names.update(g.variants)
        non_pred_names = group_names.union(variant_names)
        return [m for m in self.methods if m.name not in non_pred_names]

    @property
    def wrapping_namespace(self) -> str:
        return "namespace protocol"

    def _parse_state_set(self, tree) -> list[str]:
        return [c.value for c in tree.children if isinstance(c, Token)]

    def _add_variants(self, group: StateGroup, names: list[str]):
        for name in names:
            if name not in group.variants:
                if name[0].islower():
                    raise SyntaxError(f"State name '{name}' must start with an uppercase letter")
                group.variants.append(name)

    def _apply_trans(self, group: StateGroup, arrow: Tree, lset: list[str], rset: list[str]):
        if arrow.data == imported('state_arrow_r'):
            froms, tos, bi = lset, rset, False
        elif arrow.data == imported('state_arrow_b'):
            froms, tos, bi = lset, rset, True
        elif arrow.data == imported('state_arrow_l'):
            froms, tos, bi = rset, lset, False
        else:
            raise SyntaxError(f'Unknown state arrow: {arrow.data}')
        self._add_variants(group, froms + tos)
        group.transitions.append(StateTransition(froms, tos, bi))

    def _parse_invariant_target(self, host, tree, pos: str, scope_param_names: list[str]) -> str:
        """Parse an invariant_target tree. Bracketed returns reconstructed text.
        Bare names are registered for post-walk resolution; a placeholder is stored
        and returned; resolution overwrites `target_expr` on the host object."""
        if tree.data == imported('invariant_bare_ref'):
            bare_ref = tree.children[0]  # proto_bare_ref tree
            name = str(bare_ref.children[0])
            if len(bare_ref.children) > 1:
                args = reconstruct(bare_ref.children[1]).strip()
                return f"proto.{name}({args})"
            placeholder = f"__ARC_BARE__{name}"
            self._pending_bare_targets.append((host, name, pos, list(scope_param_names)))
            return placeholder
        elif tree.data == imported('invariant_expr'):
            return reconstruct(tree.children[0]).strip()
        else:
            raise SyntaxError(f'{pos} Unknown invariant_target: {tree.data}')

    def _parse_transitions(self, children, target_group: StateGroup, scope_param_names: list[str]):
        for c in children:
            if c.data == imported('proto_state_trans'):
                for i in range(0, len(c.children) - 1, 2):
                    self._apply_trans(target_group, c.children[i + 1],
                                      self._parse_state_set(c.children[i]),
                                      self._parse_state_set(c.children[i + 2]))
            elif c.data == imported('proto_variant_invariant'):
                kind = str(c.children[1])
                src = c.children[0].value
                pos = get_pos(c, False)
                inv = VariantInvariant(target_group.name, src, kind, "", pos, scope_param_names)
                target_expr = self._parse_invariant_target(inv, c.children[2], pos, scope_param_names)
                inv.target_expr = target_expr
                self.variant_invariants.append(inv)
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown proto_state_body: {c.data}')

    def walk(self, children, inherited_params=None):
        if inherited_params is None:
            inherited_params = []
            self._temp_predicates: list[StatePred] = []
            self._scope_state_names: list[set[str]] = [set()]
            self._all_state_names: set[str] = set()

        scope_vars: set[str] = set()
        self._scope_state_names.append(scope_vars) if inherited_params else None

        scope_param_names = [n for _, n in inherited_params]
        for c in children:
            if c.data == imported('proto_state_def'):
                state_name = c.children[1].value
                if state_name in self._all_state_names:
                    raise SyntaxError(f'{get_pos(c)} state group name "{state_name}" duplicated; state names must be unique across the protocol')
                if state_name[0].islower():
                    raise SyntaxError(f'{get_pos(c)} state group name "{state_name}" must start with an uppercase letter')
                self._all_state_names.add(state_name)
                group = StateGroup(state_name, list(inherited_params))
                self.state_groups.append(group)
                self._parse_transitions(c.children[2:], group, scope_param_names)
                for v in group.variants:
                    if v in scope_vars:
                        raise SyntaxError(f'{get_pos(c)} variant name "{v}" duplicated at same scope')
                    scope_vars.add(v)
            elif c.data == imported('proto_per_def'):
                params_tree = c.children[1]
                params = [(CppType.from_tree(p.children[0]), p.children[1].value)
                          for p in params_tree.children]
                self.walk(c.children[2:], inherited_params + params)
            elif c.data == imported('proto_pred'):
                pred_name = c.children[0].value
                kind = str(c.children[1])
                pred = StatePred(pred_name, list(inherited_params), get_pos(c, False))
                pred.kind = kind
                pred.implies_expr = self._parse_invariant_target(pred, c.children[2], get_pos(c, False), scope_param_names)
                self._temp_predicates.append(pred)
            elif c.data == imported('proto_pred_with_params'):
                pred_name = c.children[0].value
                params_tree = c.children[1]
                kind = str(c.children[2])
                params = [(CppType.from_tree(p.children[0]), p.children[1].value)
                          for p in params_tree.children]
                pred = StatePred(pred_name, inherited_params + params, get_pos(c, False))
                pred.kind = kind
                pred_scope = scope_param_names + [n for _, n in params]
                pred.implies_expr = self._parse_invariant_target(pred, c.children[3], get_pos(c, False), pred_scope)
                self._temp_predicates.append(pred)
            elif c.data == imported('proto_state_trans_top'):
                default_name = 'Main'
                if default_name not in self._all_state_names:
                    self._all_state_names.add(default_name)
                    group = StateGroup(default_name, list(inherited_params), is_top_level=True)
                    self.state_groups.append(group)
                else:
                    group = next(g for g in self.state_groups if g.name == default_name)
                trans = c.children[0]
                for i in range(0, len(trans.children) - 1, 2):
                    self._apply_trans(group, trans.children[i + 1],
                                      self._parse_state_set(trans.children[i]),
                                      self._parse_state_set(trans.children[i + 2]))
                for v in group.variants:
                    if v in scope_vars and v not in self._all_state_names:
                        pass
                    scope_vars.add(v)
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown protocol body: {c.data}')

        if inherited_params:
            return

        # Resolve bare invariant-target names to variant or predicate methods.
        variant_to_group: dict[str, StateGroup] = {}
        for g in self.state_groups:
            for v in g.variants:
                variant_to_group[v] = g
        pred_by_name: dict[str, StatePred] = {p.name: p for p in self._temp_predicates}

        for host, name, pos, scope_names in self._pending_bare_targets:
            if name in variant_to_group:
                g = variant_to_group[name]
                group_param_names = [pn for _, pn in g.params]
                missing = [pn for pn in group_param_names if pn not in scope_names]
                if missing:
                    raise SyntaxError(f"{pos} bare target '{name}' requires parameters {missing} not in scope; use bracketed form")
                args = ", ".join(group_param_names)
                resolved = f"proto.{name}({args})"
                if isinstance(host, VariantInvariant) and host.kind == 'iff':
                    host.target_variant = name
            elif name in pred_by_name:
                p = pred_by_name[name]
                pred_param_names = [pn for _, pn in p.params]
                missing = [pn for pn in pred_param_names if pn not in scope_names]
                if missing:
                    raise SyntaxError(f"{pos} bare target '{name}' requires parameters {missing} not in scope; use bracketed form")
                args = ", ".join(pred_param_names)
                resolved = f"proto.{name}({args})"
            else:
                raise SyntaxError(f"{pos} bare invariant target '{name}' is not a known state variant or predicate in protocol '{self.name}'")

            placeholder = f"__ARC_BARE__{name}"
            if isinstance(host, VariantInvariant):
                host.target_expr = host.target_expr.replace(placeholder, resolved)
            elif isinstance(host, StatePred):
                host.implies_expr = host.implies_expr.replace(placeholder, resolved)
            else:
                raise AssertionError(f"Unexpected bare-target host type: {type(host)}")

        # Validate variant invariants: source variant must exist in its declared group.
        group_by_name = {g.name: g for g in self.state_groups}
        for inv in self.variant_invariants:
            g = group_by_name[inv.source_group]
            if inv.source_variant not in g.variants:
                raise SyntaxError(f"{inv.pos} invariant source '{inv.source_variant}' is not a variant of state '{inv.source_group}'")
            inv.source_group_ref = g
            if inv.target_variant is not None:
                tg = variant_to_group.get(inv.target_variant)
                if tg is not None:
                    src_types = [str(t) for t, _ in g.params]
                    tgt_types = [str(t) for t, _ in tg.params]
                    if src_types == tgt_types:
                        inv.target_group_ref = tg

        for group in self.state_groups:
            group.validate_reachability(f"{self.namespace_name}::protocol::{self.name}")
            method = Method(self)
            method.name = 'proto' + group.name
            method.params = list(group.params)
            method.is_const = True
            method.return_type = CppType(f"Meta::States::{group.name}")
            self.methods.append(method)
            for variant in group.variants:
                vm = Method(self)
                vm.name = variant
                vm.params = list(group.params)
                vm.is_const = True
                vm.return_type = CppType("bool")
                vm.default_impl = f"return self.impl({method.name}_c"
                for type, name in group.params:
                    vm.default_impl += f", {name}{type.pack}"
                vm.default_impl += f") == ARC_This_Trait::Meta::States::{group.name}::{variant};"
                self.methods.append(vm)

        for pred in self._temp_predicates:
            method = Method(self)
            method.name = pred.name
            method.params = list(pred.params)
            method.is_const = True
            method.return_type = CppType("bool")
            if pred.implies_expr:
                method.implies = ImpliesData(
                    assert_msg=f"{pred.pos} {self.name}::{pred.name} {pred.kind}",
                    expr=pred.implies_expr,
                    kind=pred.kind,
                )
            self.methods.append(method)

        self.methods.sort(key=lambda v: (v.name, v.params))
        self.method_names = sorted(set(m.name for m in self.methods))
        self.has_const_requires = True
        self.has_mutable_requires = False


class Namespace:
    def __init__(self, name: str, repr_: 'Repr'):
        self.name: str = name
        self.repr: 'Repr' = repr_
        self.trait_names: set[str] = set()
        self.traits: list[Trait] = []
        self.trait_aliases: list[list[str]] = []
        self.cluster_names: set[str] = set()
        self.clusters: list[Cluster] = []
        self.policies: list[Policy] = []

    @property
    def protocols(self) -> list[Protocol]:
        return [t for t in self.traits if isinstance(t, Protocol)]

    def walk(self, children):
        for c in children:
            if c.data == imported('cluster'):
                self.repr.visit_cluster(self.name, c, is_domain=False)
            elif c.data == imported('domain'):
                self.repr.visit_cluster(self.name, c, is_domain=True)
            elif c.data == imported('policy'):
                self.repr.visit_policy(self.name, c)
            elif c.data == imported('protocol'):
                self.repr.visit_protocol(self.name, c)
            elif c.data == imported('trait'):
                # trait node contains [TRAIT token, trait_def or trait_alias]
                has_template = not isinstance(c.children[0], Token)
                trait_scope = c.children[-1]  # Skip token
                if trait_scope.data == imported('trait_def'):
                    self.repr.visit_trait_def(self.name, trait_scope, c.children[0] if has_template else None)
                elif trait_scope.data == imported('trait_alias'):
                    self.repr.visit_trait_alias(self.name, trait_scope)
                else:
                    raise SyntaxError(f'{get_pos(trait_scope)} Unknown trait type: {trait_scope.data}')
            else:
                raise SyntaxError(f'{get_pos(c)} Unknown namespace entity: {c.data}')

    def add_cluster(self, name: str, tree: Tree, is_domain: bool) -> Cluster | Domain:
        if name in self.cluster_names:
            raise SyntaxError(f"{get_pos(tree)} {'domain' if is_domain else 'cluster'} by name '{name}' already defined in namespace '{self.name}'")
        self.cluster_names.add(name)
        cluster = Domain(name, self) if is_domain else Cluster(name, self)
        self.clusters.append(cluster)
        return cluster

    def add_policy(self, name: str) -> Policy:
        policy = Policy(name)
        self.policies.append(policy)
        return policy

    def add_trait(self, name: str, tree: Tree) -> Trait:
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

    def add_protocol(self, name: str, tree: Tree) -> Protocol:
        if not name[0].isupper():
            raise SyntaxError(f'{get_pos(tree)} First character of protocol name {name} is not Uppercase')
        if name in self.trait_names:
            raise SyntaxError(f"{get_pos(tree)} protocol by name '{name}' already defined in namespace '{self.name}'")
        self.trait_names.add(name)
        protocol = Protocol(name, self.name)
        self.traits.append(protocol)
        return protocol

    def finalise(self):
        self.clusters.sort(key=lambda v: v.name)
        for c in self.clusters:
            c.finalise()
        self.traits.sort(key=lambda v: (0 if v.is_protocol else 1, v.name))


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
        self.has_protocol = False
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
            elif t.data == imported('cluster'):
                self.visit_cluster("", t, is_domain=False)
            elif t.data == imported('domain'):
                self.visit_cluster("", t, is_domain=True)
            elif t.data == imported('policy'):
                self.visit_policy("", t)
            elif t.data == imported('protocol'):
                self.visit_protocol("", t)
            elif t.data == imported('trait'):
                has_template = not isinstance(t.children[0], Token)
                trait_scope = t.children[-1]  # Skip token
                if trait_scope.data == imported('trait_def'):
                    self.visit_trait_def("", trait_scope, t.children[0] if has_template else None)
                elif trait_scope.data == imported('trait_alias'):
                    self.visit_trait_alias("", trait_scope)
                else:
                    raise SyntaxError(f'{get_pos(trait_scope)} Unknown trait type: {trait_scope.data}')
            else:
                raise SyntaxError(f'{get_pos(t)} Unknown token: {t.data}')
        self.includes = sorted(includes)
        self.import_modules = [f"{impt} {name}" for impt, name in sorted(import_modules, key=lambda v: v[1])]

    def visit_cluster(self, source_ns: str, tree: Tree, is_domain: bool):
        has_template = not isinstance(tree.children[0], Token)
        scope = tree.children[-1]
        name, namespace = self.split_namespace(tree, source_ns, scope.children[0].value)
        cluster = namespace.add_cluster(name, scope.children[0], is_domain=is_domain)
        if has_template:
            cluster.add_template(tree.children[0])
        cluster.walk(scope.children[1:])

    def visit_policy(self, source_ns: str, tree: Tree):
        # tree is policy node with [POLICY token, policy_scope] children
        scope = tree.children[-1]  # Skip token, get policy_scope
        name, namespace = self.split_namespace(tree, source_ns, scope.children[0].value)
        policy = namespace.add_policy(name)
        policy.walk(scope.children[1:])

    def visit_trait_def(self, source_ns: str, tree: Tree, template: Optional[Tree] = None):
        name, namespace = self.split_namespace(tree, source_ns, tree.children[0].value)
        trait = namespace.add_trait(name, tree)
        trait.walk(tree.children[1:])
        if template is not None:
            trait.add_template(template)

    def visit_trait_alias(self, source_ns: str, tree: Tree):
        # tree is trait_alias node with FQNAME tokens
        name, namespace = self.split_namespace(tree, source_ns, tree.children[0].value)
        names = [tree.children[0].value]
        for rhs in tree.children[1:]:
            names.append(reconstruct(rhs))
        namespace.add_trait_alias(names, tree)

    def visit_protocol(self, source_ns: str, tree: Tree):
        # tree is protocol node with [PROTOCOL token, protocol_scope] children
        scope = tree.children[-1]  # Skip PROTOCOL token, get protocol_scope
        name, namespace = self.split_namespace(tree, source_ns, scope.children[0].value)
        protocol = namespace.add_protocol(name, scope.children[0])
        protocol.walk(scope.children[1:])

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
        self.has_protocol = any(len(namespace.protocols) != 0 for namespace in self.namespaces)


template_loader = jinja2.FileSystemLoader(searchpath=dir_path)
template_env = jinja2.Environment(loader=template_loader, trim_blocks=True, lstrip_blocks=True)
jinja_template = template_env.get_template(f"template.{'ixx' if is_module else 'hxx'}.jinja")
output_text = jinja_template.render(repr=Repr(parsed), export="export " if is_module else "ARC_MODULE_EXPORT\n")

with open(output_file, 'a+') as file:
    file.seek(0)
    currentText = file.read()
    if currentText != output_text:
        if not quiet:
            print(f"ARC {'created' if currentText == '' else 'changed'}: {output_file}")
        file.truncate(0)
        file.write(output_text)
