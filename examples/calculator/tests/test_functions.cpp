import examples.calculator.node.functions;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#include "arc/doctest.h"

using namespace examples::calculator;
using namespace std::string_view_literals;

namespace {

auto makeVar(std::string name) -> ExprPtr
{
    return std::make_unique<Expression>(VariableExpr{std::move(name)});
}

auto makeNum(double v) -> ExprPtr
{
    return std::make_unique<Expression>(NumberExpr{v});
}

template<typename... Args>
auto makeCall(std::string name, Args&&... args) -> ExprPtr
{
    std::vector<ExprPtr> v;
    (v.push_back(std::forward<Args>(args)), ...);
    return std::make_unique<Expression>(CallExpr{std::move(name), std::move(v)});
}

auto makeBinary(BinaryOp op, ExprPtr left, ExprPtr right) -> ExprPtr
{
    return std::make_unique<Expression>(BinaryExpr{op, std::move(left), std::move(right)});
}

auto def(arc::IsTraitViewOf<Functions> auto functions,
         std::string name, std::vector<std::string> params,
         ExprPtr body, std::string source)
{
    return functions.define(std::move(name), std::move(params), std::move(body), std::move(source));
}

auto xPlus1() -> ExprPtr
{
    return makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1.0));
}

auto xPlusY() -> ExprPtr
{
    return makeBinary(BinaryOp::Add, makeVar("x"), makeVar("y"));
}

auto xTimes10() -> ExprPtr
{
    return makeBinary(BinaryOp::Mul, makeVar("x"), makeNum(10.0));
}

auto xTimes3() -> ExprPtr
{
    return makeBinary(BinaryOp::Mul, makeVar("x"), makeNum(3.0));
}

auto fCall(ExprPtr arg) -> ExprPtr
{
    return makeCall("f", std::move(arg));
}

auto fCall2(ExprPtr arg1, ExprPtr arg2) -> ExprPtr
{
    return makeCall("f", std::move(arg1), std::move(arg2));
}

auto gCall(ExprPtr arg) -> ExprPtr
{
    return makeCall("g", std::move(arg));
}

auto fPlus2() -> ExprPtr
{
    return makeBinary(BinaryOp::Add, fCall(makeVar("x")), makeNum(2.0));
}

auto fMinus1() -> ExprPtr
{
    return makeBinary(BinaryOp::Sub, makeVar("x"), makeNum(1.0));
}

auto justX() -> ExprPtr
{
    return makeVar("x");
}

auto defineF(auto functions)
{
    return functions.define(std::string("f"), std::vector<std::string>{"x"}, xPlus1(), std::string("x + 1"));
}

auto defineF2(auto functions)
{
    return functions.define(std::string("f"), std::vector<std::string>{"x", "y"}, xPlusY(), std::string("x + y"));
}

auto defineGDependsOnF(auto functions)
{
    return functions.define(std::string("g"), std::vector<std::string>{"x"}, fPlus2(), std::string("f(x) + 2"));
}

} // namespace

SCENARIO("Calling built-in unary functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling \"abs\" with [-5]")
        {
            std::array args{-5.0};
            auto result = functions.call("abs"sv, args);

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 5.0);
            }
        }
        WHEN("calling \"sqrt\" with [16]")
        {
            std::array args{16.0};
            auto result = functions.call("sqrt"sv, args);

            THEN("returns 4")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 4.0);
            }
        }
        WHEN("calling \"neg\" with [3]")
        {
            std::array args{3.0};
            auto result = functions.call("neg"sv, args);

            THEN("returns -3")
            {
                REQUIRE(result.has_value());
                CHECK(*result == -3.0);
            }
        }
    }
}

SCENARIO("Calling built-in binary functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling \"add\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("add"sv, args);

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 5.0);
            }
        }
        WHEN("calling \"max\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("max"sv, args);

            THEN("returns 3")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 3.0);
            }
        }
        WHEN("calling \"min\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("min"sv, args);

            THEN("returns 2")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 2.0);
            }
        }
        WHEN("calling \"pow\" with [2, 10]")
        {
            std::array args{2.0, 10.0};
            auto result = functions.call("pow"sv, args);

            THEN("returns 1024")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 1024.0);
            }
        }
    }
}

SCENARIO("Calling trig functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling \"sin\" with [0]")
        {
            std::array args{0.0};
            auto result = functions.call("sin"sv, args);

            THEN("returns 0")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 0.0);
            }
        }
        WHEN("calling \"cos\" with [0]")
        {
            std::array args{0.0};
            auto result = functions.call("cos"sv, args);

            THEN("returns 1")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 1.0);
            }
        }
    }
}

SCENARIO("Wrong argument count")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling \"sqrt\" with [1, 2]")
        {
            std::array args{1.0, 2.0};
            auto result = functions.call("sqrt"sv, args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("calling \"add\" with [1]")
        {
            std::array args{1.0};
            auto result = functions.call("add"sv, args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Unknown function")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling \"unknown\" with [1]")
        {
            std::array args{1.0};
            auto result = functions.call("unknown"sv, args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("call() returns unknown-function error for a user-defined function name")
{
    GIVEN("a Functions node with f(x) = 99 defined")
    {
        arc::test::Graph<node::Functions> graph;
        graph.mocks->setReturnDefault();
        auto functions = graph.node.asTrait(trait::functions);
        auto body = std::make_unique<Expression>(NumberExpr{99.0});
        functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("99"));

        WHEN("calling f(5) via call()")
        {
            std::array args{5.0};
            auto result = functions.call("f"sv, args);

            THEN("returns an error — user functions are dispatched by the Evaluator via get()")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("unknown function"));
            }
        }
    }
}

SCENARIO("Listing builtin functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto builtins = graph.node.asTrait(trait::builtinFunctions);
        WHEN("listing builtin functions")
        {
            auto result = builtins.list();

            THEN("returns all built-in function names")
            {
                CHECK_FALSE(result.empty());
                CHECK(std::ranges::contains(result, "abs"));
                CHECK(std::ranges::contains(result, "sqrt"));
                CHECK(std::ranges::contains(result, "neg"));
                CHECK(std::ranges::contains(result, "sin"));
                CHECK(std::ranges::contains(result, "cos"));
                CHECK(std::ranges::contains(result, "tan"));
                CHECK(std::ranges::contains(result, "log"));
                CHECK(std::ranges::contains(result, "ln"));
                CHECK(std::ranges::contains(result, "add"));
                CHECK(std::ranges::contains(result, "sub"));
                CHECK(std::ranges::contains(result, "mul"));
                CHECK(std::ranges::contains(result, "div"));
                CHECK(std::ranges::contains(result, "pow"));
                CHECK(std::ranges::contains(result, "min"));
                CHECK(std::ranges::contains(result, "max"));
            }
        }
    }
}

SCENARIO("Functions contract: call rejects empty function name")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling call with an empty name")
        {
            std::array<double, 1> args{1.0};

            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(functions.call(""sv, args), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Defining and retrieving user functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("defining function \"f\" with params [\"x\"] and a body expression")
        {
            auto body = xPlus1();
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("x + 1"));

            THEN("returns non-null for f/1")
            {
                REQUIRE(result.has_value());
                auto fn = functions.get("f"sv, 1);
                CHECK(fn != nullptr);
            }
            THEN("the returned UserFunction has params [\"x\"]")
            {
                auto fn = functions.get("f"sv, 1);
                REQUIRE(fn != nullptr);
                CHECK(fn->params.size() == 1);
                CHECK(fn->params[0] == "x");
            }
        }
        WHEN("defining function \"g\" with params [\"x\", \"y\"]")
        {
            auto body = xPlusY();
            auto result = functions.define(std::string("g"), std::vector<std::string>{"x", "y"}, std::move(body), std::string("x + y"));

            THEN("returns non-null for g/2 with params [x, y]")
            {
                REQUIRE(result.has_value());
                auto fn = functions.get("g"sv, 2);
                REQUIRE(fn != nullptr);
                CHECK(fn->params.size() == 2);
                CHECK(fn->params[0] == "x");
                CHECK(fn->params[1] == "y");
            }
        }
    }
}

SCENARIO("get returns nullptr for unknown functions")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("querying get for unknown/1")
        {
            THEN("returns nullptr")
            {
                auto fn = functions.get("unknown"sv, 1);
                CHECK(fn == nullptr);
            }
        }
    }
}

SCENARIO("get returns nullptr for wrong arity")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" defined (arity 1)")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);

        WHEN("querying get for f/2")
        {
            THEN("returns nullptr")
            {
                auto fn = functions.get("f"sv, 2);
                CHECK(fn == nullptr);
            }
        }
    }
}

SCENARIO("Overloaded functions with same name but different arity coexist")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("defining \"f(x) = x + 1\" (arity 1) and \"f(x, y) = x + y\" (arity 2)")
        {
            auto result1 = defineF(functions);
            auto result2 = defineF2(functions);

            THEN("both define calls return success")
            {
                REQUIRE(result1.has_value());
                REQUIRE(result2.has_value());
            }
            THEN("f/1 has params [x]")
            {
                auto fn = functions.get("f"sv, 1);
                REQUIRE(fn != nullptr);
                CHECK(fn->params.size() == 1);
                CHECK(fn->params[0] == "x");
            }
            THEN("f/2 has params [x, y]")
            {
                auto fn = functions.get("f"sv, 2);
                REQUIRE(fn != nullptr);
                CHECK(fn->params.size() == 2);
                CHECK(fn->params[0] == "x");
                CHECK(fn->params[1] == "y");
            }
        }
    }
}

SCENARIO("Redefining a user function replaces only that arity")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" and \"f(x, y) = x + y\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineF2(functions);

        WHEN("redefining \"f(x) = x * 10\" (arity 1 only)")
        {
            auto body = xTimes10();
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("x * 10"));

            THEN("define returns success")
            {
                REQUIRE(result.has_value());
            }
            THEN("f/1 has updated body")
            {
                auto fn = functions.get("f"sv, 1);
                REQUIRE(fn != nullptr);
                CHECK(fn->source == "x * 10");
            }
            THEN("f/2 is unchanged")
            {
                auto fn = functions.get("f"sv, 2);
                REQUIRE(fn != nullptr);
                CHECK(fn->source == "x + y");
            }
        }
    }
}

SCENARIO("Defining a function with a builtin name and same arity is rejected")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("calling define with name=\"sqrt\", params=[\"x\"] (arity 1, same as builtin sqrt)")
        {
            auto body = justX();
            auto result = functions.define(std::string("sqrt"), std::vector<std::string>{"x"}, std::move(body), std::string("x"));

            THEN("returns EvalError (cannot shadow builtin)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("cannot shadow builtin"));
            }
        }
        WHEN("calling define with name=\"sqrt\", params=[\"x\", \"y\"] (arity 2, different from builtin)")
        {
            auto body = xPlusY();
            auto result = functions.define(std::string("sqrt"), std::vector<std::string>{"x", "y"}, std::move(body), std::string("x + y"));

            THEN("returns success (no shadowing, different arity)")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}

SCENARIO("Defining a function referencing an undefined function is rejected")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("defining \"f(x) = g(x)\" where \"g\" with arity 1 is not a builtin or defined user function")
        {
            auto body = gCall(makeVar("x"));
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("g(x)"));

            THEN("returns EvalError (undefined function reference)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Referencing a function at wrong arity is rejected")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" defined (arity 1)")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);

        WHEN("defining \"g(x, y) = f(x, y)\" (calling f with arity 2, which doesn't exist)")
        {
            auto body = fCall2(makeVar("x"), makeVar("y"));
            auto result = functions.define(std::string("g"), std::vector<std::string>{"x", "y"}, std::move(body), std::string("f(x, y)"));

            THEN("returns EvalError (undefined function reference for f/2)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Direct recursion is rejected")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("defining \"f(x) = f(x - 1)\" (f/1 calls f/1)")
        {
            auto body = fCall(fMinus1());
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("f(x - 1)"));

            THEN("returns EvalError (recursion not allowed)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Direct recursion does not apply across arities")
{
    GIVEN("a Functions node with \"f(x, y) = x + y\" defined (arity 2)")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF2(functions);

        WHEN("defining \"f(x) = f(x, 1)\" (f/1 calls f/2 — different overload, not recursion)")
        {
            auto body = fCall2(makeVar("x"), makeNum(1.0));
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("f(x, 1)"));

            THEN("returns success")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}

SCENARIO("Indirect recursion via redefinition is rejected")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" and \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineGDependsOnF(functions);

        WHEN("redefining \"f(x) = g(x)\" (creates cycle f/1 -> g/1 -> f/1)")
        {
            auto body = gCall(makeVar("x"));
            auto result = functions.define(std::string("f"), std::vector<std::string>{"x"}, std::move(body), std::string("g(x)"));

            THEN("returns EvalError (recursion not allowed)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Non-recursive references are allowed")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);

        WHEN("defining \"g(x) = f(x) + 2\" (g depends on f, no cycle)")
        {
            auto body = fPlus2();
            auto result = functions.define(std::string("g"), std::vector<std::string>{"x"}, std::move(body), std::string("f(x) + 2"));

            THEN("returns success")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}

SCENARIO("Redefining a function updates dependency graph")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" and \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineGDependsOnF(functions);

        WHEN("redefining \"g(x) = x * 3\" (g no longer depends on f)")
        {
            auto body = xTimes3();
            auto result = functions.define(std::string("g"), std::vector<std::string>{"x"}, std::move(body), std::string("x * 3"));

            THEN("define returns success")
            {
                REQUIRE(result.has_value());
            }
            THEN("f can now be removed since g no longer depends on it")
            {
                auto removeResult = functions.remove(std::vector<std::string>{"f"});
                REQUIRE(removeResult.has_value());
            }
        }
    }
}

SCENARIO("Redefining an overloaded function with dependencies updates correctly")
{
    GIVEN("a Functions node with \"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineF2(functions);
        defineGDependsOnF(functions);

        WHEN("calling remove([\"f\"])")
        {
            auto result = functions.remove(std::vector<std::string>{"f"});

            THEN("returns EvalError (g/1 depends on f/1)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("redefining \"g(x) = x * 3\" then removing f")
        {
            auto body = xTimes3();
            auto redefineResult = functions.define(std::string("g"), std::vector<std::string>{"x"}, std::move(body), std::string("x * 3"));
            REQUIRE(redefineResult.has_value());

            auto removeResult = functions.remove(std::vector<std::string>{"f"});

            THEN("remove returns success (g no longer depends on f)")
            {
                REQUIRE(removeResult.has_value());
            }
        }
    }
}

SCENARIO("Removing user functions respects dependencies")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" and \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineGDependsOnF(functions);

        WHEN("calling remove([\"f\"]) (g depends on f)")
        {
            auto result = functions.remove(std::vector<std::string>{"f"});

            THEN("returns EvalError (g depends on f)")
            {
                REQUIRE_FALSE(result.has_value());
            }
            THEN("error message names both functions")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("f/1"));
                CHECK(result.error().message.contains("g/1"));
            }
        }
        WHEN("calling remove([\"g\"]) (nothing depends on g)")
        {
            auto result = functions.remove(std::vector<std::string>{"g"});

            THEN("returns success")
            {
                REQUIRE(result.has_value());
            }
            THEN("g/1 returns nullptr")
            {
                CHECK(functions.get("g"sv, 1) == nullptr);
            }
        }
        WHEN("calling remove([\"f\", \"g\"]) (remove both)")
        {
            auto result = functions.remove(std::vector<std::string>{"f", "g"});

            THEN("returns success (all dependencies removed together)")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}

SCENARIO("Undef removes all overloads and checks dependencies")
{
    GIVEN("a Functions node with \"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineF2(functions);
        defineGDependsOnF(functions);

        WHEN("calling remove([\"f\"])")
        {
            auto result = functions.remove(std::vector<std::string>{"f"});

            THEN("returns EvalError (g/1 depends on f/1)")
            {
                REQUIRE_FALSE(result.has_value());
            }
            THEN("error message names both functions")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("f/1"));
                CHECK(result.error().message.contains("g/1"));
            }
        }
        WHEN("calling remove([\"f\", \"g\"])")
        {
            auto result = functions.remove(std::vector<std::string>{"f", "g"});

            THEN("returns success (both f overloads and g removed)")
            {
                REQUIRE(result.has_value());
            }
            THEN("f/1 returns nullptr")
            {
                CHECK(functions.get("f"sv, 1) == nullptr);
            }
            THEN("f/2 returns nullptr")
            {
                CHECK(functions.get("f"sv, 2) == nullptr);
            }
            THEN("g/1 returns nullptr")
            {
                CHECK(functions.get("g"sv, 1) == nullptr);
            }
        }
    }
}

SCENARIO("Removing functions sequentially in dependency order")
{
    GIVEN("a Functions node with \"f(x) = x + 1\" and \"g(x) = f(x) + 2\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineGDependsOnF(functions);

        WHEN("calling remove([\"g\"]) then remove([\"f\"])")
        {
            auto result1 = functions.remove(std::vector<std::string>{"g"});
            REQUIRE(result1.has_value());

            auto result2 = functions.remove(std::vector<std::string>{"f"});

            THEN("both removals succeed")
            {
                REQUIRE(result2.has_value());
            }
        }
    }
}

SCENARIO("clear removes all user functions including overloads")
{
    GIVEN("a Functions node with \"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x)\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        defineF(functions);
        defineF2(functions);
        functions.define(std::string("g"), std::vector<std::string>{"x"}, fCall(makeVar("x")), std::string("f(x)"));

        WHEN("calling clear")
        {
            functions.clear();

            THEN("f/1 returns nullptr")
            {
                CHECK(functions.get("f"sv, 1) == nullptr);
            }
            THEN("f/2 returns nullptr")
            {
                CHECK(functions.get("f"sv, 2) == nullptr);
            }
            THEN("g/1 returns nullptr")
            {
                CHECK(functions.get("g"sv, 1) == nullptr);
            }
            THEN("builtins still work (call(\"sqrt\", [4]) returns 2)")
            {
                std::array args{4.0};
                auto result = functions.call("sqrt"sv, args);
                REQUIRE(result.has_value());
                CHECK(*result == 2.0);
            }
        }
    }
}

SCENARIO("user list includes defined function names")
{
    GIVEN("a Functions node with user function \"f(x)\" and \"f(x,y)\" defined")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        auto userFunctions = graph.node.asTrait(trait::userFunctions);
        defineF(functions);
        defineF2(functions);

        WHEN("calling user list()")
        {
            auto result = userFunctions.list();

            THEN("result contains \"f\" entries")
            {
                auto names = result | std::views::keys;
                CHECK(std::ranges::contains(names, "f"));
            }
        }
    }
}

SCENARIO("Functions contract: define rejects empty name")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling define with empty name")
        {
            THEN("triggers a contract violation")
            {
                auto body = makeNum(1.0);
                CHECK_THROWS_AS(functions.define(std::string(""), std::vector<std::string>{}, std::move(body), std::string("")), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Functions contract: get rejects empty name")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        auto functions = graph.node.asTrait(trait::functions);
        WHEN("calling get with empty name")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(functions.get(""sv, 1), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Function definition with unknown variable reference is rejected")
{
    GIVEN("a Functions node")
    {
        arc::test::Graph<node::Functions> graph;
        graph.mocks->setReturnDefault();
        auto functions = graph.node.asTrait(trait::functions);

        WHEN("defining \"f(x) = y + 1\" where y is not a parameter and not in Variables")
        {
            auto body = makeBinary(BinaryOp::Add, makeVar("y"), makeNum(1.0));
            auto result = def(functions, "f", {"x"}, std::move(body), "y + 1");

            THEN("returns EvalError (undefined variable in function body)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("undefined variable"));
            }
        }
        WHEN("defining \"f(x) = x + 1\" where x is a parameter")
        {
            auto body = makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1.0));
            auto result = def(functions, "f", {"x"}, std::move(body), "x + 1");

            THEN("succeeds (parameter is not an unknown variable)")
            {
                REQUIRE(result.has_value());
            }
        }
        WHEN("defining \"f() = y\" where y exists in Variables")
        {
            graph.mocks->methodReturns<Variables::get>(std::optional<double>{42.0});
            auto body = makeVar("y");
            auto result = def(functions, "f", {}, std::move(body), "y");

            THEN("succeeds (variable exists in Variables)")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}
