export module examples.filesystem.clusters;

import examples.filesystem.command_handler;
import examples.filesystem.console_output;
import examples.filesystem.filesystem;
import examples.filesystem.memory_storage;
import examples.filesystem.path_ops;
import examples.filesystem.repl;
import examples.filesystem.terminal_line_reader;
import examples.filesystem.traits;

namespace examples::filesystem {

cluster Filesystem [Root]
{
    fs = node::Filesystem
    pathOps = node::PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage

    [trait::DirectorySync]
    .. --> storage
}

cluster Repl
{
    repl = node::Repl
    lineReader = node::TerminalLineReader
    commands = node::CommandHandler
    output = node::ConsoleOutput
    fs = cluster::Filesystem

    [trait::LineReader]    repl --> lineReader
    [trait::Commands]      repl --> commands
    [trait::Output]        repl --> output

    [trait::Filesystem]    lineReader --> fs
                           commands   --> fs
    [trait::DirectorySync] commands   --> fs
}

}
