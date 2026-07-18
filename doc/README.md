

Background


Overview



Data Storage:


1 DataChunk 
Represents multiple columns/vectors with same cardinality

Example:
DataChunk
│
├── Vector1
│     type(i64)
|     Encoding(flat)
│     VectorBuffer
│
├── Vector2
│     type(f32)
|     Encoding(Contant)
│     Value
│
└── Vector3
      type(...)
      Encoding(...)
      VectorBuffer


2 Vector 
Represents one logical column slice/batch
type: the data type of one value in the vector
encoding: how data how logically represented 

Vector Types:

    Flat / Constant / Dictionary:
      physical encodings

        Flat: continuous array
          physical:
            [value1, value2, value3, ...]
          logical:
            [value1, value2, value3, ...]

        Constant: one value repeated for every row
          physical:
            value
          logical:
            [value, value, value, ....]

        Dictionary: an index mapping into another vector
          physical:
            indices: [value1, value2, value3]
            child:   [1, 0, 2]
          logical:
            [value2, value1, value3]


    Row / Array / Map:
      representations of complex logical types
    // TO DO 



3 Buffer
Manges memory ownership, lifetime, alignment and mutability

Buffer types:

    AlignedBuffer:
      owns and allocates memory
      guarantees alignment
      writable only when uniquely referenced

    BufferView:
      refers to externally managed memory
      read-only
      holds lifetime token / releaser
  


Buffer shared by vectors requires unique ownership for writes: controlled by atomic ref count

    reference count == 1:
      no other vector observes this buffer
      mut is permitted

    reference count > 1:
      buffer is shared
      immutable
      copy-on-write: allocate/copy before mut




Operator



