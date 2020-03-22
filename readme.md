# malloc implementation
## Version 1.0: Use implicit list with boundary tag
- header and footer for every block 
- use first fit method to choose a free block
- after extending heap with `mem_sbrk`, use `mm_free` to coalesce two free blocks if necessary
**Result:** 
Using default tracefiles in ../traces/
Perf index = 46 (util) + 5 (thru) = 50/100