
#pragma once

#include "volap/execution/projection.h"
#include "volap/core/data_chunk.h"

#include <vector>

namespace volap::execution
{

using namespace volap::core;

class Project 
{

public:

    Project(std::vector<Projection> projections);

    Project(std::initializer_list<Projection> projections);

    void execute(const DataChunk &input,
                 DataChunk &output) const;

private:


    void prepare_output(const DataChunk &input,
                        DataChunk &output) const;

    void evaluate_projection(const Projection &projection,
                             const DataChunk &input,
                             Vector &output) const;

    std::vector<Projection> projections_;

};


} // namespace volap::execution
