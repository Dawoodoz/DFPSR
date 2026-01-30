rem Build the generator if it does not already exist.
call ..\..\Source\tools\builder\buildProject.bat ..\..\Source\tools\documentation\Generator.DsrProj Windows SkipIfBinaryExists

rem Call the generator to convert text files into HTML.
..\..\Source\tools\documentation\Generator.exe . .. ..\Resources
