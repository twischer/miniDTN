project(FAT)

add_definitions(-DDISKIO_DEBUG)
add_executable(diskio_test diskio_test.c ../mbr.c ../diskio.c)
add_executable(mbr_test mbr_test.c ../mbr.c ../diskio.c)
