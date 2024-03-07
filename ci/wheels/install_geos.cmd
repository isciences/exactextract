:: BSD 3-Clause License
::
:: Copyright (c) 2007, Sean C. Gillies. 2019, Casper van der Wel. 2007-2022, Shapely Contributors.
:: All rights reserved.
::
:: Redistribution and use in source and binary forms, with or without
:: modification, are permitted provided that the following conditions are met:
::
:: 1. Redistributions of source code must retain the above copyright notice, this
::    list of conditions and the following disclaimer.
::
:: 2. Redistributions in binary form must reproduce the above copyright notice,
::    this list of conditions and the following disclaimer in the documentation
::    and/or other materials provided with the distribution.
::
:: 3. Neither the name of the copyright holder nor the names of its
::    contributors may be used to endorse or promote products derived from
::    this software without specific prior written permission.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
:: AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
:: IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
:: DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
:: FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
:: DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
:: SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
:: CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
:: OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
:: OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
::
:: Build and install GEOS on Windows system, save cache for later
::
:: This script requires environment variables to be set
::  - set GEOS_INSTALL=C:\path\to\cached\prefix -- to build or use as cache
::  - set GEOS_VERSION=3.7.3 -- to download and compile

if exist %GEOS_INSTALL% (
  echo Using cached %GEOS_INSTALL%
) else (
  echo Building %GEOS_INSTALL%

  curl -fsSO http://download.osgeo.org/geos/geos-%GEOS_VERSION%.tar.bz2
  7z x geos-%GEOS_VERSION%.tar.bz2
  7z x geos-%GEOS_VERSION%.tar
  cd geos-%GEOS_VERSION% || exit /B 1

  pip install ninja cmake
  cmake --version

  mkdir build
  cd build
  cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%GEOS_INSTALL% .. || exit /B 2
  cmake --build . || exit /B 3
  ctest . || exit /B 4
  cmake --install . || exit /B 5
  cd ..
)
