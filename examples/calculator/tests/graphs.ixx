export module examples.calculator.tests.graphs;

import examples.calculator.clusters;
import examples.calculator.evaluator;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::tests {

// A simple LineReader that returns pre-programmed lines in order then nullopt.
export struct FeedLineReader : arc::NodeImpl<trait::LineReader>
{
    void setInputs(std::vector<std::string> lines)
    {
        inputs_ = std::move(lines);
        index_ = 0;
    }

    auto readLine(std::string_view) -> std::optional<std::string>
    {
        if (index_ < inputs_.size())
            return inputs_[index_++];
        return std::nullopt;
    }

private:
    std::vector<std::string> inputs_;
    std::size_t index_ = 0;
};

// An Output that captures lines written to it.
export struct CapturingOutput : arc::NodeImpl<trait::Output>
{
    auto write(std::string_view) -> void {}

    auto writeLine(std::string_view text) -> void
    {
        lines_.emplace_back(text);
    }

    auto lines() const -> std::vector<std::string> const& { return lines_; }

private:
    std::vector<std::string> lines_;
};

export struct IntegrationTestRoot
{
    using LineReader = FeedLineReader;
    using Output = CapturingOutput;
};

export using IntegrationGraph = arc::Graph<cluster::Calculator, IntegrationTestRoot>;

// Helper to create a ready-to-use evaluator test graph with mocked dependencies.
// Reduces duplication in test_evaluator.cpp where each scenario creates and configures the graph identically.
export auto makeEvaluatorGraph()
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    return graph;
}

export struct MockVariableStore
{
    std::map<std::string, double> vars;

    void install(arc::test::Graph<node::Evaluator>& graph)
    {
        graph.mocks->define(
            [this](trait::Variables::get, std::string_view name) -> std::optional<double>
            {
                auto it = vars.find(std::string(name));
                if (it != vars.end())
                    return it->second;
                return std::nullopt;
            }
        );
        graph.mocks->define(
            [this](trait::Variables::set, std::string name, double value)
            {
                vars[name] = value;
            }
        );
        graph.mocks->define(
            [this](trait::Variables::remove, std::string_view name) -> bool
            {
                return vars.erase(std::string(name)) > 0;
            }
        );
    }
};

} // namespace examples::calculator::tests
