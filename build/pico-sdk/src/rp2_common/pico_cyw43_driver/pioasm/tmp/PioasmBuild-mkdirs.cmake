# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/pico-sdk/tools/pioasm"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pioasm"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/akshati_laptop/cornell/2023-24/sem1/ece4760/Pico/FinalProject/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
