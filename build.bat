@echo off
setlocal EnableDelayedExpansion

:: Thread System Build Script
:: Improved version with better error handling and more options

:: Display banner
echo.
echo ============================================
echo          Thread System Build Script
echo ============================================
echo.

:: Process command line arguments
set CLEAN_BUILD=0
set BUILD_DOCS=0
set CLEAN_DOCS=0
set BUILD_TYPE=Release
set BUILD_BENCHMARKS=0
set TARGET=all
set DISABLE_STD_FORMAT=0
set DISABLE_STD_JTHREAD=0
set DISABLE_STD_SPAN=0
set BUILD_CORES=0
set VERBOSE=0

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :end_parse
if "%~1"=="--clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if "%~1"=="--benchmark" (
    set BUILD_BENCHMARKS=1
    shift
    goto :parse_args
)
if "%~1"=="--all" (
    set TARGET=all
    shift
    goto :parse_args
)
if "%~1"=="--lib-only" (
    set TARGET=lib-only
    shift
    goto :parse_args
)
if "%~1"=="--samples" (
    set TARGET=samples
    shift
    goto :parse_args
)
if "%~1"=="--tests" (
    set TARGET=tests
    shift
    goto :parse_args
)
if "%~1"=="--docs" (
    set BUILD_DOCS=1
    shift
    goto :parse_args
)
if "%~1"=="--clean-docs" (
    set BUILD_DOCS=1
    set CLEAN_DOCS=1
    shift
    goto :parse_args
)
if "%~1"=="--no-format" (
    set DISABLE_STD_FORMAT=1
    shift
    goto :parse_args
)
if "%~1"=="--no-jthread" (
    set DISABLE_STD_JTHREAD=1
    shift
    goto :parse_args
)
if "%~1"=="--no-span" (
    set DISABLE_STD_SPAN=1
    shift
    goto :parse_args
)
if "%~1"=="--cores" (
    set BUILD_CORES=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--verbose" (
    set VERBOSE=1
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    goto :show_help
)
if "%~1"=="/?" (
    goto :show_help
)
echo Unknown option: %~1
goto :show_help

:end_parse

:: Check for dependencies
echo [STATUS] Checking build dependencies...

:: Check for CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH
    echo [WARNING] Please install CMake and add it to your PATH
    echo [WARNING] You can use 'dependency.bat' to install required dependencies
    exit /b 1
)

:: Check for Git
where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git not found in PATH
    echo [WARNING] Please install Git and add it to your PATH
    echo [WARNING] You can use 'dependency.bat' to install required dependencies
    exit /b 1
)

:: Check for vcpkg
if not exist "..\vcpkg\" (
    echo [WARNING] vcpkg not found in parent directory
    echo [WARNING] Running dependency script to set up vcpkg...
    
    if exist "dependency.bat" (
        call dependency.bat
        if errorlevel 1 (
            echo [ERROR] Failed to run dependency.bat
            exit /b 1
        )
    ) else (
        echo [ERROR] dependency.bat script not found
        exit /b 1
    )
)

:: Check for doxygen if building docs
if %BUILD_DOCS%==1 (
    where doxygen >nul 2>nul
    if errorlevel 1 (
        echo [WARNING] Doxygen not found but documentation was requested
        echo [WARNING] Documentation will not be generated
        set BUILD_DOCS=0
    )
)

echo [SUCCESS] All dependencies are satisfied

:: Determine the number of cores to use for building
if %BUILD_CORES%==0 (
    :: Get the number of logical processors
    for /f "tokens=2 delims==" %%i in ('wmic cpu get NumberOfLogicalProcessors /value ^| findstr NumberOfLogicalProcessors') do set BUILD_CORES=%%i
    :: If detection failed, use a safe default
    if %BUILD_CORES%==0 set BUILD_CORES=2
)

echo [STATUS] Using %BUILD_CORES% cores for compilation

:: Clean build if requested
if %CLEAN_BUILD%==1 (
    echo [STATUS] Performing clean build...
    if exist build rmdir /s /q build
)

:: Create build directory if it doesn't exist
if not exist build (
    echo [STATUS] Creating build directory...
    mkdir build
)

:: Prepare CMake arguments
set CMAKE_ARGS=-DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

:: Add feature flags based on options
if %DISABLE_STD_FORMAT%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DSET_STD_FORMAT=OFF -DFORCE_FMT_FORMAT=ON
)

if %DISABLE_STD_JTHREAD%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DSET_STD_JTHREAD=OFF
)

if %DISABLE_STD_SPAN%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DSET_STD_SPAN=OFF
)

if %BUILD_BENCHMARKS%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_BENCHMARKS=ON
)

:: Set submodule option if building library only
if "%TARGET%"=="lib-only" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_THREADSYSTEM_AS_SUBMODULE=ON
)

:: Enter build directory
pushd build

:: Run CMake configuration
echo [STATUS] Configuring project with CMake...
cmake .. %CMAKE_ARGS%

:: Check if CMake configuration was successful
if errorlevel 1 (
    echo [ERROR] CMake configuration failed. See the output above for details.
    popd
    exit /b 1
)

:: Determine build target based on option
set "BUILD_TARGET="
if "%TARGET%"=="all" (
    set "BUILD_TARGET=ALL_BUILD"
) else if "%TARGET%"=="lib-only" (
    set "BUILD_TARGET=thread_base;thread_pool;priority_thread_pool;log_module"
) else if "%TARGET%"=="samples" (
    set "BUILD_TARGET=builder_sample;thread_pool_sample;priority_thread_pool_sample;logger_sample"
) else if "%TARGET%"=="tests" (
    set "BUILD_TARGET=thread_base_test;thread_pool_test;priority_thread_pool_test;logger_test;utilities_test"
)

:: Set build verbosity
if %VERBOSE%==1 (
    set "CMAKE_BUILD_ARGS=--verbose"
) else (
    set "CMAKE_BUILD_ARGS="
)

:: Build the project
echo [STATUS] Building project in %BUILD_TYPE% mode...

:: Use MSBuild for Windows
if not "%BUILD_TARGET%"=="" (
    cmake --build . --config %BUILD_TYPE% --target %BUILD_TARGET% -- /maxcpucount:%BUILD_CORES% %CMAKE_BUILD_ARGS%
) else (
    cmake --build . --config %BUILD_TYPE% -- /maxcpucount:%BUILD_CORES% %CMAKE_BUILD_ARGS%
)

:: Check if build was successful
if errorlevel 1 (
    echo [ERROR] Build failed. See the output above for details.
    popd
    exit /b 1
)

:: Run tests if requested
if "%TARGET%"=="tests" (
    echo [STATUS] Running tests...
    
    ctest -C %BUILD_TYPE% --output-on-failure
    if errorlevel 1 (
        echo [ERROR] Some tests failed. See the output above for details.
    ) else (
        echo [SUCCESS] All tests passed!
    )
)

:: Return to original directory
popd

:: Show success message
echo [SUCCESS] Build completed successfully!

:: Generate documentation if requested
if %BUILD_DOCS%==1 (
    echo [STATUS] Generating Doxygen documentation...
    
    :: Create docs directory if it doesn't exist
    if not exist docs mkdir docs
    
    :: Clean docs if requested
    if %CLEAN_DOCS%==1 (
        echo [STATUS] Cleaning documentation directory...
        del /q docs\* 2>nul
        for /d %%x in (docs\*) do rmdir /s /q "%%x"
    )
    
    :: Check if doxygen is installed
    where doxygen >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] Doxygen is not installed. Please install it to generate documentation.
        exit /b 1
    )
    
    :: Run doxygen
    doxygen Doxyfile
    
    :: Check if documentation generation was successful
    if errorlevel 1 (
        echo [ERROR] Documentation generation failed. See the output above for details.
    ) else (
        echo [SUCCESS] Documentation generated successfully in the docs directory!
    )
)

:: Final success message
echo.
echo ============================================
echo       Thread System Build Complete
echo ============================================
echo.

if exist build\bin (
    echo Available executables:
    dir /b build\bin\
)

echo Build type: %BUILD_TYPE%
echo Target: %TARGET%

if %BUILD_BENCHMARKS%==1 (
    echo Benchmarks: Enabled
)

if %BUILD_DOCS%==1 (
    echo Documentation: Generated
)

exit /b 0

:show_help
echo Usage: %~nx0 [options]
echo.
echo Build Options:
echo   --clean           Perform a clean rebuild by removing the build directory
echo   --debug           Build in debug mode (default is release)
echo   --benchmark       Build with benchmarks enabled
echo.
echo Target Options:
echo   --all             Build all targets (default)
echo   --lib-only        Build only the core libraries
echo   --samples         Build only the sample applications
echo   --tests           Build and run the unit tests
echo.
echo Documentation Options:
echo   --docs            Generate Doxygen documentation
echo   --clean-docs      Clean and regenerate Doxygen documentation
echo.
echo Feature Options:
echo   --no-format       Disable std::format even if supported
echo   --no-jthread      Disable std::jthread even if supported
echo   --no-span         Disable std::span even if supported
echo.
echo General Options:
echo   --cores N         Use N cores for compilation (default: auto-detect)
echo   --verbose         Show detailed build output
echo   --help            Display this help and exit
echo.
exit /b 0