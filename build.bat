@echo off
setlocal EnableDelayedExpansion

:: Display help information
if "%~1"=="--help" goto :show_help
if "%~1"=="/?" goto :show_help

:: Process command line arguments
set CLEAN_BUILD=0
set BUILD_DOCS=0
set CLEAN_DOCS=0

:parse_args
if "%~1"=="" goto :end_parse
if "%~1"=="--clean" (
    set CLEAN_BUILD=1
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
echo Unknown option: %~1
goto :show_help

:end_parse

:: Clean build if requested
if %CLEAN_BUILD%==1 (
    echo Performing clean build...
    if exist build rmdir /s /q build
    mkdir build
) else (
    :: Create build directory if it doesn't exist
    if not exist build (
        echo Creating build directory...
        mkdir build
    )
)

pushd build

:: Always run cmake
echo Configuring project with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

:: Build
echo Building project...
cmake --build . --config Release

popd

echo Build completed successfully!

:: Generate documentation if requested
if %BUILD_DOCS%==1 (
    echo Generating Doxygen documentation...
    
    :: Create docs directory if it doesn't exist
    if not exist docs mkdir docs
    
    :: Clean docs if requested
    if %CLEAN_DOCS%==1 (
        echo Cleaning documentation directory...
        del /q docs\* 2>nul
        for /d %%x in (docs\*) do rmdir /s /q "%%x"
    )
    
    :: Check if doxygen is installed
    where doxygen >nul 2>nul
    if errorlevel 1 (
        echo Error: Doxygen is not installed. Please install it to generate documentation.
        exit /b 1
    )
    
    :: Run doxygen
    doxygen Doxyfile
    
    echo Documentation generated successfully in the docs directory!
)

exit /b 0

:show_help
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   --clean           Perform a clean rebuild by removing the build directory
echo   --docs            Generate Doxygen documentation
echo   --clean-docs      Clean and regenerate Doxygen documentation
echo   --help            Display this help and exit
echo.
exit /b 0