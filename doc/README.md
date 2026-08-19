

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
Vector is a move-only generic execution-column object
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



SelectionVector: a vector/array of row indexes selecting rows from another vector/chunk
(Evaluate one predicate over one vector and decides which row indexes passed.)


3 Buffer
Manges memory ownership, lifetime, alignment and mutability

Buffer types:

    Owned:
      owns and allocates memory
      guarantees alignment
      writable only when uniquely referenced

    External:
      refers to externally managed memory
      read-only
      holds lifetime token / releaser
  


4 Operator
(1) InMemoryScan operator

             owns
InMemoryScan ------> source DataChunk
                         |
                         | read only
                         v

                     scan/copy
                         |
                         v

caller owns ------> output DataChunk
                         ^
                         |
                   next(output)




X Open Questions
(1) Should chunk size during scan fixed or dynamic based on caller input?


