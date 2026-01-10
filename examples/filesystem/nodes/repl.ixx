export module examples.filesystem.repl;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem {

export struct Repl
{
    template<class Context>
    struct Node : arc::NodeUses<trait::Filesystem, trait::DirectorySync*>
    {
        auto run(int argc, char* argv[]) -> int;

    private:
        auto runCommand(std::vector<std::string>& args) -> bool;
        auto printTree(std::string_view path, std::string_view prefix) -> void;
        auto readLineWithHistory(std::vector<std::string>& history) -> std::string;
        auto getPathCompletions(std::string_view prefix) -> std::vector<std::string>;
    };
};

}
