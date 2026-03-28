module examples.calculator.batch_line_reader;

import std;

namespace examples::calculator::node {

auto BatchLineReader::readLine(std::string_view prompt) -> std::optional<std::string>
{
    if (index_ < inputs_.size())
        return inputs_[index_++];
    return std::nullopt;
}

auto BatchLineReader::setInputs(std::vector<std::string> inputs) -> void
{
    inputs_ = std::move(inputs);
    index_ = 0;
}

}
