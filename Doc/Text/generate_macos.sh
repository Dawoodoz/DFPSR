# Build the generator if it does not already exist.
chmod +x ../../Source/tools/builder/buildProject.sh;
../../Source/tools/builder/buildProject.sh ../../Source/tools/documentation/Generator.DsrProj MacOS SkipIfBinaryExists;

# Call the generator to convert text files into HTML.
../../Source/tools/documentation/Generator . .. ../Resources
