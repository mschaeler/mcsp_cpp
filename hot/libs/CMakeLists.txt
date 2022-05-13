ADD_SUBDIRECTORY(idx)
ADD_SUBDIRECTORY(hot)

target_link_libraries(my_mcsp PRIVATE hot-single-threaded-lib hot-commons-lib)
