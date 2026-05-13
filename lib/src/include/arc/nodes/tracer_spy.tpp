#if !ARC_IMPORT_STD
#include <print>
#endif

namespace arc {

std::vector<std::size_t>& TracerSpy::parentStack()
{
    thread_local std::vector<std::size_t> s;
    return s;
}

void TracerSpy::write(std::ostream& os, Format fmt) const
{
    writeTrace(os, fmt);
    writeSummary(os, fmt);
}

void TracerSpy::writeTrace(std::ostream& os, Format fmt) const
{
    for (auto const& e : events)
        writeEvent(os, e, fmt);
}

void TracerSpy::writeSummary(std::ostream& os, Format fmt) const
{
    if (fmt == Format::Jsonl)
    {
        for (auto const& [m, s] : perMethod)
        {
            std::println(os,
                R"({{"evt":"summary","method":"{}","calls":{},"exc":{},"total_ns":{},"max_ns":{}}})",
                jsonEscape(m), s.calls, s.exceptions, s.total.count(), s.maxTime.count());
        }
        std::println(os,
            R"({{"evt":"summary_meta","total_calls":{},"max_depth":{}}})",
            nextCallId, maxDepthSeen);
    }
    else
    {
        std::println(os, "=== TracerSpy summary ===");
        for (auto const& [m, s] : perMethod)
            std::println(os, "  {:<60} calls={:>6} exc={:>3} total={}ns max={}ns",
                m, s.calls, s.exceptions, s.total.count(), s.maxTime.count());
        std::println(os, "  total_calls={} max_depth={}", nextCallId, maxDepthSeen);
    }
}

void TracerSpy::recordExit(std::size_t id, std::size_t d, std::string_view name, std::string_view nodeName,
                            std::chrono::nanoseconds dt,
                            std::string_view retType, std::string retValue, bool hasValue)
{
    Event e;
    e.kind = Event::Kind::Exit;
    e.id = id;
    e.depth = d;
    e.method = name;
    e.node = nodeName;
    e.ns = dt;
    e.retType = retType;
    e.retValue = std::move(retValue);
    e.retHasValue = hasValue;
    events.push_back(std::move(e));
}

void TracerSpy::recordThrow(std::size_t id, std::size_t d, std::string_view name, std::string_view nodeName,
                             std::chrono::nanoseconds dt, std::string what)
{
    Event e;
    e.kind = Event::Kind::Throw;
    e.id = id;
    e.depth = d;
    e.method = name;
    e.node = nodeName;
    e.ns = dt;
    e.what = std::move(what);
    events.push_back(std::move(e));
}

void TracerSpy::writeEvent(std::ostream& os, Event const& e, Format fmt) const
{
    switch (fmt)
    {
    case Format::Jsonl: writeEventJsonl(os, e); break;
    case Format::Human: writeEventHuman(os, e); break;
    }
}

void TracerSpy::writeEventJsonl(std::ostream& os, Event const& e) const
{
    switch (e.kind)
    {
        case Event::Kind::Enter:
        {
            std::string argList;
            bool first = true;
            for (auto const& a : e.args)
            {
                if (!first) argList += ',';
                first = false;
                if (a.hasValue)
                    argList += std::format(R"({{"type":"{}","value":"{}"}})",
                        jsonEscape(a.type), jsonEscape(a.value));
                else
                    argList += std::format(R"({{"type":"{}"}})", jsonEscape(a.type));
            }
            std::println(os,
                R"({{"evt":"enter","id":{},"parent":{},"depth":{},"node":"{}","method":"{}","args":[{}]}})",
                e.id, e.parent, e.depth, jsonEscape(e.node), jsonEscape(e.method), argList);
            break;
        }
        case Event::Kind::Exit:
            if (e.retHasValue)
            {
                std::println(os,
                    R"({{"evt":"exit","id":{},"depth":{},"node":"{}","method":"{}","ns":{},"ret_type":"{}","ret":"{}"}})",
                    e.id, e.depth, jsonEscape(e.node), jsonEscape(e.method), e.ns.count(),
                    jsonEscape(e.retType), jsonEscape(e.retValue));
            }
            else
            {
                std::println(os,
                    R"({{"evt":"exit","id":{},"depth":{},"node":"{}","method":"{}","ns":{},"ret_type":"{}"}})",
                    e.id, e.depth, jsonEscape(e.node), jsonEscape(e.method), e.ns.count(), jsonEscape(e.retType));
            }
            break;
        case Event::Kind::Throw:
            std::println(os,
                R"({{"evt":"throw","id":{},"depth":{},"node":"{}","method":"{}","ns":{},"what":"{}"}})",
                e.id, e.depth, jsonEscape(e.node), jsonEscape(e.method), e.ns.count(), jsonEscape(e.what));
            break;
    }
}

void TracerSpy::writeEventHuman(std::ostream& os, Event const& e) const
{
    std::string const indent(e.depth * 2, ' ');
    switch (e.kind)
    {
        case Event::Kind::Enter:
        {
            std::string argList;
            bool first = true;
            for (auto const& a : e.args)
            {
                if (!first) argList += ", ";
                first = false;
                if (a.hasValue) argList += std::format("{}({})", a.type, a.value);
                else            argList += a.type;
            }
            std::string const nodeTag = e.node.empty() ? std::string{} : std::format("[{}] ", e.node);
            std::println(os, "{}-> #{} {}{}({})", indent, e.id, nodeTag, e.method, argList);
            break;
        }
        case Event::Kind::Exit:
        {
            std::string const nodeTag = e.node.empty() ? std::string{} : std::format("[{}] ", e.node);
            if (e.retHasValue)
                std::println(os, "{}<- #{} {}{} [{} ns] -> {} = {}",
                    indent, e.id, nodeTag, e.method, e.ns.count(), e.retType, e.retValue);
            else
                std::println(os, "{}<- #{} {}{} [{} ns] -> {}",
                    indent, e.id, nodeTag, e.method, e.ns.count(), e.retType);
            break;
        }
        case Event::Kind::Throw:
        {
            std::string const nodeTag = e.node.empty() ? std::string{} : std::format("[{}] ", e.node);
            std::println(os, "{}!! #{} {}{} [{} ns] threw {}",
                indent, e.id, nodeTag, e.method, e.ns.count(), e.what);
            break;
        }
    }
}

/*static*/ std::string TracerSpy::jsonEscape(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\"";
                       break;
            case '\\': out += "\\\\";
                       break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    out += std::format("\\u{:04x}", static_cast<int>(c));
                else
                    out += c;
        }
    }
    return out;
}

} // namespace arc
