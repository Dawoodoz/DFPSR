# The implementation folder
To clearly define what is a part of the external interface to the framework, the code that is mainly for internal use is placed within this implementation folder.
While you can include headers directly from the implementation by controlling exactly which version of the library you use, it should be kept to a minimum because it may break backward compatibility without warning in new releases.
