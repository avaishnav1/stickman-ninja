# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/pico-sdk/tools/elf2uf2"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/elf2uf2"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/tmp"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/src/ELF2UF2Build-stamp"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/src"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/src/ELF2UF2Build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/src/ELF2UF2Build-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/Stickman_Ninja/elf2uf2/src/ELF2UF2Build-stamp${cfgdir}") # cfgdir has leading slash
endif()
