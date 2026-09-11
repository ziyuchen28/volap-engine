#pragma once

#include "volap/core/type.h"
#include "volap/core/data_schema.h"
#include "volap/execution/projection.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace volap::execution 
{

using namespace volap::core; 

enum class BoundCastType : std::uint8_t
{
    None,
    Int64ToFloat32,
    Int64ToFloat64,
    Float32ToFloat64
};

struct BoundOperand final
{
    std::size_t column_index;

    // Actual type of the input Vector.
    Type source_type;

    // Type presented to the arithmetic kernel after any cast.
    Type execution_type;

    BoundCastType cast;
};

enum class BoundProjectionType : std::uint8_t
{
    ColumnRef,

    MultiplyFloat32,
    MultiplyFloat64
};

// As opposed to raw projection, bound projection owns the type and type casting of columns
struct BoundProjection final
{
    BoundProjectionType type;

    Type result_type;

    BoundOperand left;

    // Unused for ColumnRef.
    BoundOperand right;
};

std::vector<BoundProjection> bind_projections(const std::vector<Projection> &projection,
                                              const DataSchema &schema);

} // namespace volap::execution
