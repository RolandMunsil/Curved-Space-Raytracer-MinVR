/*
* Copyright Regents of the University of Minnesota, 2016.  This software is released under the following license: http://opensource.org/licenses/
* Source code originally developed at the University of Minnesota Interactive Visualization Lab (http://ivlab.cs.umn.edu).
*
* Code author(s):
* 		Dan Orban (dtorban)
*/

#include "VRMultithreadedApp.h"
#include <main/VRMain.h>


namespace MinVR {

	VRMultithreadedApp::VRMultithreadedApp(int argc, char** argv) {
		_main = new VRMain();

		_main->addEventHandler(this);
		_main->addRenderHandler(this);
		_main->addModelHandler(this);
		_main->initialize(argc, argv);
	}

	VRMultithreadedApp::~VRMultithreadedApp() {
		shutdown();
		delete _main;
	}

	void VRMultithreadedApp::run() {
		while (_main->mainloop()) {}
	}

	void VRMultithreadedApp::shutdown() {
		_main->shutdown();
	}

	std::string VRMultithreadedApp::getVRSetupName() {
		return _main->getName();
	}

	int VRMultithreadedApp::getLeftoverArgc() {
		return _main->getLeftoverArgc();
	}

	char** VRMultithreadedApp::getLeftoverArgv() {
		return _main->getLeftoverArgv();
	}


	void VRMultithreadedApp::onVREvent(const VRDataIndex &eventData) {
		std::string type;
		if (eventData.exists("EventType")) {
			type = (VRString)eventData.getValue("EventType");
		}
		else {
			VRERROR("VRMultithreadedAppInternal::onVREvent() received an event named " + eventData.getName() + " of unknown type.",
				"All events should have a data field named EventType but none was found for this event.");
		}

		if (type == "AnalogUpdate") {
			onAnalogChange(VRAnalogEvent(eventData));
		}
		else if (type == "ButtonDown") {
			onButtonDown(VRButtonEvent(eventData));
		}
		else if (type == "ButtonUp") {
			onButtonUp(VRButtonEvent(eventData));
		}
		else if (type == "ButtonRepeat") {
			// intentionally not forwarding ButtonRepeat events since repeats are
			// not reported consistently on all systems and for VR apps we generally
			// just listen for downs and ups, it's an automatic repeat if you
			// have received a down and have not received a corresponding up.
		}
		else if (type == "CursorMove") {
			onCursorMove(VRCursorEvent(eventData));
		}
		else if (type == "TrackerMove") {
			onTrackerMove(VRTrackerEvent(eventData));
		}
		else {
			VRERROR("VRMultithreadedApp::onVREvent() received an event of unknown type: " + type,
				"Perhaps an input device that sends a new type of event was rencently added.");
		}
	}

	void VRMultithreadedApp::onVRRenderContext(const VRDataIndex &renderData) {
		if (renderData.exists("IsGraphics")) {
			onRenderGraphicsContext(VRGraphicsState(renderData));
		}
		else if (renderData.exists("IsAudio")) {
			onRenderAudio(VRAudioState(renderData));
		}
		else if (renderData.exists("IsConsole")) {
			onRenderConsole(VRConsoleState(renderData));
		}
		else if (renderData.exists("IsHaptics")) {
			onRenderHaptics(VRHapticsState(renderData));
		}
		else {
			VRERROR("VRMultithreadedAppInternal::onRenderContext() received an unknown type of render callback",
				"Perhaps a new type of display node was recently added.");
		}
	}


	void VRMultithreadedApp::onVRRenderScene(const VRDataIndex &renderData) {
		if (renderData.exists("IsGraphics")) {
			onRenderGraphicsScene(VRGraphicsState(renderData));
		}
		else if (renderData.exists("IsAudio")) {
			// nothing to do, already called onRenderAudio() during onVRRenderContext
		}
		else if (renderData.exists("IsConsole")) {
			// nothing to do, already called onRenderConsole() during onVRRenderContext
		}
		else if (renderData.exists("IsHaptics")) {
			// nothing to do, already called onRenderHaptics() during onVRRenderContext
		}
		else {
			VRERROR("VRMultithreadedApp::onRenderScene() received an unknown type of render callback",
				"Perhaps a new type of display node was recently added.");
		}
	}

	int VRMultithreadedApp::getNumWindows()
	{
		VRDataIndex* config = _main->getConfig();
		int numWindows = config->getValueWithDefault("MinVR/NumWindows", 1);
		return numWindows;
	}
}
