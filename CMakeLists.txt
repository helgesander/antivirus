cmake_minimum_required(VERSION 3.5)

project(antivirus VERSION 0.1 LANGUAGES CXX)

add_subdirectory(antivirus_service) # TODO: fix build of antivirus_service
add_subdirectory(antivirus_gui)
add_subdirectory(database_redactor)



