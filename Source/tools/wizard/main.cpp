﻿
// TODO:
// * A catalogue of SDK examples with images and descriptions loaded automatically from their folder.
//     Offer one-click build and execution of SDK examples on multiple platforms, while explaining how the building works.
//     How can the file library execute other applications and scripts in a portable way when scripts need to select a terminal application to execute them?
//     Maybe call the builder as a static library and have it call the compiler directly in a simulated terminal window embedded into the wizard, instead of using unreliable scripts?
// * Let the user browse a file system and select a location for a new or existing project.
//     Should a multi-frame tab container be created to allow having multiple frames in the same container?
//         Can let frames have a caption for when used within a container.

#include "../../DFPSR/includeFramework.h"
#include "../../SDK/SoundEngine/soundEngine.h"

using namespace dsr;

// Global
bool running = true;
Window window;

// Visual components
Component projectList;
Component launchButton;
Component descriptionLabel;
Component previewPicture;

// Media
int boomSound;

struct Project {
	String projectFilePath;
	String executableFilePath;
	String title; // To display
	String description; // To show when selected
	DsrProcess programHandle;
	DsrProcessStatus lastStatus = DsrProcessStatus::NotStarted;
	OrderedImageRgbaU8 preview;
	Project(const ReadableString &projectFilePath);
};
List<Project> projects;

Project::Project(const ReadableString &projectFilePath)
: projectFilePath(projectFilePath) {
	String projectFolderPath = file_getRelativeParentFolder(projectFilePath);
	String extensionlessProjectPath = file_getExtensionless(projectFilePath);
	this->title = file_getPathlessName(extensionlessProjectPath);
	#ifdef USE_MICROSOFT_WINDOWS
		this->executableFilePath = string_combine(extensionlessProjectPath, U".exe");
	#else
		this->executableFilePath = extensionlessProjectPath;
	#endif
	if (file_getEntryType(this->executableFilePath) != EntryType::File) {
		this->executableFilePath = U"";
	}
	String descriptionPath = file_combinePaths(projectFolderPath, U"Description.txt");
	if (file_getEntryType(descriptionPath) == EntryType::File) {
		this->description = string_load(descriptionPath);
	} else {
		this->description = string_combine(U"Project at ", projectFolderPath, U" did not have any Description.txt to display!");
	}
	String previewPath = file_combinePaths(projectFolderPath, U"Preview.jpg");
	if (file_getEntryType(previewPath) == EntryType::File) {
		this->preview = image_load_RgbaU8(previewPath);
	} else {
		previewPath = file_combinePaths(projectFolderPath, U"Preview.gif");
		if (file_getEntryType(previewPath) == EntryType::File) {
			this->preview = image_load_RgbaU8(previewPath);
		} else {
			this->preview = OrderedImageRgbaU8();
		}
	}
}

static ReadableString findParent(const ReadableString& startPath, const ReadableString& parentName) {
	int64_t pathEndIndex = -1; // Last character of path leading to Source.
	file_getPathEntries(startPath, [&pathEndIndex, &parentName](ReadableString entry, int64_t firstIndex, int64_t lastIndex) {
		if (string_match(entry, parentName)) {
			pathEndIndex = lastIndex;
		}
	});
	if (pathEndIndex == -1) {
		throwError(U"Could not find the Source folder with SDK examples.");
		return startPath;
	} else {
		return string_until(startPath, pathEndIndex);
	}
}

static void findProjects(const ReadableString& folderPath) {
	file_getFolderContent(folderPath, [](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
		if (entryType == EntryType::Folder) {
			findProjects(entryPath);
		} else if (entryType == EntryType::File) {
			ReadableString extension = string_upperCase(file_getExtension(entryName));
			Project newProject = Project(entryPath);
			// If we find a project within folderPath...
			if (string_match(extension, U"DSRPROJ")) {
				// ...and the folder is not namned wizard...
				if (!string_match(newProject.title, U"Wizard")) {
					// ...then add it to the list of projects.
					projects.push(newProject);
				}
			}
		}
	});
}

// Returns true iff the interface needs to be redrawn.
static bool updateInterface(bool forceUpdate) {
	bool needToDraw = false;
	int projectIndex = component_getProperty_integer(projectList, U"SelectedIndex", true);
	//Application name from project name?
	if (projectIndex >= 0 && projectIndex < projects.length()) {
		DsrProcessStatus newStatus = process_getStatus(projects[projectIndex].programHandle);
		DsrProcessStatus lastStatus = projects[projectIndex].lastStatus;
		if (newStatus != lastStatus || forceUpdate) {
			if (newStatus == DsrProcessStatus::Running) {
				component_setProperty_string(descriptionLabel, U"Text", string_combine(projects[projectIndex].title, U" is running."));
			} else if (newStatus == DsrProcessStatus::Crashed) {
				component_setProperty_string(descriptionLabel, U"Text", string_combine(projects[projectIndex].title, U" crashed."));
			} else if (newStatus == DsrProcessStatus::Completed) {
				component_setProperty_string(descriptionLabel, U"Text", string_combine(projects[projectIndex].title, U" terminated safely."));
			} else if (newStatus == DsrProcessStatus::NotStarted) {
				component_setProperty_string(descriptionLabel, U"Text", projects[projectIndex].description);
			}
			needToDraw = true;
			projects[projectIndex].lastStatus = newStatus;
		}
		component_setProperty_image(previewPicture, U"Image", projects[projectIndex].preview, false);
		bool foundExecutable = string_length(projects[projectIndex].executableFilePath) > 0;
		component_setProperty_integer(launchButton, U"Visible", foundExecutable);
	}
	return needToDraw;
}

static void selectProject(int64_t projectIndex) {
	// Don't trigger new events if the selected index is already updated manually.
	if (projectIndex != component_getProperty_integer(projectList, U"SelectedIndex", true)) {
		component_setProperty_integer(projectList, U"SelectedIndex", projectIndex, false);
	}
	updateInterface(true);
}

static void populateInterface() {
	for (int p = 0; p < projects.length(); p++) {
		component_call(projectList, U"PushElement", projects[p].title);
	}
	selectProject(0);
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Get the application folder.
	String applicationFolder = file_getApplicationFolder();
	String mediaFolder = file_combinePaths(applicationFolder, U"media");

	// Start sound.
	soundEngine_initialize();
	boomSound = soundEngine_loadSoundFromFile(file_combinePaths(mediaFolder, U"Boom.wav"));

	// Create a window.
	window = window_create(U"DFPSR wizard application", 800, 600);

	// Create components using the layout.
	window_loadInterfaceFromFile(window, file_combinePaths(mediaFolder, U"Interface.lof"));

	// Create a virtual machine with reusable image generating functions.
	//   The same Media Machine Code (*.mmc) can be used for multiple themes.
	MediaMachine machine = machine_create(string_load(file_combinePaths(mediaFolder, U"Drawing.mmc")));
	// Use the virtual machine with a specific style referring to the functions in machine.
	window_applyTheme(window, theme_createFromFile(machine, file_combinePaths(mediaFolder, U"Theme.ini")));

	// Find components.
	projectList = window_findComponentByName(window, U"projectList");
	launchButton = window_findComponentByName(window, U"launchButton");
	descriptionLabel = window_findComponentByName(window, U"descriptionLabel");
	previewPicture = window_findComponentByName(window, U"previewPicture");

	// Find projects to showcase.
	//   On systems that don't allow getting the application's folder, the program must be started somewhere within the Source folder.
	String sourceFolder = findParent(applicationFolder, U"Source");
	findProjects(file_combinePaths(sourceFolder, U"SDK"));
	findProjects(file_combinePaths(sourceFolder, U"templates"));
	populateInterface();

	// Bind methods to events.
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key == DsrKey_Escape) {
				running = false;
			}
		}
	});
	component_setPressedEvent(launchButton, []() {
		soundEngine_playSound(boomSound, false);
		int projectIndex = component_getProperty_integer(projectList, U"SelectedIndex", true);
		//Application name from project name?
		if (projectIndex >= 0 && projectIndex < projects.length()) {
			if (file_getEntryType(projects[projectIndex].executableFilePath) != EntryType::File) {
				// Could not find the application.
				component_setProperty_string(descriptionLabel, U"Text", string_combine(U"Could not find the executable at ", projects[projectIndex].executableFilePath, U"!\n"), true);
			} else if (process_getStatus(projects[projectIndex].programHandle) != DsrProcessStatus::Running) {
				// Select input arguments.
				List<String> arguments;
				if (string_match(projects[projectIndex].title, U"BasicCLI")) {
					// Give some random arguments to the CLI template, so that it will do something more than just printing "Hello World".
					arguments.push(U"1");
					arguments.push(U"TWO");
					arguments.push(U"three");
					arguments.push(U"Four");
				}
				// Launch the application from the project's folder.
				file_setCurrentPath(file_getAbsoluteParentFolder(projects[projectIndex].executableFilePath));
				projects[projectIndex].programHandle = process_execute(projects[projectIndex].executableFilePath, arguments);
				updateInterface(true);
			}
		}
	});
	component_setSelectEvent(projectList, [](int64_t index) {
		soundEngine_playSound(boomSound, false);
		selectProject(index);
	});
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Execute.
	soundEngine_playSound(boomSound, false);
	while(running) {
		// Wait for actions so that we don't render until an action has been recieved.
		// This will save battery on laptops for applications that don't require animation.
		while (!(window_executeEvents(window) || updateInterface(false))) {
			time_sleepSeconds(0.01);
		}
		// Fill the background.
		AlignedImageRgbaU8 canvas = window_getCanvas(window);
		image_fill(canvas, ColorRgbaI32(64, 64, 64, 255));
		// Draw interface.
		window_drawComponents(window);
		// Show the final image.
		window_showCanvas(window);
	}

	// Close sound.
	soundEngine_terminate();
}
