@echo off

rd /s /q ".\Intermediate"
rd /s /q ".\Binaries"
rd /s /q ".\DerivedDataCache"
rd /s /q ".\Saved"
rd /s /q ".\.vs"

del /q "*.sln"

echo Cleanup completed.
pause