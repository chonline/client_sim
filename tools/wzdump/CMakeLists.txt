# wzdump: dump the directory tree of a WZ file.
#
# Useful for sanity-checking wzlib against a real WZ install and for
# diagnosing issues in WzFile parsing.

add_executable(wzdump main.cpp)
target_link_libraries(wzdump PRIVATE wzlib)
