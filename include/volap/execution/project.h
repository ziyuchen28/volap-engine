
#pragma once

#include "volap/execution/bound_projection.h"
#include "volap/core/data_chunk.h"

#include <vector>
#include <optional>

namespace volap::execution
{

using namespace volap::core;

class Project 
{

public:

    Project(std::vector<BoundProjection> projections);

    Project(std::initializer_list<BoundProjection> projections);

    Project(std::vector<Projection>, const DataSchema &);

    void execute(const DataChunk &input, DataChunk &output);

private:

    // Temporary buffer used for casting data from source type to execution type
    // during projections for mixed type columns that are castable.
    struct ProjectionScratch final
    {
        std::optional<Vector> left;
        std::optional<Vector> right;
    };

    void prepare_output(const DataChunk &input,
                        DataChunk &output) const;

     const Vector &prepare_operand(const BoundOperand &operand,
                            std::optional<Vector> &scratch,
                            const DataChunk &input);

    void execute_projection(const BoundProjection &projection,
                            ProjectionScratch &scratch,
                            const DataChunk &input,
                            Vector &output);

    std::vector<BoundProjection> projections_;
    std::vector<ProjectionScratch> scratch_;

};


} // namespace volap::execution
