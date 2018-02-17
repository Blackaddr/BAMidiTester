/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "MainComponent.h"

using std::make_shared;

//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (const String& deviceName) : name (deviceName) {}

    String name;
    ScopedPointer<MidiInput> inDevice;
    ScopedPointer<MidiOutput> outDevice;

    typedef ReferenceCountedObjectPtr<MidiDeviceListEntry> Ptr;
};

//==============================================================================
struct MidiCallbackMessage : public Message
{
    MidiCallbackMessage (const MidiMessage& msg) : message (msg) {}
    MidiMessage message;
};

//==============================================================================
class MidiDeviceListBox : public ListBox,
private ListBoxModel
{
public:
    //==============================================================================
    MidiDeviceListBox (const String& name,
                       MainContentComponent& contentComponent,
                       bool isInputDeviceList)
    : ListBox (name, this),
      parent (contentComponent),
      isInput (isInputDeviceList)
    {
        setOutlineThickness (1);
        setMultipleSelectionEnabled (true);
        setClickingTogglesRowSelection (true);
    }

    //==============================================================================
    int getNumRows() override
    {
        return isInput ? parent.getNumMidiInputs()
                       : parent.getNumMidiOutputs();
    }

    //==============================================================================
    void paintListBoxItem (int rowNumber, Graphics &g,
                           int width, int height, bool rowIsSelected) override
    {
        const auto textColour = getLookAndFeel().findColour (ListBox::textColourId);

        if (rowIsSelected)
            g.fillAll (textColour.interpolatedWith (getLookAndFeel().findColour (ListBox::backgroundColourId), 0.5));


        g.setColour (textColour);
        g.setFont (height * 0.7f);

        if (isInput)
        {
            if (rowNumber < parent.getNumMidiInputs())
                g.drawText (parent.getMidiDevice (rowNumber, true)->name,
                            5, 0, width, height,
                            Justification::centredLeft, true);
        }
        else
        {
            if (rowNumber < parent.getNumMidiOutputs())
                g.drawText (parent.getMidiDevice (rowNumber, false)->name,
                            5, 0, width, height,
                            Justification::centredLeft, true);
        }
    }

    //==============================================================================
    void selectedRowsChanged (int) override
    {
        SparseSet<int> newSelectedItems = getSelectedRows();
        if (newSelectedItems != lastSelectedItems)
        {
            for (int i = 0; i < lastSelectedItems.size(); ++i)
            {
                if (! newSelectedItems.contains (lastSelectedItems[i]))
                    parent.closeDevice (isInput, lastSelectedItems[i]);
            }

            for (int i = 0; i < newSelectedItems.size(); ++i)
            {
                if (! lastSelectedItems.contains (newSelectedItems[i]))
                    parent.openDevice (isInput, newSelectedItems[i]);
            }

            lastSelectedItems = newSelectedItems;
        }
    }

    //==============================================================================
    void syncSelectedItemsWithDeviceList (const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
    {
        SparseSet<int> selectedRows;
        for (int i = 0; i < midiDevices.size(); ++i)
            if (midiDevices[i]->inDevice != nullptr || midiDevices[i]->outDevice != nullptr)
                selectedRows.addRange (Range<int> (i, i+1));

        lastSelectedItems = selectedRows;
        updateContent();
        setSelectedRows (selectedRows, dontSendNotification);
    }

private:
    //==============================================================================
    MainContentComponent& parent;
    bool isInput;
    SparseSet<int> lastSelectedItems;
};

void setupKnob(Slider *slider, Slider::Listener *sliderListen,
	           Label *label, Label::Listener *labelListen,
	           const char *text, KnobLookAndFeel *lookfeel)
{
	slider->setSliderStyle(Slider::Rotary);
	slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	slider->setRange(0, 127, 1);
	slider->addListener(sliderListen);
	slider->setLookAndFeel(lookfeel);
	slider->setValue(63.0f);
	
	//label->attachToComponent(slider, false);
	label->setSize(slider->getWidth() * 2, slider->getHeight()/2);
	label->setBorderSize(BorderSize<int>(5));
	label->setText(text, dontSendNotification);
	dynamic_cast<ParamLabel*>(label)->setLabelName(String(text).replaceCharacter(' ', '_'));
	label->setJustificationType(Justification::centred);
	label->setFont(Font(24.0, Font::bold));
	label->setEditable(true);
	label->addListener(labelListen);
	
}

void setupEffectLabel(Label *label, Label::Listener *listener, char *text)
{
	label->setText(text, dontSendNotification);
	dynamic_cast<ParamLabel*>(label)->setLabelName(String(text).replaceCharacter(' ', '_'));
	label->setJustificationType(Justification::centred);
	label->setFont(Font(24.0, Font::bold));
	label->setEditable(true);
	label->addListener(listener);
}

void setupButton(ImageButton *button, Label *label, Label::Listener *labelListener, char *text, Image &pressed, Image &unpressed)
{
	button->setImages(false, true, true, unpressed, 1.0f, Colours::transparentBlack, Image(), 1.0f, Colour(), pressed, 1.0f, Colours::transparentBlack, 0.5f);

	label->setBorderSize(BorderSize<int>(5));
	label->setText(text, dontSendNotification);
	dynamic_cast<ParamLabel*>(label)->setLabelName(String(text).replaceCharacter(' ', '_'));
	//label->attachToComponent(button, false);
	label->setSize(button->getWidth() * 2, button->getHeight()/2);
	label->setJustificationType(Justification::centred);
	label->setFont(Font(24.0, Font::bold));
	label->setEditable(true);
	label->addListener(labelListener);
}


PedalAreaComponent::PedalAreaComponent()
{
}

PedalAreaComponent::~PedalAreaComponent()
{
}

void PedalAreaComponent::paint(Graphics &g)
{
	const float margin = 5.0f;

	const float thickness = 5.0f;
	const float cornerSize = 5.0f;
	g.setColour(Colours::black);
	g.fillRoundedRectangle(margin, margin, getWidth() - 2 * margin, getHeight() - 2 * margin, cornerSize);
	g.setColour(BAColour);
	g.drawRoundedRectangle(margin, margin, getWidth() - 2 * margin, getHeight() - 2 * margin, cornerSize, thickness);

	// Draw the logo
	//Image logo = ImageCache::getFromFile(File("logo_transparent.png"));
	Image logo = ImageCache::getFromMemory(BinaryData::logo_transparent_png, BinaryData::logo_transparent_pngSize);
	if (logo.isValid()) {
		float ratio = (float)logo.getWidth() / (float)logo.getHeight();
		float outputHeight = (float)getHeight() / 3.5f;
		float outputWidth = outputHeight * ratio;
		g.drawImage(logo, getWidth() / 2 - (int)(outputWidth / 2), getHeight() - (int)outputHeight - 3*(int)margin, (int)outputWidth, (int)outputHeight,
			0, 0, logo.getWidth(), logo.getHeight());
	}

}

void PedalAreaComponent::resized()
{

}

//==============================================================================
MainContentComponent::MainContentComponent ()
    : midiInputLabel ("Midi Input Label", "MIDI Input:"),
      midiOutputLabel ("Midi Output Label", "MIDI Output:"),
      incomingMidiLabel ("Incoming Midi Label", "Received MIDI messages:"),
      outgoingMidiLabel ("Outgoing Midi Label", "Play the keyboard to send MIDI messages..."),
      midiKeyboard (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
      midiMonitor ("MIDI Monitor"),
      pairButton ("MIDI Bluetooth devices..."),
	  buttonA ("A"),
	  buttonB("B"),
	  knob1 ("1"),
	  knob2("2"),
	  knob3("3"),
	  knob4("4"),
      midiInputSelector (new MidiDeviceListBox ("Midi Input Selector", *this, true)),
      midiOutputSelector (new MidiDeviceListBox ("Midi Input Selector", *this, false))
{
    setSize (APP_WIDTH, APP_HEIGHT);

    addLabelAndSetStyle (midiInputLabel);
    addLabelAndSetStyle (midiOutputLabel);
    addLabelAndSetStyle (incomingMidiLabel);
    addLabelAndSetStyle (outgoingMidiLabel);

    midiKeyboard.setName ("MIDI Keyboard");
    addAndMakeVisible (midiKeyboard);

    midiMonitor.setMultiLine (true);
    midiMonitor.setReturnKeyStartsNewLine (false);
    midiMonitor.setReadOnly (true);
    midiMonitor.setScrollbarsShown (true);
    midiMonitor.setCaretVisible (false);
    midiMonitor.setPopupMenuEnabled (false);
    midiMonitor.setText (String());
    addAndMakeVisible (midiMonitor);

    if (! BluetoothMidiDevicePairingDialogue::isAvailable())
        pairButton.setEnabled (false);

    addAndMakeVisible (pairButton);
    pairButton.addListener (this);

	// Load/Save Button setup
	loadButton.setButtonText("LOAD");
	saveButton.setButtonText("SAVE");
	loadButton.addListener(this);
	addAndMakeVisible(loadButton);
	saveButton.addListener(this);
	addAndMakeVisible(saveButton);

	// Button A/B Setup
	addAndMakeVisible(pedalArea);
	buttonLookAndFeel.setColour(TextButton::buttonOnColourId, Colours::green);

	//pressedButtonImg = ImageCache::getFromFile(File("led-circle-red-md.png"));
	pressedButtonImg = ImageCache::getFromMemory(BinaryData::ledcircleredmd_png, BinaryData::ledcircleredmd_pngSize);
	//unpressedButtonImg = ImageCache::getFromFile(File("led-circle-grey-md.png"));
	unpressedButtonImg = ImageCache::getFromMemory(BinaryData::ledcirclegreymd_png, BinaryData::ledcirclegreymd_pngSize);

	addAndMakeVisible(buttonA);	
	setupButton(&buttonA, &buttonALabel, this, "A", pressedButtonImg, unpressedButtonImg);
	buttonA.addListener (this);
	buttonA.setClickingTogglesState(true);
	addAndMakeVisible(buttonALabel);
	//buttonA.setLookAndFeel(&buttonLookAndFeel);

	addAndMakeVisible(buttonB);
	setupButton(&buttonB, &buttonBLabel, this, "B", pressedButtonImg, unpressedButtonImg);
	buttonB.addListener(this);
	buttonB.setClickingTogglesState(true);
	addAndMakeVisible(buttonBLabel);
	//buttonB.setLookAndFeel(&buttonLookAndFeel);

	// Effect title	
	setupEffectLabel(&effectLabel, this, "AUDIO EFFECT");
	addAndMakeVisible(effectLabel);
	
	// Knob 1/2/3/4 Setup
	setupKnob(&knob1, this, &knob1Label, this, "KNOB 1", &knobLookAndFeel);
	addAndMakeVisible(knob1);
	addAndMakeVisible(knob1Label);

	setupKnob(&knob2, this, &knob2Label, this, "KNOB 2", &knobLookAndFeel);
	addAndMakeVisible(knob2);
	addAndMakeVisible(knob2Label);

	setupKnob(&knob3, this, &knob3Label, this, "KNOB 3", &knobLookAndFeel);
	addAndMakeVisible(knob3);
	addAndMakeVisible(knob3Label);

	setupKnob(&knob4, this, &knob4Label, this, "KNOB 4", &knobLookAndFeel);
	addAndMakeVisible(knob4);
	addAndMakeVisible(knob4Label);

    keyboardState.addListener (this);
    addAndMakeVisible (midiInputSelector);
    addAndMakeVisible (midiOutputSelector);

	// Setup the param tree
	paramTree = new XmlElement("params");
	paramTree->setAttribute(effectLabel.getLabelName(), effectLabel.getText());
	paramTree->setAttribute(knob1Label.getLabelName(), knob1Label.getText());
	paramTree->setAttribute(knob2Label.getLabelName(), knob2Label.getText());
	paramTree->setAttribute(knob3Label.getLabelName(), knob3Label.getText());
	paramTree->setAttribute(knob4Label.getLabelName(), knob4Label.getText());
	paramTree->setAttribute(buttonALabel.getLabelName(), buttonALabel.getText());
	paramTree->setAttribute(buttonBLabel.getLabelName(), buttonBLabel.getText());

    startTimer (500);
}

//==============================================================================
void MainContentComponent::addLabelAndSetStyle (Label& label)
{
    label.setFont (Font (15.00f, Font::plain));
    label.setJustificationType (Justification::centredLeft);
    label.setEditable (false, false, false);
    label.setColour (TextEditor::textColourId, Colours::black);
    label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label);
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    stopTimer();
    midiInputs.clear();
    midiOutputs.clear();
    keyboardState.removeListener (this);

    midiInputSelector = nullptr;
    midiOutputSelector = nullptr;
    midiOutputSelector = nullptr;

	paramTree = nullptr;
	//if (paramTree) delete paramTree;
}

//==============================================================================
void MainContentComponent::paint (Graphics&)
{
}

void attachLabelToComp(Label *label, Component *comp, int labelHeight)
{
	Rectangle<int> compBounds = comp->getBounds();	
	label->setBounds(compBounds.getCentreX() - comp->getWidth(), compBounds.getY() - (labelHeight), 2*comp->getWidth(), labelHeight);
}

//==============================================================================
// LOCATIONS
void MainContentComponent::resized()
{
    int margin = 20;

	const int textRowHeight = 24;

	int nextRowStart = margin;

    midiInputLabel.setBounds (margin, nextRowStart,
		(getWidth() / 2) - (2 * margin), textRowHeight);

    midiOutputLabel.setBounds ((getWidth() / 2) + margin, nextRowStart,
		(getWidth() / 2) - (2 * margin), 24); nextRowStart += textRowHeight + margin;

	const int deviceListHeight = 4 * textRowHeight;
    midiInputSelector->setBounds (margin, nextRowStart,
        (getWidth() / 2) - (2 * margin),
		deviceListHeight);

    midiOutputSelector->setBounds ((getWidth() / 2) + margin, nextRowStart,
        (getWidth() / 2) - (2 * margin),
		deviceListHeight); nextRowStart += deviceListHeight + margin;

    pairButton.setBounds (margin, nextRowStart,
                          getWidth() - (2 * margin), textRowHeight);  nextRowStart += textRowHeight + margin;

	// START CUSTOM CONTROLS
	int pedalAreaStart = nextRowStart;
	//nextRowStart ;

	// effect label
	const int EFFECT_LABEL_WIDTH = 250;
	const int EFFECT_LABEL_HEIGHT = 50;
	effectLabel.setBounds((getWidth() - EFFECT_LABEL_WIDTH) / 2, nextRowStart, EFFECT_LABEL_WIDTH, EFFECT_LABEL_HEIGHT); 

	nextRowStart += EFFECT_LABEL_HEIGHT + margin;

	// KNOBS
	int prevMargin = margin;
	margin = 50;
	const int KNOB_WIDTH = 75;
	const int KNOB_HEIGHT = 75;
	const int LABEL_HEIGHT = 35;
	int knobOffset = (getWidth() - (2 * margin)) / (NUM_KNOBS - 1);
	int nextColStart = margin;
	knob1.setBounds(margin, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	attachLabelToComp(&knob1Label, &knob1, LABEL_HEIGHT);
	knob2.setBounds(nextColStart-KNOB_WIDTH/2, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	attachLabelToComp(&knob2Label, &knob2, LABEL_HEIGHT);
	knob3.setBounds(nextColStart-KNOB_WIDTH/2, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	attachLabelToComp(&knob3Label, &knob3, LABEL_HEIGHT);
	knob4.setBounds(getWidth()-margin-KNOB_WIDTH, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextRowStart += KNOB_HEIGHT/2 + margin;
	attachLabelToComp(&knob4Label, &knob4, LABEL_HEIGHT);
	margin = prevMargin;

	// BUTTONS
	const int CC_BUTTON_WIDTH = 100;
	const int CC_BUTTON_HEIGHT = 50;
	const int PEDAL_AREA_OFFSET = margin;
	const int PEDAL_AREA_WIDTH = getWidth() - 2 * margin;
	// the component is -2*margin in size, the rectange graphic is another -2*margin
	int buttonOffset = ((PEDAL_AREA_WIDTH-3*margin) / (NUM_BUTTONS+2)) + PEDAL_AREA_OFFSET;
	nextColStart = margin;
	buttonA.setBounds(buttonOffset-CC_BUTTON_WIDTH/2, nextRowStart, CC_BUTTON_WIDTH, CC_BUTTON_HEIGHT);  nextColStart += 2*buttonOffset;
	attachLabelToComp(&buttonALabel, &buttonA, LABEL_HEIGHT);
	buttonB.setBounds(getWidth()-buttonOffset+margin/2-CC_BUTTON_WIDTH/2, nextRowStart, CC_BUTTON_WIDTH, CC_BUTTON_HEIGHT);  nextRowStart += CC_BUTTON_HEIGHT + margin;
	attachLabelToComp(&buttonBLabel, &buttonB, LABEL_HEIGHT);

	nextRowStart += 24;

	int pedalAreaStop = nextRowStart;

	pedalArea.setBounds(PEDAL_AREA_OFFSET, pedalAreaStart, PEDAL_AREA_WIDTH, pedalAreaStop - pedalAreaStart);
	// END CUSTOM CONSTROLS
	const int loadSaveButtonWidth = 50;
	loadButton.setBounds(getWidth() - margin - loadSaveButtonWidth, nextRowStart, loadSaveButtonWidth, textRowHeight);
	saveButton.setBounds(getWidth() - 2*margin - 2*loadSaveButtonWidth, nextRowStart, loadSaveButtonWidth, textRowHeight);

	outgoingMidiLabel.setBounds(margin, nextRowStart, getWidth() - (2 * margin), textRowHeight); nextRowStart += textRowHeight + margin;

	const int midiKeyboardHeight = 64;
	midiKeyboard.setBounds(0, nextRowStart, getWidth(), midiKeyboardHeight); nextRowStart += midiKeyboardHeight + margin;

    incomingMidiLabel.setBounds (margin, nextRowStart,
                                 getWidth() - (2*margin), textRowHeight); nextRowStart += textRowHeight + margin;

    midiMonitor.setBounds (margin/2, nextRowStart,
                           getWidth() - margin, getHeight() - nextRowStart - margin);
}

//==============================================================================
void MainContentComponent::buttonClicked(Button* buttonThatWasClicked)
{
	bool sendMidi = false;
	int val;
	MidiMessage *m = nullptr;
	if (buttonThatWasClicked == &pairButton)
		RuntimePermissions::request(
			RuntimePermissions::bluetoothMidi,
			[](bool wasGranted) { if (wasGranted) BluetoothMidiDevicePairingDialogue::open(); });

	if (buttonThatWasClicked == &saveButton) {
		FileChooser myChooser("Please provide the XML filename you want to save...",
			//File::getSpecialLocation(File::userHomeDirectory),
			File::getCurrentWorkingDirectory(),
			"*.xml");
		if (myChooser.browseForFileToSave(true))
		{
			File xmlFile(myChooser.getResult());
			paramTree->writeToFile(xmlFile, String());
		}

	} else if (buttonThatWasClicked == &loadButton) {
		FileChooser myChooser("Please select the XML file you want to load...",
			//File::getSpecialLocation(File::userHomeDirectory),
			File::getCurrentWorkingDirectory(),
			"*.xml");
		if (myChooser.browseForFileToOpen())
		{
			File xmlFile(myChooser.getResult());
			if (paramTree) delete paramTree;
			paramTree = XmlDocument::parse(xmlFile);

			if (!paramTree) {
				midiMonitor.insertTextAtCaret(String("Error loading file\n"));
				midiMonitor.insertTextAtCaret(xmlFile.getFileName());
				midiMonitor.insertTextAtCaret("\n");
			}
		}

		
		effectLabel.setText(paramTree->getStringAttribute(StringRef(effectLabel.getLabelName()),String("Err")), dontSendNotification);
		knob1Label.setText(paramTree->getStringAttribute(StringRef(knob1Label.getLabelName()), String("Err")), dontSendNotification);
		knob2Label.setText(paramTree->getStringAttribute(StringRef(knob2Label.getLabelName()), String("Err")), dontSendNotification);
		knob3Label.setText(paramTree->getStringAttribute(StringRef(knob3Label.getLabelName()), String("Err")), dontSendNotification);
		knob4Label.setText(paramTree->getStringAttribute(StringRef(knob4Label.getLabelName()), String("Err")), dontSendNotification);
		buttonALabel.setText(paramTree->getStringAttribute(StringRef(buttonALabel.getLabelName()), String("Err")), dontSendNotification);
		buttonBLabel.setText(paramTree->getStringAttribute(StringRef(buttonBLabel.getLabelName()), String("Err")), dontSendNotification);
		
	} else if (buttonThatWasClicked == &buttonA) {
		// Send the midi CC		
		if (!buttonA.getToggleState()) { val = CC_OFF; }
		else { val = CC_ON; }
		m = new MidiMessage(MidiMessage::controllerEvent(0, buttonACCId, val));
		sendMidi = true;
	} else if (buttonThatWasClicked == &buttonB) {
		// Send the midi CC		
		if (!buttonB.getToggleState()) { val = CC_OFF; }
		else { val = CC_ON; }
		m = new MidiMessage(MidiMessage::controllerEvent(0, buttonBCCId, val));
		sendMidi = true;
	}

	if (sendMidi && m) {
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
		delete m;
	}
}

//==============================================================================
void MainContentComponent::sliderValueChanged(Slider* slider)
{
	bool sendMidi = false;
	MidiMessage *m = nullptr;
	if (slider == &knob1) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob1CCId, (int)knob1.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
    } else if (slider == &knob2) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob2CCId, (int)knob2.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	} else if (slider == &knob3) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob3CCId, (int)knob3.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	} else if (slider == &knob4) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob4CCId, (int)knob4.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	}

	if (sendMidi && m) {
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
		delete m;
	}
}

void MainContentComponent::labelTextChanged(Label *label)
{

	// Ensure it's a ParamLabel
	ParamLabel *paramLabel = dynamic_cast<ParamLabel *>(label);
	if (paramLabel) {
		// update the value
		paramTree->setAttribute(paramLabel->getLabelName(), paramLabel->getText());
	}
}

//==============================================================================
bool MainContentComponent::hasDeviceListChanged (const StringArray& deviceNames, bool isInputDevice)
{
    ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                            : midiOutputs;

    if (deviceNames.size() != midiDevices.size())
        return true;

    for (int i = 0; i < deviceNames.size(); ++i)
        if (deviceNames[i] != midiDevices[i]->name)
            return true;

    return false;
}

MidiDeviceListEntry::Ptr MainContentComponent::findDeviceWithName (const String& name, bool isInputDevice) const
{
    const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                                  : midiOutputs;

    for (int i = 0; i < midiDevices.size(); ++i)
        if (midiDevices[i]->name == name)
            return midiDevices[i];

    return nullptr;
}

void MainContentComponent::closeUnpluggedDevices (StringArray& currentlyPluggedInDevices, bool isInputDevice)
{
    ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                            : midiOutputs;

    for (int i = midiDevices.size(); --i >= 0;)
    {
        MidiDeviceListEntry& d = *midiDevices[i];

        if (! currentlyPluggedInDevices.contains (d.name))
        {
            if (isInputDevice ? d.inDevice != nullptr
                              : d.outDevice != nullptr)
                closeDevice (isInputDevice, i);

            midiDevices.remove (i);
        }
    }
}

void MainContentComponent::updateDeviceList (bool isInputDeviceList)
{
    StringArray newDeviceNames = isInputDeviceList ? MidiInput::getDevices()
                                                   : MidiOutput::getDevices();

    if (hasDeviceListChanged (newDeviceNames, isInputDeviceList))
    {

        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
            = isInputDeviceList ? midiInputs : midiOutputs;

        closeUnpluggedDevices (newDeviceNames, isInputDeviceList);

        ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;

        // add all currently plugged-in devices to the device list
        for (int i = 0; i < newDeviceNames.size(); ++i)
        {
            MidiDeviceListEntry::Ptr entry = findDeviceWithName (newDeviceNames[i], isInputDeviceList);

            if (entry == nullptr)
                entry = new MidiDeviceListEntry (newDeviceNames[i]);

            newDeviceList.add (entry);
        }

        // actually update the device list
        midiDevices = newDeviceList;

        // update the selection status of the combo-box
        if (MidiDeviceListBox* midiSelector = isInputDeviceList ? midiInputSelector : midiOutputSelector)
            midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
    }
}

//==============================================================================
void MainContentComponent::timerCallback ()
{
    updateDeviceList (true);
    updateDeviceList (false);
}

//==============================================================================
void MainContentComponent::handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
    m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
    sendToOutputs (m);
}

//==============================================================================
void MainContentComponent::handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
    m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
    sendToOutputs (m);
}

//==============================================================================
void MainContentComponent::sendToOutputs(const MidiMessage& msg)
{
    for (int i = 0; i < midiOutputs.size(); ++i)
        if (midiOutputs[i]->outDevice != nullptr)
            midiOutputs[i]->outDevice->sendMessageNow (msg);
}

//==============================================================================
void MainContentComponent::handleIncomingMidiMessage (MidiInput* /*source*/, const MidiMessage &message)
{
    // This is called on the MIDI thread
	postMessage(new MidiCallbackMessage(message));

}

//==============================================================================
void MainContentComponent::handleMessage (const Message& msg)
{
    // This is called on the message loop

    const MidiMessage& mm = dynamic_cast<const MidiCallbackMessage&> (msg).message;
    String midiString;
	
	midiString << mm.getDescription() << "\n";
    midiMonitor.insertTextAtCaret (midiString);
}

//==============================================================================
void MainContentComponent::openDevice (bool isInput, int index)
{
    if (isInput)
    {
        jassert (midiInputs[index]->inDevice == nullptr);
        midiInputs[index]->inDevice = MidiInput::openDevice (index, this);

        if (midiInputs[index]->inDevice == nullptr)
        {
            DBG ("MainContentComponent::openDevice: open input device for index = " << index << " failed!" );
            return;
        }

        midiInputs[index]->inDevice->start();
    }
    else
    {
        jassert (midiOutputs[index]->outDevice == nullptr);
        midiOutputs[index]->outDevice = MidiOutput::openDevice (index);

        if (midiOutputs[index]->outDevice == nullptr)
            DBG ("MainContentComponent::openDevice: open output device for index = " << index << " failed!" );
    }
}

//==============================================================================
void MainContentComponent::closeDevice (bool isInput, int index)
{
    if (isInput)
    {
        jassert (midiInputs[index]->inDevice != nullptr);
        midiInputs[index]->inDevice->stop();
        midiInputs[index]->inDevice = nullptr;
    }
    else
    {
        jassert (midiOutputs[index]->outDevice != nullptr);
        midiOutputs[index]->outDevice = nullptr;
    }
}

//==============================================================================
int MainContentComponent::getNumMidiInputs() const noexcept
{
    return midiInputs.size();
}

//==============================================================================
int MainContentComponent::getNumMidiOutputs() const noexcept
{
    return midiOutputs.size();
}

//==============================================================================
ReferenceCountedObjectPtr<MidiDeviceListEntry>
MainContentComponent::getMidiDevice (int index, bool isInput) const noexcept
{
    return isInput ? midiInputs[index] : midiOutputs[index];
}
