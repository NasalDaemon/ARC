export module examples.calculator.evaluator;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Evaluator
{
    template<class Context>
    struct Node : arc::Node::
        Impl<trait::Evaluator>::
        Uses<Variables, Functions>
    {
        auto evaluate(Expression const& expr) -> std::expected<double, EvalError>;
    };
};

}
