# Test Coverage

Download and install https://github.com/OpenCppCoverage/OpenCppCoverage.
Install from cmd.
```
.\OpenCppCoverageSetup-x64-0.9.9.0.exe /silent
```

ctest cmd:
```
OpenCppCoverage.exe --quiet --export_type cobertura:cobertura.xml --cover_children `
--sources ${PWD}\src --sources ${PWD}\include --modules ${PWD} `
-- ctest -C Debug --test-dir build --verbose --repeat until-pass:3 --timeout 30
```