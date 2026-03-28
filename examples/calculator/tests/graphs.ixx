export module examples.calculator.tests.graphs;

import examples.calculator.clusters;
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

} // namespace examples::calculator::tests
