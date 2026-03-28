export module examples.calculator.batch_line_reader;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct BatchLineReader : arc::NodeImpl<LineReader>
{
    BatchLineReader() = default;
    explicit BatchLineReader(std::vector<std::string> inputs) : inputs_(std::move(inputs)) {}

    auto readLine(std::string_view prompt) -> std::optional<std::string>;
    auto setInputs(std::vector<std::string> inputs) -> void;

private:
    std::vector<std::string> inputs_;
    std::size_t index_ = 0;
};

}
