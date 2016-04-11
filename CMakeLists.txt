cmake_minimum_required(VERSION 3.0)
project(qn C)

include_directories("C:/Program Files (x86)/Microsoft SQL Server/Client SDK/ODBC/110/SDK/Include/")
link_directories("C:/Program Files (x86)/Microsoft SQL Server/Client SDK/ODBC/110/SDK/Lib/x86/")

add_executable(update update.c)
add_executable(watch watch.c)