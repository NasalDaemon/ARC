#pragma once

#include "abc/graph.hxx"

#include "arc/graph.hpp"

namespace abc {

struct GraphWrapper
{
    arc::Graph<abc::AliceBob> graph;
};

}
